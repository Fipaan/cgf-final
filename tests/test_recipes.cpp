#include <algorithm>
#include <array>
#include <cassert>
#include <set>
#include <string>
#include "alchemy_recipes.hpp"

int main() {
    using namespace alchemy;
    assert(recipes.size() == 24);
    std::set<std::string> unique;
    int basics = 0;
    int success = 0;
    int danger = 0;
    int failure = 0;
    int secret = 0;
    for (std::size_t i = 0; i < recipes.size(); ++i) {
        const auto& r = recipes[i];
        assert(unique.insert(std::string(r.id)).second);
        assert(find_recipe(r.ingredients, r.count) == i);
        switch (r.kind) {
            case result_kind::basic: ++basics; break;
            case result_kind::success: ++success; break;
            case result_kind::danger: ++danger; break;
            case result_kind::failure: ++failure; break;
            case result_kind::secret: ++secret; break;
        }
    }
    assert(basics == 6 && success == 9 && danger == 3 && failure == 3 && secret == 3);
    assert(find_recipe({4, 2, 0}, 3) == 22);
    assert(find_recipe({5, 5, -1}, 2) == std::nullopt);
    assert(find_recipe({0, 2, 2}, 3) == std::nullopt);
    assert(find_recipe({0, 9, -1}, 2) == std::nullopt);
}
