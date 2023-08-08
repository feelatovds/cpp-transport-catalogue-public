#pragma once

#include <deque>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
#include <set>
#include <functional>

#include "domain.h"

namespace transport {

using namespace domain;


class TransportCatalogue {
public:
    TransportCatalogue(Stops stops,
                       PeekStops peek_stops,
                       Buses buses,
                       PeekBuses peek_buses,
                       StopBuses stop_buses,
                       domain::DistanceBetweenStops distance_between_stops);


    const Bus* FindBusByName(std::string_view name) const;
    const Stop* FindStopByName(std::string_view name) const;
    const std::set<std::string_view>* FindBusesForStop(std::string_view stop) const;
    int DistanceBetweenStops(const std::pair<Stop*, Stop*>& stops) const;

    size_t GetCountStops() const;
    size_t GetCountBuses() const;
    const Stops& GetStops() const;
    const Buses& GetBuses() const;


private:
    Stops stops_;
    PeekStops peek_stops_;
    Buses buses_;
    PeekBuses peek_buses_;
    StopBuses stop_buses_;
    domain::DistanceBetweenStops distance_between_stops_;
};

}  // namespace transport
