#pragma once

#include <transport_catalogue.pb.h>
#include <map_renderer.pb.h>

#include "transport_catalogue.h"
#include "map_renderer.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <tuple>

namespace serialization {

class Serialization {
public:
    Serialization(const transport_catalogue::TransportCatalogue& tc
            , const map_renderer::MapRenderer& mr);

    transport_catalogue_proto::TransportCatalogueProto Pack();
    // void PackInFile(const std::filesystem::path&) const;

private:
    transport_catalogue::TransportCatalogue catalogue_;
    map_renderer::MapRenderer map_renderer_;
    transport_catalogue_proto::TransportCatalogueProto proto_;

    void PackStops();
    void PackRoutes();
    void PackRenderSettings();
    void PackRoutingSettings();
};

class DeSerialization {
public:
    DeSerialization(const transport_catalogue_proto::TransportCatalogueProto&
                    , transport_catalogue::TransportCatalogue&
                    , map_renderer::MapRenderer&);
    
    void UnPack();
private:
    const transport_catalogue_proto::TransportCatalogueProto& proto_;
    transport_catalogue::TransportCatalogue& catalogue_;
    map_renderer::MapRenderer& map_renderer_;

    void UnPackTransportCatalogue();
    void UnPackMapRenderer();
};

} // namespace serialization
