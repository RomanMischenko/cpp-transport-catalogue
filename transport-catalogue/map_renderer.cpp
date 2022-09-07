#include "map_renderer.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <vector>

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

void DrawRoute(svg::Document& to_draw,
            const std::unordered_map<std::string, geo::Coordinates>& stops, 
            const std::map<std::string, std::vector<std::string>>& buses,
            const std::unordered_map<std::string, bool>& is_roundtrip,
            const map_renderer::MapSettings& map_settings,
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
            const std::unordered_map<std::string, geo::Coordinates>& stops, 
            const std::map<std::string, std::vector<std::string>>& buses,
            const std::unordered_map<std::string, bool>& is_roundtrip,
            const map_renderer::MapSettings& map_settings,
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
            const std::unordered_map<std::string, geo::Coordinates>& stops, 
            const map_renderer::MapSettings& map_settings,
            const SphereProjector& proj,
            const std::set<std::string>& unique_stops_name_on_routes) {
    for (const auto& stop_name : unique_stops_name_on_routes) {
        svg::Circle circle;
        circle.SetCenter(proj(stops.at(stop_name)));
        circle.SetRadius(map_settings.stop_radius);
        circle.SetFillColor("white"s);
        to_draw.Add(circle);
    }
    
}

void DrawStopName(svg::Document& to_draw,
            const std::unordered_map<std::string, geo::Coordinates>& stops, 
            const map_renderer::MapSettings& map_settings,
            const SphereProjector& proj,
            const std::set<std::string>& unique_stops_name_on_routes) {
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

void map_renderer::MapRenderer::Draw(std::ostream& out) const {
    svg::Document to_draw_;
    // переводим координаты на плоскость
    // Точки, подлежащие проецированию
    std::vector<geo::Coordinates> geo_coords;
    for (const auto& [route_name, stops_on_route] : buses_) {
        for (const auto& bus_name: stops_on_route) {
            geo_coords.push_back(stops_.at(bus_name));
        }
    }
    // проециоруем координаты на плоскость
    const double WIDTH = map_settings_.width;
    const double HEIGHT = map_settings_.height;
    const double PADDING = map_settings_.padding;
    // Создаём проектор сферических координат на карту
    const SphereProjector proj{
        geo_coords.begin(), geo_coords.end(), WIDTH, HEIGHT, PADDING
    };
    // выполняем отрисовку маршрутов
    DrawRoute(to_draw_, stops_, buses_, is_roundtrip_, map_settings_, proj);
    // выполняем отрисовку названий для маршрутов
    DrawRouteName(to_draw_, stops_, buses_, is_roundtrip_, map_settings_, proj);
    // выполняем отрисовку символов остановки
    DrawStopSymbol(to_draw_, stops_, map_settings_, proj, unique_stops_name_on_routes_);
    // выполняем отрисовку названий остановок
    DrawStopName(to_draw_, stops_, map_settings_, proj, unique_stops_name_on_routes_);
    // выводим SVG документ
    to_draw_.Render(out);
}

map_renderer::MapSettings& map_renderer::MapRenderer::GetMapSettings() {
    return map_settings_;
}

const map_renderer::MapSettings& map_renderer::MapRenderer::GetMapSettings() const {
    return map_settings_;
}

std::unordered_map<std::string, geo::Coordinates>& map_renderer::MapRenderer::GetStops() {
    return stops_;
}

std::map<std::string, std::vector<std::string>>& map_renderer::MapRenderer::GetBuses() {
    return buses_;
}

std::unordered_map<std::string, bool>& map_renderer::MapRenderer::GetIsRoundTrip() {
    return is_roundtrip_;
}

std::set<std::string>& map_renderer::MapRenderer::GetUniqueStops() {
    return unique_stops_name_on_routes_;
}