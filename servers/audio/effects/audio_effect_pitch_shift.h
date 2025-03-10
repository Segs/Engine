/*************************************************************************/
/*  audio_effect_pitch_shift.h                                           */
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

#include "servers/audio/audio_effect.h"
#include "core/method_enum_caster.h"

class SMBPitchShift {

    enum {
        MAX_FRAME_LENGTH = 8192
    };

    float gInFIFO[MAX_FRAME_LENGTH];
    float gOutFIFO[MAX_FRAME_LENGTH];
    float gFFTworksp[2 * MAX_FRAME_LENGTH];
    float gLastPhase[MAX_FRAME_LENGTH / 2 + 1];
    float gSumPhase[MAX_FRAME_LENGTH / 2 + 1];
    float gOutputAccum[2 * MAX_FRAME_LENGTH];
    float gAnaFreq[MAX_FRAME_LENGTH];
    float gAnaMagn[MAX_FRAME_LENGTH];
    float gSynFreq[MAX_FRAME_LENGTH];
    float gSynMagn[MAX_FRAME_LENGTH];
    long gRover;

    void smbFft(float *fftBuffer, long fftFrameSize, long sign);

public:
    void PitchShift(float pitchShift, long numSampsToProcess, long fftFrameSize, long osamp, float sampleRate, float *indata, float *outdata, int stride);

    SMBPitchShift() {
        gRover = 0;
        memset(gInFIFO, 0, MAX_FRAME_LENGTH * sizeof(float));
        memset(gOutFIFO, 0, MAX_FRAME_LENGTH * sizeof(float));
        memset(gFFTworksp, 0, 2 * MAX_FRAME_LENGTH * sizeof(float));
        memset(gLastPhase, 0, (MAX_FRAME_LENGTH / 2 + 1) * sizeof(float));
        memset(gSumPhase, 0, (MAX_FRAME_LENGTH / 2 + 1) * sizeof(float));
        memset(gOutputAccum, 0, 2 * MAX_FRAME_LENGTH * sizeof(float));
        memset(gAnaFreq, 0, MAX_FRAME_LENGTH * sizeof(float));
        memset(gAnaMagn, 0, MAX_FRAME_LENGTH * sizeof(float));
    }
};

class AudioEffectPitchShift;

class AudioEffectPitchShiftInstance : public AudioEffectInstance {
    GDCLASS(AudioEffectPitchShiftInstance,AudioEffectInstance)
    friend class AudioEffectPitchShift;
    Ref<AudioEffectPitchShift> base;

    int fft_size;
    SMBPitchShift shift_l;
    SMBPitchShift shift_r;

public:
    void process(const AudioFrame *p_src_frames, AudioFrame *p_dst_frames, int p_frame_count) override;
};

class GODOT_EXPORT AudioEffectPitchShift : public AudioEffect {
    GDCLASS(AudioEffectPitchShift,AudioEffect)

public:
    friend class AudioEffectPitchShiftInstance;

    enum FFT_Size {
        FFT_SIZE_256,
        FFT_SIZE_512,
        FFT_SIZE_1024,
        FFT_SIZE_2048,
        FFT_SIZE_4096,
        FFT_SIZE_MAX
    };

    float pitch_scale;
    int oversampling;
    FFT_Size fft_size;
    float wet = 0.0f;
    float dry = 0.0f;
    bool filter = false;

protected:
    static void _bind_methods();

public:
    Ref<AudioEffectInstance> instance() override;

    void set_pitch_scale(float p_pitch_scale);
    float get_pitch_scale() const;

    void set_oversampling(int p_oversampling);
    int get_oversampling() const;

    void set_fft_size(FFT_Size);
    FFT_Size get_fft_size() const;

    AudioEffectPitchShift();
};

