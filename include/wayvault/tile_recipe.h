#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace wayvault {

struct RecipeOp {
  std::string name;
  std::map<std::string, std::string> args;
};

struct TileRecipe {
  std::string id;
  std::vector<RecipeOp> ops;
};

TileRecipe parse_tile_recipe(const std::string& text);
std::string render_tile_recipe(const TileRecipe& recipe);
uint32_t tile_recipe_cost(const TileRecipe& recipe);
bool recipe_has_operation(const TileRecipe& recipe, const std::string& op);
std::vector<std::string> recipe_operation_names(const TileRecipe& recipe);

}  // namespace wayvault
