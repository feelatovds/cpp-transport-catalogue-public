#pragma once

#include <string>
#include <vector>
#include <map>
#include <deque>
#include <unordered_map>
#include <set>
#include <string_view>

#include "geo.h"

namespace domain {

struct Stop {
    Stop() = delete;
    Stop(std::string name, double latitude, double longitude);

    std::string stop_name;
    geo::Coordinates stop_coordinates;
};

struct Bus {
    Bus(std::string bus, std::vector<Stop*> stops);

    std::string bus_name;
    std::vector<Stop*> bus_stops;

    bool is_roundtrip = false;
};

using SourceStopRequests = std::vector<std::pair<domain::Stop, std::map<std::string, int>> >;
using SourceBusRequests = std::vector<std::pair<std::vector<std::string>, bool> >; // {name, is_roundtrip}
using SourseStatRequests = std::vector<std::map<std::string, std::string> >;


class StopDistanceHasher {
public:
    template<typename Stop>
    size_t operator()(const std::pair<Stop*, Stop*>& distance) const {
        return static_cast<size_t>(hasher_(distance.first) + 37 * hasher_(distance.second));
    }

    std::hash<const void*> hasher_;
};

using Stops = std::deque<Stop>;
using PeekStops = std::unordered_map<std::string_view, Stop*, std::hash<std::string_view> >;
using Buses = std::deque<Bus>;
using PeekBuses = std::unordered_map<std::string_view, Bus*, std::hash<std::string_view> >;
using StopBuses = std::unordered_map<std::string_view, std::set<std::string_view>, std::hash<std::string_view> >;
using DistanceBetweenStops = std::unordered_map<std::pair<Stop*, Stop*>, int, StopDistanceHasher>;

using ActualBuses = std::set<std::pair<std::string_view, bool> >;
using ActualCoordinates = std::vector<geo::Coordinates>;
using ActualStops = std::set<std::string_view>;

}  // namespace domain
