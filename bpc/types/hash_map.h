#pragma once

#include <ankerl/unordered_dense.h>

template<class Key, class T>
using HashMap = ankerl::unordered_dense::map<Key, T>;

// template<class Key, class Hash = robin_hood::hash<Key>>
// inline void hash_combine(std::size_t& s, const Key& v)
// {
//     s ^= Hash{}(v) + 0x9e3779b9 + (s << 6) + (s >> 2);
// }

// template<class KeyIterator, class Hash = robin_hood::hash<KeyIterator>>
// inline std::size_t hash_range(KeyIterator begin, KeyIterator end)
// {
//     size_t x = 0;
//     for (; begin != end; ++begin)
//     {
//         hash_combine(x, *begin);
//     }
//     return x;
// }
