#include <iostream>

#include "wayvault/io.h"
#include "wayvault/sample_builder.h"

int main(int argc, char** argv) {
  if (argc != 2) {
    std::cerr << "usage: wayvault-make-sample <output.wvl>\n";
    return 2;
  }
  try {
    wayvault::write_file(argv[1], wayvault::build_default_sample());
  } catch (const std::exception& ex) {
    std::cerr << "wayvault-make-sample: " << ex.what() << "\n";
    return 1;
  }
  return 0;
}
