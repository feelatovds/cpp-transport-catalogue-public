#include "svg.h"

namespace svg {

using namespace std::literals;

// ------------------------

std::ostream& operator<<(std::ostream& out, StrokeLineCap stroke_linecap) {
    using namespace std::literals;
    switch (stroke_linecap) {
        case StrokeLineCap::BUTT:
            out << "butt"sv;
            break;
        case StrokeLineCap::ROUND:
            out << "round"sv;
            break;
        case StrokeLineCap::SQUARE:
            out << "square"sv;
            break;
        default:
            break;
    }
    return out;
}

std::ostream& operator<<(std::ostream& out, StrokeLineJoin stroke_linejoin) {
    using namespace std::literals;
    switch (stroke_linejoin) {
        case StrokeLineJoin::MITER:
            out << "miter"sv;
            break;
        case StrokeLineJoin::MITER_CLIP:
            out << "miter-clip"sv;
            break;
        case StrokeLineJoin::ROUND:
            out << "round"sv;
            break;
        case StrokeLineJoin::BEVEL:
            out << "bevel"sv;
            break;
        case StrokeLineJoin::ARCS:
            out << "arcs"sv;
            break;
        default:
            break;
    }
    return out;
}


// ------------

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
    RenderAttrs(out);
    out << " />"sv;
}

// ---------- Polyline ------------------

Polyline& Polyline::AddPoint(Point point) {
    points_.push_back(point);
    return *this;
}

void Polyline::RenderObject(const RenderContext& context) const {
    auto& out = context.out;
    out << "<polyline points=\""sv;
    bool is_first = true;
    for (const auto& p : points_) {
        if (!is_first) {
            out << " ";
        }
        is_first = false;
        out << p.x << "," << p.y;
    }
    out << "\""sv;
    RenderAttrs(out);
    out << " />"sv;
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

Text& Text::SetFontSize(uint32_t size) {
    size_ = size;
    return *this;
}

Text& Text::SetFontFamily(std::string font_family) {
    font_family_ = std::move(font_family);
    return *this;
}

Text& Text::SetFontWeight(std::string font_weight) {
    font_weight_ = std::move(font_weight);
    return *this;
}

Text& Text::SetData(std::string data) {
    size_t i = 0;
    while (i < data.size()) {
        switch (data[i]) {
            case '\"':
                data_+= "&quot;"s;
                break;
            case '\'':
                data_+= "&apos;"s;
                break;
            case '<':
                data_+= "&lt;"s;
                break;
            case '>':
                data_+= "&gt;"s;
                break;
            case '&':
                data_+= "&amp;"s;
                break;
            default:
                data_+= data[i];
                break;
        }
        ++i;
    }

    return *this;
}

void Text::RenderObject(const RenderContext& context) const {
    auto& out = context.out;
    out << "<text " << "x=\"" << pos_.x << "\" y=\"" << pos_.y;
    out << "\" dx=\"" << offset_.x << "\" dy=\"" << offset_.y;
    out << "\" font-size=\"" << size_ << "\"";
    if (!font_family_.empty()) {
        out << " font-family=\"" << font_family_ << "\"";
    }
    if (!font_weight_.empty()) {
        out << " font-weight=\"" << font_weight_ << "\"";
    }
    RenderAttrs(out);
    out << ">";
    out << data_;
    out << "</text>";
}

// ---------- Document ------------------

void Document::AddPtr(std::unique_ptr<Object>&& obj) {
    objects_.push_back(std::move(obj));
}

void Document::Render(std::ostream& out) const {
    out << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n";
    out << "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">\n";

    for (const auto& obj : objects_) {
        out << "  ";
        obj->Render(out);

    }
    out << "</svg>";
}

}  // namespace svg