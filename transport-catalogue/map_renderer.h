#pragma once

#include "svg.h"
#include "domain.h"
#include "json.h"

namespace map_renderer {

void MapRenderer(const json::Document& doc, std::ostream& out);
    
} // namespace map_renderer