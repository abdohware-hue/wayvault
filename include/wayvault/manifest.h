#pragma once

#include <string>

#include "wayvault/container.h"

namespace wayvault {

std::string export_manifest(const VaultFile& file);
std::string export_tile_manifest_line(const VaultFile& file, const TileRecord& tile);

}  // namespace wayvault
