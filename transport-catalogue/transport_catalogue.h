#pragma once

#include "domain.h"

#include <vector>
#include <unordered_map>
#include <utility> // for pair, move
#include <string_view>
#include <deque>
#include <string>
#include <unordered_set>
#include <optional>

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
    using RoutesName = std::unordered_map<std::string_view, domain::Route *>;
    using StopsName = std::unordered_map<std::string_view, domain::Stop *>;

    struct RoutingSettings {
        int bus_wait_time = 0;
        int bus_velocity = 0;
        double bus_speed_in_m_min = 0.0;
    };

    struct RouteInfo {
        bool is_round_trip;
        std::string_view first_stop; // она же конечная для кольцевого
        std::optional<std::string_view> end_stop;
    };

    void AddRoute(std::string_view name, const std::vector<std::string>& stops, bool is_round_trip);
    void AddStop(std::string_view name, const geo::Coordinates& coordinates);

    domain::Route* FindRoute(std::string_view name) const;
    domain::Stop* FindStop(std::string_view name) const;

    const RoutesName& GetAllRoutes() const;
    const StopsName& GetAllStops() const;

    //std::ostringstream RouteInfo(const std::string& name) const;
  
    void SetRoadDistanceBetweenStops(const domain::Stop& stop_from, const domain::Stop& stop_to, double distance);

    double GetGeographicalDistanceBetweenStops(const domain::Stop& stop_from, const domain::Stop& stop_to) const;
    double GetRoadDistanceBetweenStops(const domain::Stop& stop_from, const domain::Stop& stop_to) const;

    bool GetIsRoundTrip(std::string_view route_name) const;

    RouteInfo GetRouteInfo(std::string_view route_name) const;

    void SetBusWaitTime(double time);
    void SetBusVelocity(double velocity);

    const RoutingSettings& GetRoutingSettings() const;

private:
    std::deque<domain::Route> routes_;
    RoutesName routes_name_;

    std::deque<domain::Stop> stops_;
    StopsName stops_name_;

    std::unordered_map<std::pair<const domain::Stop *, const domain::Stop *>, double, detail::HasherWithStop> geographical_distance_;
    std::unordered_map<std::pair<const domain::Stop *, const domain::Stop *>, double, detail::HasherWithStop> road_distance_;

    std::unordered_map<std::string_view, bool> is_round_trip_;

    RoutingSettings routing_settings_;
};

} // namespace transport_catalogue