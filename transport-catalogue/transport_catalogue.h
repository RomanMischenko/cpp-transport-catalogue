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

namespace detail {

class HasherWithStop {
public:
    size_t operator()(const std::pair<const domain::Stop *, const domain::Stop *>& stops) const{
        std::string str = stops.first->stop_name + stops.second->stop_name;
        return std::hash<std::string>{}(str);
    }
};

} // namespace detail

class TransportCatalogue {
public:
    void AddRoute(std::string_view name, const std::vector<std::string>& stops);
    void AddStop(std::string_view name, const geo::Coordinates& coordinates);

    domain::Route* FindRoute(std::string_view name) const;
    domain::Stop* FindStop(std::string_view name) const;

    std::ostringstream RouteInfo(const std::string& name) const;
  
    void SetRoadDistanceBetweenStops(const domain::Stop& stop_from, const domain::Stop& stop_to, double distance);

    double GetGeographicalDistanceBetweenStops(const domain::Stop& stop_from, const domain::Stop& stop_to) const;
    double GetRoadDistanceBetweenStops(const domain::Stop& stop_from, const domain::Stop& stop_to) const;

private:
    std::deque<domain::Route> routes_;
    std::unordered_map<std::string_view, domain::Route *> routes_name_;

    std::deque<domain::Stop> stops_;
    std::unordered_map<std::string_view, domain::Stop *> stops_name_;

    std::unordered_map<std::pair<const domain::Stop *, const domain::Stop *>, double, detail::HasherWithStop> geographical_distance_;
    std::unordered_map<std::pair<const domain::Stop *, const domain::Stop *>, double, detail::HasherWithStop> road_distance_;
};

} // namespace transport_catalogue