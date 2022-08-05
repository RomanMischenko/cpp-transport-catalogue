#pragma once

#include "transport_catalogue.h"
#include "map_renderer.h"
#include "json.h"
#include "transport_router.h"

namespace request_handler {

struct StatRequests {
    enum class QueryType {
        STOP,
        BUS,
        MAP,
        ROUTE,
        UNKNOWN_TYPE
    };

    struct Route {
        std::string from;
        std::string to;
    };

    QueryType type;
    int id;
    std::string name;
    std::optional<Route> route;
};

class RequestHandler {
public:

    RequestHandler() = delete;
    RequestHandler(const transport_catalogue::TransportCatalogue& data
                , const map_renderer::MapRenderer& mr
                , const transport_router::TransportRouter& router);

    json::Document DatabaseOutput();

    std::vector<StatRequests>& GetStatRequests();
private:
    std::vector<StatRequests> stat_requests_;

    const transport_catalogue::TransportCatalogue& data_;
    const map_renderer::MapRenderer& map_renderer_;
    const transport_router::TransportRouter& tr_router_;
    const graph::Router<transport_router::TransportRouter::Weight> graph_router_;
};

} // namespace request_handler