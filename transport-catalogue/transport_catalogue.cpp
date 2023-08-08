#include <deque>
#include <string>
#include <string_view>
#include <vector>
#include <set>
#include <iostream>

#include "transport_catalogue.h"

namespace transport {

TransportCatalogue::TransportCatalogue(Stops stops,
                                       PeekStops peek_stops,
                                       Buses buses,
                                       PeekBuses peek_buses,
                                       StopBuses stop_buses,
                                       domain::DistanceBetweenStops distance_between_stops)
                                       : stops_(std::move(stops))
                                       , peek_stops_(std::move(peek_stops))
                                       , buses_(std::move(buses))
                                       , peek_buses_(std::move(peek_buses))
                                       , stop_buses_(std::move(stop_buses))
                                       , distance_between_stops_(std::move(distance_between_stops)) {
}

const Bus* TransportCatalogue::FindBusByName(std::string_view name) const {
    if (peek_buses_.find(name) == peek_buses_.end()) {
        return nullptr;
    }

    return peek_buses_.at(name);
}

const Stop* TransportCatalogue::FindStopByName(std::string_view name) const {
    if (peek_stops_.find(name) == peek_stops_.end()) {
        return nullptr;
    }

    return peek_stops_.at(name);
}

const std::set<std::string_view>* TransportCatalogue::FindBusesForStop(std::string_view stop) const {
    if (peek_stops_.find(stop) == peek_stops_.end()) {
        return nullptr;
    }

    return &stop_buses_.at(stop);
}

int TransportCatalogue::DistanceBetweenStops(const std::pair<Stop*, Stop*>& stops) const {
    if (distance_between_stops_.find(stops) == distance_between_stops_.end()) {
        return 0;
    }

    return distance_between_stops_.at(stops);
}

size_t TransportCatalogue::GetCountStops() const {
    return stops_.size();
}
size_t TransportCatalogue::GetCountBuses() const {
    return buses_.size();
}
const Stops& TransportCatalogue::GetStops() const {
    return stops_;
}
const Buses& TransportCatalogue::GetBuses() const {
    return buses_;
}

}  // namespace transport