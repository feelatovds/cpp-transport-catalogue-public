#pragma once

#include <transport_catalogue.pb.h>

#include "transport_catalogue.h"
#include "map_renderer.h"
#include "transport_router.h"

#include <filesystem>
#include <iostream>
#include <fstream>
#include <vector>

namespace serialization {

using namespace domain;

class TransportCatalogueExport {
public:
    TransportCatalogueExport() = default;

    struct DesTransportCatalogue {
        transport::TransportCatalogue transport_catalogue;
        map_renderer::MapRenderer map_renderer;
        router::TransportRouter transport_router;
    };

    void Serialize(const std::filesystem::path& path,
                   const transport::TransportCatalogue& transport_catalogue,
                   const map_renderer::MapRenderer& map_renderer,
                   const router::TransportRouter& transport_router) const;
    DesTransportCatalogue Deserialize(const std::filesystem::path& path) const;

private:
    // _______________ Serialize Transport Catalogue _______________
    transport_catalogue::TransportCatalogue MakeTransportCatalogueProtoStops(const transport::TransportCatalogue& transport_catalogue) const;
    transport_catalogue::TransportCatalogue MakeTransportCatalogueProtoBuses(const transport::TransportCatalogue& transport_catalogue) const;

    // _______________ Serialize Map Renderer _______________
    transport_catalogue::MapRenderer SerializeMapRenderer(const map_renderer::MapRenderer& map_renderer) const;
    transport_catalogue::RenderSettings MakeMapRendererProtoRendererSettings(const map_renderer::MapRenderer& map_renderer) const;
    transport_catalogue::MapRenderer MakeMapRendererProtoActualCoordinates(const map_renderer::MapRenderer& map_renderer) const;

    // _______________ Serialize Transport Router _______________
    transport_catalogue::TransportRouter SerializeTransportRouter(const router::TransportRouter& transport_router) const;
    transport_catalogue::RoutingSettings MakeTransportRouterProtoRoutingSettings(const router::TransportRouter& transport_router) const;
    transport_catalogue::Graph MakeTransportRouterProtoGraph(const router::TransportRouter& transport_router) const;
    transport_catalogue::TransportRouter MakeTransportRouterProtoStopNames(const router::TransportRouter& transport_router) const;
    transport_catalogue::TransportRouter MakeTransportRouterProtoStopRoutesInternalData(const router::TransportRouter& transport_router) const;

private:
    // _______________ Deserialize Transport Catalogue _______________
    transport::TransportCatalogue DeserializeTransportCatalogue(transport_catalogue::TransportCatalogue& transport_catalogue_import,
                                                                SourceStopRequests& request_stops,
                                                                SourceBusRequests& request_buses) const;
    void DeserializeTransportCatalogueStops(transport_catalogue::TransportCatalogue& transport_catalogue_import,
                                            SourceStopRequests& request_stops) const;
    void DeserializeTransportCatalogueBuses(transport_catalogue::TransportCatalogue& transport_catalogue_import,
                                            SourceBusRequests& request_buses) const;

    // _______________ Вспомогательные функции _______________

    void AddPeekStop(const Stops::iterator pos, PeekStops& peek_stops) const;
    void AddStop(const Stop& stop, Stops& stops, PeekStops& peek_stops) const;
    void CreateStops(Stops& stops, PeekStops& peek_stops,
                     domain::DistanceBetweenStops& distance_between_stops,
                     const SourceStopRequests& request_stops) const;

    void AddPeekBus(const Buses::iterator pos, PeekBuses& peek_buses) const;
    void AddBus(const std::string& name, std::vector<Stop*> stops, Buses& buses, PeekBuses& peek_buses, bool is_roundtrip) const;
    void CreateBus(const PeekStops& peek_stops, Buses& buses, PeekBuses& peek_buses, const SourceBusRequests& request_buses) const;

    void CreateStopBuses(const Buses& buses, StopBuses& stop_buses, const Stops& stops) const;

    // _______________ Deserialize Map Renderer _______________
    map_renderer::MapRenderer DeserializeMapRenderer(transport_catalogue::TransportCatalogue& transport_catalogue_import,
                                                     const SourceStopRequests& request_stops,
                                                     const SourceBusRequests& request_buses) const;
    map_renderer::RenderSettings DeserializeMapRendererRenderSettings(transport_catalogue::MapRenderer& map_renderer) const;

    // _______________ Вспомогательные функции _______________

    void AddPeekStopMap(const Stops::iterator pos, PeekStops& peek_stops) const;
    void AddStopMap(const Stop& stop, Stops& stops, PeekStops& peek_stops) const;
    void CreateStopsMap(Stops& stops, PeekStops& peek_stops, const SourceStopRequests& request_stops) const;

    void AddPeekBusMap(const Buses::iterator pos, PeekBuses& peek_buses) const;
    void AddBusMap(const std::string& name, std::vector<Stop*> stops, Buses& buses, PeekBuses& peek_buses) const;
    void CreateBusMap(const PeekStops& peek_stops, Buses& buses, PeekBuses& peek_buses,
                      ActualBuses& actual_buses, ActualCoordinates& actual_coordinates,
                      ActualStops& actual_stops, const SourceBusRequests& request_buses) const;

    // _______________ Deserialize Transport Router _______________
    router::TransportRouter DeserializeTransportRouter(transport_catalogue::TransportCatalogue& transport_catalogue_import) const;
    graph::DirectedWeightedGraph<double> DeserializeTransportRouterGraph(transport_catalogue::Graph& graph_from_ser) const;
    graph::Router<double>::RoutesInternalData DeserializeTransportRoutesInternalData(transport_catalogue::TransportRouter& transport_router) const;

};

} //  namespace serialization