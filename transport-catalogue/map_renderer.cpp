#include "map_renderer.h"

namespace map_renderer {

RenderSettings::RenderSettings() {}

RenderSettings::RenderSettings(const Dict& source) {
    width = source.at("width"s).AsDouble();
    height = source.at("height"s).AsDouble();

    padding = source.at("padding"s).AsDouble();

    line_width = source.at("line_width"s).AsDouble();
    stop_radius = source.at("stop_radius"s).AsDouble();

    bus_label_font_size = source.at("bus_label_font_size"s).AsInt();
    auto bus_lab_off = source.at("bus_label_offset"s).AsArray();
    bus_label_offset = {bus_lab_off[0].AsDouble(), bus_lab_off[1].AsDouble()};

    stop_label_font_size = source.at("stop_label_font_size"s).AsInt();
    const auto stop_lab_off = source.at("stop_label_offset"s).AsArray();
    stop_label_offset = {stop_lab_off[0].AsDouble(), stop_lab_off[1].AsDouble()};

    if (source.at("underlayer_color"s).IsString()) {
        underlayer_color = source.at("underlayer_color"s).AsString();
    } else {
        const auto color = source.at("underlayer_color"s).AsArray();
        if (color.size() == 3) {
            underlayer_color = Rgb(color.at(0).AsInt(), color.at(1).AsInt(), color.at(2).AsInt());
        } else {
            underlayer_color = Rgba(color.at(0).AsInt(), color.at(1).AsInt(), color.at(2).AsInt(), color.at(3).AsDouble());
        }
    }
    underlayer_width = source.at("underlayer_width"s).AsDouble();

    const auto colors = source.at("color_palette"s).AsArray();
    for (const auto& color : colors) {
        if (color.IsString()) {
            color_palette.push_back(color.AsString());
        } else {
            if (color.AsArray().size() == 3) {
                color_palette.push_back(Rgb(color.AsArray().at(0).AsInt(), color.AsArray().at(1).AsInt(), color.AsArray().at(2).AsInt()));
            } else {
                color_palette.push_back(Rgba(color.AsArray().at(0).AsInt(), color.AsArray().at(1).AsInt(), color.AsArray().at(2).AsInt(), color.AsArray().at(3).AsDouble()));
            }
        }
    }
}

bool IsZero(double value) {
    return std::abs(value) < EPSILON;
}

svg::Point SphereProjector::operator()(geo::Coordinates coords) const {
    return {
            (coords.lng - min_lon_) * zoom_coeff_ + padding_,
            (max_lat_ - coords.lat) * zoom_coeff_ + padding_
    };
}


// ---------------MapRoute---------------

MapRoute::MapRoute(const ActualCoordinates& actual_coordinates,
                   const RenderSettings& render_settings,
                   const ActualBuses& actual_buses,
                   const PeekBuses& peek_buses,
                   const ActualStops& actual_stops,
                   const PeekStops& peek_stops)
                   : proj_(actual_coordinates.begin(),
                           actual_coordinates.end(),
                           render_settings.width,
                           render_settings.height,
                           render_settings.padding)
                   , actual_buses_(actual_buses)
                   , peek_buses_(peek_buses)
                   , render_settings_(render_settings)
                   , actual_stops_(actual_stops)
                   , peek_stops_(peek_stops) {
    CreateLinesBuses();
    CreateNamesBuses();
    CreateCircleStops();
    CreateNamesStops();
}

void MapRoute::Draw(svg::ObjectContainer& container) const {
    for (const auto& obj: lines_buses_) container.Add(obj);
    for (const auto& obj: names_buses_) container.Add(obj);
    for (const auto& obj: circle_stops_) container.Add(obj);
    for (const auto& obj: names_stops_) container.Add(obj);
}

void MapRoute::CreateLinesBuses() {
    int num_color = 0;
    for (const auto actual_bus : actual_buses_) {
        const auto bus_ptr = peek_buses_.at(actual_bus.first);

        Polyline lines_bus;
        for (const auto stop : bus_ptr->bus_stops) {
            const svg::Point screen_coord = proj_(stop->stop_coordinates);
            lines_bus.AddPoint(std::move(screen_coord));
        }

        lines_bus.SetStrokeColor(render_settings_.color_palette.at(num_color % render_settings_.color_palette.size()));
        ++num_color;
        lines_bus.SetFillColor({"none"});
        lines_bus.SetStrokeWidth(render_settings_.line_width);
        lines_bus.SetStrokeLineCap(StrokeLineCap::ROUND);
        lines_bus.SetStrokeLineJoin(StrokeLineJoin::ROUND);

        lines_buses_.push_back(std::move(lines_bus));
    }

}

void MapRoute::CreateNamesBuses() {
    int num_color = 0;
    for (const auto actual_bus : actual_buses_) {
        const auto bus_ptr = peek_buses_.at(actual_bus.first);
        Text bg_name_bus_start;
        bg_name_bus_start.SetData(bus_ptr->bus_name)
                .SetPosition(proj_(bus_ptr->bus_stops.at(0)->stop_coordinates))
                .SetOffset({render_settings_.bus_label_offset.at(0), render_settings_.bus_label_offset.at(1)})
                .SetFontSize(render_settings_.bus_label_font_size)
                .SetFontFamily("Verdana"s)
                .SetFontWeight("bold"s)
                .SetFillColor(render_settings_.underlayer_color)
                .SetStrokeColor(render_settings_.underlayer_color)
                .SetStrokeWidth(render_settings_.underlayer_width)
                .SetStrokeLineCap(StrokeLineCap::ROUND)
                .SetStrokeLineJoin(StrokeLineJoin::ROUND);
        names_buses_.push_back(std::move(bg_name_bus_start));

        Text name_bus_start;
        name_bus_start.SetData(bus_ptr->bus_name)
                .SetPosition(proj_(bus_ptr->bus_stops.at(0)->stop_coordinates))
                .SetOffset({render_settings_.bus_label_offset.at(0), render_settings_.bus_label_offset.at(1)})
                .SetFontSize(render_settings_.bus_label_font_size)
                .SetFontFamily("Verdana"s)
                .SetFontWeight("bold"s)
                .SetFillColor(render_settings_.color_palette.at(num_color % render_settings_.color_palette.size()));
        names_buses_.push_back(std::move(name_bus_start));

        const auto mid_bus = bus_ptr->bus_stops.size() / 2;
        if (!actual_bus.second &&
            bus_ptr->bus_stops.at(mid_bus) != bus_ptr->bus_stops.at(0)) {
            Text bg_name_bus_finish;
            bg_name_bus_finish.SetData(bus_ptr->bus_name)
                    .SetPosition(proj_(bus_ptr->bus_stops.at(mid_bus)->stop_coordinates))
                    .SetOffset({render_settings_.bus_label_offset.at(0), render_settings_.bus_label_offset.at(1)})
                    .SetFontSize(render_settings_.bus_label_font_size)
                    .SetFontFamily("Verdana"s)
                    .SetFontWeight("bold"s)
                    .SetFillColor(render_settings_.underlayer_color)
                    .SetStrokeColor(render_settings_.underlayer_color)
                    .SetStrokeWidth(render_settings_.underlayer_width)
                    .SetStrokeLineCap(StrokeLineCap::ROUND)
                    .SetStrokeLineJoin(StrokeLineJoin::ROUND);
            names_buses_.push_back(std::move(bg_name_bus_finish));

            Text name_bus_finish;
            name_bus_finish.SetData(bus_ptr->bus_name)
                    .SetPosition(proj_(bus_ptr->bus_stops.at(mid_bus)->stop_coordinates))
                    .SetOffset({render_settings_.bus_label_offset.at(0), render_settings_.bus_label_offset.at(1)})
                    .SetFontSize(render_settings_.bus_label_font_size)
                    .SetFontFamily("Verdana"s)
                    .SetFontWeight("bold"s)
                    .SetFillColor(render_settings_.color_palette.at(num_color % render_settings_.color_palette.size()));
            names_buses_.push_back(std::move(name_bus_finish));
        }

        ++num_color;
    }
}

void MapRoute::CreateCircleStops() {
    for (const auto stop : actual_stops_) {
        const auto stop_ptr = peek_stops_.at(stop);

        Circle circle_stop;
        circle_stop.SetCenter(proj_(stop_ptr->stop_coordinates));
        circle_stop.SetRadius(render_settings_.stop_radius);
        circle_stop.SetFillColor("white"s);
        circle_stops_.push_back(std::move(circle_stop));
    }
}

void MapRoute::CreateNamesStops() {
    for (const auto stop : actual_stops_) {
        const auto stop_ptr = peek_stops_.at(stop);

        Text bg_name_stop;
        bg_name_stop.SetData(stop_ptr->stop_name)
                .SetPosition(proj_(stop_ptr->stop_coordinates))
                .SetOffset({render_settings_.stop_label_offset.at(0), render_settings_.stop_label_offset.at(1)})
                .SetFontSize(render_settings_.stop_label_font_size)
                .SetFontFamily("Verdana"s)
                .SetFillColor(render_settings_.underlayer_color)
                .SetStrokeColor(render_settings_.underlayer_color)
                .SetStrokeWidth(render_settings_.underlayer_width)
                .SetStrokeLineCap(StrokeLineCap::ROUND)
                .SetStrokeLineJoin(StrokeLineJoin::ROUND);
        names_stops_.push_back(std::move(bg_name_stop));

        Text name_stop;
        name_stop.SetData(stop_ptr->stop_name)
                .SetPosition(proj_(stop_ptr->stop_coordinates))
                .SetOffset({render_settings_.stop_label_offset.at(0), render_settings_.stop_label_offset.at(1)})
                .SetFontSize(render_settings_.stop_label_font_size)
                .SetFontFamily("Verdana"s)
                .SetFillColor("black"s);
        names_stops_.push_back(std::move(name_stop));
    }
}

// ---------------MapRenderer---------------

MapRenderer::MapRenderer() {
}

MapRenderer::MapRenderer(const Dict& render_settings,
                         Stops stops,
                         PeekStops peek_stops,
                         Buses buses,
                         PeekBuses peek_buses,
                         ActualBuses actual_buses,
                         ActualCoordinates actual_coordinates,
                         ActualStops actual_stops)
                         : render_settings_(RenderSettings(render_settings))
                         , stops_(std::move(stops))
                         , peek_stops_(std::move(peek_stops))
                         , buses_(std::move(buses))
                         , peek_buses_(std::move(peek_buses))
                         , actual_buses_(std::move(actual_buses))
                         , actual_coordinates_(std::move(actual_coordinates))
                         , actual_stops_(std::move(actual_stops)) {
}
MapRenderer::MapRenderer(const RenderSettings& render_settings,
                         Stops stops,
                         PeekStops peek_stops,
                         Buses buses,
                         PeekBuses peek_buses,
                         ActualBuses actual_buses,
                         ActualCoordinates actual_coordinates,
                         ActualStops actual_stops)
                         : render_settings_(render_settings)
                         , stops_(std::move(stops))
                         , peek_stops_(std::move(peek_stops))
                         , buses_(std::move(buses))
                         , peek_buses_(std::move(peek_buses))
                         , actual_buses_(std::move(actual_buses))
                         , actual_coordinates_(std::move(actual_coordinates))
                         , actual_stops_(std::move(actual_stops)) {}


MapRoute MapRenderer::Render() const {
    return MapRoute(actual_coordinates_,
                    render_settings_,
                    actual_buses_,
                    peek_buses_,
                    actual_stops_,
                    peek_stops_);
}

const RenderSettings& MapRenderer::GetRenderSettings() const {
    return render_settings_;
}
const ActualCoordinates& MapRenderer::GetActualCoordinates() const {
    return actual_coordinates_;
}

}  // namespace map_renderer
