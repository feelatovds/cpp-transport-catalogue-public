#pragma once


#include <cstdint>
#include <array>
#include <vector>
#include <set>
#include <deque>
#include <unordered_map>
#include <string_view>
#include <functional>
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <vector>


#include "geo.h"
#include "domain.h"
#include "svg.h"
#include "json.h"

using namespace domain;
using namespace svg;
using namespace json;

namespace map_renderer {

struct RenderSettings {
    RenderSettings(const Dict& source);
    RenderSettings();

    double width;
    double height;

    double padding;

    double line_width;
    double stop_radius;

    uint32_t bus_label_font_size;
    std::array<double, 2> bus_label_offset;

    uint32_t stop_label_font_size;
    std::array<double, 2> stop_label_offset;

    Color underlayer_color;
    double underlayer_width;

    std::vector<Color> color_palette;
};

inline const double EPSILON = 1e-6;
bool IsZero(double value);

class SphereProjector {
public:
    // points_begin и points_end задают начало и конец интервала элементов geo::Coordinates
    template <typename PointInputIt>
    SphereProjector(PointInputIt points_begin, PointInputIt points_end,
                    double max_width, double max_height, double padding);

    // Проецирует широту и долготу в координаты внутри SVG-изображения
    svg::Point operator()(geo::Coordinates coords) const;

private:
    double padding_;
    double min_lon_ = 0;
    double max_lat_ = 0;
    double zoom_coeff_ = 0;
};

class MapRoute : public svg::Drawable {
public:

    MapRoute(const ActualCoordinates& actual_coordinates,
             const RenderSettings& render_settings,
             const ActualBuses& actual_buses,
             const PeekBuses& peek_buses,
             const ActualStops& actual_stops,
             const PeekStops& peek_stops);

    void Draw(svg::ObjectContainer& container) const override;

private:
    SphereProjector proj_;
    ActualBuses actual_buses_;
    PeekBuses peek_buses_;
    RenderSettings render_settings_;
    ActualStops actual_stops_;
    PeekStops peek_stops_;

    std::vector<Polyline> lines_buses_;
    std::vector<Text> names_buses_;
    std::vector<Circle> circle_stops_;
    std::vector<Text> names_stops_;

    void CreateLinesBuses();
    void CreateNamesBuses();
    void CreateCircleStops();
    void CreateNamesStops();

};

class MapRenderer {
public:
    MapRenderer();

    MapRenderer(const Dict& render_settings,
                Stops stops,
                PeekStops peek_stops,
                Buses buses,
                PeekBuses peek_buses,
                ActualBuses actual_buses,
                ActualCoordinates actual_coordinates,
                ActualStops actual_stops);

    MapRenderer(const RenderSettings& render_settings,
                Stops stops,
                PeekStops peek_stops,
                Buses buses,
                PeekBuses peek_buses,
                ActualBuses actual_buses,
                ActualCoordinates actual_coordinates,
                ActualStops actual_stops);

    MapRoute Render() const;

    const RenderSettings& GetRenderSettings() const;
    const ActualCoordinates& GetActualCoordinates() const;

private:
    RenderSettings render_settings_;
    Stops stops_;
    PeekStops peek_stops_;
    Buses buses_;
    PeekBuses peek_buses_;
    ActualBuses actual_buses_;
    ActualCoordinates actual_coordinates_;
    ActualStops actual_stops_;

};

template <typename PointInputIt>
SphereProjector::SphereProjector(PointInputIt points_begin, PointInputIt points_end,
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

}  // namespace map_renderer