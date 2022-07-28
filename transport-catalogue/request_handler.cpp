#include "request_handler.h"
#include "json_builder.h"

#include <algorithm>
#include <sstream>

// подключаем ""s
using std::literals::string_literals::operator""s;

request_handler::RequestHandler::RequestHandler(const transport_catalogue::TransportCatalogue& data, const map_renderer::MapRenderer& mr) 
: data_(data)
, map_renderer_(mr)
{}

json::Document request_handler::RequestHandler::DatabaseOutput() {
    // вычисляем количество запросов к базе данных
    size_t query_count = stat_requests_.size();
    // создаем массив ответов
    json::Builder tmp_response;
    tmp_response.StartArray();
    //std::vector<json::Node> tmp_response(query_count);
    for (size_t i = 0; i < query_count; ++i) {
        //json::Dict answer;
        tmp_response.StartDict();
        const auto& query = stat_requests_.at(i);
        if (query.type == request_handler::StatRequests::QueryType::STOP) {
            if (data_.FindStop(query.name) == nullptr) {
                tmp_response.Key("request_id"s).Value(query.id);
                tmp_response.Key("error_message"s).Value("not found"s);
                tmp_response.EndDict();
                continue;
            }
            std::vector<domain::Route *> buses{data_.FindStop(query.name)->buses_for_stop.begin(), 
                    data_.FindStop(query.name)->buses_for_stop.end()};
                // сортируем перед выводом
                auto it_begin = buses.begin();
                auto it_end = buses.end();
                std::sort(it_begin, it_end, [](const auto& lhs, const auto& rhs){
                    return lhs->route_name < rhs->route_name;
            });
            json::Array tmp_bus_name;
            for (const auto& bus : buses) {
                tmp_bus_name.push_back({bus->route_name});
            }
            tmp_response.Key("buses"s).Value(tmp_bus_name);
            tmp_response.Key("request_id"s).Value(query.id);
            tmp_response.EndDict();
        } else if (query.type == request_handler::StatRequests::QueryType::BUS) {
            if (data_.FindRoute(query.name) == nullptr) {
                tmp_response.Key("request_id"s).Value(query.id);
                tmp_response.Key("error_message"s).Value("not found"s);
                tmp_response.EndDict();
                continue;
            }
            domain::Route* route = data_.FindRoute(query.name);
            // уникальные остановки
            std::unordered_set<std::string> unique_stops;
            //вычисляем дистанции
            double geographical_distance = 0.0;
            double roat_distance = 0.0;
            size_t i_end = route->stops.size() - 1;
            for (size_t i = 0; i < i_end; ++i) {
                unique_stops.insert(route->stops.at(i)->stop_name);
                geographical_distance += data_.GetGeographicalDistanceBetweenStops(*(route->stops.at(i)), *(route->stops.at(i + 1)));
                double tmp_dist = data_.GetRoadDistanceBetweenStops(*(route->stops.at(i)), *(route->stops.at(i + 1)));
                // если искомый порядок существует
                if (tmp_dist != 0.0) {
                    roat_distance += tmp_dist;
                } else { // если нет, то ищем наоборот
                    roat_distance += data_.GetRoadDistanceBetweenStops(*(route->stops.at(i + 1)), *(route->stops.at(i)));
                }
            }
            tmp_response.Key("curvature"s).Value(roat_distance/geographical_distance);
            tmp_response.Key("request_id"s).Value(query.id);
            tmp_response.Key("route_length"s).Value(roat_distance);
            tmp_response.Key("stop_count"s).Value(static_cast<int>(route->stops.size()));
            tmp_response.Key("unique_stop_count"s).Value(static_cast<int>(unique_stops.size()));
            tmp_response.EndDict();
        } else if (query.type == request_handler::StatRequests::QueryType::MAP) {
            std::ostringstream output;
            map_renderer_.Draw(output);
            tmp_response.Key("map"s).Value(output.str());
            tmp_response.Key("request_id"s).Value(query.id);
            tmp_response.EndDict();
        }
    }
    tmp_response.EndArray();
    json::Document response{std::move(tmp_response.Build())};
    return response;
}

std::vector<request_handler::BaseRequests>& request_handler::RequestHandler::GetBaseRequests() {
    return base_requests_;
}

std::vector<request_handler::StatRequests>& request_handler::RequestHandler::GetStatRequests(){
    return stat_requests_;
}

