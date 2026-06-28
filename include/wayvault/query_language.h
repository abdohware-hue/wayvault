#pragma once

#include <map>
#include <string>
#include <vector>

#include "wayvault/container.h"

namespace wayvault {

struct QueryClause {
  std::string key;
  std::string op;
  std::string value;
};

struct QueryProgram {
  std::vector<QueryClause> clauses;
  bool require_all = true;
};

QueryProgram parse_query_program(const std::string& text);
bool evaluate_query(const QueryProgram& program, const std::map<std::string, std::string>& tags);
std::vector<TileRecord> filter_tiles_by_query(const VaultFile& file, const QueryProgram& program);
std::string explain_query(const QueryProgram& program);

}  // namespace wayvault
