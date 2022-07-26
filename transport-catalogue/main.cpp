#include <sstream>

#include "json_reader.h"
#include "request_handler.h"

int main() {
    transport_catalogue::TransportCatalogue data;
    json::Document doc = json::Load(std::cin);
    map_renderer::MapRenderer mr;
    request_handler::RequestHandler rh(data, mr);
    json_reader::jsonReader jr(doc, data, rh, mr);
    json::Document out = rh.DatabaseOutput();
    json::Print(out, std::cout);
    return 0;
}