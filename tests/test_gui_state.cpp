#include <gl/gui_state.hpp>

#include <cassert>
#include <iostream>

int main()
{
    float at_30 = 0.0f, at_144 = 0.0f;
    for (int i = 0; i < 30; ++i) at_30 = gl::animate_towards(at_30, 1.0f, 1.0f / 30, 12.0f);
    for (int i = 0; i < 144; ++i) at_144 = gl::animate_towards(at_144, 1.0f, 1.0f / 144, 12.0f);
    assert(std::abs(at_30 - at_144) < 0.00001f);
    assert(gl::animate_towards(0.4f, 1.0f, 0.0f, 12) == 0.4f);
    assert(gl::animate_towards(0.4f, 1.0f, -1.0f, 12) == 0.4f);
    assert(gl::animate_towards(0.4f, 1.0f, 100.0f, 12) <= 1.0f);
    assert(gl::ease_out(-1) == 0 && gl::ease_out(2) == 1);

    gl::gui_navigation_t state;
    auto register_buttons = [&]() {
        state.button("start", true, false, false, false, false);
        state.button("previous", false, false, false, false, false);
        state.button("settings", true, false, false, false, false);
        state.end(false);
    };
    state.begin(1, 0, false);
    register_buttons();
    state.begin(1, 1, false);
    assert(state.keyboard && state.focused == "start");
    register_buttons();
    state.begin(1, 1, false);
    assert(state.focused == "settings");
    register_buttons();
    state.begin(1, 1, false);
    assert(state.focused == "start");
    register_buttons();
    state.begin(1, -1, false);
    assert(state.focused == "settings");
    assert(state.button("settings", true, false, false, false, true));
    assert(!state.button("settings", true, false, false, false, true));
    state.end(false);

    state.begin(2, 0, false);
    assert(state.focused.empty() && state.armed.empty() && !state.keyboard);
    assert(!state.button("toggle", true, true, true, false, false));
    state.end(false);
    state.begin(2, 0, false);
    assert(state.button("toggle", true, true, false, true, false));
    state.end(true);
    state.begin(2, 0, false);
    assert(!state.button("toggle", true, true, false, true, false));
    state.end(true);

    state.begin(2, 0, false);
    state.button("toggle", true, true, true, false, false);
    state.end(false);
    state.begin(2, 0, true);
    assert(!state.button("toggle", true, false, false, true, false));
    state.end(true);
    assert(state.armed.empty());
    state.begin(2, 0, false);
    assert(!state.button("toggle", true, true, false, true, false));
    state.end(true);

    state.begin(2, 0, false);
    state.button("toggle", true, true, true, false, false);
    state.end(false);
    state.begin(3, 0, false);
    assert(!state.button("toggle", true, true, false, true, false));
    state.end(true);

    state.begin(3, 0, false);
    state.button("toggle", true, true, true, false, false);
    state.end(false);
    state.begin(3, 0, false);
    assert(!state.button("toggle", false, true, false, true, true));
    state.end(true);
    assert(state.focused.empty() && state.armed.empty());

    std::cout << "PASS: timing, keyboard traversal, disabled buttons, click cancellation, screen isolation\n";
}
