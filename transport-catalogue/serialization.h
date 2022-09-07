#pragma once

#include "json.h"

#include <transport_catalogue.pb.h>
#include <map_renderer.pb.h>

#include "transport_catalogue.h"
#include "map_renderer.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <tuple>

using TC = transport_catalogue::TransportCatalogue;
using MR = map_renderer::MapRenderer;

class MakeBase {
public:
    MakeBase() = delete;
    MakeBase(const TC& tc, const MR& mr);
    transport_catalogue_proto::TransportCatalogueProto Pack();
    void PackInFile(const std::filesystem::path&) const;

private:
    TC catalogue_;
    MR map_renderer_;
    transport_catalogue_proto::TransportCatalogueProto proto_;
};

class ProcessRequests {
public:
    ProcessRequests() = delete;
    ProcessRequests(const transport_catalogue_proto::TransportCatalogueProto&, TC&, MR&);
    void UnPack();
private:
    const transport_catalogue_proto::TransportCatalogueProto& proto_;
    TC& catalogue_;
    MR& map_renderer_;
};