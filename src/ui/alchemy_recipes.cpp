#include "alchemy_recipes.hpp"

namespace alchemy {

std::optional<std::size_t> find_recipe(std::array<int, 3> selection, int count)
{
    if (count < 1 || count > 3) return std::nullopt;
    for (int i = 0; i < count; ++i) {
        if (selection[static_cast<std::size_t>(i)] < 0 || selection[static_cast<std::size_t>(i)] >= 6) return std::nullopt;
    }
    std::sort(selection.begin(), selection.begin() + count);
    for (std::size_t i = 0; i < recipes.size(); ++i) {
        const auto& candidate = recipes[i];
        if (candidate.count != count) continue;
        if (std::equal(selection.begin(), selection.begin() + count, candidate.ingredients.begin())) return i;
    }
    return std::nullopt;
}

std::string_view category_name(result_kind kind)
{
    switch (kind) {
        case result_kind::basic: return "BASE POTION";
        case result_kind::success: return "SUCCESSFUL BREW";
        case result_kind::danger: return "UNSTABLE BREW";
        case result_kind::failure: return "FAILED REACTION";
        case result_kind::secret: return "SECRET DISCOVERED";
    }
    return "UNKNOWN";
}

} // namespace alchemy
