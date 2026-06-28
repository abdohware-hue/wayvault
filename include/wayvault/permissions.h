#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace wayvault {

struct PrincipalRule {
  std::string principal;
  std::string resource_pattern;
  uint32_t allow_mask = 0;
  uint32_t deny_mask = 0;
};

struct AccessDecision {
  bool allowed = false;
  uint32_t granted_mask = 0;
  std::vector<std::string> reasons;
};

uint32_t permission_bit(const std::string& name);
std::vector<PrincipalRule> parse_acl_text(const std::string& text);
AccessDecision decide_access(const std::vector<PrincipalRule>& rules,
                             const std::string& principal,
                             const std::string& resource,
                             uint32_t request_mask);
std::string format_access_decision(const AccessDecision& decision);

}  // namespace wayvault
