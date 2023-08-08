#pragma once

#include <iostream>
#include <string>
#include <map>
#include <optional>

#include "json_reader.h"
#include "transport_catalogue.h"
#include "map_renderer.h"
#include "domain.h"
#include "json.h"
#include "router.h"

namespace request_handler {

using namespace json_reader;
using namespace transport;
using namespace map_renderer;
using namespace domain;
using namespace router;


class RequestHandler {
public:
    RequestHandler(const TransportCatalogue& transport_catalogue,
                   const MapRenderer& map_renderer,
                   const TransportRouter& transport_router);

    std::optional<Bus> GetBusStat(const std::string_view bus_name) const;
    std::optional<std::set<std::string_view> > GetBusesByStop(const std::string_view stop_name) const;
    int GetDistanceBetweenStops(const std::pair<Stop*, Stop*>& stops) const;

    json::Document ProcessStatRequests(const SourseStatRequests& stat_requests) const;

    svg::Document RenderMap() const;

private:
    const TransportCatalogue& transport_catalogue_;
    const MapRenderer& map_renderer_;

    const TransportRouter& transport_router_;
    const Router<double> router_;


    json::Node ProcessStatStop(const std::map<std::string, std::string>& request) const;
    json::Node ProcessStatBus(const std::map<std::string, std::string>& request) const;
    json::Node ProcessMap(const std::map<std::string, std::string>& request) const;
    json::Node ProcessRoute(const std::map<std::string, std::string>& request) const;
};

}  // namespace request_handler
