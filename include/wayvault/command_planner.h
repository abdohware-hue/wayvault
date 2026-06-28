#pragma once

#include <cstddef>
#include <vector>

#include "wayvault/container.h"
#include "wayvault/stream.h"

namespace wayvault {

std::vector<StreamCommand> plan_preview_walk(const VaultFile& file, size_t max_tiles);
std::vector<StreamCommand> plan_layer_probe(const VaultFile& file, uint8_t layer);
uint32_t command_plan_digest(const std::vector<StreamCommand>& commands);

}  // namespace wayvault
