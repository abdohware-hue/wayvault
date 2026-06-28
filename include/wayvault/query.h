#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "wayvault/container.h"

namespace wayvault {

struct QueryTerm {
  std::string field;
  std::string op;
  std::string value;
};

struct QueryExpression {
  std::vector<QueryTerm> terms;
};

QueryExpression parse_query(const std::string& text);
bool tile_matches_query(const VaultFile& file, const TileRecord& tile, const QueryExpression& query);
std::vector<uint32_t> select_tiles(const VaultFile& file, const QueryExpression& query);

}  // namespace wayvault
