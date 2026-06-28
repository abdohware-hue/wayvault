#include "wayvault/csv_table.h"

#include <algorithm>
#include <cctype>
#include <sstream>

#include "wayvault/error.h"

namespace wayvault {

namespace {

std::string trim_cell(const std::string& value) {
  size_t first = 0;
  while (first < value.size() && std::isspace(static_cast<unsigned char>(value[first]))) {
    ++first;
  }
  size_t last = value.size();
  while (last > first && std::isspace(static_cast<unsigned char>(value[last - 1]))) {
    --last;
  }
  return value.substr(first, last - first);
}

std::string quote_cell(const std::string& value, char delimiter) {
  bool need_quote = value.find(delimiter) != std::string::npos ||
                    value.find('"') != std::string::npos ||
                    value.find('\n') != std::string::npos ||
                    value.find('\r') != std::string::npos;
  if (!need_quote) {
    return value;
  }
  std::string out;
  out.push_back('"');
  for (char c : value) {
    if (c == '"') {
      out.push_back('"');
      out.push_back('"');
    } else {
      out.push_back(c);
    }
  }
  out.push_back('"');
  return out;
}

std::vector<std::string> physical_lines(const std::string& text) {
  std::vector<std::string> lines;
  std::string cur;
  bool quoted = false;
  for (size_t i = 0; i < text.size(); ++i) {
    char c = text[i];
    if (c == '"') {
      if (quoted && i + 1 < text.size() && text[i + 1] == '"') {
        cur.push_back(c);
        cur.push_back(text[++i]);
        continue;
      }
      quoted = !quoted;
      cur.push_back(c);
    } else if ((c == '\n' || c == '\r') && !quoted) {
      if (c == '\r' && i + 1 < text.size() && text[i + 1] == '\n') {
        ++i;
      }
      lines.push_back(cur);
      cur.clear();
    } else {
      cur.push_back(c);
    }
  }
  if (!cur.empty() || (!text.empty() && text.back() == '\n')) {
    lines.push_back(cur);
  }
  return lines;
}

bool all_empty(const std::vector<std::string>& cells) {
  for (const std::string& cell : cells) {
    if (!cell.empty()) {
      return false;
    }
  }
  return true;
}

}  // namespace

std::vector<std::string> parse_csv_record(const std::string& line, char delimiter) {
  std::vector<std::string> cells;
  std::string cur;
  bool quoted = false;
  for (size_t i = 0; i < line.size(); ++i) {
    char c = line[i];
    if (quoted) {
      if (c == '"') {
        if (i + 1 < line.size() && line[i + 1] == '"') {
          cur.push_back('"');
          ++i;
        } else {
          quoted = false;
        }
      } else {
        cur.push_back(c);
      }
    } else {
      if (c == '"' && cur.empty()) {
        quoted = true;
      } else if (c == delimiter) {
        cells.push_back(cur);
        cur.clear();
      } else {
        cur.push_back(c);
      }
    }
  }
  if (quoted) {
    throw ParseError("csv record ended inside a quoted field");
  }
  cells.push_back(cur);
  return cells;
}

CsvTable parse_csv_table(const std::string& text, CsvOptions options) {
  CsvTable table;
  std::vector<std::string> lines = physical_lines(text);
  size_t row_start = 0;
  if (options.has_header && !lines.empty()) {
    table.columns = parse_csv_record(lines.front(), options.delimiter);
    if (options.trim_cells) {
      for (std::string& c : table.columns) {
        c = trim_cell(c);
      }
    }
    row_start = 1;
  }
  for (size_t row_index = row_start; row_index < lines.size(); ++row_index) {
    std::vector<std::string> cells = parse_csv_record(lines[row_index], options.delimiter);
    if (options.trim_cells) {
      for (std::string& c : cells) {
        c = trim_cell(c);
      }
    }
    if (!options.keep_empty_rows && all_empty(cells)) {
      continue;
    }
    if (table.columns.empty()) {
      for (size_t i = 0; i < cells.size(); ++i) {
        table.columns.push_back("column" + std::to_string(i + 1));
      }
    }
    CsvRow row;
    for (size_t i = 0; i < table.columns.size(); ++i) {
      row.values[table.columns[i]] = i < cells.size() ? cells[i] : std::string();
    }
    table.rows.push_back(row);
  }
  return table;
}

std::string write_csv_table(const CsvTable& table, CsvOptions options) {
  std::ostringstream out;
  if (options.has_header) {
    for (size_t i = 0; i < table.columns.size(); ++i) {
      if (i != 0) {
        out << options.delimiter;
      }
      out << quote_cell(table.columns[i], options.delimiter);
    }
    out << "\n";
  }
  for (const CsvRow& row : table.rows) {
    for (size_t i = 0; i < table.columns.size(); ++i) {
      if (i != 0) {
        out << options.delimiter;
      }
      auto it = row.values.find(table.columns[i]);
      out << quote_cell(it == row.values.end() ? std::string() : it->second, options.delimiter);
    }
    out << "\n";
  }
  return out.str();
}

std::vector<CsvRow> filter_rows_with_value(const CsvTable& table, const std::string& column, const std::string& value) {
  std::vector<CsvRow> out;
  for (const CsvRow& row : table.rows) {
    auto it = row.values.find(column);
    if (it != row.values.end() && it->second == value) {
      out.push_back(row);
    }
  }
  return out;
}

std::map<std::string, size_t> count_by_column(const CsvTable& table, const std::string& column) {
  std::map<std::string, size_t> counts;
  for (const CsvRow& row : table.rows) {
    auto it = row.values.find(column);
    counts[it == row.values.end() ? std::string() : it->second] += 1;
  }
  return counts;
}

CsvTable select_columns(const CsvTable& table, const std::vector<std::string>& columns) {
  CsvTable out;
  out.columns = columns;
  for (const CsvRow& row : table.rows) {
    CsvRow next;
    for (const std::string& column : columns) {
      auto it = row.values.find(column);
      next.values[column] = it == row.values.end() ? std::string() : it->second;
    }
    out.rows.push_back(next);
  }
  return out;
}

std::string csv_table_summary(const CsvTable& table) {
  std::ostringstream out;
  out << "columns=" << table.columns.size() << " rows=" << table.rows.size();
  for (const std::string& column : table.columns) {
    out << "\n  " << column << " distinct=" << count_by_column(table, column).size();
  }
  return out.str();
}

}  // namespace wayvault
