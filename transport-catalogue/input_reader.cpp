#include "input_reader.h"

void QueryStringProcessing(Query& query, std::string& text) {
    std::string word_tmp;
    
    auto pos_queries_type_begin = text.find_first_not_of(' ');
    auto pos_queries_type_end = text.find_first_of(' ', pos_queries_type_begin + 1);
    word_tmp = text.substr(pos_queries_type_begin, (pos_queries_type_end - pos_queries_type_begin));
    if (word_tmp == "Stop") {
        // окончание строки
        auto text_npos = text.npos;
        query.type = QueryType::STOP;

        // добавляем имя запроса
        auto pos_colon = text.find(':');
        auto pos_queries_name_begin = text.find_first_not_of(' ', pos_queries_type_end + 1);
        query.name = text.substr(pos_queries_name_begin, (pos_colon - pos_queries_name_begin));
        //query.name = std::move(word_tmp);
        
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
            pos_comma = text.find(',', pos_comma + 1);
            // находим расстояние до
            auto dist_to_begin = text.find_first_not_of(' ', pos_comma + 1);
            auto dist_to_end = text.find_last_not_of(' ', pos_separator - 1) + 1;
            // находим остановку до
            pos_comma = text.find(',', pos_comma + 1);
            auto stop_to_begin = text.find_first_not_of(' ', pos_separator + 3);
            auto stop_to_end = pos_comma;
            while (pos_comma != text_npos) {
                // вычисляем дистанцию
                double distance = std::stod(text.substr(dist_to_begin, dist_to_end - dist_to_begin - 1));
                // добавляем остановку
                query.road_distance_to_stop.emplace(
                    std::pair{query.name, text.substr(stop_to_begin, stop_to_end - stop_to_begin)},
                    distance
                );
                // вычисляем следующий промежуток
                pos_separator = text.find(" to ", pos_separator + 1);
                dist_to_begin = text.find_first_not_of(' ', pos_comma + 1);
                dist_to_end = text.find_last_not_of(' ', pos_separator - 1) + 1;
                pos_comma = text.find(',', pos_comma + 1);
                stop_to_begin = text.find_first_not_of(' ', pos_separator + 3);
                stop_to_end = pos_comma;
            }
            // если была всего 1 остановка, то не попадаем в цикл
            // необходимо её учесть 
            // и/или добавить последнюю
            double distance = std::stod(text.substr(dist_to_begin, dist_to_end - dist_to_begin - 1));
            query.road_distance_to_stop.emplace(
                std::pair{query.name, text.substr(stop_to_begin, stop_to_end - stop_to_begin)},
                distance
            );
        }
    } else if (word_tmp == "Bus") {
        // окончание строки
        auto text_npos = text.npos;
        // поиск разделителя
        auto pos_dash = text.find('-');
        auto pos_greater_than = text.find('>');
        // если маршрут прямой
        if (pos_dash != text_npos) {
            query.type = QueryType::BUS_LINE;

            // добавляем имя запроса
            auto pos_colon = text.find(':');
            auto pos_queries_name_begin = text.find_first_not_of(' ', pos_queries_type_end + 1);
            query.name = text.substr(pos_queries_name_begin, (pos_colon - pos_queries_name_begin));
            //query.name = std::move(word_tmp);

            // добавляем остановки
            auto stop_name_begin = text.find_first_not_of(' ', pos_colon + 1);
            auto stop_name_end = text.find_last_not_of(' ', pos_dash - 1) + 1;
            int stop_count = 0; // счетчик остановок
            // пока имеется разделитель
            while (pos_dash != text_npos) {
                // расширяем хранилище
                if (query.stops.size() <= stop_count) {
                    query.stops.resize(stop_count * 2 + 1);
                }
                // добавляем остановку
                query.stops.at(stop_count++) = text.substr(stop_name_begin, stop_name_end - stop_name_begin);
                // находим следующую остановку
                stop_name_begin = text.find_first_not_of(' ', pos_dash + 1);
                pos_dash = text.find('-', pos_dash + 1);
                stop_name_end = text.find_last_not_of(' ', pos_dash - 1) + 1;
            }
            // если была всего 1 остановка, то не попадаем в цикл
            // необходимо её учесть 
            // и/или добавить последнюю
            if (query.stops.size() <= stop_count) {
                query.stops.resize(stop_count * 2 + 1);
            }
            query.stops.at(stop_count++) = text.substr(stop_name_begin, stop_name_end - stop_name_begin);
            // так как маршрут прямой, то добавляем обратную дорогу
            // при этом размер массива будет равен = (кол-во остановок * 2 - 1)
            query.stops.resize(stop_count * 2 - 1);
            size_t route_size = query.stops.size();
            for (auto i = 0; i < query.stops.size() / 2; ++i) {
                query.stops.at(route_size - 1 - i) = query.stops.at(i);
            }
        } else if (pos_greater_than != text_npos) { // если маршрут кольцевой
            query.type = QueryType::BUS_ROUTE;

            // добавляем имя запроса
            auto pos_colon = text.find(':');
            auto pos_queries_name_begin = text.find_first_not_of(' ', pos_queries_type_end + 1);
            query.name = text.substr(pos_queries_name_begin, (pos_colon - pos_queries_name_begin));

            // добавляем остановки
            auto stop_name_begin = text.find_first_not_of(' ', pos_colon + 1);
            auto stop_name_end = text.find_last_not_of(' ', pos_greater_than - 1) + 1;
            int stop_count = 0; // счетчик остановок
            // пока имеется разделитель
            while (pos_greater_than != text_npos) {
                // расширяем хранилище
                if (query.stops.size() <= stop_count) {
                    query.stops.resize(stop_count * 2 + 1);
                }
                // добавляем остановку
                query.stops.at(stop_count++) = text.substr(stop_name_begin, stop_name_end - stop_name_begin);
                // находим следующую остановку
                stop_name_begin = text.find_first_not_of(' ', pos_greater_than + 1);
                pos_greater_than = text.find('>', pos_greater_than + 1);
                stop_name_end = text.find_last_not_of(' ', pos_greater_than - 1) + 1;
            }
            // если была всего 1 остановка, то не попадаем в цикл
            // необходимо её учесть 
            // и/или добавить последнюю
            if (query.stops.size() <= stop_count) {
                query.stops.resize(stop_count * 2 + 1);
            }
            query.stops.at(stop_count++) = text.substr(stop_name_begin, stop_name_end - stop_name_begin);
            // изменяем размер хранилища под кол-во остановок
            query.stops.resize(stop_count);
        }
    }
}

void UpdateDatabase(std::vector<Query>& queries, TransportCatalogue& data) {
    // вариант с сорировкой
    // ...
    // два прохода
    // сначала добавляем остановки
    for (auto& query : queries) {
        if (query.type != QueryType::STOP) {
            continue;
        }
        data.AddStop(query.name, query.coordinates);
    }
    // добавляем маршруты
    for (auto& query : queries) {
        if (query.type == QueryType::STOP) {
            continue;
        }
        data.AddRoute(query.name, query.stops);
    }
    // добавляем дистанцию между маршрутами
    for (auto& query : queries) {
        if (query.type != QueryType::STOP) {
            continue;
        }
        for (auto& [from_to, distance] : query.road_distance_to_stop) {
            std::pair<Stop *, Stop *> tmp_pair;
            tmp_pair.first = data.FindStop(from_to.first);
            tmp_pair.second = data.FindStop(from_to.second);
            data.AddingDistanceBetweenStops(
                tmp_pair,
                distance
            );
        }
    }
}

void InputReader(std::istream& input, TransportCatalogue& data) {
    // обрабатываем целое число
    std::string text_tmp;
    std::getline(input, text_tmp);
    int N = std::stoi(text_tmp);

    // создаем базу запросов
    std::vector<Query> queries(N);
    // обрабатываем запросы
    for (int i = 0; i < N; ++i) {
        std::getline(input, text_tmp);
        QueryStringProcessing(queries.at(i), text_tmp);
    }

    // обновляем каталог
    UpdateDatabase(queries, data);
}