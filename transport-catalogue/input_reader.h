#pragma once

#include "transport_catalogue.h"

#include <iostream>
#include <string>
#include <deque>
#include <vector>

namespace transport_catalogue {
namespace input_reader {

enum class QueryType {
    STOP,
    BUS_LINE,
    BUS_ROUTE
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

struct Query {
    QueryType type;
    std::string name;
    std::vector<std::string> stops;
    coordinates::Coordinates coordinates;
    std::unordered_map<std::pair<std::string, std::string>, double, detail::HasherWithString> road_distance_to_stop;
};

void InputReader(std::istream& input, ::transport_catalogue::TransportCatalogue& data);

} // namespace input_reader
} // namespace transport_catalogue