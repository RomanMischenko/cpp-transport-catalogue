#include "serialization.h"

using std::literals::string_literals::operator""s;

MakeBase::MakeBase(const transport_catalogue::TransportCatalogue& catalogue
                , const map_renderer::MapRenderer& map_renderer) 
: catalogue_(catalogue)
, map_renderer_(map_renderer)
{}

void PackStops(const transport_catalogue::TransportCatalogue& catalogue,
                transport_catalogue_proto::TransportCatalogueProto& proto) {
    // добавляем остановки
    for (const auto& stop_in_tc : catalogue.GetAllStops()) {
        ::transport_catalogue_proto::Stop* new_stop_in_proto = proto.add_stops_in_tc();
        ::transport_catalogue_proto::Coordinates* coord = new_stop_in_proto->mutable_stop_coordinates();
        
        const std::string_view stop_name = stop_in_tc.first;
        const double lat = stop_in_tc.second->coordinates.lat;
        const double lng = stop_in_tc.second->coordinates.lng;
        
        new_stop_in_proto->set_stop_name(std::string(stop_name));
        coord->set_latitude(lat);
        coord->set_longitude(lng);   
    }
    // добавляем дистацию между ними
    for (const auto& dist : catalogue.GetRoadDistance()) {
        const std::string_view stop_from = dist.first.first->stop_name;
        const std::string_view stop_to = dist.first.second->stop_name;
        const double dist_bw_stops = dist.second;
        proto.add_road_distances_stop_name_from(std::string(stop_from));
        proto.add_road_distances_stop_name_to(std::string(stop_to));
        proto.add_distances_between_stops_proto(dist_bw_stops);
    }
}

void PackRoutes(const transport_catalogue::TransportCatalogue& catalogue,
                transport_catalogue_proto::TransportCatalogueProto& proto) {
    for (const auto& route_in_tc : catalogue.GetAllRoutes()) {
        ::transport_catalogue_proto::Route* new_route_in_proto = proto.add_routes_in_tc();

        const std::string_view route_name = route_in_tc.first;
        const bool is_round_trip = catalogue.GetIsRoundTrip(route_name);
        
        new_route_in_proto->set_route_name(std::string(route_name));
        new_route_in_proto->set_is_round_trip(is_round_trip);

        const auto& stops_in_tc = route_in_tc.second->stops;
        for (const auto& stop_in_tc : stops_in_tc) {
            new_route_in_proto->add_stops_in_route(std::string(stop_in_tc->stop_name));
        }
    }
}

void InsertColor(::transport_catalogue_proto::Color* color_proto, const svg::Color& color_mr) {
    if (std::holds_alternative<std::string>(color_mr)) {
        color_proto->set_color_string(std::get<std::string>(color_mr));
    } else if (std::holds_alternative<svg::Rgb>(color_mr)) {
        color_proto->set_color_r(std::get<svg::Rgb>(color_mr).red);
        color_proto->set_color_g(std::get<svg::Rgb>(color_mr).green);
        color_proto->set_color_b(std::get<svg::Rgb>(color_mr).blue);
    } else if (std::holds_alternative<svg::Rgba>(color_mr)) {
        color_proto->set_color_r(std::get<svg::Rgba>(color_mr).red);
        color_proto->set_color_g(std::get<svg::Rgba>(color_mr).green);
        color_proto->set_color_b(std::get<svg::Rgba>(color_mr).blue);
        color_proto->set_color_a(std::get<svg::Rgba>(color_mr).opacity);
    }
}

void PackRenderSettings(const map_renderer::MapRenderer& map_renderer,
                transport_catalogue_proto::TransportCatalogueProto& proto) {
    ::transport_catalogue_proto::RenderSettings* render_settings_in_proto = proto.mutable_render_settings();
    const map_renderer::MapSettings& render_settings_in_mr = map_renderer.GetMapSettings();

    render_settings_in_proto->set_width(render_settings_in_mr.width);
    render_settings_in_proto->set_height(render_settings_in_mr.height);

    render_settings_in_proto->set_padding(render_settings_in_mr.padding);

    render_settings_in_proto->set_stop_radius(render_settings_in_mr.stop_radius);
    render_settings_in_proto->set_line_width(render_settings_in_mr.line_width);

    render_settings_in_proto->set_bus_label_font_size(render_settings_in_mr.bus_label_font_size);
    render_settings_in_proto->set_bus_label_offset_dx(render_settings_in_mr.bus_label_offset.x);
    render_settings_in_proto->set_bus_label_offset_dy(render_settings_in_mr.bus_label_offset.y);

    render_settings_in_proto->set_stop_label_font_size(render_settings_in_mr.stop_label_font_size);
    render_settings_in_proto->set_stop_label_offset_dx(render_settings_in_mr.stop_label_offset.x);
    render_settings_in_proto->set_stop_label_offset_dy(render_settings_in_mr.stop_label_offset.y);

    InsertColor(render_settings_in_proto->mutable_underlayer_color(), render_settings_in_mr.underlayer_color);

    render_settings_in_proto->set_underlayer_width(render_settings_in_mr.underlayer_width);

    const size_t color_palette_size = render_settings_in_mr.color_palette.size();
    for (size_t i = 0; i < color_palette_size; ++i) {
        ::transport_catalogue_proto::Color* new_color_in_palette = render_settings_in_proto->add_color_palette();
        InsertColor(new_color_in_palette, render_settings_in_mr.color_palette.at(i));
    }
}

void PackRoutingSettings(const transport_catalogue::TransportCatalogue& catalogue,
                transport_catalogue_proto::TransportCatalogueProto& proto) {
    ::transport_catalogue_proto::RoutingSettings* routint_setting_proto = proto.mutable_router_setting();
    transport_catalogue::TransportCatalogue::RoutingSettings routint_setting_tc = catalogue.GetRoutingSettings();
    routint_setting_proto->set_bus_wait_time(routint_setting_tc.bus_wait_time);
    routint_setting_proto->set_bus_velocity(routint_setting_tc.bus_velocity);
    routint_setting_proto->set_bus_speed_in_m_min(routint_setting_tc.bus_speed_in_m_min);
}

transport_catalogue_proto::TransportCatalogueProto MakeBase::Pack() {
    PackStops(catalogue_, proto_);
    PackRoutes(catalogue_, proto_);
    PackRenderSettings(map_renderer_, proto_);
    PackRoutingSettings(catalogue_, proto_);
    return proto_;
}

ProcessRequests::ProcessRequests(const transport_catalogue_proto::TransportCatalogueProto& proto) 
: proto_(proto)
{}

transport_catalogue::TransportCatalogue UnPackTC(const transport_catalogue_proto::TransportCatalogueProto& proto) {
    transport_catalogue::TransportCatalogue catalogue;
    // сначала добавляем остановки
    size_t stops_count = proto.stops_in_tc_size();
    for (size_t i = 0; i < stops_count; ++i) {
        const ::transport_catalogue_proto::Stop& stop = proto.stops_in_tc(i);
        catalogue.AddStop(stop.stop_name(), {stop.stop_coordinates().latitude(), stop.stop_coordinates().longitude()});
    }
    // добавляем маршруты
    size_t routes_count = proto.routes_in_tc_size();
    for (size_t i = 0; i < routes_count; ++i) {
        const ::transport_catalogue_proto::Route& route = proto.routes_in_tc(i);
        std::vector<std::string> stops;
        size_t stops_count = route.stops_in_route_size();
        for (size_t index = 0; index < stops_count; ++index) {
            stops.push_back(route.stops_in_route(index));
        }
        bool is_roundtrip = route.is_round_trip();

        catalogue.AddRoute(route.route_name(), stops, is_roundtrip);
    }
    // добавляем RoutingSettings
    const ::transport_catalogue_proto::RoutingSettings& router_setting = proto.router_setting();
    catalogue.SetRoutingSettings(static_cast<int>(router_setting.bus_wait_time())
                                ,static_cast<int>(router_setting.bus_velocity())
                                ,static_cast<int>(router_setting.bus_speed_in_m_min()));

    return catalogue;
}

map_renderer::MapRenderer UnPackMR(const transport_catalogue_proto::TransportCatalogueProto& proto) {
    map_renderer::MapRenderer map_renderer;

    const ::transport_catalogue_proto::RenderSettings& render_setting = proto.render_settings();
    auto& map_settings = map_renderer.GetMapSettings();

    map_settings.width = render_setting.width();
    map_settings.height = render_setting.height();

    map_settings.padding = render_setting.padding();

    map_settings.line_width = render_setting.line_width();
    map_settings.stop_radius = render_setting.stop_radius();

    map_settings.bus_label_font_size = static_cast<int>(render_setting.bus_label_font_size());
    map_settings.bus_label_offset.x = render_setting.bus_label_offset_dx();
    map_settings.bus_label_offset.y = render_setting.bus_label_offset_dy();

    map_settings.stop_label_font_size = static_cast<int>(render_setting.stop_label_font_size());
    map_settings.stop_label_offset.x = render_setting.stop_label_offset_dx();
    map_settings.stop_label_offset.y = render_setting.stop_label_offset_dy();

    const ::transport_catalogue_proto::Color& color = render_setting.underlayer_color();
    if (color.color_string() != ""s) {
        map_settings.underlayer_color = color.color_string();
    } else if (color.color_a() != 0.0) {
        map_settings.underlayer_color = svg::Rgba{static_cast<uint8_t>(color.color_r())
                                                ,static_cast<uint8_t>(color.color_g())
                                                ,static_cast<uint8_t>(color.color_b())
                                                ,color.color_a()};
    } else {
        map_settings.underlayer_color = svg::Rgb{static_cast<uint8_t>(color.color_r())
                                                ,static_cast<uint8_t>(color.color_g())
                                                ,static_cast<uint8_t>(color.color_b())};
    }

    map_settings.underlayer_width = render_setting.underlayer_width();

    size_t color_palette_size = render_setting.color_palette_size();
    for (size_t i = 0; i < color_palette_size; ++i) {
        const ::transport_catalogue_proto::Color& color_in_palette = render_setting.color_palette(i);
        if (color_in_palette.color_string() != ""s) {
            map_settings.underlayer_color = color_in_palette.color_string();
        } else if (color_in_palette.color_a() != 0.0) {
            map_settings.underlayer_color = svg::Rgba{static_cast<uint8_t>(color_in_palette.color_r())
                                                    ,static_cast<uint8_t>(color_in_palette.color_g())
                                                    ,static_cast<uint8_t>(color_in_palette.color_b())
                                                    ,color_in_palette.color_a()};
        } else {
            map_settings.underlayer_color = svg::Rgb{static_cast<uint8_t>(color_in_palette.color_r())
                                                ,static_cast<uint8_t>(color_in_palette.color_g())
                                                ,static_cast<uint8_t>(color_in_palette.color_b())};
        }
    }

    return map_renderer;
}

std::tuple<transport_catalogue::TransportCatalogue, map_renderer::MapRenderer> ProcessRequests::UnPack() const {
    transport_catalogue::TransportCatalogue catalogue = UnPackTC(proto_);
    map_renderer::MapRenderer map_renderer = UnPackMR(proto_);

    return std::make_tuple(catalogue, map_renderer);
}