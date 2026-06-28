#include "wayvault/permissions.h"

#include <sstream>

#include "wayvault/path_matcher.h"

namespace wayvault {

uint32_t permission_bit(const std::string& name) {
  if (name == "read") return 1u << 0;
  if (name == "write") return 1u << 1;
  if (name == "delete") return 1u << 2;
  if (name == "admin") return 1u << 3;
  if (name == "preview") return 1u << 4;
  return 0;
}

namespace {

uint32_t parse_mask(const std::string& text) {
  uint32_t mask = 0;
  std::stringstream ss(text);
  std::string part;
  while (std::getline(ss, part, '+')) {
    mask |= permission_bit(part);
  }
  return mask;
}

}  // namespace

std::vector<PrincipalRule> parse_acl_text(const std::string& text) {
  std::vector<PrincipalRule> rules;
  std::istringstream input(text);
  std::string line;
  while (std::getline(input, line)) {
    if (line.empty() || line[0] == '#') continue;
    std::stringstream ss(line);
    std::string principal;
    std::string resource;
    std::string allow;
    std::string deny;
    std::getline(ss, principal, '|');
    std::getline(ss, resource, '|');
    std::getline(ss, allow, '|');
    std::getline(ss, deny, '|');
    rules.push_back(PrincipalRule{principal, resource, parse_mask(allow), parse_mask(deny)});
  }
  return rules;
}

AccessDecision decide_access(const std::vector<PrincipalRule>& rules,
                             const std::string& principal,
                             const std::string& resource,
                             uint32_t request_mask) {
  AccessDecision decision;
  uint32_t allowed = 0;
  uint32_t denied = 0;
  for (const PrincipalRule& rule : rules) {
    if (rule.principal != principal && rule.principal != "*") continue;
    if (!path_matches(compile_path_pattern(rule.resource_pattern), resource)) continue;
    allowed |= rule.allow_mask;
    denied |= rule.deny_mask;
    decision.reasons.push_back(rule.principal + " matched " + rule.resource_pattern);
  }
  decision.granted_mask = allowed & ~denied;
  decision.allowed = (decision.granted_mask & request_mask) == request_mask;
  if (denied & request_mask) decision.reasons.push_back("explicit deny");
  if (!decision.allowed) decision.reasons.push_back("missing permission bits");
  return decision;
}

std::string format_access_decision(const AccessDecision& decision) {
  std::ostringstream out;
  out << (decision.allowed ? "allowed" : "denied") << " mask=" << decision.granted_mask;
  for (const std::string& reason : decision.reasons) out << "\n  " << reason;
  return out.str();
}

}  // namespace wayvault
