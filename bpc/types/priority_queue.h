#pragma once

#include "types/vector.h"
#include <algorithm>
#include <queue>

template<class T, class Compare = std::greater<T>>
class PriorityQueue: public std::priority_queue<T, Vector<T>, Compare>
{
  public:
//    inline auto& data()
//    {
//        return std::priority_queue<T, Vector<T>, Compare>::c;
//    }

    inline void clear()
    {
        std::priority_queue<T, Vector<T>, Compare>::c.clear();
    }

    inline bool contains(const T& x)
    {
        const auto& container = std::priority_queue<T, Vector<T>, Compare>::c;
        return std::find(container.begin(), container.end(), x) != container.end();
    }
};
