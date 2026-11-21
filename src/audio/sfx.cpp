#include "sfx.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

#include <core/log.hpp>

using namespace audio;
using V = math::vec3_t<gl::f32>;

sfx_t::sfx_t(ma_device_t& device) :
    device(device)
{
    std::lock_guard<std::mutex> guard(device.lock);
    device.extra_mix = [this](gl::f32 *out, ma_uint32 frames) { mix(out, frames); };
}

sfx_t::~sfx_t()
{
    std::lock_guard<std::mutex> guard(device.lock); // the audio thread is not inside `mix` after this
    device.extra_mix = nullptr;
}

bool sfx_t::load(const std::string& name, const char *path, gl::f32 volume)
{
    if (samples.contains(name)) return false;

    drmp3_config config{}; // zeros: as it is in the file
    drmp3_uint64 frames = 0;
    gl::f32 *data = drmp3_open_file_and_read_pcm_frames_f32(path, &config, &frames, nullptr);
    if (data == nullptr || config.channels == 0 || config.sampleRate == 0) {
        errorfn("[AUDIO] Couldn't decode {}", path);
        return false;
    }

    // mono at the rate of the device
    std::vector<gl::f32> mono((size_t)frames);
    for (size_t i = 0; i < mono.size(); ++i) {
        gl::f32 sum = 0.0f;
        for (drmp3_uint32 c = 0; c < config.channels; ++c) sum += data[i * config.channels + c];
        mono[i] = sum / (gl::f32)config.channels;
    }
    drmp3_free(data, nullptr);

    sample_t sample;
    sample.volume = volume;
    const double ratio = (double)config.sampleRate / (double)device.device.sampleRate;
    sample.pcm.resize((size_t)((double)mono.size() / ratio));
    for (size_t i = 0; i < sample.pcm.size(); ++i) {
        const double src = (double)i * ratio;
        const size_t a = (size_t)src;
        const size_t b = std::min(a + 1, mono.size() - 1);
        const gl::f32 t = (gl::f32)(src - (double)a);
        sample.pcm[i] = mono[a] + (mono[b] - mono[a]) * t;
    }

    std::lock_guard<std::mutex> guard(device.lock);
    samples.emplace(name, std::move(sample)); // map nodes don't move, the voices point into them
    return true;
}

void sfx_t::set_listener(V pos, V right)
{
    std::lock_guard<std::mutex> guard(device.lock);
    listener_pos   = pos;
    listener_right = right;
}

sfx_t::voice_t *sfx_t::find(const std::string& name, const std::string& owner)
{
    for (voice_t& v : voices) {
        if (!v.done && v.name == name && v.owner == owner) return &v;
    }
    return nullptr;
}

sfx_t::voice_t& sfx_t::add_voice(const sample_t& sample, const std::string& name, const emit_t& emit)
{
    // too many voices: the oldest of this sound goes first, then the oldest of everything that is not a loop
    const auto drop_oldest = [this](auto pred) {
        voice_t *oldest = nullptr;
        for (voice_t& v : voices) {
            if (!v.done && pred(v) && (oldest == nullptr || v.born < oldest->born)) oldest = &v;
        }
        if (oldest != nullptr) oldest->done = true;
    };
    size_t same = 0, live = 0;
    for (const voice_t& v : voices) {
        if (v.done) continue;
        ++live;
        if (v.name == name) ++same;
    }
    if (same >= MAX_PER_NAME) drop_oldest([&](const voice_t& v) { return v.name == name && !v.looping; });
    if (live >= MAX_VOICES)   drop_oldest([](const voice_t& v) { return !v.looping; });

    std::erase_if(voices, [](const voice_t& v) { return v.done; });
    voices.emplace_back();
    voice_t& v = voices.back();
    v.sample      = &sample;
    v.name        = name;
    v.owner       = emit.owner;
    v.positional  = emit.at.has_value();
    v.at          = emit.at.value_or(V());
    v.gain        = v.gain_target = emit.gain;
    v.born        = counter++;
    return v;
}

void sfx_t::play(const std::string& name, const emit_t& emit)
{
    const auto it = samples.find(name);
    if (it == samples.end()) return;

    if (emit.min_gap > 0.0f) {
        const std::string key = emit.owner + "|" + name;
        const gl::time_point now = gl::clock::now();
        const auto last = last_play.find(key);
        if (last != last_play.end() && gl::duration(now - last->second).count() < emit.min_gap) return;
        last_play[key] = now;
    }

    std::lock_guard<std::mutex> guard(device.lock);
    if (voice_t *own = find(name, emit.owner); own != nullptr && !own->looping && !emit.owner.empty()) {
        own->done = true; // the same owner again: starts over, nobody else is cut
    }
    add_voice(it->second, name, emit);
}

void sfx_t::loop(const std::string& name, const emit_t& emit, bool on)
{
    const auto it = samples.find(name);
    if (it == samples.end()) return;

    std::lock_guard<std::mutex> guard(device.lock);
    voice_t *v = find(name, emit.owner);
    if (v == nullptr) {
        if (!on) return;
        v = &add_voice(it->second, name, emit);
        v->looping = true;
        v->gain    = 0.0f; // fades in
    }
    v->positional  = emit.at.has_value();
    v->at          = emit.at.value_or(v->at);
    v->gain_target = on ? emit.gain : 0.0f;
}

void sfx_t::mix(gl::f32 *out, ma_uint32 frames)
{
    const ma_uint32 channels = device.device.playback.channels;
    const gl::f32 dt = (gl::f32)frames / (gl::f32)device.device.sampleRate;

    for (voice_t& v : voices) {
        if (v.done) continue;
        // smooth gain: no clicks when a loop is turned down or off
        v.gain += (v.gain_target - v.gain) * std::min(1.0f, dt * 10.0f);
        if (v.looping && v.gain_target == 0.0f && v.gain < 0.002f) { v.done = true; continue; }

        // distance and pan, once per block
        gl::f32 left = 1.0f, right = 1.0f;
        if (v.positional && channels == 2) {
            const V d = v.at - listener_pos;
            const gl::f32 dist = math::vec3::len(d);
            const gl::f32 att  = REF_DISTANCE / (REF_DISTANCE + std::max(0.0f, dist - REF_DISTANCE));
            gl::f32 pan = 0.0f; // -1 left, 1 right; right under the listener it is in the middle
            if (dist > 0.3f) pan = std::clamp(math::vec3::dot(d * (1.0f / dist), listener_right), -1.0f, 1.0f);
            const gl::f32 angle = (pan + 1.0f) * (std::numbers::pi_v<gl::f32> / 4.0f);
            left  = att * std::numbers::sqrt2_v<gl::f32> * std::cos(angle);
            right = att * std::numbers::sqrt2_v<gl::f32> * std::sin(angle);
        }

        const gl::f32 amp = v.sample->volume * v.gain;
        const std::vector<gl::f32>& pcm = v.sample->pcm;
        for (ma_uint32 f = 0; f < frames; ++f) {
            if (v.pos >= pcm.size()) {
                if (!v.looping) { v.done = true; break; }
                v.pos = 0;
            }
            const gl::f32 s = pcm[v.pos++] * amp;
            if (channels == 2) {
                out[f * 2]     += s * left;
                out[f * 2 + 1] += s * right;
            } else {
                for (ma_uint32 c = 0; c < channels; ++c) out[f * channels + c] += s;
            }
        }
    }
    std::erase_if(voices, [](const voice_t& v) { return v.done; });
}
