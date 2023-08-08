#pragma once

#include <string_view>
#include <vector>
#include <optional>

#include "json.h"
#include "router.h"
#include "transport_catalogue.h"
#include "json_builder.h"

namespace router {

using namespace json;
using namespace graph;
using namespace transport;

static const double SCALE_VELOCITY_FACTOR = 1000.0 / 60.0; // (N) km/h = (N * SVF) m/min

struct RoutingSettings {
    RoutingSettings(const Dict& routing_settings)
    : bus_wait_time(routing_settings.at("bus_wait_time").AsInt())
    , bus_velocity(routing_settings.at("bus_velocity").AsDouble()) {
    }

    RoutingSettings(int time, double velocity)
    : bus_wait_time(time)
    , bus_velocity(velocity) {
    }

    int bus_wait_time;
    double bus_velocity;
};

class TransportRouter {
private:
    using Graph = DirectedWeightedGraph<double>;
public:
    TransportRouter(const Dict& routing_settings, const TransportCatalogue& transport_catalogue);

    TransportRouter(RoutingSettings routing_settings,
                    DirectedWeightedGraph<double> graph,
                    std::vector<std::string> stop_names,
                    Router<double>::RoutesInternalData internal_data);

    const Graph& GetGraph() const;
    VertexId GetVertexIdInput(std::string_view stop_name) const;

    std::optional<json::Node> GetRouteAsNode(const Router<double>& router, std::string_view from, std::string_view to) const;

    const RoutingSettings& GetRoutingSettings() const;
    const std::vector<std::string>& GetStopNames() const;


    const Router<double>& GetRouter() const;

private:
    RoutingSettings routing_settings_;
    Graph graph_;
    Router<double> router_;
    std::vector<std::string> stop_names_;

    void FillGraph(const TransportCatalogue& transport_catalogue);
    void FillVertex(const TransportCatalogue& transport_catalogue);
    void FillEdges(const TransportCatalogue& transport_catalogue);
};

}  //  namespace router