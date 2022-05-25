#pragma once

#include "transport_catalogue.h"

#include <iostream>

namespace transport_catalogue {
namespace stat_reader {

void DatabaseOutput(std::istream& input, transport_catalogue::TransportCatalogue& data);

} // namespace stat_reader
} // namespace transport_catalogue