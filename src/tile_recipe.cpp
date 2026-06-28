#include "wayvault/tile_recipe.h"

#include <sstream>

namespace wayvault {

TileRecipe parse_tile_recipe(const std::string& text) {
  TileRecipe recipe;
  std::istringstream input(text);
  std::string line;
  while (std::getline(input, line)) {
    if (line.empty() || line[0] == '#') continue;
    if (recipe.id.empty()) {
      recipe.id = line;
      continue;
    }
    std::stringstream ss(line);
    RecipeOp op;
    std::getline(ss, op.name, ' ');
    std::string arg;
    while (ss >> arg) {
      size_t eq = arg.find('=');
      if (eq != std::string::npos) op.args[arg.substr(0, eq)] = arg.substr(eq + 1);
    }
    recipe.ops.push_back(op);
  }
  return recipe;
}

std::string render_tile_recipe(const TileRecipe& recipe) {
  std::ostringstream out;
  out << recipe.id << '\n';
  for (const RecipeOp& op : recipe.ops) {
    out << op.name;
    for (const auto& kv : op.args) out << ' ' << kv.first << '=' << kv.second;
    out << '\n';
  }
  return out.str();
}

uint32_t tile_recipe_cost(const TileRecipe& recipe) {
  uint32_t cost = 0;
  for (const RecipeOp& op : recipe.ops) {
    if (op.name == "decode") cost += 10;
    else if (op.name == "resample") cost += 25;
    else if (op.name == "filter") cost += 15;
    else if (op.name == "label") cost += 8;
    else cost += 3;
    cost += static_cast<uint32_t>(op.args.size());
  }
  return cost;
}

bool recipe_has_operation(const TileRecipe& recipe, const std::string& op) {
  for (const RecipeOp& candidate : recipe.ops) {
    if (candidate.name == op) return true;
  }
  return false;
}

std::vector<std::string> recipe_operation_names(const TileRecipe& recipe) {
  std::vector<std::string> names;
  for (const RecipeOp& op : recipe.ops) names.push_back(op.name);
  return names;
}

}  // namespace wayvault
