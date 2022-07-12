#include <sstream>

#include "json_reader.h"
#include "request_handler.h"
#include "map_renderer.h"

int main() {
    transport_catalogue::TransportCatalogue data;
    json::Document doc = json::Load(std::cin);
    json_reader::jsonReader(doc, data);
    json::Document output = request_handler::DatabaseOutput(doc, data);
    json::Print(output, std::cout);
    return 0;
}