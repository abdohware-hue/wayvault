#pragma once

#include <map>
#include <string>
#include <vector>

namespace wayvault {

struct WorkflowStep {
  std::string id;
  std::vector<std::string> depends_on;
  std::map<std::string, std::string> parameters;
};

struct WorkflowPlan {
  std::vector<WorkflowStep> steps;
};

struct WorkflowResult {
  std::vector<std::string> order;
  std::vector<std::string> cycles;
  bool valid = true;
};

WorkflowResult resolve_workflow_order(const WorkflowPlan& plan);
WorkflowPlan parse_workflow_text(const std::string& text);
std::string describe_workflow_result(const WorkflowResult& result);
std::vector<WorkflowStep> workflow_ready_steps(const WorkflowPlan& plan, const std::vector<std::string>& completed);

}  // namespace wayvault
