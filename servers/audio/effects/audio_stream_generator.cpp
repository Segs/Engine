/*************************************************************************/
/*  audio_stream_generator.cpp                                           */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2019 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2019 Godot Engine contributors (cf. AUTHORS.md).   */
/*                                                                       */
/* Permission is hereby granted, free of charge, to any person obtaining */
/* a copy of this software and associated documentation files (the       */
/* "Software"), to deal in the Software without restriction, including   */
/* without limitation the rights to use, copy, modify, merge, publish,   */
/* distribute, sublicense, and/or sell copies of the Software, and to    */
/* permit persons to whom the Software is furnished to do so, subject to */
/* the following conditions:                                             */
/*                                                                       */
/* The above copyright notice and this permission notice shall be        */
/* included in all copies or substantial portions of the Software.       */
/*                                                                       */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */
/*************************************************************************/

#include "audio_stream_generator.h"
#include "core/method_bind.h"

IMPL_GDCLASS(AudioStreamGenerator)
IMPL_GDCLASS(AudioStreamGeneratorPlayback)

void AudioStreamGenerator::set_mix_rate(float p_mix_rate) {
    mix_rate = p_mix_rate;
}

float AudioStreamGenerator::get_mix_rate() const {

    return mix_rate;
}

void AudioStreamGenerator::set_buffer_length(float p_seconds) {

    buffer_len = p_seconds;
}
float AudioStreamGenerator::get_buffer_length() const {

    return buffer_len;
}

Ref<AudioStreamPlayback> AudioStreamGenerator::instance_playback() {

    Ref<AudioStreamGeneratorPlayback> playback(make_ref_counted<AudioStreamGeneratorPlayback>());
    playback->generator = this;
    int target_buffer_size = mix_rate * buffer_len;
    playback->buffer.resize(nearest_shift(target_buffer_size));
    playback->buffer.clear();
    return playback;
}
String AudioStreamGenerator::get_stream_name() const {

    return "UserFeed";
}

float AudioStreamGenerator::get_length() const {
    return 0;
}

void AudioStreamGenerator::_bind_methods() {
    SE_BIND_METHOD(AudioStreamGenerator,set_mix_rate);
    SE_BIND_METHOD(AudioStreamGenerator,get_mix_rate);

    SE_BIND_METHOD(AudioStreamGenerator,set_buffer_length);
    SE_BIND_METHOD(AudioStreamGenerator,get_buffer_length);

    ADD_PROPERTY(PropertyInfo(VariantType::FLOAT, "mix_rate", PropertyHint::Range, "20,192000,1"), "set_mix_rate", "get_mix_rate");
    ADD_PROPERTY(PropertyInfo(VariantType::FLOAT, "buffer_length", PropertyHint::Range, "0.01,10,0.01"), "set_buffer_length", "get_buffer_length");
}

AudioStreamGenerator::AudioStreamGenerator() {
    mix_rate = 44100;
    buffer_len = 0.5;
}

////////////////

bool AudioStreamGeneratorPlayback::push_frame(const Vector2 &p_frame) {
    if (buffer.space_left() < 1) {
        return false;
    }

    AudioFrame f = p_frame;

    buffer.write(&f, 1);
    return true;
}

bool AudioStreamGeneratorPlayback::can_push_buffer(int p_frames) const {
    return buffer.space_left() >= p_frames;
}
bool AudioStreamGeneratorPlayback::push_buffer(const PoolVector2Array &p_frames) {

    int to_write = p_frames.size();
    if (buffer.space_left() < to_write) {
        return false;
    }

    PoolVector2Array::Read r = p_frames.read();
    if (sizeof(real_t) == 4) {
        //write directly
        buffer.write((const AudioFrame *)r.ptr(), to_write);
    } else {
        //convert from double
        AudioFrame buf[2048];
        int ofs = 0;
        while (to_write) {

            int w = MIN(to_write, 2048);
            for (int i = 0; i < w; i++) {
                buf[i] = r[i + ofs];
            }
            buffer.write(buf, w);
            ofs += w;
            to_write -= w;
        }
    }
    return true;
}

int AudioStreamGeneratorPlayback::get_frames_available() const {
    return buffer.space_left();
}

int AudioStreamGeneratorPlayback::get_skips() const {
    return skips;
}

void AudioStreamGeneratorPlayback::clear_buffer() {
    ERR_FAIL_COND(active);
    buffer.clear();
    mixed = 0;
}

void AudioStreamGeneratorPlayback::_mix_internal(AudioFrame *p_buffer, int p_frames) {

    int read_amount = buffer.data_left();
    if (p_frames < read_amount) {
        read_amount = p_frames;
    }

    buffer.read(p_buffer, read_amount);

    if (read_amount < p_frames) {
        //skipped, not ideal
        for (int i = read_amount; i < p_frames; i++) {
            p_buffer[i] = AudioFrame(0, 0);
        }

        skips++;
    }

    mixed += p_frames / generator->get_mix_rate();
}
float AudioStreamGeneratorPlayback::get_stream_sampling_rate() {
    return generator->get_mix_rate();
}

void AudioStreamGeneratorPlayback::start(float p_from_pos) {

    if (mixed == 0.0f) {
        _begin_resample();
    }
    skips = 0;
    active = true;
    mixed = 0.0;
}

void AudioStreamGeneratorPlayback::stop() {
    active = false;
}
bool AudioStreamGeneratorPlayback::is_playing() const {

    return active; //always playing, can't be stopped
}

int AudioStreamGeneratorPlayback::get_loop_count() const {
    return 0;
}

float AudioStreamGeneratorPlayback::get_playback_position() const {
    return mixed;
}
void AudioStreamGeneratorPlayback::seek(float p_time) {
    //no seek possible
}

void AudioStreamGeneratorPlayback::_bind_methods() {
    SE_BIND_METHOD(AudioStreamGeneratorPlayback,push_frame);
    SE_BIND_METHOD(AudioStreamGeneratorPlayback,can_push_buffer);
    SE_BIND_METHOD(AudioStreamGeneratorPlayback,push_buffer);
    SE_BIND_METHOD(AudioStreamGeneratorPlayback,get_frames_available);
    SE_BIND_METHOD(AudioStreamGeneratorPlayback,get_skips);
    SE_BIND_METHOD(AudioStreamGeneratorPlayback,clear_buffer);
}

AudioStreamGeneratorPlayback::AudioStreamGeneratorPlayback() {
    generator = nullptr;
    skips = 0;
    active = false;
    mixed = 0;
}
