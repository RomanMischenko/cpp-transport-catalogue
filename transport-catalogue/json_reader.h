#pragma once
#include "json.h"
#include "transport_catalogue.h"
#include "request_handler.h"
#include "map_renderer.h"
#include "transport_router.h"
namespace json_reader {

class jsonReader {
public:
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

    jsonReader() = delete;

    jsonReader(const json::Document& doc,
                transport_catalogue::TransportCatalogue& data,
                map_renderer::MapRenderer& mr);

    jsonReader(const json::Document& doc,
                request_handler::RequestHandler& rh);

private:
    std::vector<BaseRequests> ReadBaseRequests(const json::Document& doc);
    void ReadMapSettings(const json::Document& doc, map_renderer::MapRenderer& mr);

    void ReadStatRequests(const json::Document& doc, request_handler::RequestHandler& rh);
    void ReadRoutingSettings(const json::Document& doc, transport_catalogue::TransportCatalogue& data);
    void ReadStopAndBus(const json::Document& doc, map_renderer::MapRenderer& mr);
};

} // namespace input_reader