#pragma once
#include "geo.h"

#include <iostream>
#include <string>
#include <deque>
#include <vector>
#include <unordered_map>

namespace domain {

namespace detail {

class HasherWithString {
public:
    size_t operator()(const std::pair<std::string, std::string>& stops) const{
        std::string str = stops.first + stops.second;
        return std::hash<std::string>{}(str);
    }
};

} // namespace detail

} // namespace domain
