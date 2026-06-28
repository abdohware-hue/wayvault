#include "wayvault/query_language.h"

#include <algorithm>
#include <cctype>
#include <sstream>

#include "wayvault/error.h"
#include "wayvault/index.h"

namespace wayvault {

namespace {

std::string trim_query(const std::string& s) {
  size_t first = 0;
  while (first < s.size() && std::isspace(static_cast<unsigned char>(s[first]))) ++first;
  size_t last = s.size();
  while (last > first && std::isspace(static_cast<unsigned char>(s[last - 1]))) --last;
  return s.substr(first, last - first);
}

std::vector<std::string> split_query(const std::string& text, const std::string& sep) {
  std::vector<std::string> parts;
  size_t pos = 0;
  while (pos <= text.size()) {
    size_t next = text.find(sep, pos);
    if (next == std::string::npos) {
      parts.push_back(trim_query(text.substr(pos)));
      break;
    }
    parts.push_back(trim_query(text.substr(pos, next - pos)));
    pos = next + sep.size();
  }
  return parts;
}

QueryClause parse_clause(const std::string& text) {
  static const std::vector<std::string> ops = {">=", "<=", "!=", "=", ">", "<", "~"};
  for (const std::string& op : ops) {
    size_t pos = text.find(op);
    if (pos == std::string::npos) continue;
    QueryClause clause;
    clause.key = trim_query(text.substr(0, pos));
    clause.op = op;
    clause.value = trim_query(text.substr(pos + op.size()));
    if (clause.key.empty()) throw ParseError("query clause has no key");
    return clause;
  }
  throw ParseError("query clause has no supported operator");
}

bool compare_numbers(const std::string& left, const std::string& op, const std::string& right) {
  double a = std::stod(left);
  double b = std::stod(right);
  if (op == ">") return a > b;
  if (op == "<") return a < b;
  if (op == ">=") return a >= b;
  if (op == "<=") return a <= b;
  if (op == "=") return a == b;
  if (op == "!=") return a != b;
  return false;
}

}  // namespace

QueryProgram parse_query_program(const std::string& text) {
  QueryProgram program;
  std::string mode = "&&";
  if (text.find("||") != std::string::npos) {
    mode = "||";
    program.require_all = false;
  }
  for (const std::string& part : split_query(text, mode)) {
    if (!part.empty()) program.clauses.push_back(parse_clause(part));
  }
  return program;
}

bool evaluate_query(const QueryProgram& program, const std::map<std::string, std::string>& tags) {
  if (program.clauses.empty()) return true;
  size_t passed = 0;
  for (const QueryClause& clause : program.clauses) {
    auto it = tags.find(clause.key);
    bool ok = false;
    if (it != tags.end()) {
      const std::string& actual = it->second;
      if (clause.op == "=") ok = actual == clause.value;
      else if (clause.op == "!=") ok = actual != clause.value;
      else if (clause.op == "~") ok = actual.find(clause.value) != std::string::npos;
      else ok = compare_numbers(actual, clause.op, clause.value);
    }
    if (ok) ++passed;
    if (program.require_all && !ok) return false;
    if (!program.require_all && ok) return true;
  }
  return program.require_all ? passed == program.clauses.size() : false;
}

std::vector<TileRecord> filter_tiles_by_query(const VaultFile& file, const QueryProgram& program) {
  std::vector<TileRecord> out;
  for (const TileRecord& tile : file.tiles) {
    std::map<std::string, std::string> tags;
    tags["id"] = std::to_string(tile.id);
    tags["layer"] = std::to_string(tile.layer);
    tags["codec"] = std::to_string(tile.codec);
    tags["width"] = std::to_string(tile.width);
    tags["height"] = std::to_string(tile.height);
    tags["name"] = tile_display_name(file, tile);
    if (evaluate_query(program, tags)) out.push_back(tile);
  }
  return out;
}

std::string explain_query(const QueryProgram& program) {
  std::ostringstream out;
  out << (program.require_all ? "all" : "any") << " of " << program.clauses.size() << " clauses";
  for (const QueryClause& clause : program.clauses) {
    out << "\n  " << clause.key << " " << clause.op << " " << clause.value;
  }
  return out.str();
}

}  // namespace wayvault
