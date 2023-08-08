#pragma once

#include <string>
#include <vector>
#include <map>

#include "json.h"
#include "domain.h"
#include "transport_catalogue.h"
#include "map_renderer.h"
#include "transport_router.h"


#include "serialization.h"

namespace json_reader {

using namespace json;
using namespace domain;
using namespace transport;
using namespace map_renderer;
using namespace router;


class JsonReader {
public:
    JsonReader(std::istream& input, int cas);
    const SourseStatRequests& GetRequestsStat() const;
    Dict GetSerializationSettings() const;

    TransportCatalogue CreateTransportCatalogue() const;
    MapRenderer CreateMapRenderer() const;
    TransportRouter CreateTransportRouter(const TransportCatalogue& transport_catalogue) const;

private:
    // Data from JSON
    json::Document document_;
    SourceStopRequests request_stops_;
    SourceBusRequests request_buses_;
    SourseStatRequests request_stat_;
    Dict render_settings_;
    Dict routing_settings_;
    Dict serialization_settings_;

    void ParseDocument(int cas);
    void ParseBaseRequests(const Array& base_requests);
    void ParseStatRequests(const Array& stat_requests);

    void ParseBaseStopRequests(const Dict& stop_request);
    void ParseBaseBusRequests(const Dict& bus_request);

    void ParseStatStopRequests(const Dict& stop_request);
    void ParseStatBusRequests(const Dict& bus_request);
    void ParseStatMapRequests(const Dict& map_request);
    void ParseStatRouteRequests(const Dict& route_request);


    // For creating sources to Transport Catalogue
    void AddPeekStop(const Stops::iterator pos, PeekStops& peek_stops) const;
    void AddStop(const Stop& stop, Stops& stops, PeekStops& peek_stops) const;
    void CreateStops(Stops& stops, PeekStops& peek_stops, domain::DistanceBetweenStops& distance_between_stops) const;

    void AddPeekBus(const Buses::iterator pos, PeekBuses& peek_buses) const;
    void AddBus(const std::string& name, std::vector<Stop*> stops, Buses& buses, PeekBuses& peek_buses, bool is_roundtrip) const;
    void CreateBus(const PeekStops& peek_stops, Buses& buses, PeekBuses& peek_buses) const;

    void CreateStopBuses(Buses& buses, StopBuses& stop_buses, Stops& stops) const;


    // For creating sources to MapRenderer
    void AddPeekStopMap(const Stops::iterator pos, PeekStops& peek_stops) const;
    void AddStopMap(const Stop& stop, Stops& stops, PeekStops& peek_stops) const;
    void CreateStopsMap(Stops& stops, PeekStops& peek_stops) const;

    void AddPeekBusMap(const Buses::iterator pos, PeekBuses& peek_buses) const;
    void AddBusMap(const std::string& name, std::vector<Stop*> stops, Buses& buses, PeekBuses& peek_buses) const;
    void CreateBusMap(const PeekStops& peek_stops, Buses& buses, PeekBuses& peek_buses,
                      ActualBuses& actual_buses, ActualCoordinates& actual_coordinates, ActualStops& actual_stops) const;

};

}  // namespace json_reader
