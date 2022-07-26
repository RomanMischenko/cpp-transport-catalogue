#pragma once

#include "transport_catalogue.h"
#include "map_renderer.h"
#include "json.h"

namespace request_handler {

struct BaseRequests {
    enum class QueryType {
        STOP,
        BUS_LINE,
        BUS_ROUTE
    };

    QueryType type;
    std::string name;
    std::vector<std::string> stops;
    geo::Coordinates coordinates;
    std::unordered_map<std::pair<std::string, std::string>, double, domain::detail::HasherWithString> road_distance_to_stop;
};

struct StatRequests {
    enum class QueryType {
        STOP,
        BUS,
        MAP
    };

    QueryType type;
    int id;
    std::string name;
};

class RequestHandler {
public:

    RequestHandler() = delete;
    RequestHandler(const transport_catalogue::TransportCatalogue& data, const map_renderer::MapRenderer& mr);

    json::Document DatabaseOutput();

    std::vector<BaseRequests>& GetBaseRequests();
    std::vector<StatRequests>& GetStatRequests();
private:
    std::vector<BaseRequests> base_requests_;
    std::vector<StatRequests> stat_requests_;

    const transport_catalogue::TransportCatalogue& data_;
    const map_renderer::MapRenderer& map_renderer_;
};

} // namespace request_handler