#pragma once

#include "transport_catalogue.h"

#include <iostream>
#include <string>
#include <deque>
#include <vector>

namespace input_reader {
namespace detail {

enum class QueryType {
    STOP,
    BUS_LINE,
    BUS_ROUTE
};

class HasherWithString {
public:
    size_t operator()(const std::pair<std::string, std::string>& stops) const{
        std::string str = stops.first + stops.second;
        return std::hash<std::string>{}(str);
    }
};

struct Query {
    QueryType type;
    std::string name;
    std::vector<std::string> stops;
    coordinates::Coordinates coordinates;
    std::unordered_map<std::pair<std::string, std::string>, double, HasherWithString> road_distance_to_stop;
};

} // namespace detail

void ProcessingRequestStop(detail::Query& query, const std::string& text);

void ProcessingRequestBus(detail::Query& query, const std::string& text);

void QueryStringProcessing(detail::Query& query, const std::string& text);

void UpdateDatabase(std::vector<detail::Query>& queries, transport_catalogue::TransportCatalogue& data);

void InputReader(std::istream& input, transport_catalogue::TransportCatalogue& data);

} // namespace input_reader