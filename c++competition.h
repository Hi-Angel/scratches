// just bunch of random function prototypes and aliases that may turn out to be useful in competetive programming.

#ifndef UTILS_H
#define UTILS_H

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <functional>
#include <map>
#include <set>
#include <utility>
#include <variant>
#include <vector>
#include <zlib.h>

template<typename T>
using Maybe      = std::variant<T, std::monostate>;
using u32        = uint32_t;
using u64        = uint64_t;
using u16        = uint16_t;
using ByteVec    = std::vector<char>;
using uchar      = unsigned char;

#define PERR_RET_ON_FAIL(func, arg1, arg2, arg3, msg)   \
    if (func(arg1, arg2, arg3) < 0) {                   \
        perror(msg);                                    \
        return -1;                                      \
    }

#define PERR_RET_ON_FAIL_VAL(val, msg)          \
    if (val < 0) {                              \
        perror(msg);                            \
        return -1;                              \
    }

#define DESERIALIZE_ARRAY(_arr)                 \
    for (uint _i = 0; _i < _arr.size(); ++_i) { \
        archive(_arr[_i]);                      \
    }

#define DESERIALIZE_SHR_ARR(_ptr, _arr_sz)                              \
    _ptr = std::shared_ptr<decltype(_ptr)::element_type[]>{new decltype(_ptr)::element_type[_arr_sz] }; \
    for (uint _i = 0; _i < _arr_sz; ++_i)                               \
        archive(_ptr[_i]);

#define DESERIALIZE_VECTOR(_vec,_vec_sz)                    \
    _vec.reserve(_vec_sz);                                  \
    _vec.clear(); /* in case the object is second-hand */   \
    decltype(_vec)::value_type tmp;                         \
    for (uint _i = 0; _i < _vec_sz; ++_i) {                 \
        archive(tmp);                                       \
        _vec.push_back(tmp);                                \
    }

#define SERIALIZE_SHRPTR(_ptr, _sz) \
    for (uint i = 0; i < _sz; ++i)  \
        archive(_ptr[i]);

// implements a foreach-like cycle over `name_postfix` stored in object `obj`
// The requirement to work is that `obj` holds sz_##name_postfix as length of the ptr.
#define FOREACH_SHRPTR(iter, obj, name_postfix)                         \
    if (obj.n_##name_postfix > 0)                                       \
        for (auto [iter, _i] = std::tuple<decltype(obj.name_postfix)::element_type*, uint>{&obj.name_postfix[0], 0}; \
             _i < obj.n_##name_postfix && (iter = &obj.name_postfix[_i]); \
             ++_i)

template<typename T>
std::vector<T> operator+(const std::vector<T>& lhs, const std::vector<T>& rhs) {
    std::vector<T> vec;
    vec.insert(vec.end(), lhs.begin(), lhs.end());
    vec.insert(vec.end(), rhs.begin(), rhs.end());
    return vec;
}

template<typename T, size_t Sz>
std::vector<T> operator+(const std::array<T, Sz>& lhs, const std::vector<T>& rhs) {
    std::vector<T> vec;
    vec.insert(vec.end(), lhs.begin(), lhs.end());
    vec.insert(vec.end(), rhs.begin(), rhs.end());
    return vec;
}

template<typename T, size_t Sz>
std::vector<T> operator+(const std::vector<T>& lhs, const std::array<T, Sz>& rhs) {
    std::vector<T> vec;
    vec.insert(vec.end(), lhs.begin(), lhs.end());
    vec.insert(vec.end(), rhs.begin(), rhs.end());
    return vec;
}

template<typename T>
void assert_n_print(bool t, const T& to_print) {
    if (!t) {
        printf("%s\n", to_print.show(0).c_str());
        throw("assertion failed");
    }
}

template<typename Key, typename Val>
Maybe<Key> lookup_key(std::map<Key, Val> m, Val v) {
    auto it = std::find_if(m.begin(), m.end(), [&v](std::pair<Key,Val> p) {return p.second == v;});
    return (it == m.end())?
        Maybe<Key>{std::monostate{}} : Maybe<Key>{it->first};
}

template <template<typename> class container,
          typename T>
bool is_infix(const T& value, const container<T>& c) {
    return std::find(c.begin(), c.end(), value) != c.end();
}

template <template<typename> class container,
          typename T>
bool is_infix(const container<T> subset, const container<T> set) {
    return std::search(set.begin(), set.end(), subset.begin(), subset.end()) != set.end();
}

template <typename container, typename Ret>
std::vector<Ret> transform (const container& in,
                            const std::function<Ret(const decltype(in[0]))>& op) {
    std::vector<Ret> ret;
    ret.reserve(ret.size());
    for (auto& elem : in)
        ret.push_back(op(elem));
    return ret;
}

// like std::accumulate, without iterators
template <typename container, typename Accum, typename Func>
Accum accumulate (const container& in,
                  Accum acc,
                  const Func f) {
    for (auto& elem : in)
        acc = f(acc, elem);
    return acc;
}

template <template<typename> class container,
          typename In1,
          typename In2,
          typename Ret>
container<Ret> zip_with (const container<In1>& in1, const container<In2>& in2, const std::function<Ret(const In1, const In2)>& op) {
    assert(in1.size() == in2.size());
    container<Ret> ret;
    for (uint i = 0; i < in1.size(); ++i)
        ret.push_back(op(in1[i], in2[i]));
    return ret;
}

template<size_t Sz, typename From>
constexpr std::array<uchar, Sz> byte_array(const From& v) {
    static_assert(sizeof(v) == Sz);
    std::array<uchar, Sz> ret = {};
    *reinterpret_cast<From*>(&ret[0]) = v;
    return ret;
}

template<typename T>
constexpr T div_ceil(T t1, T t2) { return (t1 + t2 - 1) / t2; }

template<typename iterator>
bool do_segments_overlap(iterator a_start, iterator a_end, iterator b_start, iterator b_end) {
    return !(a_end < b_start || a_start > b_end);
}

#endif //UTILS_H
