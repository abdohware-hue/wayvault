#pragma once

#include <cstddef>
#include <map>
#include <string>
#include <vector>

namespace wayvault {

struct CsvRow {
  std::map<std::string, std::string> values;
};

struct CsvTable {
  std::vector<std::string> columns;
  std::vector<CsvRow> rows;
};

struct CsvOptions {
  char delimiter = ',';
  bool has_header = true;
  bool trim_cells = true;
  bool keep_empty_rows = false;
};

CsvTable parse_csv_table(const std::string& text, CsvOptions options);
std::string write_csv_table(const CsvTable& table, CsvOptions options);
std::vector<std::string> parse_csv_record(const std::string& line, char delimiter);
std::vector<CsvRow> filter_rows_with_value(const CsvTable& table, const std::string& column, const std::string& value);
std::map<std::string, size_t> count_by_column(const CsvTable& table, const std::string& column);
CsvTable select_columns(const CsvTable& table, const std::vector<std::string>& columns);
std::string csv_table_summary(const CsvTable& table);

}  // namespace wayvault
