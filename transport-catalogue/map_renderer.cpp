#include "map_renderer.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <vector>
#include <set>

// подключаем ""s
using std::literals::string_literals::operator""s;
using std::literals::string_view_literals::operator""sv;

inline const double EPSILON = 1e-6;
bool IsZero(double value) {
    return std::abs(value) < EPSILON;
}

class SphereProjector {
public:
    // points_begin и points_end задают начало и конец интервала элементов geo::Coordinates
    template <typename PointInputIt>
    SphereProjector(PointInputIt points_begin, PointInputIt points_end,
                    double max_width, double max_height, double padding)
        : padding_(padding) //
    {
        // Если точки поверхности сферы не заданы, вычислять нечего
        if (points_begin == points_end) {
            return;
        }

        // Находим точки с минимальной и максимальной долготой
        const auto [left_it, right_it] = std::minmax_element(
            points_begin, points_end,
            [](auto lhs, auto rhs) { return lhs.lng < rhs.lng; });
        min_lon_ = left_it->lng;
        const double max_lon = right_it->lng;

        // Находим точки с минимальной и максимальной широтой
        const auto [bottom_it, top_it] = std::minmax_element(
            points_begin, points_end,
            [](auto lhs, auto rhs) { return lhs.lat < rhs.lat; });
        const double min_lat = bottom_it->lat;
        max_lat_ = top_it->lat;

        // Вычисляем коэффициент масштабирования вдоль координаты x
        std::optional<double> width_zoom;
        if (!IsZero(max_lon - min_lon_)) {
            width_zoom = (max_width - 2 * padding) / (max_lon - min_lon_);
        }

        // Вычисляем коэффициент масштабирования вдоль координаты y
        std::optional<double> height_zoom;
        if (!IsZero(max_lat_ - min_lat)) {
            height_zoom = (max_height - 2 * padding) / (max_lat_ - min_lat);
        }

        if (width_zoom && height_zoom) {
            // Коэффициенты масштабирования по ширине и высоте ненулевые,
            // берём минимальный из них
            zoom_coeff_ = std::min(*width_zoom, *height_zoom);
        } else if (width_zoom) {
            // Коэффициент масштабирования по ширине ненулевой, используем его
            zoom_coeff_ = *width_zoom;
        } else if (height_zoom) {
            // Коэффициент масштабирования по высоте ненулевой, используем его
            zoom_coeff_ = *height_zoom;
        }
    }

    // Проецирует широту и долготу в координаты внутри SVG-изображения
    svg::Point operator()(geo::Coordinates coords) const {
        return {
            (coords.lng - min_lon_) * zoom_coeff_ + padding_,
            (max_lat_ - coords.lat) * zoom_coeff_ + padding_
        };
    }

private:
    double padding_;
    double min_lon_ = 0;
    double max_lat_ = 0;
    double zoom_coeff_ = 0;
};

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

void SetMapSettings(const json::Document& doc, MapSettings& map_settings) {
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

void SetStopAndBus(const json::Document& doc, 
                std::unordered_map<std::string_view, geo::Coordinates>& stops, 
                std::map<std::string_view, std::vector<std::string_view>>& buses,
                std::unordered_map<std::string_view, bool>& is_roundtrip,
                std::set<std::string_view>& unique_stops_name_on_routes) {
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

void DrawRoute(svg::Document& to_draw,
            const std::unordered_map<std::string_view, geo::Coordinates>& stops, 
            const std::map<std::string_view, std::vector<std::string_view>>& buses,
            const std::unordered_map<std::string_view, bool>& is_roundtrip,
            const MapSettings& map_settings,
            const SphereProjector& proj) {
    
    // Выполняем отрисовку
    // счетчик цветовой палитры
    int color_counter = 0;
    int color_counter_max = map_settings.color_palette.size();
    for (const auto& [route_name, stops_on_route] : buses) {
        // задаем параменты линии
        svg::Polyline line;
        if (color_counter == color_counter_max) {
            color_counter = 0;
        }
        line.SetStrokeColor(map_settings.color_palette.at(color_counter)).SetFillColor("none"s).SetStrokeWidth(map_settings.line_width).SetStrokeLineCap(svg::StrokeLineCap::ROUND).SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
        color_counter++;
        // добавляем точку
        auto it_begin = stops_on_route.begin();
        auto it_end = stops_on_route.end();
        auto it_rbegon = stops_on_route.rbegin();
        auto it_rend = stops_on_route.rend();
        for (auto& it = it_begin; it != it_end; ++it) {
            line.AddPoint(proj(stops.at(*it)));
        }
        // если маршрут некольцевой
        // рисуем в обратном направлении
        if (!is_roundtrip.at(route_name)) {
            for (auto it = std::next(it_rbegon); it != it_rend; ++it) {
                line.AddPoint(proj(stops.at(*it)));
            }
        }
        to_draw.Add(line);
    }
}

void DrawRouteName(svg::Document& to_draw,
            const std::unordered_map<std::string_view, geo::Coordinates>& stops, 
            const std::map<std::string_view, std::vector<std::string_view>>& buses,
            const std::unordered_map<std::string_view, bool>& is_roundtrip,
            const MapSettings& map_settings,
            const SphereProjector& proj) {
    
    // счетчик цветовой палитры
    int color_counter = 0;
    int color_counter_max = map_settings.color_palette.size();
    for (const auto& [route_name, stops_on_route] : buses) {
        if (color_counter == color_counter_max) {
            color_counter = 0;
        }
        // первая и последняя остановка
        auto it_begin = stops_on_route.begin();
        auto it_end = stops_on_route.rbegin();
        // выводим название первой остановки
        svg::Text substrate, text; // подложка и надпись
        // общие свойства
        substrate.SetPosition(proj(stops.at(*it_begin)));
        substrate.SetOffset(map_settings.bus_label_offset);
        substrate.SetFontSize(map_settings.bus_label_font_size);
        substrate.SetFontFamily("Verdana"s);
        substrate.SetFontWeight("bold"s);
        substrate.SetData(std::string(route_name));
        text.SetPosition(proj(stops.at(*it_begin)));
        text.SetOffset(map_settings.bus_label_offset);
        text.SetFontSize(map_settings.bus_label_font_size);
        text.SetFontFamily("Verdana"s);
        text.SetFontWeight("bold"s);
        text.SetData(std::string(route_name));
        // дополнительные свойства подложки
        substrate.SetFillColor(map_settings.underlayer_color);
        substrate.SetStrokeColor(map_settings.underlayer_color);
        substrate.SetStrokeWidth(map_settings.underlayer_width);
        substrate.SetStrokeLineCap(svg::StrokeLineCap::ROUND);
        substrate.SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
        // дополнительное свойство надписи
        text.SetFillColor(map_settings.color_palette.at(color_counter));
        // отрисовка
        to_draw.Add(substrate);
        to_draw.Add(text);
        // если маршрут некольцевой 
        // и конечные не совпадают, — для второй конечной
        if (!is_roundtrip.at(route_name) && *it_begin != *it_end) {
            svg::Text text_end, substrate_end; // надпись и подложка
            // общие свойства
            substrate_end.SetPosition(proj(stops.at(*it_end)));
            substrate_end.SetOffset(map_settings.bus_label_offset);
            substrate_end.SetFontSize(map_settings.bus_label_font_size);
            substrate_end.SetFontFamily("Verdana"s);
            substrate_end.SetFontWeight("bold"s);
            substrate_end.SetData(std::string(route_name));
            text_end.SetPosition(proj(stops.at(*it_end)));
            text_end.SetOffset(map_settings.bus_label_offset);
            text_end.SetFontSize(map_settings.bus_label_font_size);
            text_end.SetFontFamily("Verdana"s);
            text_end.SetFontWeight("bold"s);
            text_end.SetData(std::string(route_name));
            // дополнительные свойства подложки
            substrate_end.SetFillColor(map_settings.underlayer_color);
            substrate_end.SetStrokeColor(map_settings.underlayer_color);
            substrate_end.SetStrokeWidth(map_settings.underlayer_width);
            substrate_end.SetStrokeLineCap(svg::StrokeLineCap::ROUND);
            substrate_end.SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
            // дополнительное свойство надписи
            text_end.SetFillColor(map_settings.color_palette.at(color_counter));
            // отрисовка
            to_draw.Add(substrate_end);
            to_draw.Add(text_end);
        }
        color_counter++;
    }
}

void DrawStopSymbol(svg::Document& to_draw,
            const std::unordered_map<std::string_view, geo::Coordinates>& stops, 
            const MapSettings& map_settings,
            const SphereProjector& proj,
            const std::set<std::string_view>& unique_stops_name_on_routes) {
    for (const auto& stop_name : unique_stops_name_on_routes) {
        svg::Circle circle;
        circle.SetCenter(proj(stops.at(stop_name)));
        circle.SetRadius(map_settings.stop_radius);
        circle.SetFillColor("white"s);
        to_draw.Add(circle);
    }
    
}

void DrawStopName(svg::Document& to_draw,
            const std::unordered_map<std::string_view, geo::Coordinates>& stops, 
            const MapSettings& map_settings,
            const SphereProjector& proj,
            const std::set<std::string_view>& unique_stops_name_on_routes) {
    for (const auto& stop_name : unique_stops_name_on_routes) {
        svg::Text substrate, text; // подложка и надпись
        // общие свойства
        substrate.SetPosition(proj(stops.at(stop_name)));
        substrate.SetOffset(map_settings.stop_label_offset);
        substrate.SetFontSize(map_settings.stop_label_font_size);
        substrate.SetFontFamily("Verdana"s);
        substrate.SetData(std::string(stop_name));
        text.SetPosition(proj(stops.at(stop_name)));
        text.SetOffset(map_settings.stop_label_offset);
        text.SetFontSize(map_settings.stop_label_font_size);
        text.SetFontFamily("Verdana"s);
        text.SetData(std::string(stop_name));
        // дополнительные свойства подложки
        substrate.SetFillColor(map_settings.underlayer_color);
        substrate.SetStrokeColor(map_settings.underlayer_color);
        substrate.SetStrokeWidth(map_settings.underlayer_width);
        substrate.SetStrokeLineCap(svg::StrokeLineCap::ROUND);
        substrate.SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
        // дополнительное свойство надписи
        text.SetFillColor("black"s);
        // отрисовка
        to_draw.Add(substrate);
        to_draw.Add(text);
    }
}
    
void map_renderer::MapRenderer(const json::Document& doc, std::ostream& out) {
    svg::Document to_draw;
    MapSettings map_settings;
    std::unordered_map<std::string_view, geo::Coordinates> stops;
    std::map<std::string_view, std::vector<std::string_view>> buses;
    std::unordered_map<std::string_view, bool> is_roundtrip;
    std::set<std::string_view> unique_stops_name_on_routes;
    // устанавливаем настройки render_settings
    SetMapSettings(doc, map_settings);
    // считываем остановки и автобусы из документа
    SetStopAndBus(doc, stops, buses, is_roundtrip, unique_stops_name_on_routes);
    
    // переводим координаты на плоскость
    // Точки, подлежащие проецированию
    std::vector<geo::Coordinates> geo_coords;
    for (const auto& [route_name, stops_on_route] : buses) {
        for (const auto& bus_name: stops_on_route) {
            geo_coords.push_back(stops.at(bus_name));
        }
    }
    // проециоруем координаты на плоскость
    const double WIDTH = map_settings.width;
    const double HEIGHT = map_settings.height;
    const double PADDING = map_settings.padding;
    // Создаём проектор сферических координат на карту
    const SphereProjector proj{
        geo_coords.begin(), geo_coords.end(), WIDTH, HEIGHT, PADDING
    };

    // выполняем отрисовку маршрутов
    DrawRoute(to_draw, stops, buses, is_roundtrip, map_settings, proj);
    // выполняем отрисовку названий для маршрутов
    DrawRouteName(to_draw, stops, buses, is_roundtrip, map_settings, proj);
    // выполняем отрисовку символов остановки
    DrawStopSymbol(to_draw, stops, map_settings, proj, unique_stops_name_on_routes);
    // выполняем отрисовку названий остановок
    DrawStopName(to_draw, stops, map_settings, proj, unique_stops_name_on_routes);
    // выводим SVG документ
    to_draw.Render(out);
}