#include "wayvault/schema_validator.h"

#include <algorithm>
#include <cctype>
#include <sstream>

#include "wayvault/error.h"

namespace wayvault {

namespace {

std::string trim_schema(const std::string& text) {
  size_t first = 0;
  while (first < text.size() && std::isspace(static_cast<unsigned char>(text[first]))) {
    ++first;
  }
  size_t last = text.size();
  while (last > first && std::isspace(static_cast<unsigned char>(text[last - 1]))) {
    --last;
  }
  return text.substr(first, last - first);
}

std::vector<std::string> split_schema(const std::string& text, char sep) {
  std::vector<std::string> out;
  std::string cur;
  bool quoted = false;
  for (char c : text) {
    if (c == '"') {
      quoted = !quoted;
      cur.push_back(c);
    } else if (c == sep && !quoted) {
      out.push_back(trim_schema(cur));
      cur.clear();
    } else {
      cur.push_back(c);
    }
  }
  out.push_back(trim_schema(cur));
  return out;
}

FieldKind parse_kind(const std::string& value) {
  if (value == "string") return FieldKind::String;
  if (value == "int" || value == "integer") return FieldKind::Integer;
  if (value == "number" || value == "float") return FieldKind::Number;
  if (value == "bool" || value == "boolean") return FieldKind::Boolean;
  if (value == "enum") return FieldKind::Enum;
  throw ParseError("unknown schema field kind: " + value);
}

bool parse_bool_text(const std::string& value) {
  return value == "1" || value == "true" || value == "yes" || value == "required";
}

bool is_integer_text(const std::string& value) {
  if (value.empty()) return false;
  size_t pos = value[0] == '-' || value[0] == '+' ? 1 : 0;
  if (pos == value.size()) return false;
  for (; pos < value.size(); ++pos) {
    if (!std::isdigit(static_cast<unsigned char>(value[pos]))) return false;
  }
  return true;
}

bool is_number_text(const std::string& value) {
  if (value.empty()) return false;
  char* end = nullptr;
  std::strtod(value.c_str(), &end);
  return end != nullptr && *end == '\0';
}

bool enum_contains(const FieldRule& rule, const std::string& value) {
  return std::find(rule.enum_values.begin(), rule.enum_values.end(), value) != rule.enum_values.end();
}

void add_issue(std::vector<SchemaIssue>& issues, const std::string& field, const std::string& message, int severity) {
  issues.push_back(SchemaIssue{field, message, severity});
}

}  // namespace

Schema parse_schema_text(const std::string& text) {
  Schema schema;
  std::istringstream input(text);
  std::string line;
  while (std::getline(input, line)) {
    line = trim_schema(line);
    if (line.empty() || line[0] == '#') {
      continue;
    }
    if (line == "allow_unknown=false") {
      schema.allow_unknown = false;
      continue;
    }
    FieldRule rule;
    std::vector<std::string> parts = split_schema(line, ';');
    for (const std::string& part : parts) {
      if (part.empty()) continue;
      size_t eq = part.find('=');
      if (eq == std::string::npos) {
        if (rule.name.empty()) rule.name = trim_schema(part);
        continue;
      }
      std::string key = trim_schema(part.substr(0, eq));
      std::string value = trim_schema(part.substr(eq + 1));
      if (key == "name") rule.name = value;
      else if (key == "kind") rule.kind = parse_kind(value);
      else if (key == "required") rule.required = parse_bool_text(value);
      else if (key == "default") rule.default_value = value;
      else if (key == "enum") rule.enum_values = split_schema(value, '|');
      else if (key == "min") { rule.min_number = std::stod(value); rule.has_min = true; }
      else if (key == "max") { rule.max_number = std::stod(value); rule.has_max = true; }
      else throw ParseError("unknown schema property: " + key);
    }
    if (rule.name.empty()) {
      throw ParseError("schema field without a name");
    }
    schema.fields.push_back(rule);
  }
  return schema;
}

bool schema_allows_value(const FieldRule& rule, const std::string& value, std::string* reason) {
  if (value.empty() && !rule.required) {
    return true;
  }
  if (rule.kind == FieldKind::String) {
    return true;
  }
  if (rule.kind == FieldKind::Boolean) {
    if (value == "true" || value == "false" || value == "1" || value == "0") return true;
    if (reason) *reason = "expected boolean";
    return false;
  }
  if (rule.kind == FieldKind::Integer) {
    if (!is_integer_text(value)) {
      if (reason) *reason = "expected integer";
      return false;
    }
  } else if (rule.kind == FieldKind::Number) {
    if (!is_number_text(value)) {
      if (reason) *reason = "expected number";
      return false;
    }
  } else if (rule.kind == FieldKind::Enum) {
    if (!enum_contains(rule, value)) {
      if (reason) *reason = "not one of the allowed values";
      return false;
    }
    return true;
  }
  if (rule.kind == FieldKind::Integer || rule.kind == FieldKind::Number) {
    double number = std::stod(value);
    if (rule.has_min && number < rule.min_number) {
      if (reason) *reason = "below minimum";
      return false;
    }
    if (rule.has_max && number > rule.max_number) {
      if (reason) *reason = "above maximum";
      return false;
    }
  }
  return true;
}

std::vector<SchemaIssue> validate_record(const Schema& schema, const std::map<std::string, std::string>& record) {
  std::vector<SchemaIssue> issues;
  for (const FieldRule& rule : schema.fields) {
    auto it = record.find(rule.name);
    if (it == record.end() || it->second.empty()) {
      if (rule.required) {
        add_issue(issues, rule.name, "missing required field", 3);
      }
      continue;
    }
    std::string reason;
    if (!schema_allows_value(rule, it->second, &reason)) {
      add_issue(issues, rule.name, reason, 2);
    }
  }
  if (!schema.allow_unknown) {
    for (const auto& kv : record) {
      bool known = false;
      for (const FieldRule& rule : schema.fields) {
        if (rule.name == kv.first) {
          known = true;
          break;
        }
      }
      if (!known) {
        add_issue(issues, kv.first, "unknown field", 1);
      }
    }
  }
  return issues;
}

std::map<std::string, std::string> apply_schema_defaults(const Schema& schema, std::map<std::string, std::string> record) {
  for (const FieldRule& rule : schema.fields) {
    if (!rule.default_value.empty() && !record.count(rule.name)) {
      record[rule.name] = rule.default_value;
    }
  }
  return record;
}

std::string render_schema_issues(const std::vector<SchemaIssue>& issues) {
  std::ostringstream out;
  for (const SchemaIssue& issue : issues) {
    out << issue.field << ": " << issue.message << " severity=" << issue.severity << "\n";
  }
  return out.str();
}

}  // namespace wayvault
