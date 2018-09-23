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
#include <variant>
#include <vector>

using namespace std;

using f32 = float;
using i32 = int32_t;
using i16 = int16_t;
using u32 = uint32_t;

// some terms:
f32 KEYW_FREQ = 0.1; // arbitrary percentage, for prototyping purposes, for a keyword
// a keyword: reference to one of "most frequent words"
i16 SZ_WIN = 6; // arbitrary number, for prototyping purposes
// window: a window-size tuple, where elements are either keywords or 0-indexed non-keyword that may be repeated throughout the window.

template<class T>
using Maybe = variant<monostate,T>;
using Freq         = u32;
using Token        = string;
using TokensFreq   = map<Token, Freq>;
using TokenFreqRef = reference_wrapper<pair<const Token, Freq>>;
using KeywordRef   = TokenFreqRef;

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

// an "accumulate" without requirement for copy-assignability of accum
template <class InputIterator, class T, class BinOp>
T fold(InputIterator first, InputIterator last, T&& accum, BinOp binop) {
  while (first != last) {
    accum = binop(std::move(accum), *first);
    ++first;
  }
  return move(accum);
}

enum class TokenTypes {
    // todo: do I really need to take into account newlines in tokens?
    SkipWhitespace, // a whitespace that does not belong to Specials. Can be used as initial state
    AlphaNumeric, // alphanumeric, _
    Specials // non-alphanumeric, but specials, _, spaces, newlines.
        //todo: add "Finished" to move last processee from processee
};

struct TokenizeState {
    TokenizeState(const TokenizeState&)       = delete;
    TokenizeState(TokenizeState&&)            = default;
    TokenizeState& operator=(TokenizeState&&) = default;

    TokensFreq tokens;
    vector<TokenFreqRef> text;
    string processee;
    TokenTypes type_processed;

    TokenizeState&& operator()(string processee_arg, TokenTypes type_processed_arg) {
        processee = processee_arg;
        type_processed = type_processed_arg;
        return move(*this);
    }

    TokenizeState operator()(TokenTypes type_processed_arg) {
        type_processed = type_processed_arg;
        return move(*this);
    }
};

bool is_alphanumeric(char ch) { return isdigit(ch) || isalpha(ch); }

TokenizeState tokenize(TokenizeState&& state, char ch) {
    switch (state.type_processed) {
        case TokenTypes::AlphaNumeric: {
            if (is_alphanumeric(ch)) {
                state.processee.push_back(ch);
                return move(state);
            } else { // finish a token, start a new one
                get_or_insert(state.tokens, state.processee, 0u).second += 1u;
                state.text.push_back(ref(*state.tokens.find(state.processee)));
                return tokenize(state("", isspace(ch)? TokenTypes::SkipWhitespace : TokenTypes::Specials),
                                ch);
            }
        }
        case TokenTypes::Specials: {
            if (!is_alphanumeric(ch)) {
                state.processee.push_back(ch);
                return move(state);
            } else { // finish a token, start a new one
                get_or_insert(state.tokens, state.processee, 0u).second += 1u;
                state.text.push_back(ref(*state.tokens.find(state.processee)));
                return tokenize(state("", isspace(ch)? TokenTypes::SkipWhitespace : TokenTypes::AlphaNumeric),
                                ch);
            }
        }
        case TokenTypes::SkipWhitespace: {
            if (isspace(ch))
                return move(state);
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
struct Slice {
    T* start, *past_end;

    constexpr
    Slice(T* start, T* past_end): start(start), past_end(past_end) {}

    constexpr
    Slice(vector<T>& vec): start(&vec[0]), past_end(&vec[vec.size()-1]) {}

    constexpr
    T* begin() const { return start; }
    constexpr
    T* end() const { return past_end; }
    constexpr
    uint size() { return past_end - start; }

    constexpr
    bool operator!=(Slice<T> rhs) const {
        return start != rhs.start || past_end != rhs.past_end;
    }

    constexpr
    T& operator[](uint i) const {
        return start[i];
    }
};

template<class T, class Accum>
Accum foreach_frame(function<Accum(Accum,Slice<T>)> f,
                    const Slice<T>& range, Accum&& accum,
                    const uint sz_frame, const uint sz_step) {
    Slice<T> frame(range.start,
                   (range.start + sz_frame >= range.past_end)? range.past_end : range.past_end - sz_frame);
    do {
        accum = f(move(accum), frame);
        const auto advance_frame = [sz_step, &range](T*& edge) {
                edge = (edge + sz_step >= range.past_end)? range.past_end : edge + sz_step;
            };
        advance_frame(frame.start);
        advance_frame(frame.past_end);
    } while(frame.start < range.past_end);
    return move(accum);
}

// In window what's not a keyword is an arg. Args can be repeated, so from left to
// right args being assigned an index. todo: example.
struct ArgIndex {
    uint val; // 0-based
};

struct Window {
    vector<variant<ArgIndex, KeywordRef>> store;
    uint n_args; // number of "ArgIndex"s

    void push(ArgIndex i) {
        store.push_back(i);
        if (i.val+1 > n_args) // new argument
            n_args++;
    }
    void push(KeywordRef key_ref) {store.push_back(key_ref);}
};


template<class T>
T& get_ref(Maybe<T>& mb_t) {
    T& ret = get<1>(mb_t);
    return ret;
}

#define box2args(container) container.begin(), container.end()

void print_state_tokens(TokenizeState& state) {
    vector<TokenFreqRef> sorted = fold(box2args(state.tokens),
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
        printf("%u: '%s'\n", x.get().second, x.get().first.c_str());
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

        vector<char> file_as_vec = get_ref(mb_file).to_vec();
        state = fold(box2args(file_as_vec),
                     move(state),
                     tokenize);
    }

    function<vector<Window>(vector<Window> windows, const Slice<TokenFreqRef>)> collect_windows = [](vector<Window> windows, const Slice<TokenFreqRef> s) {
            windows.push_back(Window());
            Window& window = windows.back();

            for(TokenFreqRef t : s) {
                if (t.get().second >= KEYW_FREQ) {
                    window.push(t);
                } else {
                    // if word in window: use its index, otherwise use n_args+1
                    for (uint i = 0; i < window.store.size()-1; ++i)
                        if (t.get().first == s[i].get().first)
                            window.push(ArgIndex{get<ArgIndex>(window.store[i])});
                        else
                            window.push(ArgIndex{window.n_args});
                }
            }
            return windows;
        };
    vector<Window> windows = foreach_frame(collect_windows, Slice(state.text), {}, SZ_WIN, 1);
}
