/*************************************************************************/
/*  audio_driver_wasapi.h                                                */
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

#ifdef WASAPI_ENABLED

#include "core/os/mutex.h"
#include "core/os/thread.h"
#include "servers/audio_server.h"

#include <audioclient.h>
#include <mmdeviceapi.h>
#include <windows.h>

class AudioDriverWASAPI : public AudioDriver {

    class AudioDeviceWASAPI {
    public:
        IAudioClient *audio_client = nullptr;
        IAudioRenderClient *render_client = nullptr;
        IAudioCaptureClient *capture_client = nullptr;
        bool active = false;

        WORD format_tag=0;
        WORD bits_per_sample=0;
        unsigned int channels=0;
        unsigned int frame_size=0;

        String device_name;
        String new_device;

        AudioDeviceWASAPI() :
                device_name("Default"),
                new_device("Default") {
        }
    };

    AudioDeviceWASAPI audio_input;
    AudioDeviceWASAPI audio_output;

    Mutex *mutex;
    Thread *thread;

    Vector<int32_t> samples_in;

    unsigned int channels;
    int mix_rate;
    int buffer_frames;

    bool thread_exited;
    mutable bool exit_thread;

    static _FORCE_INLINE_ void write_sample(WORD format_tag, int bits_per_sample, BYTE *buffer, int i, int32_t sample);
    static _FORCE_INLINE_ int32_t read_sample(WORD format_tag, int bits_per_sample, BYTE *buffer, int i);
    static void thread_func(void *p_udata);

    Error init_render_device(bool reinit = false);
    Error init_capture_device(bool reinit = false);

    Error finish_render_device();
    Error finish_capture_device();

    Error audio_device_init(AudioDeviceWASAPI *p_device, bool p_capture, bool reinit);
    Error audio_device_finish(AudioDeviceWASAPI *p_device);
    Array audio_device_get_list(bool p_capture);

public:
    virtual const char *get_name() const {
        return "WASAPI";
    }

    Error init() override;
    void start() override;
    int get_mix_rate() const override;
    SpeakerMode get_speaker_mode() const override;
    Array get_device_list() override;
    StringView get_device() override;
    void set_device(StringView device) override;
    void lock() override;
    void unlock() override;
    void finish() override;

    Error capture_start() override;
    Error capture_stop() override;
    Array capture_get_device_list() override;
    void capture_set_device(StringView p_name) override;
    String capture_get_device() override;

    AudioDriverWASAPI();
};

#endif //WASAPI_ENABLED
