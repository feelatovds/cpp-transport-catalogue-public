#include <string>
#include <vector>
#include <cassert>
#include <map>

#include "json_reader.h"

namespace json_reader {

JsonReader::JsonReader(std::istream& input, int cas)
: document_(Load(input)) {
    ParseDocument(cas);
}

const SourseStatRequests& JsonReader::GetRequestsStat() const {
    return request_stat_;
}

Dict JsonReader::GetSerializationSettings() const {
    return serialization_settings_;
}

// ---------------Parsing JSON---------------

void JsonReader::ParseDocument(int cas) {
    if (cas == 0) { // make_base
        const Array& base = document_.GetRoot().AsDict().at("base_requests"s).AsArray();
        routing_settings_ = document_.GetRoot().AsDict().at("routing_settings"s).AsDict();
        render_settings_ = document_.GetRoot().AsDict().at("render_settings"s).AsDict();
        serialization_settings_ = document_.GetRoot().AsDict().at("serialization_settings"s).AsDict();
        ParseBaseRequests(base);
    } else if (cas == 1) { // process_requests
        const Array& stat = document_.GetRoot().AsDict().at("stat_requests"s).AsArray();
        serialization_settings_ = document_.GetRoot().AsDict().at("serialization_settings"s).AsDict();
        ParseStatRequests(stat);
    }
}

void JsonReader::ParseBaseRequests(const Array& base_requests) {
    for (auto it = base_requests.begin(); it != base_requests.end(); ++it) {
        const Dict& request = it->AsDict();
        if (request.at("type"s) == "Stop"s) {
            ParseBaseStopRequests(request);
        } else if (request.at("type"s) == "Bus"s) {
            ParseBaseBusRequests(request);
        } else {
            assert(false);
        }
    }
}

void JsonReader::ParseStatRequests(const Array& stat_requests) {
    for (auto it = stat_requests.begin(); it != stat_requests.end(); ++it) {
        const Dict& request = it->AsDict();
        if (request.at("type"s) == "Stop"s) {
            ParseStatStopRequests(request);
        } else if (request.at("type"s) == "Bus"s) {
            ParseStatBusRequests(request);
        } else if (request.at("type") == "Map"s) {
            ParseStatMapRequests(request);
        } else if (request.at("type") == "Route"s) {
            ParseStatRouteRequests(request);
        }
        else {
            assert(false);
        }
    }
}

void JsonReader::ParseBaseStopRequests(const Dict& stop_request) {
    std::string name = stop_request.at("name"s).AsString();
    double latitude = stop_request.at("latitude"s).AsDouble();
    double longitude = stop_request.at("longitude"s).AsDouble();
    domain::Stop stop(std::move(name), latitude, longitude);

    std::map<std::string, int> distance_to_stop;
    for(const auto& [stop_, distance_] : stop_request.at("road_distances"s).AsDict()) {
        distance_to_stop.insert({stop_, distance_.AsInt()});
    }

    request_stops_.push_back({std::move(stop), std::move(distance_to_stop)});
}

void JsonReader::ParseBaseBusRequests(const Dict& bus_request) {
    std::string name = bus_request.at("name"s).AsString();
    bool is_roundtrip = bus_request.at("is_roundtrip"s).AsBool();

    std::vector<std::string> bus;
    bus.push_back(std::move(name));

    for (const auto& stop : bus_request.at("stops"s).AsArray()) {
        bus.push_back(stop.AsString());
    }

    if (!is_roundtrip) {
        for (size_t i = bus.size() - 2; i != 0; --i) {
            bus.push_back(bus[i]);
        }
    }

    request_buses_.push_back({std::move(bus), is_roundtrip});
}

void JsonReader::ParseStatStopRequests(const Dict& stop_request) {
    std::map<std::string, std::string> request;
    request.insert({"id"s, std::to_string(stop_request.at("id"s).AsInt())});
    request.insert({"type"s, "Stop"s});
    request.insert({"name"s, stop_request.at("name"s).AsString()});

    request_stat_.push_back(std::move(request));
}

void JsonReader::ParseStatBusRequests(const Dict& bus_request) {
    std::map<std::string, std::string> request;
    request.insert({"id"s, std::to_string(bus_request.at("id"s).AsInt())});
    request.insert({"type"s, "Bus"s});
    request.insert({"name"s, bus_request.at("name"s).AsString()});

    request_stat_.push_back(std::move(request));
}

void JsonReader::ParseStatMapRequests(const Dict& map_request) {
    std::map<std::string, std::string> request;
    request.insert({"id"s, std::to_string(map_request.at("id"s).AsInt())});
    request.insert({"type"s, "Map"s});

    request_stat_.push_back(std::move(request));
}

void JsonReader::ParseStatRouteRequests(const Dict& route_request) {
    std::map<std::string, std::string> request;
    request.insert({"id"s, std::to_string(route_request.at("id"s).AsInt())});
    request.insert({"type"s, "Route"s});
    request.insert({"from"s, route_request.at("from"s).AsString()});
    request.insert({"to"s, route_request.at("to"s).AsString()});

    request_stat_.push_back(std::move(request));
}


// ---------------Creating Transport Catalogue---------------

TransportCatalogue JsonReader::CreateTransportCatalogue() const {
    Stops stops_to_tc;
    PeekStops peek_stops_to_tc;
    domain::DistanceBetweenStops distance_between_stops_to_tc;
    CreateStops(stops_to_tc, peek_stops_to_tc, distance_between_stops_to_tc);

    Buses buses_to_tc;
    PeekBuses peek_buses_to_tc;
    CreateBus(peek_stops_to_tc, buses_to_tc, peek_buses_to_tc);

    StopBuses stop_buses_to_tc;
    CreateStopBuses(buses_to_tc, stop_buses_to_tc, stops_to_tc);

    return TransportCatalogue(std::move(stops_to_tc),
                              std::move(peek_stops_to_tc),
                              std::move(buses_to_tc),
                              std::move(peek_buses_to_tc),
                              std::move(stop_buses_to_tc),
                              std::move(distance_between_stops_to_tc));
}

void JsonReader::AddPeekStop(const Stops::iterator pos, PeekStops& peek_stops) const {
    peek_stops[pos->stop_name] = &(*pos);
}

void JsonReader::AddStop(const Stop& stop, Stops& stops, PeekStops& peek_stops) const {
    AddPeekStop(stops.insert(stops.end(), stop), peek_stops);
}

void JsonReader::CreateStops(Stops& stops, PeekStops& peek_stops, domain::DistanceBetweenStops& distance_between_stops) const {
    for (const auto& stop : request_stops_) {
        AddStop(stop.first, stops, peek_stops);
    }

    for (const auto& stop : request_stops_) {
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

void JsonReader::AddPeekBus(const Buses::iterator pos, PeekBuses& peek_buses) const {
    peek_buses[pos->bus_name] = &(*pos);
}
void JsonReader::AddBus(const std::string& name, std::vector<Stop*> stops, Buses& buses, PeekBuses& peek_buses, bool is_roundtrip) const {
    Bus bus(name, std::move(stops));
    bus.is_roundtrip = is_roundtrip;
    AddPeekBus(buses.insert(buses.end(), std::move(bus)), peek_buses);
}
void JsonReader::CreateBus(const PeekStops& peek_stops, Buses& buses, PeekBuses& peek_buses) const {
    for (const auto& bus_pair : request_buses_) {
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

void JsonReader::CreateStopBuses(Buses& buses, StopBuses& stop_buses, Stops& stops) const {
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


// ---------------Creating Map Renderer---------------

MapRenderer JsonReader::CreateMapRenderer() const {
    Stops stops_to_mr;
    PeekStops peek_stops_to_mr;
    CreateStopsMap(stops_to_mr, peek_stops_to_mr);

    Buses buses_to_mr;
    PeekBuses peek_buses_to_mr;
    ActualBuses actual_buses_to_mr;
    ActualCoordinates actual_coordinates_to_mr;
    ActualStops actual_stops_to_mr;
    CreateBusMap(peek_stops_to_mr, buses_to_mr, peek_buses_to_mr,
                 actual_buses_to_mr, actual_coordinates_to_mr, actual_stops_to_mr);

    return MapRenderer(render_settings_,
                       std::move(stops_to_mr),
                       std::move(peek_stops_to_mr),
                       std::move(buses_to_mr),
                       std::move(peek_buses_to_mr),
                       std::move(actual_buses_to_mr),
                       std::move(actual_coordinates_to_mr),
                       std::move(actual_stops_to_mr));
}

void JsonReader::AddPeekStopMap(const Stops::iterator pos, PeekStops& peek_stops) const {
    peek_stops[pos->stop_name] = &(*pos);
}
void JsonReader::AddStopMap(const Stop& stop, Stops& stops, PeekStops& peek_stops) const {
    AddPeekStopMap(stops.insert(stops.end(), stop), peek_stops);
}
void JsonReader::CreateStopsMap(Stops& stops, PeekStops& peek_stops) const {
    for (const auto& stop : request_stops_) {
        AddStopMap(stop.first, stops, peek_stops);
    }
}

void JsonReader::AddPeekBusMap(const Buses::iterator pos, PeekBuses& peek_buses) const {
    peek_buses[pos->bus_name] = &(*pos);
}
void JsonReader::AddBusMap(const std::string& name, std::vector<Stop*> stops, Buses& buses, PeekBuses& peek_buses) const {
    Bus bus(name, std::move(stops));
    AddPeekBusMap(buses.insert(buses.end(), std::move(bus)), peek_buses);
}
void JsonReader::CreateBusMap(const PeekStops& peek_stops, Buses& buses, PeekBuses& peek_buses,
                  ActualBuses& actual_buses, ActualCoordinates& actual_coordinates, ActualStops& actual_stops) const {
    for (const auto& bus_pair : request_buses_) {
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

// ---------------Creating Transport Router---------------

TransportRouter JsonReader::CreateTransportRouter(const TransportCatalogue& transport_catalogue) const {
    return TransportRouter(routing_settings_, transport_catalogue);
}

}  // namespace json_reader
