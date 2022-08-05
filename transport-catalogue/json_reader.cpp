#include "json_reader.h"

// подключаем ""s
using std::literals::string_literals::operator""s;

void ProcessingBaseRequestStop(json_reader::jsonReader::BaseRequests& query, const json::Dict& stop) {
    query.type = json_reader::jsonReader::BaseRequests::QueryType::STOP;
    query.name = stop.at("name"s).AsString();
    query.coordinates = {stop.at("latitude"s).AsDouble(), stop.at("longitude"s).AsDouble()};
    // если имеется расстояние в метрах
    if (stop.count("road_distances"s)) {
        const auto& road_distances = stop.at("road_distances"s).AsMap();
        for (const auto& [stop_name, distance] : road_distances) {
            query.road_distance_to_stop.emplace(
                std::pair{query.name, stop_name},
                distance.AsInt()
            );
        }
    }
}

void ProcessingBaseRequestBus(json_reader::jsonReader::BaseRequests& query, const json::Dict& bus) {
    query.name = bus.at("name"s).AsString();
    // получаем остановки из базы данных
    const auto& stops = bus.at("stops"s).AsArray();
    // добавляем остновки
    for (const auto& stop : stops) {
        query.stops.push_back(stop.AsString());
    }
    // если маршрут прямой
    if (!(bus.at("is_roundtrip"s).AsBool())) {
        query.type = json_reader::jsonReader::BaseRequests::QueryType::BUS_LINE;
        // добавляем обратный маршрут
        query.stops.resize(query.stops.size() * 2 - 1);
        size_t route_size = query.stops.size();
        for (size_t i = 0; i < static_cast<size_t>(route_size / 2); ++i) {
            query.stops.at(route_size - 1 - i) = query.stops.at(i);
        }
    } else {
        query.type = json_reader::jsonReader::BaseRequests::QueryType::BUS_ROUTE;
    }

}

std::vector<json_reader::jsonReader::BaseRequests> json_reader::jsonReader::ReadBaseRequests(const json::Document& doc) {
    // вычисляем количество запросов на обновление базы
    size_t query_count = doc.GetRoot().AsMap().at("base_requests").AsArray().size();
    // создаем базу запросов
    std::vector<BaseRequests> base_requests;
    base_requests.resize(query_count);
    // обрабатываем запросы
    for (size_t i = 0; i < query_count; ++i) {
        // получаем доступ к базе json
        const auto& query = doc.GetRoot().AsMap().at("base_requests").AsArray().at(i).AsMap();
        // наполняем базу
        if (query.at("type").AsString() == "Bus"s) {
            ProcessingBaseRequestBus(base_requests.at(i), query);
        } else {
            ProcessingBaseRequestStop(base_requests.at(i), query);
        }
    }
    return base_requests;
}

void ProcessingStatRequestStop(request_handler::StatRequests& query, const json::Dict& Request) {
    query.type = request_handler::StatRequests::QueryType::STOP;
    query.id = Request.at("id").AsInt();
    query.name = Request.at("name").AsString();
}

void ProcessingStatRequestBus(request_handler::StatRequests& query, const json::Dict& Request) {
    query.type = request_handler::StatRequests::QueryType::BUS;
    query.id = Request.at("id").AsInt();
    query.name = Request.at("name").AsString();
}

void ProcessingStatRequestMap(request_handler::StatRequests& query, const json::Dict& Request) {
    query.type = request_handler::StatRequests::QueryType::MAP;
    query.id = Request.at("id").AsInt();
}

void ProcessingStatRequestRoute(request_handler::StatRequests& query, const json::Dict& Request) {
    query.type = request_handler::StatRequests::QueryType::ROUTE;
    query.id = Request.at("id").AsInt();
    query.route = {Request.at("from").AsString(), Request.at("to").AsString()};
}


void ProcessingStatRequestUnknownType(request_handler::StatRequests& query, const json::Dict& stop) {
    query.type = request_handler::StatRequests::QueryType::UNKNOWN_TYPE;
    query.id = stop.at("id").AsInt();
}


void json_reader::jsonReader::ReadStatRequests(const json::Document& doc, request_handler::RequestHandler& rh) {
    // вычисляем количество запросов к базе данных
    size_t query_count = doc.GetRoot().AsMap().at("stat_requests").AsArray().size();
    // создаем базу запросов
    auto& stat_requests = rh.GetStatRequests();
    stat_requests.resize(query_count);
    // обрабатываем запросы
    for (size_t i = 0; i < query_count; ++i) {
        const auto& query = doc.GetRoot().AsMap().at("stat_requests").AsArray().at(i).AsMap();
        if (query.at("type").AsString() == "Stop"s) {
            ProcessingStatRequestStop(stat_requests.at(i), query);
        } else if (query.at("type").AsString() == "Bus"s) {
            ProcessingStatRequestBus(stat_requests.at(i), query);
        } else if (query.at("type").AsString() == "Map"s) {
            ProcessingStatRequestMap(stat_requests.at(i), query);
        } else if (query.at("type").AsString() == "Route"s) {
            ProcessingStatRequestRoute(stat_requests.at(i), query);
        } else {
            ProcessingStatRequestUnknownType(stat_requests.at(i), query);
        }
    }
}

void json_reader::jsonReader::ReadRoutingSettings(const json::Document& doc, transport_catalogue::TransportCatalogue& data) {
    const auto& routing_settings = doc.GetRoot().AsMap().at("routing_settings"s).AsMap();
    data.SetBusWaitTime(routing_settings.at("bus_wait_time").AsInt());
    data.SetBusVelocity(routing_settings.at("bus_velocity").AsInt());
}

void InsertColor(svg::Color& color, const json::Node& node) {
    if (node.IsString()) {
        color = node.AsString();
    } else if (node.AsArray().size() == 3) {
        svg::Rgb rgb{
            static_cast<uint8_t>(node.AsArray().at(0).AsInt()),
            static_cast<uint8_t>(node.AsArray().at(1).AsInt()),
            static_cast<uint8_t>(node.AsArray().at(2).AsInt())
        };
        svg::Color tmp_color{std::move(rgb)};
        color = {std::move(tmp_color)};
    } else if (node.AsArray().size() == 4) {
        svg::Rgba rgba{
            static_cast<uint8_t>(node.AsArray().at(0).AsInt()),
            static_cast<uint8_t>(node.AsArray().at(1).AsInt()),
            static_cast<uint8_t>(node.AsArray().at(2).AsInt()),
            node.AsArray().at(3).AsDouble()
        };
        svg::Color tmp_color{std::move(rgba)};
        color = {std::move(tmp_color)};
    }
}

void json_reader::jsonReader::ReadMapSettings(const json::Document& doc, map_renderer::MapRenderer& mr) {
    auto& map_settings = mr.GetMapSettings();
    const auto& render_settings = doc.GetRoot().AsMap().at("render_settings"s).AsMap();
    map_settings.width = render_settings.at("width").AsDouble();
    map_settings.height = render_settings.at("height").AsDouble();
    
    map_settings.padding = render_settings.at("padding").AsDouble();

    map_settings.line_width = render_settings.at("line_width").AsDouble();
    map_settings.stop_radius = render_settings.at("stop_radius").AsDouble();

    map_settings.bus_label_font_size = render_settings.at("bus_label_font_size").AsInt();
    map_settings.bus_label_offset.x = render_settings.at("bus_label_offset").AsArray().at(0).AsDouble();
    map_settings.bus_label_offset.y = render_settings.at("bus_label_offset").AsArray().at(1).AsDouble();
    
    map_settings.stop_label_font_size = render_settings.at("stop_label_font_size").AsInt();
    map_settings.stop_label_offset.x = render_settings.at("stop_label_offset").AsArray().at(0).AsDouble();
    map_settings.stop_label_offset.y = render_settings.at("stop_label_offset").AsArray().at(1).AsDouble();

    InsertColor(map_settings.underlayer_color, render_settings.at("underlayer_color"));
    map_settings.underlayer_width = render_settings.at("underlayer_width").AsDouble();

    const size_t color_size = render_settings.at("color_palette").AsArray().size();
    map_settings.color_palette.resize(color_size);
    const auto& color = render_settings.at("color_palette").AsArray();
    for (size_t i = 0; i < color_size; ++i) {
        InsertColor(map_settings.color_palette.at(i), color.at(i));
    }
}

void json_reader::jsonReader::ReadStopAndBus(const json::Document& doc, map_renderer::MapRenderer& mr) {
    auto& stops = mr.GetStops();
    auto& buses = mr.GetBuses();
    auto& is_roundtrip = mr.GetIsRoundTrip();
    auto& unique_stops_name_on_routes = mr.GetUniqueStops();
    // считываем документ
    const auto& base_requests = doc.GetRoot().AsMap().at("base_requests"s).AsArray();
    const size_t base_requests_size = base_requests.size();
    // заполняем
    for (size_t i = 0; i < base_requests_size; ++i) {
        // остановки
        if (base_requests.at(i).AsMap().at("type"s).AsString() == "Stop"s) {
            stops[base_requests.at(i).AsMap().at("name"s).AsString()] = {
                base_requests.at(i).AsMap().at("latitude"s).AsDouble(),
                base_requests.at(i).AsMap().at("longitude"s).AsDouble()
            };
        } else { // автобусы
            const auto& bus_name = base_requests.at(i).AsMap().at("name"s).AsString();
            const auto& buses_in_base = base_requests.at(i).AsMap().at("stops"s).AsArray();
            is_roundtrip[bus_name] = base_requests.at(i).AsMap().at("is_roundtrip"s).AsBool();
            for (const auto& bus : buses_in_base) {
                buses[bus_name].push_back(bus.AsString());
            }
        }
    }
    // все остановки в порядке возратка по одному
    for (const auto& [route_name, stops_on_route] : buses) {
        for (const auto& stop_name : stops_on_route) {
            unique_stops_name_on_routes.insert(stop_name);
        }
    }
}

void UpdateDatabase(const std::vector<json_reader::jsonReader::BaseRequests>& queries, transport_catalogue::TransportCatalogue& data) {
    // сначала добавляем остановки
    for (auto& query : queries) {
        if (query.type != json_reader::jsonReader::BaseRequests::QueryType::STOP) {
            continue;
        }
        data.AddStop(query.name, query.coordinates);
    }
    // добавляем маршруты
    for (auto& query : queries) {
        if (query.type == json_reader::jsonReader::BaseRequests::QueryType::STOP) {
            continue;
        }
        bool is_round_trip = false;
        if (query.type == json_reader::jsonReader::BaseRequests::QueryType::BUS_ROUTE) {
            is_round_trip = true;
        }
        data.AddRoute(query.name, query.stops, is_round_trip);
    }
    // добавляем дистанцию между маршрутами
    for (auto& query : queries) {
        if (query.type != json_reader::jsonReader::BaseRequests::QueryType::STOP) {
            continue;
        }
        for (auto& [from_to, distance] : query.road_distance_to_stop) {
            data.SetRoadDistanceBetweenStops(
                *data.FindStop(from_to.first),
                *data.FindStop(from_to.second),
                distance
            );
        }
    }
}

json_reader::jsonReader::jsonReader(const json::Document& doc,
                transport_catalogue::TransportCatalogue& data,
                map_renderer::MapRenderer& mr) {
    std::vector<BaseRequests> b_r = ReadBaseRequests(doc);
    ReadMapSettings(doc, mr);
    ReadStopAndBus(doc, mr);
    ReadRoutingSettings(doc, data);
    UpdateDatabase(b_r, data);
}

json_reader::jsonReader::jsonReader(const json::Document& doc,
                request_handler::RequestHandler& rh) {
    ReadStatRequests(doc, rh);
}