/*
 * audio/sfx.hpp -
 *     Sound effects as voices: every `play` starts its own voice, so sounds stack instead of cutting each other
 *     off. A voice belongs to an owner (the object that makes the sound) and may have a place in the world: it
 *     gets quieter with the distance to the listener and is panned left and right by where it is. Everything goes
 *     through a soft limiter, so many voices at once get dense instead of clipping.
 *
 *     Files are decoded once at `load` (mono, at the rate of the device), voices only keep a position in them.
 */
#pragma once

#include <map>
#include <optional>
#include <string>
#include <vector>

#include <3d/vec.hpp>
#include <audio/audio.hpp>

namespace audio {

// who plays a sound and where
struct emit_t {
    // one voice per (owner, sound): the same owner playing it again restarts its own voice, other owners are not
    // touched. Empty = nobody in particular
    std::string owner;
    // nullopt = at the listener (steps, interface sounds): no distance, no pan
    std::optional<math::vec3_t<gl::f32>> at;
    gl::f32 gain    = 1.0f;
    gl::f32 min_gap = 0.0f; // seconds: ignored if this owner played this sound more recently (a resting contact rattles)
};

/* Usage:
```cpp
sfx_t sfx(playback_device);
sfx.load("splash", "assets/sound/splash.mp3", 0.45f);

// every frame
sfx.set_listener(camera.pos, camera.right());

sfx.play("splash", {.owner = "cauldron", .at = bowl_position});
sfx.play("chime");                                         // not in the world
sfx.loop("bubble", {.owner = "cauldron", .at = bowl_position, .gain = 0.5f}, true); // call every frame while it runs
```
*/
struct sfx_t {
    static constexpr size_t MAX_VOICES   = 24;
    static constexpr size_t MAX_PER_NAME = 5;      // same sound from different owners
    static constexpr gl::f32 REF_DISTANCE = 1.5f;  // up to here a sound is at full volume

    struct sample_t {
        std::vector<gl::f32> pcm; // mono
        gl::f32 volume = 1.0f;
    };

    struct voice_t {
        const sample_t *sample = nullptr;
        std::string name, owner;
        size_t pos = 0;
        bool looping = false;
        bool alive_requested = true; // loops: refreshed by `loop(..., true)`, fades out when it stops
        bool positional = false;
        math::vec3_t<gl::f32> at;
        gl::f32 gain = 1.0f, gain_target = 1.0f; // the gain follows its target smoothly
        bool done = false;
        unsigned long long born = 0;
    };

    ma_device_t& device;
    std::map<std::string, sample_t> samples;
    std::vector<voice_t> voices;
    std::map<std::string, gl::time_point> last_play;
    unsigned long long counter = 0;

    math::vec3_t<gl::f32> listener_pos, listener_right = math::vec3_t<gl::f32>(1.0f, 0.0f, 0.0f);

    sfx_t(ma_device_t& device);
    ~sfx_t();
    sfx_t(const sfx_t&) = delete;
    sfx_t& operator=(const sfx_t&) = delete;

    bool load(const std::string& name, const char *path, gl::f32 volume);
    void set_listener(math::vec3_t<gl::f32> pos, math::vec3_t<gl::f32> right);

    void play(const std::string& name, const emit_t& emit = {});
    // a looping voice for (owner, name): call it every frame, with `on` true it is kept (its place and gain follow
    // `emit`), with false it fades out. The gain changes smoothly
    void loop(const std::string& name, const emit_t& emit, bool on);

    // the audio thread, from `ma_device_t::callback` (the device lock is held)
    void mix(gl::f32 *out, ma_uint32 frames);

private:
    voice_t *find(const std::string& name, const std::string& owner);
    voice_t& add_voice(const sample_t& sample, const std::string& name, const emit_t& emit);
};

} // namespace audio
