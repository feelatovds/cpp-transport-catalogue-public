#include "serialization.h"
#include "json_builder.h"

namespace serialization {

void TransportCatalogueExport::Serialize(const std::filesystem::path& path,
                                         const transport::TransportCatalogue& transport_catalogue,
                                         const map_renderer::MapRenderer& map_renderer,
                                         const router::TransportRouter& transport_router) const {
    std::ofstream out_file(path, std::ios::binary);
    transport_catalogue::TransportCatalogue transport_catalogue_export;

    *transport_catalogue_export.mutable_stops() = MakeTransportCatalogueProtoStops(transport_catalogue).stops();
    *transport_catalogue_export.mutable_buses() = MakeTransportCatalogueProtoBuses(transport_catalogue).buses();
    *transport_catalogue_export.mutable_map_renderer() = SerializeMapRenderer(map_renderer);
    *transport_catalogue_export.mutable_transport_router() = SerializeTransportRouter(transport_router);

    transport_catalogue_export.SerializeToOstream(&out_file);
}

TransportCatalogueExport::DesTransportCatalogue TransportCatalogueExport::Deserialize(const std::filesystem::path& path) const {
    // импортируем каталог в переменную из файла
    std::ifstream in_file(path, std::ios::binary);
    transport_catalogue::TransportCatalogue transport_catalogue_import;
    transport_catalogue_import.ParseFromIstream(&in_file);

    SourceStopRequests request_stops;
    SourceBusRequests request_buses;
    transport::TransportCatalogue transport_catalogue = DeserializeTransportCatalogue(transport_catalogue_import,
                                                                                     request_stops,
                                                                                     request_buses);
    map_renderer::MapRenderer map_renderer = DeserializeMapRenderer(transport_catalogue_import,
                                                                   request_stops,
                                                                   request_buses);
    router::TransportRouter transport_router = DeserializeTransportRouter(transport_catalogue_import);

    return DesTransportCatalogue{std::move(transport_catalogue),
                                 std::move(map_renderer),
                                 std::move(transport_router)
    };
}

// _______________ Serialize Transport Catalogue _______________

transport_catalogue::TransportCatalogue TransportCatalogueExport::MakeTransportCatalogueProtoStops(const transport::TransportCatalogue& transport_catalogue) const {
    transport_catalogue::TransportCatalogue transport_catalogue_temp;
    for (const auto& stop_as_tc_from: transport_catalogue.GetStops()) {
        transport_catalogue::Stop stop;
        stop.set_name(stop_as_tc_from.stop_name);
        stop.mutable_coordinates()->set_lat(stop_as_tc_from.stop_coordinates.lat);
        stop.mutable_coordinates()->set_lng(stop_as_tc_from.stop_coordinates.lng);

        for (const auto& stop_as_tc_to: transport_catalogue.GetStops()) {
            auto stop_from = transport_catalogue.FindStopByName(stop_as_tc_from.stop_name);
            auto stop_to = transport_catalogue.FindStopByName(stop_as_tc_to.stop_name);
            auto dist = transport_catalogue.DistanceBetweenStops( {const_cast<Stop*>(stop_from), const_cast<Stop*>(stop_to)} );
            if (dist != 0) {
                transport_catalogue::RoadDistances rd;
                rd.set_stop_to(stop_as_tc_to.stop_name);
                rd.set_distance(dist);
                *stop.add_distances() = rd;
            }
        }

        *transport_catalogue_temp.add_stops() = std::move(stop);
    }

    return transport_catalogue_temp;
}
transport_catalogue::TransportCatalogue TransportCatalogueExport::MakeTransportCatalogueProtoBuses(const transport::TransportCatalogue& transport_catalogue) const {
    transport_catalogue::TransportCatalogue transport_catalogue_temp;
    for (const auto& bus_as_tc: transport_catalogue.GetBuses()) {
        transport_catalogue::Bus bus;
        bus.set_name(bus_as_tc.bus_name);
        bus.set_is_roundtrip(bus_as_tc.is_roundtrip);

        for (const auto stop: bus_as_tc.bus_stops) {
            bus.add_stops(stop->stop_name);
        }

        *transport_catalogue_temp.add_buses() = std::move(bus);
    }

    return transport_catalogue_temp;
}

// _______________ Serialize Map Renderer _______________

transport_catalogue::MapRenderer TransportCatalogueExport::SerializeMapRenderer(const map_renderer::MapRenderer& map_renderer) const {
    transport_catalogue::MapRenderer map_renderer_temp;
    *map_renderer_temp.mutable_render_settings() = MakeMapRendererProtoRendererSettings(map_renderer);
    *map_renderer_temp.mutable_actual_coordinates() = MakeMapRendererProtoActualCoordinates(map_renderer).actual_coordinates();

    return map_renderer_temp;
}
transport_catalogue::RenderSettings TransportCatalogueExport::MakeMapRendererProtoRendererSettings(const map_renderer::MapRenderer& map_renderer) const {
    const map_renderer::RenderSettings render_settings = map_renderer.GetRenderSettings();
    transport_catalogue::RenderSettings render_settings_export;

    render_settings_export.set_width(render_settings.width);
    render_settings_export.set_height(render_settings.height);
    render_settings_export.set_padding(render_settings.padding);
    render_settings_export.set_line_width(render_settings.line_width);
    render_settings_export.set_stop_radius(render_settings.stop_radius);
    render_settings_export.set_bus_label_font_size(render_settings.bus_label_font_size);
    render_settings_export.add_bus_label_offset(render_settings.bus_label_offset[0]);
    render_settings_export.add_bus_label_offset(render_settings.bus_label_offset[1]);
    render_settings_export.set_stop_label_font_size(render_settings.stop_label_font_size);
    render_settings_export.add_stop_label_offset(render_settings.stop_label_offset[0]);
    render_settings_export.add_stop_label_offset(render_settings.stop_label_offset[1]);

    const Color& color = render_settings.underlayer_color;
    if (std::holds_alternative<std::monostate>(color)) {
        render_settings_export.mutable_underlayer_color()->mutable_monostate()->set_m(true);
    } else if (std::holds_alternative<std::string>(color)) {
        render_settings_export.mutable_underlayer_color()->mutable_str()->set_s(std::get<std::string>(color));
    } else if (std::holds_alternative<Rgb>(color)) {
        transport_catalogue::Rgb rgb;
        rgb.set_red(std::get<Rgb>(color).red);
        rgb.set_green(std::get<Rgb>(color).green);
        rgb.set_blue(std::get<Rgb>(color).blue);
        *(render_settings_export.mutable_underlayer_color()->mutable_rgb()) = std::move(rgb);
    } else if (std::holds_alternative<Rgba>(color)) {
        transport_catalogue::Rgba rgba;
        rgba.set_red(std::get<Rgba>(color).red);
        rgba.set_green(std::get<Rgba>(color).green);
        rgba.set_blue(std::get<Rgba>(color).blue);
        rgba.set_opacity(std::get<Rgba>(color).opacity);
        *(render_settings_export.mutable_underlayer_color()->mutable_rgba()) = std::move(rgba);
    }
    render_settings_export.set_underlayer_width(render_settings.underlayer_width);

    for (const auto &color: render_settings.color_palette) {
        if (std::holds_alternative<std::monostate>(color)) {
            render_settings_export.add_color_palette()->mutable_monostate()->set_m(true);
        } else if (std::holds_alternative<std::string>(color)) {
            render_settings_export.add_color_palette()->mutable_str()->set_s(std::get<std::string>(color));
        } else if (std::holds_alternative<Rgb>(color)) {
            transport_catalogue::Rgb rgb;
            rgb.set_red(std::get<Rgb>(color).red);
            rgb.set_green(std::get<Rgb>(color).green);
            rgb.set_blue(std::get<Rgb>(color).blue);
            *(render_settings_export.add_color_palette()->mutable_rgb()) = std::move(rgb);
        } else if (std::holds_alternative<Rgba>(color)) {
            transport_catalogue::Rgba rgba;
            rgba.set_red(std::get<Rgba>(color).red);
            rgba.set_green(std::get<Rgba>(color).green);
            rgba.set_blue(std::get<Rgba>(color).blue);
            rgba.set_opacity(std::get<Rgba>(color).opacity);
            *(render_settings_export.add_color_palette()->mutable_rgba()) = std::move(rgba);
        }
    }

    return render_settings_export;
}
transport_catalogue::MapRenderer TransportCatalogueExport::MakeMapRendererProtoActualCoordinates(const map_renderer::MapRenderer& map_renderer) const {
    transport_catalogue::MapRenderer map_renderer_temp;
    for (const auto& coord_as_tc: map_renderer.GetActualCoordinates()) {
        transport_catalogue::Coordinates coord;
        coord.set_lat(coord_as_tc.lat);
        coord.set_lng(coord_as_tc.lng);
        *map_renderer_temp.add_actual_coordinates() = std::move(coord);
    }

    return map_renderer_temp;
}

// _______________ Serialize Transport Router _______________

transport_catalogue::TransportRouter TransportCatalogueExport::SerializeTransportRouter(const router::TransportRouter& transport_router) const {
    transport_catalogue::TransportRouter transport_router_temp;
    *transport_router_temp.mutable_routing_settings() = MakeTransportRouterProtoRoutingSettings(transport_router);
    *transport_router_temp.mutable_graph() = MakeTransportRouterProtoGraph(transport_router);
    *transport_router_temp.mutable_stop_names() = MakeTransportRouterProtoStopNames(transport_router).stop_names();
    *transport_router_temp.mutable_routes_internal_data() = MakeTransportRouterProtoStopRoutesInternalData(transport_router).routes_internal_data();

    return transport_router_temp;
}
transport_catalogue::RoutingSettings TransportCatalogueExport::MakeTransportRouterProtoRoutingSettings(const router::TransportRouter& transport_router) const {
    transport_catalogue::RoutingSettings routing_settings;
    routing_settings.set_bus_wait_time(transport_router.GetRoutingSettings().bus_wait_time);
    routing_settings.set_bus_velocity(transport_router.GetRoutingSettings().bus_velocity);
    return routing_settings;
}
transport_catalogue::Graph TransportCatalogueExport::MakeTransportRouterProtoGraph(const router::TransportRouter& transport_router) const {
    transport_catalogue::Graph graph_target;
    const auto& graph_as_tc = transport_router.GetGraph();
    for (size_t i = 0; i < graph_as_tc.GetEdgeCount(); ++i) {
        const auto& edge = graph_as_tc.GetEdge(i);
        transport_catalogue::Edge edge_tmp;
        edge_tmp.mutable_vert_from()->set_vertex_id(edge.from);
        edge_tmp.mutable_vert_to()->set_vertex_id(edge.to);
        edge_tmp.set_weight(edge.weight);
        edge_tmp.set_name(edge.name);
        edge_tmp.set_span_count(edge.span_count);

        *graph_target.add_edges() = std::move(edge_tmp);
    }

    for (size_t i = 0; i < graph_as_tc.GetVertexCount(); ++i) {
        transport_catalogue::IncidenceList incidence_list;
        const auto& incident_edges = graph_as_tc.GetIncidentEdges(i);
        for (auto it = incident_edges.begin(); it != incident_edges.end(); ++it) {
            incidence_list.add_edges_id(std::move(*it));
        }
        *graph_target.add_incidence_lists() = std::move(incidence_list);
    }

    return graph_target;
}
transport_catalogue::TransportRouter TransportCatalogueExport::MakeTransportRouterProtoStopNames(const router::TransportRouter& transport_router) const {
    transport_catalogue::TransportRouter transport_router_temp;
    for (const auto& stop: transport_router.GetStopNames()) {
        transport_router_temp.add_stop_names(stop);
    }

    return transport_router_temp;
}
transport_catalogue::TransportRouter TransportCatalogueExport::MakeTransportRouterProtoStopRoutesInternalData(const router::TransportRouter& transport_router) const {
    transport_catalogue::TransportRouter transport_router_temp;
    const auto& router = transport_router.GetRouter();
    for (const auto& routes_tc: router.GetRoutesInternalData()) {
        transport_catalogue::RoutesInternalData vector;
        for (const auto& route_tc: routes_tc) {
            if (route_tc) {
                transport_catalogue::RouteInternalDataVectorElem vector_elem;
                vector_elem.mutable_data()->set_weight(route_tc->weight);
                if (route_tc->prev_edge) {
                    vector_elem.mutable_data()->mutable_prev_edge()->set_edge_id(*route_tc->prev_edge);
                }

                *vector.add_route_internal_data() = std::move(vector_elem);
            }
        }

        *transport_router_temp.add_routes_internal_data() = std::move(vector);
    }

    return transport_router_temp;
}


// _______________ Deserialize Transport Catalogue _______________

transport::TransportCatalogue TransportCatalogueExport::DeserializeTransportCatalogue(transport_catalogue::TransportCatalogue& transport_catalogue_import,
                                                                                      SourceStopRequests& request_stops,
                                                                                      SourceBusRequests& request_buses) const {
    // заполняем контейнеры с остановками для создания транспортного справочника
    Stops stops_to_tc;
    PeekStops peek_stops_to_tc;
    domain::DistanceBetweenStops distance_between_stops_to_tc;
    DeserializeTransportCatalogueStops(transport_catalogue_import, request_stops);
    CreateStops(stops_to_tc,
                peek_stops_to_tc,
                distance_between_stops_to_tc,
                request_stops);

    // заполняем контейнеры с автобусами для создания транспортного справочника
    Buses buses_to_tc;
    PeekBuses peek_buses_to_tc;
    DeserializeTransportCatalogueBuses(transport_catalogue_import, request_buses);
    CreateBus(peek_stops_to_tc,
              buses_to_tc,
              peek_buses_to_tc,
              request_buses);

    // ещё контейнер для создания транспортного справочника
    StopBuses stop_buses_to_tc; //
    CreateStopBuses(buses_to_tc, stop_buses_to_tc, stops_to_tc);

    return transport::TransportCatalogue(std::move(stops_to_tc),
                                         std::move(peek_stops_to_tc),
                                         std::move(buses_to_tc),
                                         std::move(peek_buses_to_tc),
                                         std::move(stop_buses_to_tc),
                                         std::move(distance_between_stops_to_tc));
}
void TransportCatalogueExport::DeserializeTransportCatalogueStops(transport_catalogue::TransportCatalogue& transport_catalogue_import,
                                                                  SourceStopRequests& request_stops) const {
    // помещаем остановки в переменную request_stops
    // using SourceStopRequests = std::vector<std::pair<domain::Stop, std::map<std::string, int>> >;
    for (auto& stop: transport_catalogue_import.stops()) {
        Stop stop_to_tc = Stop(std::move(stop.name()), stop.coordinates().lat(), stop.coordinates().lng());
        std::map<std::string, int> distance_to_stop;
        for(auto& road_distance: stop.distances()) {
            distance_to_stop.insert({std::move(road_distance.stop_to()), road_distance.distance()});
        }

        request_stops.push_back({std::move(stop_to_tc), std::move(distance_to_stop)});
    }
}
void TransportCatalogueExport::DeserializeTransportCatalogueBuses(transport_catalogue::TransportCatalogue& transport_catalogue_import,
                                                                  SourceBusRequests& request_buses) const {
    // помещаем автобусы в переменную request_buses
    // ВАЖНО: первый элемент в векторе названий остановок - название маршрута
    // using SourceBusRequests = std::vector<std::pair<std::vector<std::string>, bool> >; // {names_stops, is_roundtrip}
    for (auto& bus: transport_catalogue_import.buses()) {
        std::vector<std::string> names_stops;
        names_stops.push_back(std::move(bus.name()));
        for (int i = 0; i < bus.stops_size(); ++i) {
            names_stops.push_back(std::move(bus.stops(i)));
        }

        request_buses.push_back( {std::move(names_stops), bus.is_roundtrip()} );
    }
}

// _______________ Вспомогательные функции _______________

void TransportCatalogueExport::AddPeekStop(const Stops::iterator pos, PeekStops& peek_stops) const {
    peek_stops[pos->stop_name] = &(*pos);
}
void TransportCatalogueExport::AddStop(const Stop& stop, Stops& stops, PeekStops& peek_stops) const {
    AddPeekStop(stops.insert(stops.end(), stop), peek_stops);
}
void TransportCatalogueExport::CreateStops(Stops& stops, PeekStops& peek_stops,
                                           domain::DistanceBetweenStops& distance_between_stops,
                                           const SourceStopRequests& request_stops) const {
    for (const auto& stop : request_stops) {
        AddStop(stop.first, stops, peek_stops);
    }

    for (const auto& stop : request_stops) {
        const std::string& stop_from = stop.first.stop_name;
        Stop* stop_from_address = peek_stops.at(stop_from);

        for (const auto& [stop_to, distance] : stop.second) {
            Stop* stop_to_address = peek_stops.at(stop_to);
            if (distance_between_stops.find({stop_to_address, stop_from_address}) != distance_between_stops.end()) {
                distance_between_stops[{stop_from_address, stop_to_address}] = distance;
                continue;
            }
            distance_between_stops[{stop_from_address, stop_to_address}] = distance;
            distance_between_stops[{stop_to_address, stop_from_address}] = distance;
        }
    }
}

void TransportCatalogueExport::AddPeekBus(const Buses::iterator pos, PeekBuses& peek_buses) const {
    peek_buses[pos->bus_name] = &(*pos);
}
void TransportCatalogueExport::AddBus(const std::string& name, std::vector<Stop*> stops, Buses& buses, PeekBuses& peek_buses, bool is_roundtrip) const {
    Bus bus(name, std::move(stops));
    bus.is_roundtrip = is_roundtrip;
    AddPeekBus(buses.insert(buses.end(), std::move(bus)), peek_buses);
}
void TransportCatalogueExport::CreateBus(const PeekStops& peek_stops, Buses& buses, PeekBuses& peek_buses, const SourceBusRequests& request_buses) const {
    for (const auto& bus_pair : request_buses) {
        const auto& bus = bus_pair.first;
        const std::string& name = bus.at(0);
        std::vector<Stop*> stops;
        for (size_t i = 1; i < bus.size(); ++i) {
            const std::string& stop = bus.at(i);
            stops.push_back(peek_stops.at(stop));
        }
        bool is_roundtrip = bus_pair.second;
        AddBus(name, std::move(stops), buses, peek_buses, is_roundtrip);
    }
}

void TransportCatalogueExport::CreateStopBuses(const Buses& buses, StopBuses& stop_buses, const Stops& stops) const {
    for (const Bus& bus : buses) {
        for (const Stop* stop : bus.bus_stops) {
            stop_buses[stop->stop_name].insert(bus.bus_name);
        }
    }

    for (const Stop& stop : stops) {
        if (stop_buses.find(stop.stop_name) != stop_buses.end()) {
            continue;
        }
        stop_buses.insert({stop.stop_name, {}});
    }
}

// _______________ Deserialize Map Renderer _______________

map_renderer::MapRenderer TransportCatalogueExport::DeserializeMapRenderer(transport_catalogue::TransportCatalogue& transport_catalogue_import,
                                                                           const SourceStopRequests& request_stops,
                                                                           const SourceBusRequests& request_buses) const {
    transport_catalogue::MapRenderer map_renderer_import = std::move(transport_catalogue_import.map_renderer());

    // создаем render_settings
    map_renderer::RenderSettings render_settings = DeserializeMapRendererRenderSettings(map_renderer_import);

    // на основе request_stops заполняем контейнеры для создания MapRender
    Stops stops_to_mr;
    PeekStops peek_stops_to_mr;
    CreateStopsMap(stops_to_mr, peek_stops_to_mr, request_stops);

    // на основе request_buses заполняем контейнеры для создания MapRender
    Buses buses_to_mr;
    PeekBuses peek_buses_to_mr;
    ActualBuses actual_buses_to_mr;
    ActualCoordinates actual_coordinates_to_mr;
    ActualStops actual_stops_to_mr;
    CreateBusMap(peek_stops_to_mr, buses_to_mr, peek_buses_to_mr,
                 actual_buses_to_mr, actual_coordinates_to_mr, actual_stops_to_mr, request_buses);


    return map_renderer::MapRenderer(std::move(render_settings),
                                     std::move(stops_to_mr),
                                     std::move(peek_stops_to_mr),
                                     std::move(buses_to_mr),
                                     std::move(peek_buses_to_mr),
                                     std::move(actual_buses_to_mr),
                                     std::move(actual_coordinates_to_mr),
                                     std::move(actual_stops_to_mr));
}
map_renderer::RenderSettings TransportCatalogueExport::DeserializeMapRendererRenderSettings(transport_catalogue::MapRenderer& map_renderer) const {
    map_renderer::RenderSettings render_settings;
    transport_catalogue::RenderSettings render_settings_import = std::move(map_renderer.render_settings());
    render_settings.width = std::move(render_settings_import.width());
    render_settings.height = std::move(render_settings_import.height());
    render_settings.padding = std::move(render_settings_import.padding());
    render_settings.line_width = std::move(render_settings_import.line_width());
    render_settings.stop_radius = std::move(render_settings_import.stop_radius());
    render_settings.bus_label_font_size = std::move(render_settings_import.bus_label_font_size());
    render_settings.bus_label_offset[0] = std::move(render_settings_import.bus_label_offset(0));
    render_settings.bus_label_offset[1] = std::move(render_settings_import.bus_label_offset(1));
    render_settings.stop_label_font_size = std::move(render_settings_import.stop_label_font_size());
    render_settings.stop_label_offset[0] = std::move(render_settings_import.stop_label_offset(0));
    render_settings.stop_label_offset[1] = std::move(render_settings_import.stop_label_offset(1));

    if (render_settings_import.underlayer_color().has_monostate()) {
        render_settings.underlayer_color = std::monostate{};
    } else if (render_settings_import.underlayer_color().has_str()) {
        render_settings.underlayer_color = std::move(render_settings_import.underlayer_color().str().s());
    } else if (render_settings_import.underlayer_color().has_rgb()) {
        uint8_t red = static_cast<uint8_t>(std::move(render_settings_import.underlayer_color().rgb().red()));
        uint8_t green = static_cast<uint8_t>(std::move(render_settings_import.underlayer_color().rgb().green()));
        uint8_t blue = static_cast<uint8_t>(std::move(render_settings_import.underlayer_color().rgb().blue()));
        render_settings.underlayer_color = Rgb(red, green, blue);
    } else if (render_settings_import.underlayer_color().has_rgba()) {
        uint8_t red = static_cast<uint8_t>(std::move(render_settings_import.underlayer_color().rgba().red()));
        uint8_t green = static_cast<uint8_t>(std::move(render_settings_import.underlayer_color().rgba().green()));
        uint8_t blue = static_cast<uint8_t>(std::move(render_settings_import.underlayer_color().rgba().blue()));
        double opacity = std::move(render_settings_import.underlayer_color().rgba().opacity());
        render_settings.underlayer_color = Rgba(red, green, blue, opacity);
    }

    render_settings.underlayer_width = std::move(render_settings_import.underlayer_width());
    for (auto& p: render_settings_import.color_palette()) {
        if (p.has_monostate()) {
            render_settings.color_palette.push_back(std::monostate{});
        } else if (p.has_str()) {
            render_settings.color_palette.push_back(std::move(p.str().s()));
        } else if (p.has_rgb()) {
            uint8_t red = static_cast<uint8_t>(std::move(p.rgb().red()));
            uint8_t green = static_cast<uint8_t>(std::move(p.rgb().green()));
            uint8_t blue = static_cast<uint8_t>(std::move(p.rgb().blue()));
            render_settings.color_palette.push_back(Rgb(red, green, blue));
        } else if (p.has_rgba()) {
            uint8_t red = static_cast<uint8_t>(std::move(p.rgba().red()));
            uint8_t green = static_cast<uint8_t>(std::move(p.rgba().green()));
            uint8_t blue = static_cast<uint8_t>(std::move(p.rgba().blue()));
            double opacity = std::move(p.rgba().opacity());
            render_settings.color_palette.push_back(Rgba(red, green, blue, opacity));
        }
    }

    return render_settings;
}

// _______________ Вспомогательные функции _______________

void TransportCatalogueExport::AddPeekStopMap(const Stops::iterator pos, PeekStops& peek_stops) const {
    peek_stops[pos->stop_name] = &(*pos);
}
void TransportCatalogueExport::AddStopMap(const Stop& stop, Stops& stops, PeekStops& peek_stops) const {
    AddPeekStopMap(stops.insert(stops.end(), stop), peek_stops);
}
void TransportCatalogueExport::CreateStopsMap(Stops& stops, PeekStops& peek_stops, const SourceStopRequests& request_stops) const {
    for (const auto& stop : request_stops) {
        AddStopMap(stop.first, stops, peek_stops);
    }
}

void TransportCatalogueExport::AddPeekBusMap(const Buses::iterator pos, PeekBuses& peek_buses) const {
    peek_buses[pos->bus_name] = &(*pos);
}
void TransportCatalogueExport::AddBusMap(const std::string& name, std::vector<Stop*> stops, Buses& buses, PeekBuses& peek_buses) const {
    Bus bus(name, std::move(stops));
    AddPeekBusMap(buses.insert(buses.end(), std::move(bus)), peek_buses);
}
void TransportCatalogueExport::CreateBusMap(const PeekStops& peek_stops, Buses& buses, PeekBuses& peek_buses,
                  ActualBuses& actual_buses, ActualCoordinates& actual_coordinates,
                  ActualStops& actual_stops, const SourceBusRequests& request_buses) const {
    for (const auto& bus_pair : request_buses) {
        const auto& bus = bus_pair.first;
        const std::string& name = bus.at(0);

        std::vector<Stop*> stops;
        for (size_t i = 1; i < bus.size(); ++i) {
            const std::string& stop = bus.at(i);
            stops.push_back(peek_stops.at(stop));
        }

        AddBusMap(name, std::move(stops), buses, peek_buses);
        if (bus.size() > 1) {
            const auto bus_pos = peek_buses.at(name);
            actual_buses.insert({bus_pos->bus_name, bus_pair.second});
            for (const auto stop : bus_pos->bus_stops) {
                actual_coordinates.push_back(stop->stop_coordinates);
                actual_stops.insert(stop->stop_name);
            }
        }

    }
}

// _______________ Deserialize Transport Router _______________

router::TransportRouter TransportCatalogueExport::DeserializeTransportRouter(transport_catalogue::TransportCatalogue& transport_catalogue_import) const {
    transport_catalogue::TransportRouter transport_router_import = std::move(transport_catalogue_import.transport_router());

    // создаем routing_settings
    router::RoutingSettings routing_settings_to_tc(std::move(transport_router_import.routing_settings().bus_wait_time()),
                                                   std::move(transport_router_import.routing_settings().bus_velocity()));

    // создаем graph
    transport_catalogue::Graph graph_from_ser = std::move(transport_router_import.graph());
    graph::DirectedWeightedGraph<double> graph_to_tc = DeserializeTransportRouterGraph(graph_from_ser);

    // создаем stop_names
    std::vector<std::string> stop_names_to_tc;
    for (auto &stop: transport_router_import.stop_names()) {
        stop_names_to_tc.emplace_back(std::move(stop));
    }

    // создаем routes_internal_data
    graph::Router<double>::RoutesInternalData internal_data_to_tc = DeserializeTransportRoutesInternalData(transport_router_import);

    return router::TransportRouter(std::move(routing_settings_to_tc),
                                   std::move(graph_to_tc),
                                   std::move(stop_names_to_tc),
                                   std::move(internal_data_to_tc));
}
graph::DirectedWeightedGraph<double> TransportCatalogueExport::DeserializeTransportRouterGraph(transport_catalogue::Graph& graph_from_ser) const {
    std::vector<graph::Edge<double>> edges_to_graph;
    std::vector<std::vector<graph::EdgeId> > incidence_lists_to_graph;

    for (auto& edge_from_ser: graph_from_ser.edges()) {
        graph::Edge<double> edge;
        edge.from = std::move(edge_from_ser.vert_from().vertex_id());
        edge.to = std::move(edge_from_ser.vert_to().vertex_id());
        edge.weight = std::move(edge_from_ser.weight());
        edge.name = std::move(edge_from_ser.name());
        edge.span_count = std::move(edge_from_ser.span_count());

        edges_to_graph.push_back(std::move(edge));
    }

    for (size_t i = 0; i < graph_from_ser.incidence_lists_size(); ++i) {
        std::vector<graph::EdgeId> incidence_list_to;
        for (size_t j = 0; j < graph_from_ser.incidence_lists(i).edges_id_size(); ++j) {
            incidence_list_to.emplace_back(std::move(graph_from_ser.incidence_lists(i).edges_id(j)));
        }
        incidence_lists_to_graph.emplace_back(std::move(incidence_list_to));
    }

    return graph::DirectedWeightedGraph<double> (std::move(edges_to_graph), std::move(incidence_lists_to_graph));;
}
graph::Router<double>::RoutesInternalData TransportCatalogueExport::DeserializeTransportRoutesInternalData(transport_catalogue::TransportRouter& transport_router) const {
    graph::Router<double>::RoutesInternalData internal_data_to_tc;
    for (auto& routes: transport_router.routes_internal_data()) {
        std::vector <std::optional<graph::Router<double>::RouteInternalData>> routes_to_tc;
        for (auto &route: routes.route_internal_data()) {
            switch (route.elem_case()) {
                case transport_catalogue::RouteInternalDataVectorElem::ElemCase::ELEM_NOT_SET:
                    routes_to_tc.push_back(std::nullopt);
                    break;
                case transport_catalogue::RouteInternalDataVectorElem::ElemCase::kData:
                    graph::Router<double>::RouteInternalData route_to_tc{};
                    route_to_tc.weight = std::move(route.data().weight());
                    switch (route.data().prev_case()) {
                        case transport_catalogue::RouteInternalData::PrevCase::kPrevEdge:
                            route_to_tc.prev_edge = std::make_optional(std::move(route.data().prev_edge().edge_id()));
                            break;
                        case transport_catalogue::RouteInternalData::PrevCase::PREV_NOT_SET:
                            route_to_tc.prev_edge = std::nullopt;
                            break;
                    }
                    routes_to_tc.push_back(std::move(route_to_tc));
                    break;
            }
        }

        internal_data_to_tc.push_back(std::move(routes_to_tc));
    }

    return internal_data_to_tc;
}

}