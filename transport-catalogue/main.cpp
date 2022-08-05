#include <sstream>

#include "json_reader.h"
#include "request_handler.h"
#include "transport_router.h"

using namespace std;


int main() {
    json::Document doc = json::Load(std::cin);
    // создаем транспортный справочник
    transport_catalogue::TransportCatalogue data;
    map_renderer::MapRenderer mr;
    json_reader::jsonReader jr_1(doc, data, mr);
    // создаем каталог маршрутов
    transport_router::TransportRouter rt(data);
    rt.BuildGraph();
    // обрабатываем запросы к базе
    request_handler::RequestHandler rh(data, mr, rt);
    json_reader::jsonReader jr_2(doc, rh);
    
    //const std::unordered_map<std::string_view, std::pair<size_t, size_t>>& stops_id_by_name_in_graph = rt.GetStopsIDInGraph();
    //auto find_route_1 = router.BuildRoute(stops_id_by_name_in_graph.at("Lipetskaya ulitsa 40").first, stops_id_by_name_in_graph.at("Lipetskaya ulitsa 40").first);
    //auto find_route_2 = router.BuildRoute(stops_id_by_name_in_graph.at("Biryulyovo Zapadnoye").first, stops_id_by_name_in_graph.at("Pokrovskaya").first);


    json::Document out = rh.DatabaseOutput();
    json::Print(out, std::cout);
    return 0;
}