#pragma once
#include "json.h"
#include "transport_catalogue.h"
#include "request_handler.h"
#include "map_renderer.h"

namespace json_reader {

class jsonReader {
public:
    jsonReader() = delete;

    jsonReader(const json::Document& doc, 
                transport_catalogue::TransportCatalogue& data,
                request_handler::RequestHandler& rh,
                map_renderer::MapRenderer& mr);
private:
    void ReadJson(const json::Document& doc, 
                    transport_catalogue::TransportCatalogue& data,
                    request_handler::RequestHandler& rh, 
                    map_renderer::MapRenderer& mr);

    void ReadBaseRequests(const json::Document& doc, request_handler::RequestHandler& rh);
    void ReadStatRequests(const json::Document& doc, request_handler::RequestHandler& rh);
    void ReadMapSettings(const json::Document& doc, map_renderer::MapRenderer& mr);
    void ReadStopAndBus(const json::Document& doc, map_renderer::MapRenderer& mr);
};

} // namespace input_reader