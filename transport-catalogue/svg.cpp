#include "svg.h"
#include <iomanip>

namespace svg {

using namespace std::literals;

std::ostream& operator<<(std::ostream& out, StrokeLineCap str) {
    switch (str) {
        case StrokeLineCap::BUTT:
            out << "butt"sv;
            break;
        case StrokeLineCap::ROUND:
            out << "round"sv;
            break;
        case StrokeLineCap::SQUARE:
            out << "square"sv;
            break;
    }
    return out;
}

std::ostream& operator<<(std::ostream& out, StrokeLineJoin str) {
    switch (str) {
        case StrokeLineJoin::ARCS:
            out << "arcs"sv;
            break;
        case StrokeLineJoin::BEVEL:
            out << "bevel"sv;
            break;
        case StrokeLineJoin::MITER:
            out << "miter"sv;
            break;
        case StrokeLineJoin::MITER_CLIP:
            out << "miter-clip"sv;
            break;
        case StrokeLineJoin::ROUND:
            out << "round"sv;
            break;
    }
    return out;
}

std::ostream& operator<<(std::ostream& out, Color color) {
    std::ostringstream strm;
    std::visit(OstreamColorPrinter{strm}, color);
    out << strm.str();
    return out;
}

void OstreamColorPrinter::operator()(std::monostate) const {
    out << "none"sv;
}

void OstreamColorPrinter::operator()(std::string str) const {
    out << str;
}

void OstreamColorPrinter::operator()(Rgb rgb) const {
    out << "rgb("sv << +rgb.red << ","sv << +rgb.green << ","sv << +rgb.blue << ")"sv;
}

void OstreamColorPrinter::operator()(Rgba rgba) const {
    out << "rgba("sv << +rgba.red << ","sv << +rgba.green << ","sv << +rgba.blue << ","sv << /* std::setprecision(1) <<  */rgba.opacity << ")"sv;
}

void Object::Render(const RenderContext& context) const {
    context.RenderIndent();

    // Делегируем вывод тега своим подклассам
    RenderObject(context);

    context.out << std::endl;
}

// ---------- Circle ------------------

Circle& Circle::SetCenter(Point center)  {
    center_ = center;
    return *this;
}

Circle& Circle::SetRadius(double radius)  {
    radius_ = radius;
    return *this;
}

void Circle::RenderObject(const RenderContext& context) const {
    auto& out = context.out;
    out << "<circle cx=\""sv << center_.x << "\" cy=\""sv << center_.y << "\" "sv;
    out << "r=\""sv << radius_ << "\""sv;
    // Выводим атрибуты, унаследованные от PathProps
    RenderAttrs(context.out);
    out << "/>"sv;
}

// ---------- Polyline ------------------

Polyline& Polyline::AddPoint(Point p) {
    points_.push_back(p);
    return *this;
}

void Polyline::RenderObject(const RenderContext& context) const {
    auto& out = context.out;
    out << "<polyline points=\""sv;
    size_t points_size = points_.size();
    if (points_size == 0) {
        out << "\" />";
        return;
    }
    for (size_t i = 0; i < points_size - 1; ++i) {
        out << points_[i].x << ","sv << points_[i].y << " "sv;
    }
    out << points_[points_size - 1].x << ","sv << points_[points_size - 1].y << "\""sv;
    // Выводим атрибуты, унаследованные от PathProps
    RenderAttrs(context.out);
    out << "/>"sv;
}

// ---------- Text ------------------

Text& Text::SetPosition(Point pos) {
    pos_ = pos;
    return *this;
}

Text& Text::SetOffset(Point offset) {
    offset_ = offset;
    return *this;
}

Text& Text::SetFontFamily(std::string name) { // по умолчанию не выводится
    font_family_ = name;
    return *this;
}

Text& Text::SetData(std::string data) { // по умолчанию текст пуст
    data_ = data;
    return *this;
}

Text& Text::SetFontSize(uint32_t size) {
    size_ = size;
    return *this;
}

Text& Text::SetFontWeight(std::string font_weight) { // по умолчанию не выводится
    font_weight_ = font_weight;
    return *this;
}

// <text x="35" y="20" dx="0" dy="6" font-size="12" font-family="Verdana" font-weight="bold">Hello C++</text>
void Text::RenderObject(const RenderContext& context) const {
    auto& out = context.out;
    out << "<text"sv;
    // Выводим атрибуты, унаследованные от PathProps
    RenderAttrs(context.out);
    out << " x=\""sv << pos_.x << "\" y=\""sv << pos_.y << "\""sv;
    out << " dx=\""sv << offset_.x << "\" dy=\""sv << offset_.y << "\""sv;
    out << " font-size=\""sv << size_ << "\""sv;
    if (font_family_.empty() == false) {
        out << " font-family=\""sv << font_family_ << "\""sv;
    }
    if (font_weight_.empty() == false) {
        out << " font-weight=\""sv << font_weight_ << "\""sv;
    }
    out << ">"sv;
    for (const auto& i : data_) {
        if (i == '"') {
            out << "&quot;"sv;
        } else if (i == '\'') {
            out << "&apos;"sv;
        } else if (i == '<') {
            out << "&lt;"sv;
        } else if (i == '>') {
            out << "&gt;"sv;
        } else if (i == '&') {
            out << "&amp;"sv;
        } else {
            out << i;
        }
    }
    out << "</text>"sv;
}

// ---------- Document ------------------

void Document::AddPtr(std::unique_ptr<Object>&& obj) {
    objects_.emplace_back((std::move(obj)));
}

void Document::Render(std::ostream& out) const {
    out << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>"sv << std::endl;
    out << "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">"sv << std::endl;
    for (const auto& doc : objects_) {
        RenderContext ctx(out, 2, 2);
        doc->Render(ctx);
    }
    out << "</svg>"sv;
}

}  // namespace svg