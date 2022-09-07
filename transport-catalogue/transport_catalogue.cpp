#include "transport_catalogue.h"

#include <unordered_set>
#include <iostream>
#include <sstream>

namespace transport_catalogue {

using RoutesName = std::unordered_map<std::string_view, domain::Route *>;
using StopsName = std::unordered_map<std::string_view, domain::Stop *>;
using Distance = std::unordered_map<std::pair<const domain::Stop *, const domain::Stop *>, double, detail::HasherWithStop>;

void TransportCatalogue::AddRoute(std::string name, const std::vector<std::string>& stops, bool is_round_trip) {
    // добавление маршрута
    domain::Route route;
    route.route_name = name;
    for (auto& stop : stops) {
        route.stops.push_back(FindStop(stop));
    }
    routes_.push_back(route);

    domain::Route& last_added_route = routes_.back();
    routes_name_.insert({last_added_route.route_name, &last_added_route});
    // добавление маршрутов через остановки
    for (auto& stop : last_added_route.stops) {
        stop->buses_for_stop.insert(&last_added_route);
    }
    // добавление географической дистанции
    auto i_end = route.stops.size() - 1;
    for (size_t i = 0; i < i_end; ++i) {
        std::pair<const domain::Stop *, const domain::Stop *> pair_for_distance_calc;
        pair_for_distance_calc.first = route.stops.at(i);
        pair_for_distance_calc.second = route.stops.at(i + 1);
        if (!geographical_distance_.count(pair_for_distance_calc)) {
            double distance = ComputeDistance(pair_for_distance_calc.first->coordinates, 
                pair_for_distance_calc.second->coordinates);
            geographical_distance_.insert({pair_for_distance_calc, distance});
        }
    }
    // добавление информации о типе маршрута
    is_round_trip_.insert({last_added_route.route_name, is_round_trip});
}

void TransportCatalogue::AddStop(std::string name, const geo::Coordinates& coordinates) {
    stops_.push_back({std::string(name), coordinates});
    stops_name_.insert({stops_.back().stop_name, &stops_.back()});
}

domain::Stop* TransportCatalogue::FindStop(std::string_view name) const {
    if (stops_name_.count(name)) {
        return stops_name_.at(name);
    } else {
        return nullptr;
    }
}

domain::Route* TransportCatalogue::FindRoute(std::string_view name) const {
    if (routes_name_.count(name)) {
        return routes_name_.at(name);
    } else {
        return nullptr;
    }
    
}
/* 
std::ostringstream TransportCatalogue::RouteInfo(const std::string& name) const {
    std::ostringstream out;
    if (routes_name_.count(name) == 0) {
        out << "Bus " << name
            << ": not found";
        return out;
    }
    domain::Route* route = FindRoute(name);
    // уникальные остановки
    std::unordered_set<std::string> unique_stops;
    //вычисляем дистанции
    double geographical_distance = 0.0;
    double roat_distance = 0.0;
    size_t i_end = route->stops.size() - 1;
    for (size_t i = 0; i < i_end; ++i) {
        unique_stops.insert(route->stops.at(i)->stop_name);
        geographical_distance += geographical_distance_.at({route->stops.at(i), route->stops.at(i + 1)});
        // если искомый порядок существует
        if (road_distance_.count({route->stops.at(i), route->stops.at(i + 1)})) {
            roat_distance += road_distance_.at({route->stops.at(i), route->stops.at(i + 1)});
        // если нет, то ищем наоборот
        } else if (road_distance_.count({route->stops.at(i + 1), route->stops.at(i)})) {
            roat_distance += road_distance_.at({route->stops.at(i + 1), route->stops.at(i)});
        }
    }
    
    out << "Bus " << name << ": "
        << route->stops.size() << " stops on route, "
        << unique_stops.size() << " unique stops, "
        << roat_distance << " route length, "
        << roat_distance/geographical_distance << " curvature";
    return out;
}
 */
const RoutesName& TransportCatalogue::GetAllRoutes() const {
    return routes_name_;
}

const StopsName& TransportCatalogue::GetAllStops() const {
    return stops_name_;
}

const Distance& TransportCatalogue::GetRoadDistance() const {
    return road_distance_;
}

void TransportCatalogue::SetRoadDistanceBetweenStops(const domain::Stop& stop_from, const domain::Stop& stop_to, double distance) {
    std::pair<const domain::Stop *, const domain::Stop *> tmp_pair;
    tmp_pair.first = &stop_from;
    tmp_pair.second = &stop_to;
    road_distance_[tmp_pair] = distance;
}

double TransportCatalogue::GetGeographicalDistanceBetweenStops(const domain::Stop& stop_from, const domain::Stop& stop_to) const {
    std::pair<const domain::Stop *, const domain::Stop *> tmp_pair;
    tmp_pair.first = &stop_from;
    tmp_pair.second = &stop_to;
    if (geographical_distance_.count(tmp_pair)) {
        return geographical_distance_.at(tmp_pair);
    }
    return 0.0;
}

double TransportCatalogue::GetRoadDistanceBetweenStops(const domain::Stop& stop_from, const domain::Stop& stop_to) const {
    std::pair<const domain::Stop *, const domain::Stop *> forward_direction = {&stop_from, &stop_to};
    std::pair<const domain::Stop *, const domain::Stop *> reverse_direction = {&stop_to, &stop_from};
    if (road_distance_.count(forward_direction)) {
        return road_distance_.at(forward_direction);
    } else if (road_distance_.count(reverse_direction)) {
        return road_distance_.at(reverse_direction);
    } else {
        throw std::logic_error(std::string("can't find the distance between these stops"));
    }
    return 0.0;
}

bool TransportCatalogue::GetIsRoundTrip(std::string_view route_name) const {
    return is_round_trip_.at(route_name);
}

TransportCatalogue::RouteInfo TransportCatalogue::GetRouteInfo(std::string_view route_name) const {
    RouteInfo route_info;
    const auto& route = FindRoute(route_name);
    route_info.first_stop = (route->stops.front())->stop_name;
    if (GetIsRoundTrip(route_name)) {
        route_info.is_round_trip = true;
    } else {
        route_info.is_round_trip = false;
        size_t middle_stop = route->stops.size() / 2;
        route_info.end_stop = route->stops.at(middle_stop)->stop_name;
    }
    return route_info;
}

void TransportCatalogue::SetBusWaitTime(double time) {
    routing_settings_.bus_wait_time = time;
}
void TransportCatalogue::SetBusVelocity(double velocity) {
    routing_settings_.bus_velocity = velocity;
    routing_settings_.bus_speed_in_m_min = velocity * 1000.0 / 60.0;
}

const TransportCatalogue::RoutingSettings& TransportCatalogue::GetRoutingSettings() const {
    return routing_settings_;
}

void TransportCatalogue::SetRoutingSettings(int bus_wait_time, int bus_velocity, double bus_speed_in_m_min) {
    routing_settings_.bus_wait_time = bus_wait_time;
    routing_settings_.bus_velocity = bus_velocity;
    routing_settings_.bus_speed_in_m_min = bus_speed_in_m_min;
}

} // namespace transport_catalogue