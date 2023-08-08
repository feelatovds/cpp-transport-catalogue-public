#include <iostream>
#include <string>
#include <map>
#include <iomanip>

#include "json_builder.h"
#include "request_handler.h"

namespace request_handler {

RequestHandler::RequestHandler(const TransportCatalogue& transport_catalogue,
                               const MapRenderer& map_renderer,
                               const TransportRouter& transport_router)
: transport_catalogue_(transport_catalogue)
, map_renderer_(map_renderer)
, transport_router_(transport_router)
, router_(transport_router_.GetGraph()) {
}

std::optional<Bus> RequestHandler::GetBusStat(const std::string_view bus_name) const {
    if (transport_catalogue_.FindBusByName(bus_name) != nullptr) {
        return *transport_catalogue_.FindBusByName(bus_name);
    }

    return {};
}

std::optional<std::set<std::string_view> > RequestHandler::GetBusesByStop(const std::string_view stop_name) const {
    if (transport_catalogue_.FindBusesForStop(stop_name) != nullptr) {
        return *transport_catalogue_.FindBusesForStop(stop_name);
    }
    return {};
}

int RequestHandler::GetDistanceBetweenStops(const std::pair<Stop*, Stop*>& stops) const {
    return transport_catalogue_.DistanceBetweenStops(stops);
}

json::Document RequestHandler::ProcessStatRequests(const SourseStatRequests& stat_requests) const {
    auto stat = json::Builder{};
    auto array = stat.StartArray();

    for (const auto& request : stat_requests) {
        if (request.at("type"s) == "Stop"s) {
            array.Value(ProcessStatStop(request).AsDict());
        } else if (request.at("type"s) == "Bus"s) {
            array.Value(ProcessStatBus(request).AsDict());
        } else if (request.at("type"s) == "Map"s) {
            array.Value(ProcessMap(request).AsDict());
        } else if (request.at("type"s) == "Route"s) {
            array.Value(ProcessRoute(request).AsDict());
        }
    }
    json::Document document(std::move(stat.EndArray().Build()));
    return document;
}

json::Node RequestHandler::ProcessStatStop(const std::map<std::string, std::string>& request) const {
    auto answer = json::Builder{};
    auto stop = answer.StartDict();
    stop.Key("request_id").Value(std::stoi(request.at("id")));

    const auto buses = GetBusesByStop(request.at("name"));
    if (!buses.has_value()) {
        stop.Key("error_message").Value("not found");
        return answer.EndDict().Build();
    }

    auto buses_list =
    stop.Key("buses").StartArray();
    for (const auto& bus : *buses) {
        buses_list.Value(static_cast<std::string>(bus));
    }
    buses_list.EndArray();
    return answer.EndDict().Build();
}

json::Node RequestHandler::ProcessStatBus(const std::map<std::string, std::string>& request) const {
    auto answer = json::Builder{};
    auto bus = answer.StartDict();

    bus.Key("request_id").Value(std::stoi(request.at("id")));

    const auto buses = GetBusStat(request.at("name"));
    if (!buses.has_value()) {
        bus.Key("error_message").Value("not found");
        return answer.EndDict().Build();
    }

    double shortest_distance = 0.0;
    int real_distance = 0;
    for (size_t l = 0, r = 1; l + 1 < buses->bus_stops.size(); ++l, ++r) {
        Stop* left_bus_ptr = buses->bus_stops.at(l);
        Stop* right_bus_ptr = buses->bus_stops.at(r);
        shortest_distance += ComputeDistance(left_bus_ptr->stop_coordinates,
                                             right_bus_ptr->stop_coordinates);
        real_distance += GetDistanceBetweenStops({left_bus_ptr, right_bus_ptr});
    }

    bus
        .Key("curvature").Value(real_distance / shortest_distance)
        .Key("route_length").Value(real_distance)
        .Key("stop_count").Value(static_cast<int>(buses->bus_stops.size()))
        .Key("unique_stop_count").Value(static_cast<int>(std::set<Stop *>(buses->bus_stops.begin(), buses->bus_stops.end()).size()));
    return answer.EndDict().Build();
}

json::Node RequestHandler::ProcessMap(const std::map<std::string, std::string>& request) const {
    svg::Document doc = RenderMap();
    std::ostringstream buf;
    doc.Render(buf);

    /* ВАЖНО: эта строка выводит рисунок в виде svg-формата в файл, в который затем будут выведены ответы на запросы.
     * При необходимости от этого можно отказаться
     */
    std::cout << buf.str() << std::endl;

    return
    json::Builder{}
        .StartDict()
            .Key("request_id").Value(std::stoi(request.at("id")))
            .Key("map").Value(buf.str())
        .EndDict()
    .Build();
}

svg::Document RequestHandler::RenderMap() const {
    svg::Document doc;
    const auto map_route = map_renderer_.Render();
    map_route.Draw(doc);
    return doc;
}

json::Node RequestHandler::ProcessRoute(const std::map<std::string, std::string>& request) const {
    auto answer = json::Builder{};
    auto route = answer.StartDict();
    route.Key("request_id").Value(std::stoi(request.at("id")));

    /* Node::Array */
    auto route_items = transport_router_.GetRouteAsNode(router_, request.at("from"), request.at("to"));
    if (!route_items.has_value()) {
        route.Key("error_message").Value(static_cast<std::string>("not found"));
        return answer.EndDict().Build();
    }

    route.Key("items").Value(route_items->AsArray());
    double weight = 0.0;
    for (const auto& item: route_items->AsArray()) {
        weight += item.AsDict().at("time").AsDouble();
    }
    route.Key("total_time").Value(weight);

    return answer.EndDict().Build();
}

}  // namespace request_handler
