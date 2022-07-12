#pragma once

#include "transport_catalogue.h"
#include "json.h"

namespace request_handler {

json::Document DatabaseOutput(const json::Document& doc, transport_catalogue::TransportCatalogue& data);

} // namespace request_handler