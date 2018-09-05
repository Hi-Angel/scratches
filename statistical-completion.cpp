#include <cassert>
#include <fcntl.h>
#include <unistd.h>
#include <cstdio>
#include <cctype>
#include <cstdint>
#include <functional>
#include <string>
#include <map>
#include <set>
#include <numeric>
#include <variant>
#include <vector>

// most frequent words: arbitrary percentage, for prototyping purposes
// a keyword: reference to one of "most frequent words"
// window size: arbitrary number, for prototyping purposes
// window: a window-size tuple, where elements are either keywords or 0-indexed non-keyword that may be repeated throughtout the window.

using namespace std;

using f32 = float;
using i32 = int32_t;
using i16 = int16_t;
using u32 = uint32_t;

// arbitrary choosen lowest percentage for a word that we'd consider a keyword
f32 KEYW_FREQ = 0.1;
i16 WIN_SZ = 6;

enum class TokenTypes {
    // todo: do I really need to take into account newlines in tokens?
    SkipWhitespace, // a whitespace that does not belong to Specials. Can be used as initial state
    AlphaNumeric, // alphanumeric, _
    Specials // non-alphanumeric, but specials, _, spaces, newlines.
        //todo: add "Finished" to move last processee from processee
};

template<class T>
using Maybe = variant<monostate,T>;
using Freq         = u32;
using Token        = string;
using TokensFreq   = map<Token, Freq>;
using TokenFreqRef = reference_wrapper<pair<const Token, Freq>>;

template<class Key, class Val>
pair<const Key,Val>& get_or_insert(map<Key, Val>& m, Key k, Val v) {
    typename map<Key,Val>::iterator it = m.find(k);
    if (it != m.end())
        return *it;
    else {
        m[k] = v;
        return *m.find(k);
    }
}

struct TokenizeState {
    TokensFreq tokens;
    vector<TokenFreqRef> text;
    string processee;
    TokenTypes type_processed;

    TokenizeState operator()(string processee_arg, TokenTypes type_processed_arg) {
        return TokenizeState{tokens, text, processee_arg, type_processed_arg};
    }

    TokenizeState operator()(TokenTypes type_processed_arg) {
        return TokenizeState{tokens, text, processee, type_processed_arg};
    }
};

bool is_alphanumeric(char ch) { return isdigit(ch) || isalpha(ch); }

TokenizeState tokenize(TokenizeState state, char ch) {
    switch (state.type_processed) {
        case TokenTypes::AlphaNumeric: {
            if (is_alphanumeric(ch)) {
                state.processee.push_back(ch);
                return state;
            } else { // finish a token, start a new one
                get_or_insert(state.tokens, state.processee, 0u).second += 1u;
                state.text.push_back(ref(*state.tokens.find(state.processee)));
                return tokenize(state("",
                                      isspace(ch)? TokenTypes::SkipWhitespace : TokenTypes::Specials),
                                ch);
            }
        }
        case TokenTypes::Specials: {
            if (!is_alphanumeric(ch)) {
                state.processee.push_back(ch);
                return state;
            } else { // finish a token, start a new one
                get_or_insert(state.tokens, state.processee, 0u).second += 1u;
                state.text.push_back(ref(*state.tokens.find(state.processee)));
                return tokenize(state("",
                                      isspace(ch)? TokenTypes::SkipWhitespace : TokenTypes::AlphaNumeric),
                                ch);
            }
        }
        case TokenTypes::SkipWhitespace: {
            if (isspace(ch))
                return state;
            else if (is_alphanumeric(ch)) // finish a token, start a new one
                return tokenize(state(TokenTypes::AlphaNumeric), ch);
            else
                return tokenize(state(TokenTypes::Specials), ch);

        }
        default:
            assert(false);
    }
}

class File {
    int fd;

    char* line_buf = 0;
    size_t sz_line_buf = 0;

    File(int fd, size_t sz_file) : fd(fd), sz_file(sz_file) {};
    File(const File&) = delete;

public:
    size_t sz_file;
    File(File&&)      = default;

    static
    Maybe<File> open(const char* filename) {
        int fd = ::open(filename, O_RDONLY);
        if (fd == -1) {
            perror("open");
            return monostate{};
        }

        off_t sz_file = lseek(fd, 0L, SEEK_END);
        if (sz_file == -1) {
            perror("lseek");
            abort();
        }
        lseek(fd, 0L, SEEK_SET);

        return Maybe<File>{File{fd, (size_t)sz_file}};
    }

    vector<char> to_vec() {
        vector<char> file_content(sz_file);
        if (read(fd, file_content.data(), sz_file) == -1) {
            perror("read");
            abort(); // no point in proceeding
        }
        return file_content;
    }
};


template <class T>
struct iterate {
    T* beg, *past_end;
    iterate(T* beg, T* past_end) : beg(beg), past_end(past_end) {}
    iterate(const iterate&) = delete;
    iterate(iterate&&)      = default;
    iterate&& skip(unsigned s) { return move(iterate{beg+s, past_end}); }

    constexpr
    T* begin() { return beg; }
    constexpr
    T* end() { return past_end; }
};

struct c_str_iter {
    char* iter;

    constexpr
    c_str_iter(char* iter) : iter(iter) {}

    constexpr
    char* operator++(){ return ++iter; }
    constexpr
    char operator[](uint i){ return (iter[i] == 0)? 0 : iter[i]; }
    constexpr
    char operator==(const c_str_iter& rhs) {
        return (rhs.iter == 0)? true : iter == rhs.iter;
    }
};


template <>
struct iterate<char> {
    c_str_iter beg;
    iterate(char* beg) : beg(beg) {}
    iterate(const iterate&) = delete;
    iterate(iterate&&)      = default;

    constexpr
    c_str_iter begin() { return beg; }

    constexpr
    c_str_iter end() { return c_str_iter((char*)0); }
};

template<class T>
T& unbox_ref(Maybe<T>& mb_t) {
    T& ret = get<1>(mb_t);
    return ret;
}

#define box2args(container) container.begin(), container.end()

void print_state_tokens(TokenizeState state) {
    vector<TokenFreqRef> sorted = accumulate(box2args(state.tokens),
                                             vector<TokenFreqRef>{},
                                             [](vector<TokenFreqRef> accum, TokenFreqRef arg) {
                                                 for (uint i = accum.size();;--i) {
                                                     if (!i || accum[i-1].get().second > arg.get().second) {
                                                         accum.insert(accum.begin()+i, arg);
                                                         break;
                                                     }
                                                 }
                                                 return accum;
                                             });
    assert(sorted.size() == state.tokens.size());

    for (TokenFreqRef x : sorted)
        printf("%u: %s\n", x.get().second, x.get().first.c_str());
}

int main(int argc, char *argv[]) {
    if (argc <= 1) {
        puts("Please, enter filenames to read form");
        return -1;
    }

    TokenizeState state = TokenizeState{ .tokens = {},
                                         .text = {},
                                         .processee = "",
                                         .type_processed = TokenTypes::SkipWhitespace };

    for (char* filename : iterate(argv, argv + argc).skip(1)) {
        printf("Opening a file %s\n", filename);
        Maybe<File> mb_file = File::open(filename);
        if (holds_alternative<monostate>(mb_file))
            return -1;

        vector<char> file_as_vec = unbox_ref(mb_file).to_vec();
        state = accumulate(box2args(file_as_vec),
                           state,
                           tokenize);
    }

    print_state_tokens(state);
}
