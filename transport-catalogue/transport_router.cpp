#include "transport_router.h"

namespace router {

TransportRouter::TransportRouter(const Dict& routing_settings, const TransportCatalogue& transport_catalogue)
: routing_settings_(RoutingSettings(routing_settings))
, graph_(transport_catalogue.GetCountStops() * 2)
, router_(graph_) {
    routing_settings_.bus_velocity *= SCALE_VELOCITY_FACTOR;
    FillGraph(transport_catalogue);
}

TransportRouter::TransportRouter(RoutingSettings routing_settings,
                DirectedWeightedGraph<double> graph,
                std::vector<std::string> stop_names,
                Router<double>::RoutesInternalData internal_data)
: routing_settings_(std::move(routing_settings))
, graph_(std::move(graph))
, stop_names_(std::move(stop_names))
, router_(graph_, std::move(internal_data)) {
}

const TransportRouter::Graph& TransportRouter::GetGraph() const {
    return graph_;
}

VertexId TransportRouter::GetVertexIdInput(std::string_view stop_name) const {
    return 2 * (std::find(stop_names_.begin(), stop_names_.end(), stop_name) - stop_names_.begin());
}

std::optional<json::Node> TransportRouter::GetRouteAsNode(const Router<double>& router, std::string_view from, std::string_view to) const {
    VertexId vertex_from = GetVertexIdInput(from);
    VertexId vertex_to = GetVertexIdInput(to);

    std::optional<Router<double>::RouteInfo> candidate_route = router.BuildRoute(vertex_from, vertex_to);
    if (!candidate_route.has_value()) {
        return std::nullopt;
    }

    auto answer = json::Builder{};
    auto route = answer.StartArray();
    for (const auto& edge_id : candidate_route->edges) {
        auto item = route.StartDict();
        const auto& edge = GetGraph().GetEdge(edge_id);
        if (edge.span_count == 0) { // wait
            item.Key("type").Value(static_cast<std::string>("Wait"));
            item.Key("stop_name").Value(static_cast<std::string>(edge.name));
        } else {
            item.Key("type").Value(static_cast<std::string>("Bus"));
            item.Key("bus").Value(static_cast<std::string>(edge.name));
            item.Key("span_count").Value(static_cast<int>(edge.span_count));
        }
        item.Key("time").Value(edge.weight);
        item.EndDict();
    }

    route.EndArray();
    return answer.Build();
}

const RoutingSettings& TransportRouter::GetRoutingSettings() const {
    return routing_settings_;
}

const std::vector<std::string>& TransportRouter::GetStopNames() const {
    return stop_names_;
}

const Router<double>& TransportRouter::GetRouter() const {
    return router_;
}

void TransportRouter::FillVertex(const TransportCatalogue& transport_catalogue) {
    const double WEIGHT = routing_settings_.bus_wait_time;
    size_t from = 0, to = 1;
    for (const auto& stop: transport_catalogue.GetStops()) {
        graph_.AddEdge({from, to, WEIGHT, stop.stop_name, 0});
        stop_names_.push_back(stop.stop_name);
        from += 2;
        to += 2;
    }
}

void TransportRouter::FillEdges(const TransportCatalogue& transport_catalogue) {
    for (const auto& bus: transport_catalogue.GetBuses()) {
        if (bus.is_roundtrip) {
            const auto& stops = bus.bus_stops;
            for (size_t from = 0; from + 1 < stops.size(); ++from) {
                std::string_view stop_from_name = stops[from]->stop_name;
                VertexId stop_from_vertex =
                        1 + 2 * (std::find(stop_names_.begin(), stop_names_.end(), stop_from_name) - stop_names_.begin());
                double weight = 0.0;
                size_t span_count = 0;
                for (size_t to = from + 1; to < stops.size(); ++to) {
                    if (from == 0 && to == stops.size() - 1) continue;
                    std::string_view stop_to_name = stops[to]->stop_name;
                    VertexId stop_to_vertex =
                            2 * (std::find(stop_names_.begin(), stop_names_.end(), stop_to_name) - stop_names_.begin());

                    auto stop_from = transport_catalogue.FindStopByName(stop_from_name);
                    auto stop_to = transport_catalogue.FindStopByName(stop_to_name);
                    weight += transport_catalogue.DistanceBetweenStops( {const_cast<Stop*>(stop_from), const_cast<Stop*>(stop_to)} )
                              / routing_settings_.bus_velocity;

                    stop_from_name = stop_to_name;
                    ++span_count;
                    graph_.AddEdge({stop_from_vertex, stop_to_vertex, weight, bus.bus_name, span_count});
                }
            }
        } else {
            const auto& stops = bus.bus_stops;
            size_t mid = stops.size() / 2;

            // туда
            for (size_t from = 0; from < mid; ++from) {
                std::string_view stop_from_name = stops[from]->stop_name;
                VertexId stop_from_vertex =
                        1 + 2 * (std::find(stop_names_.begin(), stop_names_.end(), stop_from_name) - stop_names_.begin());
                double weight = 0.0;
                size_t span_count = 0;
                for (size_t to = from + 1; to <= mid; ++to) {
                    std::string_view stop_to_name = stops[to]->stop_name;
                    VertexId stop_to_vertex =
                            2 * (std::find(stop_names_.begin(), stop_names_.end(), stop_to_name) - stop_names_.begin());

                    auto stop_from = transport_catalogue.FindStopByName(stop_from_name);
                    auto stop_to = transport_catalogue.FindStopByName(stop_to_name);
                    weight += transport_catalogue.DistanceBetweenStops( {const_cast<Stop*>(stop_from), const_cast<Stop*>(stop_to)} )
                              / routing_settings_.bus_velocity;

                    stop_from_name = stop_to_name;
                    ++span_count;
                    graph_.AddEdge({stop_from_vertex, stop_to_vertex, weight, bus.bus_name, span_count});
                }
            }

            // обратно
            for (size_t from = mid; from + 1 < stops.size(); ++from) {
                std::string_view stop_from_name = stops[from]->stop_name;
                VertexId stop_from_vertex =
                        1 + 2 * (std::find(stop_names_.begin(), stop_names_.end(), stop_from_name) - stop_names_.begin());
                double weight = 0.0;
                size_t span_count = 0;
                for (size_t to = from + 1; to < stops.size(); ++to) {
                    std::string_view stop_to_name = stops[to]->stop_name;
                    VertexId stop_to_vertex =
                            2 * (std::find(stop_names_.begin(), stop_names_.end(), stop_to_name) - stop_names_.begin());

                    auto stop_from = transport_catalogue.FindStopByName(stop_from_name);
                    auto stop_to = transport_catalogue.FindStopByName(stop_to_name);
                    weight += transport_catalogue.DistanceBetweenStops( {const_cast<Stop*>(stop_from), const_cast<Stop*>(stop_to)} )
                              / routing_settings_.bus_velocity;

                    stop_from_name = stop_to_name;
                    ++span_count;
                    graph_.AddEdge({stop_from_vertex, stop_to_vertex, weight, bus.bus_name, span_count});
                }
            }
        }
    }
}

void TransportRouter::FillGraph(const TransportCatalogue& transport_catalogue) {
    FillVertex(transport_catalogue);
    FillEdges(transport_catalogue);
}

}  //  namespace router