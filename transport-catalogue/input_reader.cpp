#include "input_reader.h"

void input_reader::ProcessingRequestStop(detail::Query& query, const std::string& text) {
    auto pos_colon = text.find(':');
    // окончание строки
    auto text_npos = text.npos;
    query.type = detail::QueryType::STOP;

    // поиск разделителя
    auto pos_separator = text.find(" to ");
    auto pos_comma = text.find(',');
    // добавляем координаты
    // широта
    auto latitude_begin = text.find_first_not_of(' ', pos_colon + 1);
    auto latitude_end = text.find_last_not_of(' ', pos_comma);
    query.coordinates.lat = std::stod(std::string(text.substr(latitude_begin, latitude_end - latitude_begin)));
    // долгота
    auto longitude_begin = text.find_first_not_of(' ', pos_comma + 1);
    auto longitude_end = text.find_last_not_of(' ') + 1;
    query.coordinates.lng = std::stod(std::string(text.substr(longitude_begin, longitude_end - longitude_begin)));
    // если имеется расстояние в метрах
    if (pos_separator != text_npos) {
        // находим следующий разделитель
        pos_comma = text.find(',', pos_comma + 1);
        do {
            // находим расстояние до
            auto dist_to_begin = text.find_first_not_of(' ', pos_comma + 1);
            auto dist_to_end = text.find_last_not_of(' ', pos_separator - 1) + 1;
            // находим остановку до
            pos_comma = text.find(',', pos_comma + 1);
            auto stop_to_begin = text.find_first_not_of(' ', pos_separator + 3);
            auto stop_to_end = pos_comma;
            // находим следующий разделитель " to "
            pos_separator = text.find(" to ", pos_separator + 1);
            // вычисляем дистанцию
            double distance = std::stod(text.substr(dist_to_begin, dist_to_end - dist_to_begin - 1));
            // добавляем остановку
            query.road_distance_to_stop.emplace(
                std::pair{query.name, text.substr(stop_to_begin, stop_to_end - stop_to_begin)},
                distance
            );   
        } while (pos_comma != text_npos);
    }
}

void input_reader::ProcessingRequestBus(detail::Query& query, const std::string& text) {
    auto pos_colon = text.find(':');
    // окончание строки
    auto text_npos = text.npos;
    // поиск разделителя
    auto pos_dash = text.find('-');
    auto pos_greater_than = text.find('>');

    // функция добавления остановок в запрос
    auto AddStopInQuery = [pos_colon, text_npos](detail::Query& query, const std::string& text, char sep, size_t pos_sep){
        // добавляем остановки
        auto stop_name_begin = text.find_first_not_of(' ', pos_colon + 1);
        auto stop_name_end = text.find_last_not_of(' ', pos_sep - 1) + 1;
        // пока имеется разделитель
        while (pos_sep != text_npos) {
            // добавляем остановку
            query.stops.push_back(text.substr(stop_name_begin, stop_name_end - stop_name_begin));
            // находим следующую остановку
            stop_name_begin = text.find_first_not_of(' ', pos_sep + 1);
            pos_sep = text.find(sep, pos_sep + 1);
            stop_name_end = text.find_last_not_of(' ', pos_sep - 1) + 1;
        }
        // если была всего 1 остановка, то не попадаем в цикл
        // необходимо её учесть 
        // и/или добавить последнюю
        query.stops.push_back(text.substr(stop_name_begin, stop_name_end - stop_name_begin));
    };

    // если маршрут прямой
    if (pos_dash != text_npos) {
        query.type = detail::QueryType::BUS_LINE;   

        AddStopInQuery(query, text, '-', pos_dash);

        // добавляем обратный маршрут
        query.stops.resize(query.stops.size() * 2 - 1);
        size_t route_size = query.stops.size();
        for (auto i = 0; i < route_size / 2; ++i) {
            query.stops.at(route_size - 1 - i) = query.stops.at(i);
        }
    } else if (pos_greater_than != text_npos) { // если маршрут кольцевой
        query.type = detail::QueryType::BUS_ROUTE;
        
        AddStopInQuery(query, text, '>', pos_greater_than);
    }
}

void input_reader::QueryStringProcessing(detail::Query& query, const std::string& text) {
    std::string command;
    // поиск команды
    auto pos_queries_type_begin = text.find_first_not_of(' ');
    auto pos_queries_type_end = text.find_first_of(' ', pos_queries_type_begin + 1);
    command = text.substr(pos_queries_type_begin, (pos_queries_type_end - pos_queries_type_begin));
    // добавляем имя запроса
    auto pos_colon = text.find(':');
    auto pos_queries_name_begin = text.find_first_not_of(' ', pos_queries_type_end + 1);
    query.name = text.substr(pos_queries_name_begin, (pos_colon - pos_queries_name_begin));
    if (command == "Stop") {
        ProcessingRequestStop(query, text);
    } else if (command == "Bus") {
        ProcessingRequestBus(query, text);
    }
}

void input_reader::UpdateDatabase(std::vector<detail::Query>& queries, transport_catalogue::TransportCatalogue& data) {
    // сначала добавляем остановки
    for (auto& query : queries) {
        if (query.type != detail::QueryType::STOP) {
            continue;
        }
        data.AddStop(query.name, query.coordinates);
    }
    // добавляем маршруты
    for (auto& query : queries) {
        if (query.type == detail::QueryType::STOP) {
            continue;
        }
        data.AddRoute(query.name, query.stops);
    }
    // добавляем дистанцию между маршрутами
    for (auto& query : queries) {
        if (query.type != detail::QueryType::STOP) {
            continue;
        }
        for (auto& [from_to, distance] : query.road_distance_to_stop) {
            data.SetDistanceBetweenStops(
                *data.FindStop(from_to.first),
                *data.FindStop(from_to.second),
                distance
            );
        }
    }
}

void input_reader::InputReader(std::istream& input, transport_catalogue::TransportCatalogue& data) {
    // обрабатываем целое число
    std::string text_tmp;
    std::getline(input, text_tmp);
    int number_of_requests = std::stoi(text_tmp);

    // создаем базу запросов
    std::vector<detail::Query> queries(number_of_requests);
    // обрабатываем запросы
    for (int i = 0; i < number_of_requests; ++i) {
        std::getline(input, text_tmp);
        QueryStringProcessing(queries.at(i), text_tmp);
    }

    // обновляем каталог
    UpdateDatabase(queries, data);
}