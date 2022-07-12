#include "transport_catalogue.h"

#include <unordered_set>
#include <iostream>
#include <sstream>

namespace transport_catalogue {

void TransportCatalogue::AddRoute(std::string_view name, const std::vector<std::string>& stops) {
    // добавление маршрута
    Route route;
    route.route_name = name;
    for (auto& stop : stops) {
        route.stops.push_back(FindStop(stop));
    }
    routes_.push_back(route);

    Route& last_added_route = routes_.back();
    routes_name_.insert({last_added_route.route_name, &last_added_route});
    // добавление маршрутов через остановки
    for (auto& stop : last_added_route.stops) {
        stop->buses_for_stop.insert(&last_added_route);
    }
    // добавление географической дистанции
    auto i_end = route.stops.size() - 1;
    for (size_t i = 0; i < i_end; ++i) {
        std::pair<const Stop *, const Stop *> pair_for_distance_calc;
        pair_for_distance_calc.first = route.stops.at(i);
        pair_for_distance_calc.second = route.stops.at(i + 1);
        if (!geographical_distance_.count(pair_for_distance_calc)) {
            double distance = ComputeDistance(pair_for_distance_calc.first->coordinates, 
                pair_for_distance_calc.second->coordinates);
            geographical_distance_.insert({pair_for_distance_calc, distance});
        }
    }
}

void TransportCatalogue::AddStop(std::string_view name, const geo::Coordinates& coordinates) {
    stops_.push_back({std::string(name), coordinates});
    stops_name_.insert({stops_.back().stop_name, &stops_.back()});
}

Stop* TransportCatalogue::FindStop(std::string_view name) const {
    if (stops_name_.count(name)) {
        return stops_name_.at(name);
    } else {
        return nullptr;
    }
}

Route* TransportCatalogue::FindRoute(std::string_view name) const {
    if (routes_name_.count(name)) {
        return routes_name_.at(name);
    } else {
        return nullptr;
    }
    
}

std::ostringstream TransportCatalogue::RouteInfo(const std::string& name) const {
    std::ostringstream out;
    if (routes_name_.count(name) == 0) {
        out << "Bus " << name
            << ": not found";
        return out;
    }
    Route* route = FindRoute(name);
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

void TransportCatalogue::SetRoadDistanceBetweenStops(const Stop& stop_from, const Stop& stop_to, double distance) {
    std::pair<const Stop *, const Stop *> tmp_pair;
    tmp_pair.first = &stop_from;
    tmp_pair.second = &stop_to;
    road_distance_[tmp_pair] = distance;
}

double TransportCatalogue::GetGeographicalDistanceBetweenStops(const Stop& stop_from, const Stop& stop_to) const {
    std::pair<const Stop *, const Stop *> tmp_pair;
    tmp_pair.first = &stop_from;
    tmp_pair.second = &stop_to;
    if (geographical_distance_.count(tmp_pair)) {
        return geographical_distance_.at(tmp_pair);
    }
    return 0.0;
}

double TransportCatalogue::GetRoadDistanceBetweenStops(const Stop& stop_from, const Stop& stop_to) const {
    std::pair<const Stop *, const Stop *> tmp_pair;
    tmp_pair.first = &stop_from;
    tmp_pair.second = &stop_to;
    if (road_distance_.count(tmp_pair)) {
        return road_distance_.at(tmp_pair);
    }
    return 0.0;
}

} // namespace transport_catalogue