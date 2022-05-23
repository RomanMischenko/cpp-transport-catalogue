#include "transport_catalogue.h"

#include <unordered_set>
#include <iostream>
#include <sstream>

void TransportCatalogue::AddRoute(std::string& name, std::vector<std::string>& stops) {
    // добавление маршрута
    Route route;
    route.route_name = std::move(name);
    for (auto& stop : stops) {
        route.stops.push_back(FindStop(stop));
    }
    routes_.push_back(route);
    routes_name_.insert({routes_.back().route_name, &routes_.back()});
    // добавление маршрутов через остановки
    for (auto& stop : routes_.back().stops) {
        stop->buses_for_stop.insert(&(routes_.back()));
    }
    // добавление географической дистанции
    auto i_end = route.stops.size() - 1;
    for (size_t i = 0; i < i_end; ++i) {
        std::pair<Stop *, Stop *> pair_for_distance_calc;
        pair_for_distance_calc.first = route.stops.at(i);
        pair_for_distance_calc.second = route.stops.at(i + 1);
        if (!geographical_distance_.count(pair_for_distance_calc)) {
            double distance = ComputeDistance(pair_for_distance_calc.first->coordinates, 
                pair_for_distance_calc.second->coordinates);
            geographical_distance_.insert({pair_for_distance_calc, distance});
        }
    }
}

void TransportCatalogue::AddStop(std::string& name, Coordinates& coordinates) {
    stops_.push_back({std::move(name), coordinates});
    stops_name_.insert({stops_.back().stop_name, &stops_.back()});
}

Stop* TransportCatalogue::FindStop(const std::string_view& name) const {
    if (stops_name_.count(name)) {
        return stops_name_.at(name);
    } else {
        return nullptr;
    }
}

Route* TransportCatalogue::FindRoute(const std::string_view& name) const {
    return routes_name_.at(name);
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

void TransportCatalogue::AddingDistanceBetweenStops(std::pair<Stop *, Stop *>& stops, double distance) {
    road_distance_[std::move(stops)] = distance;
}

double TransportCatalogue::GetDistanceBetweenStops(std::pair<Stop *, Stop *>& stops) {
    if (geographical_distance_.count(stops)) {
        return geographical_distance_.at(stops);
    }
    return 0.0;
}

std::deque<Stop>& TransportCatalogue::GetStops() {
    return stops_;
}

std::unordered_map<std::string_view, Stop *>& TransportCatalogue::GetStopsName() {
    return stops_name_;
}

std::deque<Route>& TransportCatalogue::GetRoutes() {
    return routes_;
}
std::unordered_map<std::string_view, Route *>& TransportCatalogue::GetRoutesName() {
    return routes_name_;
}

std::unordered_map<std::pair<Stop *, Stop *>, double, HasherWithStop>& TransportCatalogue::GetDistance() {
    return geographical_distance_;
}