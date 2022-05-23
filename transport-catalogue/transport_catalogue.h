#pragma once

#include "geo.h"

#include <vector>
#include <unordered_map>
#include <utility> // for pair, move
#include <string_view>
#include <deque>
#include <string>
#include <unordered_set>

struct Stop;

struct Route {
    std::string route_name;
    std::vector<Stop *> stops;
};

/* struct SetSort {
    bool operator()(const Route* lhs, const Route* rhs) const {
        return lhs->route_name < lhs->route_name;
    }
}; */

struct Stop {
    std::string stop_name;
    Coordinates coordinates;
    std::unordered_set<Route* /* , SetSort */> buses_for_stop = {};
};


class HasherWithStop {
public:
    size_t operator()(const std::pair<Stop *, Stop *>& stops) const{
        std::string str = stops.first->stop_name + stops.second->stop_name;
        return std::hash<std::string>{}(str);
    }
};


class TransportCatalogue {
 
public:
    void AddRoute(std::string& name, std::vector<std::string>& stops);
    void AddStop(std::string& name, Coordinates& coordinates);

    Route* FindRoute(const std::string_view& name) const;
    Stop* FindStop(const std::string_view& name) const;

    std::ostringstream RouteInfo(const std::string& name) const;

    void AddingDistanceBetweenStops(std::pair<Stop *, Stop *>& stops, double distance);

    double GetDistanceBetweenStops(std::pair<Stop *, Stop *>& stops);

    // для тестирования приватных значений
    std::deque<Stop>& GetStops();
    std::unordered_map<std::string_view, Stop *>& GetStopsName();

    std::deque<Route>& GetRoutes();
    std::unordered_map<std::string_view, Route *>& GetRoutesName();

    std::unordered_map<std::pair<Stop *, Stop *>, double, HasherWithStop>& GetDistance();


private:
    std::deque<Route> routes_;
    std::unordered_map<std::string_view, Route *> routes_name_;

    std::deque<Stop> stops_;
    std::unordered_map<std::string_view, Stop *> stops_name_;

    std::unordered_map<std::pair<Stop *, Stop *>, double, HasherWithStop> geographical_distance_;
    std::unordered_map<std::pair<Stop *, Stop *>, double, HasherWithStop> road_distance_;
};