#include "stat_reader.h"

#include <sstream>
#include <algorithm>

void stat_reader::DatabaseOutput(std::istream& input, transport_catalogue::TransportCatalogue& data) {
    // обрабатываем целое число
    std::string text_tmp;
    std::getline(input, text_tmp);
    int number_of_requests = std::stoi(text_tmp);

    for (int i = 0; i < number_of_requests; ++i) {
        std::getline(input, text_tmp);
        auto pos_queries_type_begin = text_tmp.find_first_not_of(' ');
        auto pos_queries_type_end = text_tmp.find_first_of(' ', pos_queries_type_begin + 1);
        auto type = text_tmp.substr(pos_queries_type_begin, (pos_queries_type_end - pos_queries_type_begin));

        auto pos_queries_begin = text_tmp.find_first_not_of(' ', pos_queries_type_end + 1);
        auto pos_queries_end = text_tmp.find_last_not_of(' ');
        auto queries = text_tmp.substr(pos_queries_begin, (pos_queries_end - pos_queries_begin) + 1);

        if (type == "Bus") {
            std::cout << data.RouteInfo(queries).str() << std::endl;
        } else if (type == "Stop") {
            if (data.FindStop(queries) == nullptr) {
                std::cout << "Stop " << queries << ": not found" << std::endl;
            } else if (data.FindStop(queries)->buses_for_stop.size() == 0) {
                std::cout << "Stop " << queries << ": no buses" << std::endl;
            } else {
                std::vector<transport_catalogue::detail::Route *> stops{data.FindStop(queries)->buses_for_stop.begin(), 
                    data.FindStop(queries)->buses_for_stop.end()};
                // сортируем перед выводом
                auto it_begin = stops.begin();
                auto it_end = stops.end();
                std::sort(it_begin, it_end, [](const auto& lhs, const auto& rhs){
                    return lhs->route_name < rhs->route_name;
                });
                std::cout << "Stop " << queries << ": buses ";
                for (int i = 0; i < (stops.size() - 1); ++i) {
                    std::cout << stops[i]->route_name << " ";
                }
                std::cout << stops[stops.size() - 1]->route_name << std::endl;
            }
        }
    }
}