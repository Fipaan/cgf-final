#pragma once

#include <algorithm>
#include <cmath>
#include <string>
#include <string_view>
#include <vector>

namespace gl {
    inline float animate_towards(float value, float target, float seconds, float speed)
    {
        return value + (target - value) * -std::expm1(-speed * std::max(seconds, 0.0f));
    }

    inline float ease_out(float progress)
    {
        const float t = 1.0f - std::clamp(progress, 0.0f, 1.0f);
        return 1.0f - t * t * t;
    }

    struct gui_navigation_t {
        int scope = -1;
        std::string focused, armed;
        std::vector<std::string> previous, current;
        bool keyboard = false;
        bool consumed = false;

        void begin(int next_scope, int direction, bool pointer_moved)
        {
            if (scope != next_scope) {
                scope = next_scope;
                focused.clear();
                armed.clear();
                previous.clear();
                keyboard = false;
            }
            current.clear();
            consumed = false;
            if (pointer_moved) keyboard = false;
            if (direction != 0 && !previous.empty()) {
                auto it = std::find(previous.begin(), previous.end(), focused);
                int index = it == previous.end() ? (direction > 0 ? -1 : 0) : int(it - previous.begin());
                index = (index + direction + int(previous.size())) % int(previous.size());
                focused = previous[static_cast<std::size_t>(index)];
                keyboard = true;
                armed.clear();
            }
        }

        bool button(std::string_view id, bool enabled, bool hover, bool pressed, bool released, bool activate)
        {
            const std::string key(id);
            if (!enabled) {
                if (armed == key) armed.clear();
                return false;
            }
            current.push_back(key);
            if (pressed && hover && !consumed) {
                armed = key;
                focused = key;
                keyboard = false;
            }
            const bool mouse_action = released && armed == key && hover;
            const bool key_action = activate && keyboard && focused == key;
            if ((mouse_action || key_action) && !consumed) {
                consumed = true;
                armed.clear();
                return true;
            }
            return false;
        }

        void end(bool released)
        {
            if (released) armed.clear();
            if (std::find(current.begin(), current.end(), focused) == current.end()) focused.clear();
            if (std::find(current.begin(), current.end(), armed) == current.end()) armed.clear();
            previous = current;
        }
    };
}
