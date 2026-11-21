/*
 * audio/audio.hpp -
 *     Convenient abstraction on top of audio API of dr_* libs and miniaudio
 */
#pragma once

#include <core/audio/miniaudio.hpp>
#include <core/audio/dr.hpp>
#include <core/gl/first.hpp>
#include <vector>

namespace audio {

enum sound_type_t {
    SOUND_NONE,
    SOUND_MP3,
};

struct sound_t;

struct ma_device_t {
    bool initialized = false;
    ma_device device;
    std::vector<sound_t*> sounds;
    
    float *out_buffer;
    float out_buffer_size;

    ma_device_t();
    ~ma_device_t();
    bool init(ma_uint32 channels, ma_uint32 sample_rate, ma_format format = ma_format_f32);
    bool start();

    void add(sound_t& sound);
    bool load(sound_t& sound, std::string_view path, sound_type_t type = SOUND_NONE);

    void callback(float *out, ma_uint32 frame_count);
    static void callback_raw(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount);
};

struct sound_t {
    bool initialized  = false;
    sound_type_t type = SOUND_NONE;
    union {
        drmp3 mp3;
    } as;
    // TODO: store converters in ma_device_t and use hash maps to access them
    ma_data_converter converter;
    gl::f32 volume;

    sound_t(gl::f32 volume = 1.0f);
    ~sound_t();
    bool load(std::string_view path, sound_type_t type = SOUND_NONE, ma_uint32 channels = 0, ma_uint32 sample_rate = 0);

    uint32_t channels();
    uint32_t sample_rate();

    bool read_pcm_frames_f32(ma_uint64 frames_to_read, float *out, ma_uint64& frames_read);
    bool seek_to_pcm_frame(ma_uint64 frame_index);
    ma_uint64 get_pcm_frame_count();
    bool reset();
    bool end();
};

} // namespace audio