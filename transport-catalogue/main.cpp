#include <fstream>
#include <iostream>
#include <string_view>
#include <filesystem>

#include "json_reader.h"
#include "transport_catalogue.h"
#include "request_handler.h"
#include "map_renderer.h"
#include "domain.h"
#include "transport_router.h"
#include "serialization.h"

using namespace json_reader;
using namespace transport;
using namespace map_renderer;
using namespace request_handler;
using namespace domain;
using namespace router;
using namespace serialization;

using namespace std::literals;

void PrintUsage(std::ostream& stream = std::cerr) {
    stream << "Usage: transport_catalogue [make_base|process_requests]\n"sv;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        PrintUsage();
        return 1;
    }

    const std::string_view mode(argv[1]);

    if (mode == "make_base"sv) {

        // make base here
        int cas = 0;
        JsonReader json_reader(std::cin, cas);
        TransportCatalogue transport_catalogue = json_reader.CreateTransportCatalogue();
        MapRenderer map_renderer = json_reader.CreateMapRenderer();
        TransportRouter transport_router = json_reader.CreateTransportRouter(transport_catalogue);
        TransportCatalogueExport transport_catalogue_export;
        const auto path = static_cast<std::filesystem::path>(json_reader.GetSerializationSettings().at("file"s).AsString());
        transport_catalogue_export.Serialize(path, transport_catalogue, map_renderer, transport_router);

    } else if (mode == "process_requests"sv) {

        // process requests here
        int cas = 1;
        JsonReader json_reader(std::cin, cas);
        const auto path = static_cast<std::filesystem::path>(json_reader.GetSerializationSettings().at("file"s).AsString());
        TransportCatalogueExport transport_catalogue_import;
        serialization::TransportCatalogueExport::DesTransportCatalogue TransportCatalogueImport = transport_catalogue_import.Deserialize(path);

        RequestHandler request_handler(TransportCatalogueImport.transport_catalogue,
                                       TransportCatalogueImport.map_renderer,
                                       TransportCatalogueImport.transport_router);

        json::Document document = request_handler.ProcessStatRequests(json_reader.GetRequestsStat());
        Print(document, std::cout);

    } else {
        PrintUsage();
        return 1;
    }
}