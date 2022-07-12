#include "json_reader.h"

// подключаем ""s
using std::literals::string_literals::operator""s;

void ProcessingRequestStop(domain::Query& query, const json::Dict& stop) {
    query.type = domain::QueryType::STOP;
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

void ProcessingRequestBus(domain::Query& query, const json::Dict& bus) {
    query.name = bus.at("name"s).AsString();
    // получаем остановки из базы данных
    const auto& stops = bus.at("stops"s).AsArray();
    // добавляем остновки
    for (const auto& stop : stops) {
        query.stops.push_back(stop.AsString());
    }
    // если маршрут прямой
    if (!(bus.at("is_roundtrip"s).AsBool())) {
        query.type = domain::QueryType::BUS_LINE;
        // добавляем обратный маршрут
        query.stops.resize(query.stops.size() * 2 - 1);
        size_t route_size = query.stops.size();
        for (size_t i = 0; i < static_cast<size_t>(route_size / 2); ++i) {
            query.stops.at(route_size - 1 - i) = query.stops.at(i);
        }
    } else {
        query.type = domain::QueryType::BUS_ROUTE;
    }

}

void UpdateDatabase(std::vector<domain::Query>& queries, transport_catalogue::TransportCatalogue& data) {
    // сначала добавляем остановки
    for (auto& query : queries) {
        if (query.type != domain::QueryType::STOP) {
            continue;
        }
        data.AddStop(query.name, query.coordinates);
    }
    // добавляем маршруты
    for (auto& query : queries) {
        if (query.type == domain::QueryType::STOP) {
            continue;
        }
        data.AddRoute(query.name, query.stops);
    }
    // добавляем дистанцию между маршрутами
    for (auto& query : queries) {
        if (query.type != domain::QueryType::STOP) {
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

void json_reader::jsonReader(const json::Document& doc, ::transport_catalogue::TransportCatalogue& data) {

    // вычисляем количество запросов на обновление базы
    size_t query_count = doc.GetRoot().AsMap().at("base_requests").AsArray().size();
    // создаем базу запросов
    std::vector<domain::Query> queries(query_count);
    // обрабатываем запросы
    for (size_t i = 0; i < query_count; ++i) {
        // получаем доступ к базе json
        const auto& query = doc.GetRoot().AsMap().at("base_requests").AsArray().at(i).AsMap();
        // наполняем базу
        if (query.at("type").AsString() == "Bus"s) {
            ProcessingRequestBus(queries.at(i), query);
        } else {
            ProcessingRequestStop(queries.at(i), query);
        }
    }

    // обновляем каталог
    UpdateDatabase(queries, data);
}