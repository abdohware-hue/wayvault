#include "wayvault/query.h"

#include <algorithm>
#include <cstdlib>
#include <sstream>

#include "wayvault/index.h"

namespace wayvault {
namespace {

bool parse_u32(const std::string& text, uint32_t* out) {
  char* end = nullptr;
  unsigned long value = std::strtoul(text.c_str(), &end, 10);
  if (end == text.c_str() || *end != '\0' || value > 0xfffffffful) {
    return false;
  }
  *out = static_cast<uint32_t>(value);
  return true;
}

bool contains(const std::string& haystack, const std::string& needle) {
  return haystack.find(needle) != std::string::npos;
}

}  // namespace

QueryExpression parse_query(const std::string& text) {
  QueryExpression query;
  std::istringstream in(text);
  std::string token;
  while (in >> token) {
    size_t pos = token.find('=');
    std::string op = "=";
    if (pos == std::string::npos) {
      pos = token.find('~');
      op = "~";
    }
    if (pos == std::string::npos) {
      pos = token.find('>');
      op = ">";
    }
    if (pos == std::string::npos) {
      continue;
    }
    QueryTerm term;
    term.field = token.substr(0, pos);
    term.op = op;
    term.value = token.substr(pos + 1);
    query.terms.push_back(term);
  }
  return query;
}

bool tile_matches_query(const VaultFile& file, const TileRecord& tile, const QueryExpression& query) {
  for (const QueryTerm& term : query.terms) {
    if (term.field == "id") {
      uint32_t value = 0;
      if (!parse_u32(term.value, &value) || tile.id != value) {
        return false;
      }
    } else if (term.field == "codec") {
      uint32_t value = 0;
      if (!parse_u32(term.value, &value) || tile.codec != value) {
        return false;
      }
    } else if (term.field == "layer") {
      uint32_t value = 0;
      if (!parse_u32(term.value, &value) || tile.layer != value) {
        return false;
      }
    } else if (term.field == "name") {
      std::string name = tile_display_name(file, tile);
      if (term.op == "~") {
        if (!contains(name, term.value)) {
          return false;
        }
      } else if (name != term.value) {
        return false;
      }
    } else if (term.field == "area") {
      uint32_t value = 0;
      uint32_t area = static_cast<uint32_t>(tile.width) * static_cast<uint32_t>(tile.height);
      if (!parse_u32(term.value, &value)) {
        return false;
      }
      if (term.op == ">") {
        if (!(area > value)) {
          return false;
        }
      } else if (area != value) {
        return false;
      }
    }
  }
  return true;
}

std::vector<uint32_t> select_tiles(const VaultFile& file, const QueryExpression& query) {
  std::vector<uint32_t> out;
  for (const TileRecord& tile : file.tiles) {
    if (tile_matches_query(file, tile, query)) {
      out.push_back(tile.id);
    }
  }
  std::sort(out.begin(), out.end());
  return out;
}

}  // namespace wayvault
