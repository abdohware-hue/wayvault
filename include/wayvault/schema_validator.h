#pragma once

#include <map>
#include <string>
#include <vector>

namespace wayvault {

enum class FieldKind {
  String,
  Integer,
  Number,
  Boolean,
  Enum
};

struct FieldRule {
  std::string name;
  FieldKind kind = FieldKind::String;
  bool required = false;
  std::string default_value;
  std::vector<std::string> enum_values;
  double min_number = 0.0;
  double max_number = 0.0;
  bool has_min = false;
  bool has_max = false;
};

struct SchemaIssue {
  std::string field;
  std::string message;
  int severity = 0;
};

struct Schema {
  std::vector<FieldRule> fields;
  bool allow_unknown = true;
};

Schema parse_schema_text(const std::string& text);
std::vector<SchemaIssue> validate_record(const Schema& schema, const std::map<std::string, std::string>& record);
std::map<std::string, std::string> apply_schema_defaults(const Schema& schema, std::map<std::string, std::string> record);
std::string render_schema_issues(const std::vector<SchemaIssue>& issues);
bool schema_allows_value(const FieldRule& rule, const std::string& value, std::string* reason);

}  // namespace wayvault
