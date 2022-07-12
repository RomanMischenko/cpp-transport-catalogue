#include "request_handler.h"
#include "map_renderer.h"

#include <algorithm>
#include <sstream>

// подключаем ""s
using std::literals::string_literals::operator""s;

json::Document request_handler::DatabaseOutput(const json::Document& doc, transport_catalogue::TransportCatalogue& data) {
    // вычисляем количество запросов к базе данных
    size_t query_count = doc.GetRoot().AsMap().at("stat_requests"s).AsArray().size();
    // создаем массив ответов
    std::vector<json::Node> tmp_response(query_count);
    
    for (size_t i = 0; i < query_count; ++i) {
        json::Dict answer;
        // получаем доступ к базе json
        const auto& query = doc.GetRoot().AsMap().at("stat_requests"s).AsArray().at(i).AsMap();
        if (query.at("type").AsString() == "Stop"s) { // если тип запроса Stop
            if (data.FindStop(query.at("name"s).AsString()) == nullptr) {
                answer.emplace(
                    "request_id"s,
                    query.at("id").AsInt()
                );
                answer.emplace(
                    "error_message"s,
                    "not found"s
                );
                tmp_response.at(i) = answer;
                continue;
            }
            std::vector<transport_catalogue::Route *> buses{data.FindStop(query.at("name").AsString())->buses_for_stop.begin(), 
                    data.FindStop(query.at("name").AsString())->buses_for_stop.end()};
                // сортируем перед выводом
                auto it_begin = buses.begin();
                auto it_end = buses.end();
                std::sort(it_begin, it_end, [](const auto& lhs, const auto& rhs){
                    return lhs->route_name < rhs->route_name;
                });
            json::Array tmp_bus_name;
            for (const auto& bus : buses) {
                tmp_bus_name.push_back(bus->route_name);
            }
            answer.emplace(
                "buses"s,
                tmp_bus_name
            );
            answer.emplace(
                "request_id"s,
                query.at("id").AsInt()
            );
            tmp_response.at(i) = answer;
        } else if (query.at("type").AsString() == "Bus"s) {
            if (data.FindRoute(query.at("name"s).AsString()) == nullptr) {
                answer.emplace(
                    "request_id"s,
                    query.at("id").AsInt()
                );
                answer.emplace(
                    "error_message"s,
                    "not found"s
                );
                tmp_response.at(i) = answer;
                continue;
            }
            transport_catalogue::Route* route = data.FindRoute(query.at("name").AsString());
            // уникальные остановки
            std::unordered_set<std::string> unique_stops;
            //вычисляем дистанции
            double geographical_distance = 0.0;
            double roat_distance = 0.0;
            size_t i_end = route->stops.size() - 1;
            for (size_t i = 0; i < i_end; ++i) {
                unique_stops.insert(route->stops.at(i)->stop_name);
                geographical_distance += data.GetGeographicalDistanceBetweenStops(*(route->stops.at(i)), *(route->stops.at(i + 1)));
                double tmp_dist = data.GetRoadDistanceBetweenStops(*(route->stops.at(i)), *(route->stops.at(i + 1)));
                // если искомый порядок существует
                if (tmp_dist != 0.0) {
                    roat_distance += tmp_dist;
                } else { // если нет, то ищем наоборот
                    roat_distance += data.GetRoadDistanceBetweenStops(*(route->stops.at(i + 1)), *(route->stops.at(i)));
                }
            }
            answer.emplace(
                "curvature"s,
                roat_distance/geographical_distance
            );
            answer.emplace(
                "request_id"s,
                query.at("id").AsInt()
            );
            answer.emplace(
                "route_length"s,
                roat_distance
            );
            answer.emplace(
                "stop_count"s,
                static_cast<int>(route->stops.size())
            );
            answer.emplace(
                "unique_stop_count"s,
                static_cast<int>(unique_stops.size())
            );
            tmp_response.at(i) = answer;
        } else if (query.at("type").AsString() == "Map"s) {
            std::ostringstream output;
            map_renderer::MapRenderer(doc, output);
            answer.emplace(
                "map"s,
                output.str()
            );
            answer.emplace(
                "request_id"s,
                query.at("id").AsInt()
            );
            tmp_response.at(i) = answer;
        }
        
    }
    json::Document response{std::move(tmp_response)};
    return response;
}
