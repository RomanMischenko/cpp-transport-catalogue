#pragma once

#include "geo.h"

#include <vector>
#include <unordered_map>
#include <utility> // for pair, move
#include <string_view>
#include <deque>
#include <string>
#include <unordered_set>

namespace transport_catalogue {
namespace detail {

struct Stop;

struct Route {
    std::string route_name;
    std::vector<Stop *> stops;
};

struct Stop {
    std::string stop_name;
    coordinates::Coordinates coordinates;
    std::unordered_set<Route *> buses_for_stop = {};
};


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
    void AddStop(std::string_view name, const coordinates::Coordinates& coordinates);

    detail::Route* FindRoute(std::string_view name) const;
    detail::Stop* FindStop(std::string_view name) const;

    std::ostringstream RouteInfo(const std::string& name) const;
  
    void SetDistanceBetweenStops(const detail::Stop& stop_from, const detail::Stop& stop_to, double distance);

    double GetDistanceBetweenStops(const detail::Stop& stop_from, const detail::Stop& stop_to) const;

private:
    std::deque<detail::Route> routes_;
    std::unordered_map<std::string_view, detail::Route *> routes_name_;

    std::deque<detail::Stop> stops_;
    std::unordered_map<std::string_view, detail::Stop *> stops_name_;

    std::unordered_map<std::pair<const detail::Stop *, const detail::Stop *>, double, detail::HasherWithStop> geographical_distance_;
    std::unordered_map<std::pair<const detail::Stop *, const detail::Stop *>, double, detail::HasherWithStop> road_distance_;
};

} // namespace transport_catalogue