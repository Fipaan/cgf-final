#include "audio.hpp"

#include <core/log.hpp>
#include <cassert>

using namespace audio;

ma_device_t::ma_device_t()
{}

ma_device_t::~ma_device_t()
{
    if (initialized) {
        ma_device_uninit(&device);
        initialized = false;
    }
}

bool ma_device_t::init(ma_uint32 channels, ma_uint32 sample_rate, ma_format format)
{
    assert(!initialized);

    ma_device_config config = ma_device_config_init(ma_device_type_playback);
    config.playback.format   = format;
    config.playback.channels = channels;
    config.sampleRate        = sample_rate;
    config.dataCallback      = ma_device_t::callback_raw;
    config.pUserData         = this;

    initialized = ma_device_init(NULL, &config, &device) == MA_SUCCESS;
    if (initialized) infofn("[AUDIO] Successfully initialized playback device!");
    else errorfn("[AUDIO] Failed to initialize playback device.");

    out_buffer = (float*)malloc(sample_rate * sizeof(*out_buffer));
    assert(out_buffer != nullptr);
    out_buffer_size = sample_rate;

    return initialized;
}

bool ma_device_t::start()
{
    ma_result result = ma_device_start(&device);

    bool ok = result == MA_SUCCESS;
    if (ok) {
        infofn("[AUDIO] Successfully started playback device!");
    } else {
        const char *desc = ma_result_description(result);
        errorfn("[AUDIO] Couldn't start playback device: {}", desc);
    }

    return ok;
}

void ma_device_t::add(sound_t& sound)
{
    sounds.push_back(&sound);
}

bool ma_device_t::load(sound_t& sound, std::string_view path, sound_type_t type)
{
    if (!sound.load(path, type, device.playback.channels, device.sampleRate)) return false;
    add(sound);
    return true;
}

void ma_device_t::callback(gl::f32* out, ma_uint32 frame_count)
{
    assert(initialized);
    assert(device.playback.format == ma_format_f32);

    for (auto sound_ptr : sounds) {
        auto& sound = *sound_ptr;

        ma_uint64 frames_read;
        if (!sound.read_pcm_frames_f32(frame_count, out_buffer, frames_read)) continue;

        ma_uint64 samples_read = frames_read * device.playback.channels;
        for (ma_uint64 sample = 0; sample < samples_read; ++sample) {
            out[sample] += out_buffer[sample] * sound.volume;
        }
    }
}

void ma_device_t::callback_raw(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{
    ma_device_t& device = *(ma_device_t*)pDevice->pUserData;
    assert(&device.device == pDevice);

    (void)pInput;
    device.callback((gl::f32*)pOutput, frameCount);
}

sound_t::sound_t(gl::f32 volume)
{
    this->volume = volume;
}
sound_t::~sound_t()
{
    if (initialized) {
        ma_data_converter_uninit(&converter, NULL);
        initialized = false;
    }
}

bool sound_t::load(std::string_view path, sound_type_t type_, ma_uint32 channels_, ma_uint32 sample_rate_)
{
    assert(!initialized);
    if (type_ == SOUND_NONE) {
        if (path.ends_with(".mp3")) type = SOUND_MP3;
        else {
            errorfn("[AUDIO] Unknown format of \"{}\"", path);
            return false;
        }
    } else type = type_;

    switch (type) {
    case SOUND_MP3:
        if (!drmp3_init_file(&as.mp3, path.data(), NULL)) {
            errorfn("[AUDIO] Failed to open mp3 file \"{}\"", path);
            return false;
        }
        if (!end()) return false;
        break;
    case SOUND_NONE:
    default:         assert(false && "Unreachable");
    }

    infofn("[AUDIO] Successfully opened \"{}\"", path);

    if (channels_    == 0) channels_    = channels();
    if (sample_rate_ == 0) sample_rate_ = sample_rate();

    ma_data_converter_config converter_config = ma_data_converter_config_init(
        ma_format_f32,
        ma_format_f32,
        channels(),
        channels_,
        sample_rate(),
        sample_rate_
    );

    ma_result result = ma_data_converter_init(&converter_config, NULL, &converter);

    initialized = result == MA_SUCCESS;
    if (!initialized) {
        const char *desc = ma_result_description(result);
        errorfn("[AUDIO] Couldn't initialize converter: {}", desc);
    }

    return initialized;
}
        
uint32_t sound_t::channels()
{
    switch (type) {
    case SOUND_MP3:  return as.mp3.channels;
    case SOUND_NONE:
    default:         assert(false && "Invalid sound type");
    }
    assert(false && "Unreachable");
}

uint32_t sound_t::sample_rate()
{
    switch (type) {
    case SOUND_MP3:  return as.mp3.sampleRate;
    case SOUND_NONE:
    default:         assert(false && "Invalid sound type");
    }
    assert(false && "Unreachable");
}
        
bool sound_t::read_pcm_frames_f32(ma_uint64 frames_to_read, float *out, ma_uint64& frames_read)
{
    ma_result result;
    // frames_read = frames_to_read;
    ma_uint64 input_frames;
    result = ma_data_converter_get_required_input_frame_count(
        &converter,
        frames_to_read,
        &input_frames
    );
    if (result != MA_SUCCESS) {
        const char *desc = ma_result_description(result);
        errorfn("[AUDIO] Failed to get required input frame count: {}", desc);
        return false;
    }
    if (input_frames == 0) return true;
    std::vector<float> input(
        input_frames * channels()
    );
    ma_uint64 input_frames_read;
    frames_read = frames_to_read;

    switch (type) {
    case SOUND_MP3:
        input_frames_read = drmp3_read_pcm_frames_f32(
            &as.mp3,
            input_frames,
            input.data()
        );
        break;
    case SOUND_NONE:
    default:         assert(false && "Invalid sound type");
    }
    result = ma_data_converter_process_pcm_frames(
        &converter,
        input.data(),
        &input_frames_read,
        out,
        &frames_read
    );
    bool ok = result == MA_SUCCESS;
    if (!ok) {
        assert(false && "TODO: proper error message");
    }
    return ok;
}

bool sound_t::seek_to_pcm_frame(ma_uint64 frame_index)
{
    switch (type) {
    case SOUND_MP3:  return drmp3_seek_to_pcm_frame(&as.mp3, frame_index);
    case SOUND_NONE:
    default:         assert(false && "Invalid sound type");
    }
    assert(false && "Unreachable");
}

ma_uint64 sound_t::get_pcm_frame_count()
{
    switch (type) {
    case SOUND_MP3:  return drmp3_get_pcm_frame_count(&as.mp3);
    case SOUND_NONE:
    default:         assert(false && "Invalid sound type");
    }
    assert(false && "Unreachable");
}

bool sound_t::reset()
{
    return seek_to_pcm_frame(0);
}

bool sound_t::end()
{
    return seek_to_pcm_frame(get_pcm_frame_count());
}