#include <iostream>

#include "wayvault/container.h"
#include "wayvault/index.h"
#include "wayvault/layout.h"
#include "wayvault/manifest.h"
#include "wayvault/stats.h"
#include "wayvault/validator.h"
#include "wayvault/io.h"
#include "wayvault/metadata.h"
#include "wayvault/stream.h"

int main(int argc, char** argv) {
  if (argc != 2) {
    std::cerr << "usage: wayvault-dump <file.wvl>\n";
    return 2;
  }

  try {
    std::vector<unsigned char> bytes = wayvault::read_file(argv[1]);
    wayvault::VaultFile file = wayvault::parse_vault(bytes.data(), bytes.size());
    std::cout << wayvault::describe_vault(file) << "\n";
    std::cout << wayvault::metadata_report(file) << "\n";
    std::cout << wayvault::stats_report(file) << "\n";
    auto diagnostics = wayvault::validate_vault(file);
    if (!diagnostics.empty()) {
      std::cout << diagnostics.report();
    }
    wayvault::VaultIndex index = wayvault::build_index(file);
    std::cout << "index entries=" << index.by_id.size() << "\n";
    if (wayvault::find_section(file, wayvault::TAG_CMDS) != nullptr) {
      wayvault::StreamState state = wayvault::execute_commands(file, wayvault::parse_commands(file));
      std::cout << "stream checksum=" << state.checksum << " decoded=" << state.decoded_tiles
                << " sampled=" << state.sampled_views << "\n";
    }
  } catch (const std::exception& ex) {
    std::cerr << "wayvault-dump: " << ex.what() << "\n";
    return 1;
  }

  return 0;
}
