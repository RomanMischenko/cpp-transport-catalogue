#pragma once

#include "svg.h"
#include "domain.h"

#include <unordered_map>
#include <map>
#include <set>
#include <string_view>

namespace map_renderer {

struct MapSettings {
    double width = 0.0;
    double height = 0.0;
    double padding = 0.0;
    double line_width = 0.0;
    double stop_radius = 0.0;
    int bus_label_font_size = 0;
    svg::Point bus_label_offset = {0.0, 0.0};
    int stop_label_font_size = 0;
    svg::Point stop_label_offset = {0.0, 0.0};
    svg::Color underlayer_color;
    double underlayer_width;
    std::vector<svg::Color> color_palette;
};

class MapRenderer {
public:
    void Draw(std::ostream& out) const;

    MapSettings& GetMapSettings();
    const MapSettings& GetMapSettings() const;
    std::unordered_map<std::string, geo::Coordinates>& GetStops();
    std::map<std::string, std::vector<std::string>>& GetBuses();
    std::unordered_map<std::string, bool>& GetIsRoundTrip();
    std::set<std::string>& GetUniqueStops();
private:
    MapSettings map_settings_;
    std::unordered_map<std::string, geo::Coordinates> stops_;
    std::map<std::string, std::vector<std::string>> buses_;
    std::unordered_map<std::string, bool> is_roundtrip_;
    std::set<std::string> unique_stops_name_on_routes_;
};

} // namespace map_renderer