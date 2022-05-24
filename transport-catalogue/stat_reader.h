#pragma once

#include "transport_catalogue.h"

#include <iostream>

namespace stat_reader {

void DatabaseOutput(std::istream& input, transport_catalogue::TransportCatalogue& data);

}// namespace stat_reader