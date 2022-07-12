#pragma once
#include "json.h"
#include "transport_catalogue.h"

namespace json_reader {

void jsonReader(const json::Document& doc, ::transport_catalogue::TransportCatalogue& data);

} // namespace input_reader