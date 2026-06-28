#pragma once

#include "wayvault/container.h"
#include "wayvault/diagnostics.h"

namespace wayvault {

struct ValidationOptions {
  bool require_data_section = true;
  bool allow_duplicate_tile_ids = false;
  bool warn_on_empty_metadata = true;
  uint32_t max_tile_area = 1u << 20;
};

DiagnosticSet validate_vault(const VaultFile& file, const ValidationOptions& options = ValidationOptions{});
void validate_or_throw(const VaultFile& file, const ValidationOptions& options = ValidationOptions{});

}  // namespace wayvault
