#include "transport_router.h"

namespace transport_router {

TransportRouter::TransportRouter(const TransportCatalogue& data) 
: data_(data)
, graph_(data.GetAllStops().size() * 2)
{}

graph::Router<TransportRouter::Weight> TransportRouter::BuildRouter() const {
    graph::Router<Weight> router(graph_);
    return router;
}

void TransportRouter::AddEdge(std::string_view stop_from
                            , std::string_view stop_to
                            , std::string_view route_name
                            , size_t other_stops_count
                            , double dist_between_stops) {
    if (stop_from == stop_to) {
        size_t stop_id_1 = stops_id_by_name_in_graph_.at(stop_from).first;
        size_t stop_id_2 = stops_id_by_name_in_graph_.at(stop_from).second;

        Weight weight_between_one_stop;
        weight_between_one_stop.from = stop_from;
        weight_between_one_stop.to = stop_from;
        weight_between_one_stop.route_name = route_name;
        weight_between_one_stop.travel_time = 0;
        weight_between_one_stop.wait_time = data_.GetRoutingSettings().bus_wait_time;
        weight_between_one_stop.stops_in_way_count = 0;
        
        graph_.AddEdge({stop_id_1, stop_id_2, weight_between_one_stop});
    } else {
        size_t stop_id_from = stops_id_by_name_in_graph_.at(stop_from).second;
        size_t stop_id_to = stops_id_by_name_in_graph_.at(stop_to).first;
        // время в пути между ними
        double travel_time_between_stops = dist_between_stops / data_.GetRoutingSettings().bus_speed_in_m_min;
        
        Weight weight_between_two_stops;
        weight_between_two_stops.from = stop_from;
        weight_between_two_stops.to = stop_to;
        weight_between_two_stops.route_name = route_name;
        weight_between_two_stops.travel_time = travel_time_between_stops;
        weight_between_two_stops.wait_time = 0;
        weight_between_two_stops.stops_in_way_count = other_stops_count;
                    
        graph_.AddEdge({stop_id_from, stop_id_to, weight_between_two_stops});
    }
}

void TransportRouter::AddEdgeBetweenOneStop(std::string_view stop_name
                                          , std::string_view route_name) {
    size_t stop_id_1 = stops_id_by_name_in_graph_.at(stop_name).first;
    size_t stop_id_2 = stops_id_by_name_in_graph_.at(stop_name).second;
    Weight weight_between_one_stop;
    weight_between_one_stop.from = stop_name;
    weight_between_one_stop.to = stop_name;
    weight_between_one_stop.route_name = route_name;
    weight_between_one_stop.travel_time = 0;
    weight_between_one_stop.wait_time = data_.GetRoutingSettings().bus_wait_time;
    weight_between_one_stop.stops_in_way_count = 0;
    graph_.AddEdge({stop_id_1, stop_id_2, weight_between_one_stop});
}

void TransportRouter::AddEdgeBetweenTwoStop(std::string_view stop_from
                                          , std::string_view stop_to
                                          , std::string_view route_name
                                          , size_t stops_in_way_count
                                          , double dist_between_stops) {
    size_t stop_id_from = stops_id_by_name_in_graph_.at(stop_from).second;
    size_t stop_id_to = stops_id_by_name_in_graph_.at(stop_to).first;
    // время в пути между ними
    double travel_time_between_stops = dist_between_stops 
                                        / data_.GetRoutingSettings().bus_speed_in_m_min;

    Weight weight_between_two_stops;
    weight_between_two_stops.from = stop_from;
    weight_between_two_stops.to = stop_to;
    weight_between_two_stops.route_name = route_name;
    weight_between_two_stops.travel_time = travel_time_between_stops;
    weight_between_two_stops.wait_time = 0;
    weight_between_two_stops.stops_in_way_count = stops_in_way_count;
                
    graph_.AddEdge({stop_id_from, stop_id_to, weight_between_two_stops});
}

void TransportRouter::BuildGraph() {
    const auto& all_stops = data_.GetAllStops();
    const auto& all_routes = data_.GetAllRoutes();
    size_t stops_count = all_stops.size();
    // связываем id вершин графа с остановками
    size_t stop_id = 0;
    for (const auto& [stop_name, _] : all_stops) {
        size_t stop_id_1 = stop_id++;
        size_t stop_id_2 = stop_id++;
        stops_id_by_name_in_graph_.emplace(stop_name, std::pair{stop_id_1, stop_id_2});
        AddEdgeBetweenOneStop(stop_name, std::string_view("between_stops"));
    }
    // добавляем ребра
    for (const auto& [route_name, route] : all_routes) {
        transport_catalogue::TransportCatalogue::RouteInfo route_info = data_.GetRouteInfo(route_name);
        size_t stops_in_route_count = route->stops.size();
        const auto& stops_in_route = route->stops;

        if (stops_count == 0 || stops_count == 1) {
            continue;
        }
        if (route_info.is_round_trip == true) {
            // из начальной остановки
            // мы можем добраться до любой оставшейся остановки без пересадки
            // как минимум 2 остановки
            size_t start_stop_index = 0;
            for (/* ... */; start_stop_index < stops_in_route_count - 1; ++start_stop_index) {
                double dist_between_stops = 0;
                std::string_view stop_name_from = stops_in_route.at(start_stop_index)->stop_name;
                size_t end_stop_index = start_stop_index + 1;
                for (/* ... */; end_stop_index < stops_in_route_count; ++end_stop_index) {
                    std::string_view stop_name_to = stops_in_route.at(end_stop_index)->stop_name;
                    if (stop_name_from == stop_name_to) {
                        continue;
                    }
                    dist_between_stops += data_.GetRoadDistanceBetweenStops(
                        *data_.FindStop(
                            stops_in_route.at(end_stop_index - 1)->stop_name
                        ),
                        *data_.FindStop(
                            stops_in_route.at(end_stop_index)->stop_name
                    ));
                    size_t stops_in_way_count = end_stop_index - start_stop_index;
                    AddEdgeBetweenTwoStop(stop_name_from, stop_name_to, route_name, stops_in_way_count, dist_between_stops);
                }
            }
        } else {
            size_t middle_stop_index = stops_in_route_count / 2;
            // добавляем прямой маршрут
            for (size_t start_stop_index = 0; start_stop_index < middle_stop_index; ++start_stop_index) {
                double dist_between_stops = 0;
                std::string_view stop_name_from = stops_in_route.at(start_stop_index)->stop_name;
                size_t end_stop_index = start_stop_index + 1;
                for (/* ... */; end_stop_index <= middle_stop_index; ++end_stop_index) {
                    std::string_view stop_name_to = stops_in_route.at(end_stop_index)->stop_name;
                    dist_between_stops += data_.GetRoadDistanceBetweenStops(
                        *data_.FindStop(
                            stops_in_route.at(end_stop_index - 1)->stop_name
                        ),
                        *data_.FindStop(
                            stops_in_route.at(end_stop_index)->stop_name
                    ));
                    size_t stops_in_way_count = end_stop_index - start_stop_index;
                    AddEdgeBetweenTwoStop(stop_name_from, stop_name_to, route_name, stops_in_way_count, dist_between_stops);
                }
            }
            // добавляем обратный маршрут
            for (/* ... */; middle_stop_index < stops_in_route_count; ++middle_stop_index) {
                double dist_between_stops = 0;
                std::string_view stop_name_from = stops_in_route.at(middle_stop_index)->stop_name;
                size_t end_stop_index = middle_stop_index + 1;
                for (/* ... */; end_stop_index < stops_in_route_count; ++end_stop_index) {
                    std::string_view stop_name_to = stops_in_route.at(end_stop_index)->stop_name;
                    dist_between_stops += data_.GetRoadDistanceBetweenStops(
                        *data_.FindStop(
                            stops_in_route.at(end_stop_index - 1)->stop_name
                        ),
                        *data_.FindStop(
                            stops_in_route.at(end_stop_index)->stop_name
                    ));
                    size_t stops_in_way_count = end_stop_index - middle_stop_index;
                    AddEdgeBetweenTwoStop(stop_name_from, stop_name_to, route_name, stops_in_way_count, dist_between_stops);
                }
            }
        }
    }
}

const std::unordered_map<std::string_view, std::pair<size_t, size_t>>& TransportRouter::GetStopsIDInGraph() const {
    return stops_id_by_name_in_graph_;
}

const graph::DirectedWeightedGraph<TransportRouter::Weight>& TransportRouter::GetGraph() const {
    return graph_;
}

} // namespace transport_router
