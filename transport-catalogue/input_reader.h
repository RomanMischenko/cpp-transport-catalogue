#pragma once

#include "transport_catalogue.h"

#include <iostream>
#include <string>
#include <deque>
#include <vector>

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
    Coordinates coordinates;
    std::unordered_map<std::pair<std::string, std::string>, double, HasherWithString> road_distance_to_stop;
};

void QueryStringProcessing(Query& query, std::string& text);

void UpdateDatabase(std::vector<Query>& queries, TransportCatalogue& data);

void InputReader(std::istream& input, TransportCatalogue& data);