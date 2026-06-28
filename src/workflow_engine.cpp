#include "wayvault/workflow_engine.h"

#include <algorithm>
#include <set>
#include <sstream>

namespace wayvault {

namespace {

std::vector<std::string> split_workflow(const std::string& text, char sep) {
  std::vector<std::string> out;
  std::string cur;
  for (char c : text) {
    if (c == sep) {
      if (!cur.empty()) out.push_back(cur);
      cur.clear();
    } else {
      cur.push_back(c);
    }
  }
  if (!cur.empty()) out.push_back(cur);
  return out;
}

const WorkflowStep* find_step(const WorkflowPlan& plan, const std::string& id) {
  for (const WorkflowStep& step : plan.steps) {
    if (step.id == id) return &step;
  }
  return nullptr;
}

void visit_step(const WorkflowPlan& plan,
                const std::string& id,
                std::set<std::string>& visiting,
                std::set<std::string>& visited,
                WorkflowResult& result) {
  if (visited.count(id)) return;
  if (visiting.count(id)) {
    result.valid = false;
    result.cycles.push_back(id);
    return;
  }
  const WorkflowStep* step = find_step(plan, id);
  if (step == nullptr) {
    result.valid = false;
    result.cycles.push_back("missing:" + id);
    return;
  }
  visiting.insert(id);
  for (const std::string& dep : step->depends_on) {
    visit_step(plan, dep, visiting, visited, result);
  }
  visiting.erase(id);
  visited.insert(id);
  result.order.push_back(id);
}

}  // namespace

WorkflowPlan parse_workflow_text(const std::string& text) {
  WorkflowPlan plan;
  std::istringstream input(text);
  std::string line;
  while (std::getline(input, line)) {
    if (line.empty() || line[0] == '#') continue;
    size_t colon = line.find(':');
    WorkflowStep step;
    step.id = colon == std::string::npos ? line : line.substr(0, colon);
    if (colon != std::string::npos) {
      for (const std::string& dep : split_workflow(line.substr(colon + 1), ',')) {
        step.depends_on.push_back(dep);
      }
    }
    plan.steps.push_back(step);
  }
  return plan;
}

WorkflowResult resolve_workflow_order(const WorkflowPlan& plan) {
  WorkflowResult result;
  std::set<std::string> visiting;
  std::set<std::string> visited;
  for (const WorkflowStep& step : plan.steps) {
    visit_step(plan, step.id, visiting, visited, result);
  }
  std::vector<std::string> unique_order;
  std::set<std::string> seen;
  for (const std::string& id : result.order) {
    if (seen.insert(id).second) unique_order.push_back(id);
  }
  result.order = unique_order;
  return result;
}

std::vector<WorkflowStep> workflow_ready_steps(const WorkflowPlan& plan, const std::vector<std::string>& completed) {
  std::set<std::string> done(completed.begin(), completed.end());
  std::vector<WorkflowStep> ready;
  for (const WorkflowStep& step : plan.steps) {
    if (done.count(step.id)) continue;
    bool all_done = true;
    for (const std::string& dep : step.depends_on) {
      if (!done.count(dep)) {
        all_done = false;
        break;
      }
    }
    if (all_done) ready.push_back(step);
  }
  std::stable_sort(ready.begin(), ready.end(), [](const WorkflowStep& a, const WorkflowStep& b) {
    return a.id < b.id;
  });
  return ready;
}

std::string describe_workflow_result(const WorkflowResult& result) {
  std::ostringstream out;
  out << (result.valid ? "valid" : "invalid") << " steps=" << result.order.size();
  for (const std::string& id : result.order) out << "\n  " << id;
  for (const std::string& cycle : result.cycles) out << "\n  problem: " << cycle;
  return out.str();
}

}  // namespace wayvault
