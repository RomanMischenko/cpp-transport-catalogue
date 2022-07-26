#pragma once
#include "geo.h"

#include <iostream>
#include <string>
#include <deque>
#include <vector>
#include <unordered_set>

namespace domain {

struct Stop;

struct Route {
    std::string route_name;
    std::vector<Stop *> stops;
};

struct Stop {
    std::string stop_name;
    geo::Coordinates coordinates;
    std::unordered_set<Route *> buses_for_stop = {};
};

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
