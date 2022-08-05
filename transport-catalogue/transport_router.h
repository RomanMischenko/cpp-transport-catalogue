#pragma once

#include "transport_catalogue.h"
#include "router.h"

#include <optional>

namespace transport_router {

class TransportRouter {
    using Data = transport_catalogue::TransportCatalogue;
    using StopsID = std::unordered_map<std::string_view, std::pair<size_t, size_t>>;
public:
    TransportRouter() = delete;
    TransportRouter(const Data&);

    struct Weight {
        std::string_view from;
        std::string_view to;
        std::string_view route_name;
        double travel_time = 0.0;
        double wait_time = 0.0;
        int stops_in_way_count = 0;
    };

    const StopsID& GetStopsIDInGraph() const;

    const graph::DirectedWeightedGraph<Weight>& GetGraph() const;

    void BuildGraph();
    graph::Router<Weight> BuildRouter() const;

private:
    const Data& data_;
    graph::DirectedWeightedGraph<Weight> graph_;
    StopsID stops_id_by_name_in_graph_;
    

    void AddEdge(std::string_view stop_from
                , std::string_view stop_to
                , std::string_view route_name
                , size_t other_stops_count
                , double dist_between_stops);
};

inline bool operator<(TransportRouter::Weight lhs, TransportRouter::Weight rhs) {
    double lhs_total_time = lhs.travel_time + lhs.wait_time;
    double rhs_total_time = rhs.travel_time + rhs.wait_time;
    return lhs_total_time < rhs_total_time ? true : false;
}

inline bool operator>(TransportRouter::Weight lhs, TransportRouter::Weight rhs) {
    return !(lhs < rhs);
}

inline TransportRouter::Weight operator+(TransportRouter::Weight lhs, TransportRouter::Weight rhs) {
    TransportRouter::Weight tmp;
    tmp.travel_time = lhs.travel_time + rhs.travel_time;
    tmp.wait_time = lhs.wait_time + rhs.wait_time;
    tmp.stops_in_way_count = lhs.stops_in_way_count + rhs.stops_in_way_count;
    return tmp;
}

} // namespace transport_router