#include "request_handler.h"

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
    std::vector<json::Node> tmp_response(query_count);
    for (size_t i = 0; i < query_count; ++i) {
        json::Dict answer;
        const auto& query = stat_requests_.at(i);
        if (query.type == request_handler::StatRequests::QueryType::STOP) {
            if (data_.FindStop(query.name) == nullptr) {
                answer.emplace(
                    "request_id"s,
                    query.id
                );
                answer.emplace(
                    "error_message"s,
                    "not found"s
                );
                tmp_response.at(i) = answer;
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
                tmp_bus_name.push_back(bus->route_name);
            }
            answer.emplace(
                "buses"s,
                tmp_bus_name
            );
            answer.emplace(
                "request_id"s,
                query.id
            );
            tmp_response.at(i) = answer;
        } else if (query.type == request_handler::StatRequests::QueryType::BUS) {
            if (data_.FindRoute(query.name) == nullptr) {
                answer.emplace(
                    "request_id"s,
                    query.id
                );
                answer.emplace(
                    "error_message"s,
                    "not found"s
                );
                tmp_response.at(i) = answer;
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
            answer.emplace(
                "curvature"s,
                roat_distance/geographical_distance
            );
            answer.emplace(
                "request_id"s,
                query.id
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
        } else if (query.type == request_handler::StatRequests::QueryType::MAP) {
            std::ostringstream output;
            map_renderer_.Draw(output);
            answer.emplace(
                "map"s,
                output.str()
            );
            answer.emplace(
                "request_id"s,
                query.id
            );
            tmp_response.at(i) = answer;
        }
    }
    json::Document response{std::move(tmp_response)};
    return response;
}

std::vector<request_handler::BaseRequests>& request_handler::RequestHandler::GetBaseRequests() {
    return base_requests_;
}

std::vector<request_handler::StatRequests>& request_handler::RequestHandler::GetStatRequests(){
    return stat_requests_;
}

