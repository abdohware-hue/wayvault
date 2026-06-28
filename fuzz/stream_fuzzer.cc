#include <cstddef>
#include <cstdint>

#include "wayvault/command_planner.h"
#include "wayvault/container.h"
#include "wayvault/stream.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  try {
    wayvault::VaultFile file = wayvault::parse_vault(data, size);
    auto planned = wayvault::plan_preview_walk(file, 8);
    (void)wayvault::command_plan_digest(planned);
    (void)wayvault::execute_commands(file, wayvault::parse_commands(file));
  } catch (const wayvault::ParseError&) {
  }
  return 0;
}
