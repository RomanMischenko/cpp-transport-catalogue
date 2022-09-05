#include "serialization.h"
#include "json_builder.h"

#include <transport_catalogue.pb.h>
#include <map_renderer.pb.h>

#include <filesystem>
#include <fstream>

Serializator::Serializator(std::istream& input)
: document_(json::Load(input))
{}

void InsertColor(::transport_catalogue::Color* color, const json::Node& node) {
    if (node.IsString()) {
        color->set_color_string(node.AsString());
    } else if (node.AsArray().size() == 3) {
        color->set_color_r(node.AsArray().at(0).AsInt());
        color->set_color_g(node.AsArray().at(1).AsInt());
        color->set_color_b(node.AsArray().at(2).AsInt());
    } else if (node.AsArray().size() == 4) {
        color->set_color_r(node.AsArray().at(0).AsInt());
        color->set_color_g(node.AsArray().at(1).AsInt());
        color->set_color_b(node.AsArray().at(2).AsInt());
        color->set_color_a(node.AsArray().at(3).AsDouble());
    }
}

void Serializator::Pack() const {
    using std::literals::string_literals::operator""s;

    transport_catalogue::TransportCatalogueProto proto;

    size_t query_count = document_.GetRoot().AsMap().at("base_requests"s).AsArray().size();
    for (size_t i = 0; i < query_count; ++i) {
        const auto& query = document_.GetRoot().AsMap().at("base_requests"s).AsArray().at(i).AsMap();
        if (query.at("type"s).AsString() == "Bus"s) {
            ::transport_catalogue::Route* route = proto.add_routes_in_tc();

            route->set_route_name(query.at("name"s).AsString());
            route->set_is_round_trip(query.at("is_roundtrip"s).AsBool());
            route->set_route_type("Bus"s);
            const auto& stops = query.at("stops"s).AsArray();
            for (const auto& stop : stops) {
                route->add_stops_in_route(stop.AsString());
            }
        } else {
            ::transport_catalogue::Stop* stop = proto.add_stops_in_tc();
            ::transport_catalogue::Coordinates* coord = stop->mutable_stop_coordinates();
            
            coord->set_latitude(query.at("latitude"s).AsDouble());
            coord->set_longitude(query.at("longitude"s).AsDouble());
            stop->set_stop_name(query.at("name"s).AsString());
            stop->set_stop_type("Stop"s);

            if (query.count("road_distances"s)) {
                const auto& road_distances = query.at("road_distances"s).AsMap();
                for (const auto& [stop_name, distance] : road_distances) {
                    stop->add_road_distances_stop_name(stop_name);
                    stop->add_distances_between_stops(distance.AsInt());
                }
            }
        }
    }

    query_count = document_.GetRoot().AsMap().at("render_settings"s).AsMap().size();

    for (size_t i = 0; i < query_count; ++i) {
    }

    const auto& query = document_.GetRoot().AsMap().at("render_settings"s).AsMap();
    ::transport_catalogue::RenderSettings* render_settings = proto.mutable_render_settings();

    render_settings->set_width(query.at("width").AsDouble());
    render_settings->set_height(query.at("height").AsDouble());

    render_settings->set_padding(query.at("padding").AsDouble());

    render_settings->set_stop_radius(query.at("stop_radius").AsDouble());
    render_settings->set_line_width(query.at("line_width").AsDouble());

    render_settings->set_bus_label_font_size(query.at("bus_label_font_size").AsInt());
    render_settings->set_bus_label_offset_dx(query.at("bus_label_offset").AsArray().at(0).AsDouble());
    render_settings->set_bus_label_offset_dy(query.at("bus_label_offset").AsArray().at(1).AsDouble());

    render_settings->set_stop_label_font_size(query.at("stop_label_font_size").AsInt());
    render_settings->set_stop_label_offset_dx(query.at("stop_label_offset").AsArray().at(0).AsDouble());
    render_settings->set_stop_label_offset_dy(query.at("stop_label_offset").AsArray().at(1).AsDouble());

    InsertColor(render_settings->mutable_underlayer_color(), query.at("underlayer_color"));

    render_settings->set_underlayer_width(query.at("underlayer_width").AsDouble());

    const size_t color_size = query.at("color_palette").AsArray().size();
    const auto& colors = query.at("color_palette").AsArray();
    for (size_t i = 0; i < color_size; ++i) {
        ::transport_catalogue::Color* new_color_in_palette = render_settings->add_color_palette();
        InsertColor(new_color_in_palette, colors.at(i));
    }

    ::transport_catalogue::RoutingSettings* router_setting = proto.mutable_router_setting();
    const auto& query_routing_settings = document_.GetRoot().AsMap().at("routing_settings"s).AsMap();
    router_setting->set_bus_wait_time(query_routing_settings.at("bus_wait_time"s).AsInt());
    router_setting->set_bus_velocity(query_routing_settings.at("bus_velocity"s).AsInt());

    const std::filesystem::path path = document_.GetRoot().AsMap().at("serialization_settings").AsMap().at("file").AsString();
    std::ofstream out_file(path, std::ios::binary);
    proto.SerializeToOstream(&out_file);
}

DeSerializator::DeSerializator(std::istream& input) 
: process_requests_(json::Load(input))
{}

json::Document DeSerializator::UnPack() const {
    using std::literals::string_literals::operator""s;

    transport_catalogue::TransportCatalogueProto proto;

    const std::filesystem::path path = process_requests_.GetRoot().AsMap().at("serialization_settings").AsMap().at("file").AsString();
    std::ifstream in_file(path, std::ios::binary);
    if (!proto.ParseFromIstream(&in_file)) {
        std::cerr << "Can't open file" << std::endl;
        abort();
    }

    json::Builder builder;

    builder.StartDict();

    builder.Key("base_requests"s);
    builder.StartArray();
    // добавляем автобусы
    size_t routes_count = proto.routes_in_tc_size();
    for (size_t i = 0; i < routes_count; ++i) {
        const ::transport_catalogue::Route& route = proto.routes_in_tc(i);
        builder.StartDict();
        builder.Key("is_roundtrip"s).Value(route.is_round_trip());
        builder.Key("name"s).Value(route.route_name());
        builder.Key("type"s).Value(route.route_type());
        builder.Key("stops"s).StartArray();
        size_t stops_count = route.stops_in_route_size();
        for (size_t index = 0; index < stops_count; ++index) {
            builder.Value(route.stops_in_route(index));
        }
        builder.EndArray();
        
        builder.EndDict(); // route
    }
    // добавляем остановки
    size_t stops_count = proto.stops_in_tc_size();
    for (size_t i = 0; i < stops_count; ++i) {
        const ::transport_catalogue::Stop& stop = proto.stops_in_tc(i);
        builder.StartDict();
        builder.Key("latitude"s).Value(stop.stop_coordinates().latitude());
        builder.Key("longitude"s).Value(stop.stop_coordinates().longitude());
        builder.Key("name"s).Value(stop.stop_name());
        builder.Key("type"s).Value(stop.stop_type());
        builder.Key("road_distances"s).StartDict();
        size_t road_dist_count = stop.road_distances_stop_name_size();
        for (size_t index = 0; index < road_dist_count; ++index) {
            builder.Key(stop.road_distances_stop_name(index));
            builder.Value(static_cast<int>(stop.distances_between_stops(index)));
        }
        builder.EndDict(); // road_dist

        builder.EndDict(); // stop
    }

    builder.EndArray(); // base_request

    const ::transport_catalogue::RenderSettings& render_setting = proto.render_settings();

    builder.Key("render_settings"s);
    builder.StartDict();
    builder.Key("width"s).Value(render_setting.width());
    builder.Key("height"s).Value(render_setting.height());
    builder.Key("padding"s).Value(render_setting.padding());
    builder.Key("stop_radius"s).Value(render_setting.stop_radius());
    builder.Key("line_width"s).Value(render_setting.line_width());
    builder.Key("bus_label_font_size"s).Value(static_cast<int>(render_setting.bus_label_font_size()));
    builder.Key("bus_label_offset"s).StartArray();
    builder.Value(render_setting.bus_label_offset_dx());
    builder.Value(render_setting.bus_label_offset_dy());
    builder.EndArray();
    builder.Key("stop_label_font_size"s).Value(static_cast<int>(render_setting.stop_label_font_size()));
    builder.Key("stop_label_offset"s).StartArray();
    builder.Value(render_setting.stop_label_offset_dx());
    builder.Value(render_setting.stop_label_offset_dy());
    builder.EndArray();

    const ::transport_catalogue::Color& color = render_setting.underlayer_color();

    if (color.color_string() != ""s) {
        builder.Key("underlayer_color"s).Value(color.color_string());
    } else if (color.color_a() != 0.0) {
        builder.Key("underlayer_color"s).StartArray();
        builder.Value(static_cast<int>(color.color_r()));
        builder.Value(static_cast<int>(color.color_g()));
        builder.Value(static_cast<int>(color.color_b()));
        builder.Value(color.color_a());
        builder.EndArray();
    } else {
        builder.Key("underlayer_color"s).StartArray();
        builder.Value(static_cast<int>(color.color_r()));
        builder.Value(static_cast<int>(color.color_g()));
        builder.Value(static_cast<int>(color.color_b()));
        builder.EndArray();
    }

    builder.Key("underlayer_width"s).Value(render_setting.underlayer_width());

    builder.Key("color_palette"s).StartArray();
    size_t color_palette_size = render_setting.color_palette_size();
    for (size_t i = 0; i < color_palette_size; ++i) {
        const ::transport_catalogue::Color& color = render_setting.color_palette(i);
        if (color.color_string() != ""s) {
            builder.Value(color.color_string());
        } else if (color.color_a() != 0.0) {
            builder.StartArray();
            builder.Value(static_cast<int>(color.color_r()));
            builder.Value(static_cast<int>(color.color_g()));
            builder.Value(static_cast<int>(color.color_b()));
            builder.Value(color.color_a());
            builder.EndArray();
        } else {
            builder.StartArray();
            builder.Value(static_cast<int>(color.color_r()));
            builder.Value(static_cast<int>(color.color_g()));
            builder.Value(static_cast<int>(color.color_b()));
            builder.EndArray();
        }
    }
    builder.EndArray(); // color_palette

    builder.EndDict(); // render_settings

    builder.Key("routing_settings"s);
    const ::transport_catalogue::RoutingSettings& router_setting = proto.router_setting();
    builder.StartDict();
    builder.Key("bus_wait_time"s).Value(static_cast<int>(router_setting.bus_wait_time()));
    builder.Key("bus_velocity"s).Value(static_cast<int>(router_setting.bus_velocity()));


    builder.EndDict(); // routing_settings

    builder.EndDict(); 

    

    json::Document result(builder.Build());
    //json::Print(result, std::cout);
    return result;
}

const json::Document& DeSerializator::GetProcessRequests() const {
    return process_requests_;
}