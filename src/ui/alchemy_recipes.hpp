#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <optional>
#include <string_view>

namespace alchemy {

enum class result_kind { basic, success, danger, failure, secret };

struct recipe_t {
    std::string_view id;
    std::string_view name;
    std::string_view description;
    result_kind kind;
    std::array<int, 3> ingredients;
    int count;
};

inline constexpr std::array<std::string_view, 6> ingredients = {
    "Growth", "Shrink", "Speed", "Gravity", "Explosion", "Restore"
};

inline constexpr std::array<std::array<float, 3>, 6> ingredient_colors = {{
    {0.94f, 0.33f, 0.34f},
    {0.35f, 0.63f, 0.97f},
    {0.99f, 0.78f, 0.31f},
    {0.72f, 0.46f, 0.93f},
    {1.00f, 0.54f, 0.26f},
    {0.33f, 0.82f, 0.62f}
}};

inline constexpr std::array<recipe_t, 24> recipes = {{
    {"growth", "Growth Potion", "Player or object becomes 2x larger.", result_kind::basic, {0, -1, -1}, 1},
    {"shrink", "Shrink Potion", "Player or object shrinks to 50%.", result_kind::basic, {1, -1, -1}, 1},
    {"speed", "Speed Potion", "Player movement speed becomes 2x.", result_kind::basic, {2, -1, -1}, 1},
    {"gravity", "Gravity Potion", "Loose objects float in the room.", result_kind::basic, {3, -1, -1}, 1},
    {"explosion", "Explosion Potion", "Small blast scatters nearby objects.", result_kind::basic, {4, -1, -1}, 1},
    {"restore", "Restoration Potion", "Restore sizes, speed and object positions.", result_kind::basic, {5, -1, -1}, 1},
    {"unstable_size", "Unstable Size", "Alternate sizes every 3s for 15s.", result_kind::danger, {0, 1, -1}, 2},
    {"giant_run", "Giant Sprint", "Player size 2x and movement speed 1.5x.", result_kind::success, {0, 2, -1}, 2},
    {"giant_room", "Giant Room", "Loose objects become 3x and float.", result_kind::success, {0, 3, -1}, 2},
    {"mega_explosion", "Mega Explosion", "Powerful shockwave enlarges and scatters objects.", result_kind::success, {0, 4, -1}, 2},
    {"neutralization_growth", "Neutralization I", "Effects cancel out. No potion is produced.", result_kind::failure, {0, 5, -1}, 2},
    {"tiny_ninja", "Tiny Ninja", "Player size 25% and movement speed 3x.", result_kind::success, {1, 2, -1}, 2},
    {"micro_world", "Micro World", "Room objects shrink to 20% and float.", result_kind::success, {1, 3, -1}, 2},
    {"mini_bombs", "Mini Bombs", "Five small blasts appear around the room.", result_kind::success, {1, 4, -1}, 2},
    {"neutralization_shrink", "Neutralization II", "Ingredients cancel out. Nothing happens.", result_kind::failure, {1, 5, -1}, 2},
    {"super_jump", "Super Jump", "Player speed 2x and jumping height 4x.", result_kind::success, {2, 3, -1}, 2},
    {"chaotic_acceleration", "Chaotic Acceleration", "Uncontrolled bouncing and speed for 10s.", result_kind::danger, {2, 4, -1}, 2},
    {"reverse_acceleration", "Reverse Acceleration", "Player speed is 30% for 15s.", result_kind::danger, {2, 5, -1}, 2},
    {"anti_gravity_wave", "Anti-Gravity Wave", "Objects fly up to the ceiling, then fall.", result_kind::success, {3, 4, -1}, 2},
    {"neutralization_gravity", "Neutralization III", "Mixture dissolves with no effect.", result_kind::failure, {3, 5, -1}, 2},
    {"reverse_explosion", "Reverse Explosion", "Scattered objects return to the cauldron.", result_kind::success, {4, 5, -1}, 2},
    {"room_chaos", "Room Chaos", "Object sizes and positions change for 20s.", result_kind::secret, {0, 1, 3}, 3},
    {"apocalypse", "Apocalypse", "Huge explosion, enlarged objects and camera shake.", result_kind::secret, {0, 2, 4}, 3},
    {"time_rewind", "Time Rewind", "Restore objects and teleport player to spawn.", result_kind::secret, {1, 3, 5}, 3}
}};

inline std::optional<std::size_t> find_recipe(std::array<int, 3> selection, int count) {
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

inline std::string_view category_name(result_kind kind) {
    switch (kind) {
        case result_kind::basic: return "BASE POTION";
        case result_kind::success: return "SUCCESSFUL BREW";
        case result_kind::danger: return "UNSTABLE BREW";
        case result_kind::failure: return "FAILED REACTION";
        case result_kind::secret: return "SECRET DISCOVERED";
    }
    return "UNKNOWN";
}

}
