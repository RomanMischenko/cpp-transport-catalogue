#include "request_handler.h"
#include "json_builder.h"

#include <algorithm>
#include <sstream>

// подключаем ""s
using std::literals::string_literals::operator""s;
using std::literals::string_view_literals::operator""sv;

using Query = request_handler::StatRequests;
using Data = transport_catalogue::TransportCatalogue;
using MapRenderer = map_renderer::MapRenderer;
using TransportRouter = transport_router::TransportRouter;
using GraphRouter = graph::Router<transport_router::TransportRouter::Weight>;

request_handler::RequestHandler::RequestHandler(const Data& data, const MapRenderer& mr, const transport_router::TransportRouter& router) 
: data_(data)
, map_renderer_(mr)
, tr_router_(router)
, graph_router_(tr_router_.BuildRouter())
{}

bool ProcessingQueryTypeStop(const Data& data, const Query& query, json::Builder& tmp_response);
bool ProcessingQueryTypeBus(const Data& data, const Query& query, json::Builder& tmp_response);
bool ProcessingQueryTypeMap(const MapRenderer& map_renderer_, const Query& query, json::Builder& tmp_response);
bool ProcessingQueryTypeRoute(const TransportRouter& tr_router, const GraphRouter& rg_router, const Query& query, json::Builder& tmp_response);
bool ProcessingQueryTypeUnknownRequestType(const Query& query, json::Builder& tmp_response);

json::Document request_handler::RequestHandler::DatabaseOutput() {
    // вычисляем количество запросов к базе данных
    size_t query_count = stat_requests_.size();
    // создаем массив ответов
    json::Builder tmp_response;
    tmp_response.StartArray();
    for (size_t i = 0; i < query_count; ++i) {
        const Query& query = stat_requests_.at(i);
        if (query.type == request_handler::StatRequests::QueryType::STOP) {
            if (ProcessingQueryTypeStop(data_, query, tmp_response) == false) {
                continue;
            }
        } else if (query.type == request_handler::StatRequests::QueryType::BUS) {
            if (ProcessingQueryTypeBus(data_, query, tmp_response) == false) {
                continue;
            }
        } else if (query.type == request_handler::StatRequests::QueryType::MAP) {
            if (ProcessingQueryTypeMap(map_renderer_, query, tmp_response) == false) {
                continue;
            }
        } else if (query.type == request_handler::StatRequests::QueryType::ROUTE) {
            if (ProcessingQueryTypeRoute(tr_router_, graph_router_, query, tmp_response) == false) {
                continue;
            }
        } else {
            if (ProcessingQueryTypeUnknownRequestType(query, tmp_response) == false) {
                continue;
            }
        }
    }
    tmp_response.EndArray();
    json::Document response{std::move(tmp_response.Build())};
    return response;
}

std::vector<request_handler::StatRequests>& request_handler::RequestHandler::GetStatRequests(){
    return stat_requests_;
}

bool ProcessingQueryTypeStop(const Data& data, const Query& query, json::Builder& tmp_response) {
    tmp_response.StartDict();
    if (data.FindStop(query.name) == nullptr) {
        tmp_response.Key("request_id"s).Value(query.id);
        tmp_response.Key("error_message"s).Value("not found"s);
        tmp_response.EndDict();
        return false;
    }
    std::vector<domain::Route *> buses{data.FindStop(query.name)->buses_for_stop.begin(), 
            data.FindStop(query.name)->buses_for_stop.end()};
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
    return true;
}

bool ProcessingQueryTypeBus(const Data& data, const Query& query, json::Builder& tmp_response) {
    tmp_response.StartDict();
    if (data.FindRoute(query.name) == nullptr) {
        tmp_response.Key("request_id"s).Value(query.id);
        tmp_response.Key("error_message"s).Value("not found"s);
        tmp_response.EndDict();
        return false;
    }
    domain::Route* route = data.FindRoute(query.name);
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
    tmp_response.Key("curvature"s).Value(roat_distance/geographical_distance);
    tmp_response.Key("request_id"s).Value(query.id);
    tmp_response.Key("route_length"s).Value(roat_distance);
    tmp_response.Key("stop_count"s).Value(static_cast<int>(route->stops.size()));
    tmp_response.Key("unique_stop_count"s).Value(static_cast<int>(unique_stops.size()));
    tmp_response.EndDict();
    return true;
}

bool ProcessingQueryTypeMap(const MapRenderer& map_renderer_, const Query& query, json::Builder& tmp_response) {
    tmp_response.StartDict();
    std::ostringstream output;
    map_renderer_.Draw(output);
    tmp_response.Key("map"s).Value(output.str());
    tmp_response.Key("request_id"s).Value(query.id);
    tmp_response.EndDict();
    return true;
}

bool ProcessingQueryTypeRoute(const TransportRouter& tr_router, const GraphRouter& gr_router, const Query& query, json::Builder& tmp_response) {
    tmp_response.StartDict();
    const std::unordered_map<std::string_view, std::pair<size_t, size_t>>& stops_id_by_name_in_graph = tr_router.GetStopsIDInGraph();
    std::string_view stop_from = query.route.value().from;
    std::string_view stop_to = query.route.value().to;
    auto find_route = gr_router.BuildRoute(stops_id_by_name_in_graph.at(stop_from).first, stops_id_by_name_in_graph.at(stop_to).first);
    if (find_route.has_value() == false) {
        tmp_response.Key("request_id"s).Value(query.id);
        tmp_response.Key("error_message"s).Value("not found"s);
        tmp_response.EndDict();
        return false;
    }

    if (find_route.has_value() == true) {
        if (find_route.value().edges.size() == 0) {
            tmp_response.Key("items"s).StartArray().EndArray();
            tmp_response.Key("request_id"s).Value(query.id);
            tmp_response.Key("total_time"s).Value(0);
            tmp_response.EndDict();
            return false;
        } else {
            tmp_response.Key("items"s).StartArray();
            double total_time = 0.0;
            for (const auto& edge_id : find_route.value().edges) {
                const auto& edge = tr_router.GetGraph().GetEdge(edge_id);
                total_time += edge.weight.travel_time + edge.weight.wait_time;
                if (edge.weight.route_name == "between_stops"sv) {
                    tmp_response.StartDict();
                    tmp_response.Key("stop_name"s).Value(std::string(edge.weight.from));
                    tmp_response.Key("time"s).Value(edge.weight.travel_time + edge.weight.wait_time);
                    tmp_response.Key("type"s).Value("Wait"s);
                    tmp_response.EndDict();
                } else {
                    tmp_response.StartDict();
                    tmp_response.Key("bus"s).Value(std::string(edge.weight.route_name));
                    tmp_response.Key("span_count"s).Value(edge.weight.stops_in_way_count);
                    tmp_response.Key("time"s).Value(edge.weight.travel_time + edge.weight.wait_time);
                    tmp_response.Key("type"s).Value("Bus"s);
                    tmp_response.EndDict();
                }
            }
            tmp_response.EndArray();
            tmp_response.Key("request_id"s).Value(query.id);
            tmp_response.Key("total_time"s).Value(total_time);
            tmp_response.EndDict();
            return true;
        }
    }
    
    return true;
}

bool ProcessingQueryTypeUnknownRequestType(const Query& query, json::Builder& tmp_response) {
    tmp_response.StartDict();
    tmp_response.Key("error_message"s).Value("unknown request type");
    tmp_response.Key("request_id"s).Value(query.id);
    tmp_response.EndDict();
    return true;
}
