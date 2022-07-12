#pragma once

#include "domain.h"

#include <vector>
#include <unordered_map>
#include <utility> // for pair, move
#include <string_view>
#include <deque>
#include <string>
#include <unordered_set>

namespace transport_catalogue {

struct Stop;

struct Route {
    std::string route_name;
    std::vector<Stop *> stops;
};

struct Stop {
    std::string stop_name;
    geo::Coordinates coordinates;
    std::unordered_set<Route *> buses_for_stop = {};
};

namespace detail {

class HasherWithStop {
public:
    size_t operator()(const std::pair<const Stop *, const Stop *>& stops) const{
        std::string str = stops.first->stop_name + stops.second->stop_name;
        return std::hash<std::string>{}(str);
    }
};

} // namespace detail

class TransportCatalogue {
 
public:
    void AddRoute(std::string_view name, const std::vector<std::string>& stops);
    void AddStop(std::string_view name, const geo::Coordinates& coordinates);

    Route* FindRoute(std::string_view name) const;
    Stop* FindStop(std::string_view name) const;

    std::ostringstream RouteInfo(const std::string& name) const;
  
    void SetRoadDistanceBetweenStops(const Stop& stop_from, const Stop& stop_to, double distance);

    double GetGeographicalDistanceBetweenStops(const Stop& stop_from, const Stop& stop_to) const;
    double GetRoadDistanceBetweenStops(const Stop& stop_from, const Stop& stop_to) const;

private:
    std::deque<Route> routes_;
    std::unordered_map<std::string_view, Route *> routes_name_;

    std::deque<Stop> stops_;
    std::unordered_map<std::string_view, Stop *> stops_name_;

    std::unordered_map<std::pair<const Stop *, const Stop *>, double, detail::HasherWithStop> geographical_distance_;
    std::unordered_map<std::pair<const Stop *, const Stop *>, double, detail::HasherWithStop> road_distance_;
};

} // namespace transport_catalogue