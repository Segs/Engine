/*************************************************************************/
/*  audio_stream_player.h                                                */
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

#pragma once

#include "scene/main/node.h"
#include "servers/audio/audio_stream.h"

class GODOT_EXPORT AudioStreamPlayer : public Node {

    GDCLASS(AudioStreamPlayer,Node)

public:
    enum MixTarget : uint8_t {
        MIX_TARGET_STEREO,
        MIX_TARGET_SURROUND,
        MIX_TARGET_CENTER
    };

private:
    Ref<AudioStreamPlayback> stream_playback;
    Ref<AudioStream> stream;
    Vector<AudioFrame> mix_buffer;
    Vector<AudioFrame> fadeout_buffer;

    SafeNumeric<float> setseek;
    SafeFlag active;
    SafeFlag setstop;
    SafeFlag stop_has_priority;
    StringName bus;

    float mix_volume_db;
    float pitch_scale;
    float volume_db;
    MixTarget mix_target;
    bool use_fadeout=false;
    bool autoplay;
    bool stream_paused;
    bool stream_paused_fade;

    void _mix_internal(bool p_fadeout);
    void _mix_audio();
    static void _mix_audios(void *self) { reinterpret_cast<AudioStreamPlayer *>(self)->_mix_audio(); }
public:
    void _set_playing(bool p_enable);
    bool _is_active() const;

    void _bus_layout_changed();
    void _mix_to_bus(const AudioFrame *p_frames, int p_amount);

protected:
    void _validate_property(PropertyInfo &property) const override;
    void _notification(int p_what);
    static void _bind_methods();

public:
    void set_stream(Ref<AudioStream> p_stream);
    Ref<AudioStream> get_stream() const;

    void set_volume_db(float p_volume);
    float get_volume_db() const;

    void set_pitch_scale(float p_pitch_scale);
    float get_pitch_scale() const;

    void play(float p_from_pos = 0.0);
    void seek(float p_seconds);
    void stop();
    bool is_playing() const;
    float get_playback_position();

    void set_bus(const StringName &p_bus);
    StringName get_bus() const;

    void set_autoplay(bool p_enable);
    bool is_autoplay_enabled();

    void set_mix_target(MixTarget p_target);
    MixTarget get_mix_target() const;

    void set_stream_paused(bool p_pause);
    bool get_stream_paused() const;

    Ref<AudioStreamPlayback> get_stream_playback();

    AudioStreamPlayer();
    ~AudioStreamPlayer() override;
};


