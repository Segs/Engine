/*************************************************************************/
/*  audio_effect_limiter.cpp                                             */
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

#include "audio_effect_limiter.h"
#include "core/method_bind.h"

IMPL_GDCLASS(AudioEffectLimiterInstance)
IMPL_GDCLASS(AudioEffectLimiter)

void AudioEffectLimiterInstance::process(const AudioFrame *p_src_frames, AudioFrame *p_dst_frames, int p_frame_count) {

    float threshdb = base->threshold;
    float ceiling = Math::db2linear(base->ceiling);
    float ceildb = base->ceiling;
    float makeup = Math::db2linear(ceildb - threshdb);
    float sc = -base->soft_clip;
    float scv = Math::db2linear(sc);
    float peakdb = ceildb + 25;
    float scmult = Math::abs((ceildb - sc) / (peakdb - sc));

    for (int i = 0; i < p_frame_count; i++) {

        float spl0 = p_src_frames[i].l;
        float spl1 = p_src_frames[i].r;
        spl0 = spl0 * makeup;
        spl1 = spl1 * makeup;
        float sign0 = (spl0 < 0.0 ? -1.0 : 1.0);
        float sign1 = (spl1 < 0.0 ? -1.0 : 1.0);
        float abs0 = Math::abs(spl0);
        float abs1 = Math::abs(spl1);
        float overdb0 = Math::linear2db(abs0) - ceildb;
        float overdb1 = Math::linear2db(abs1) - ceildb;

        if (abs0 > scv) {
            spl0 = sign0 * (scv + Math::db2linear(overdb0 * scmult));
        }
        if (abs1 > scv) {
            spl1 = sign1 * (scv + Math::db2linear(overdb1 * scmult));
        }

        spl0 = MIN(ceiling, Math::abs(spl0)) * (spl0 < 0.0 ? -1.0 : 1.0);
        spl1 = MIN(ceiling, Math::abs(spl1)) * (spl1 < 0.0 ? -1.0 : 1.0);

        p_dst_frames[i].l = spl0;
        p_dst_frames[i].r = spl1;
    }
}

Ref<AudioEffectInstance> AudioEffectLimiter::instance() {
    Ref<AudioEffectLimiterInstance> ins(make_ref_counted<AudioEffectLimiterInstance>());
    ins->base = Ref<AudioEffectLimiter>(this);

    return ins;
}

void AudioEffectLimiter::set_threshold_db(float p_threshold) {

    threshold = p_threshold;
}

float AudioEffectLimiter::get_threshold_db() const {

    return threshold;
}

void AudioEffectLimiter::set_ceiling_db(float p_ceiling) {

    ceiling = p_ceiling;
}
float AudioEffectLimiter::get_ceiling_db() const {

    return ceiling;
}

void AudioEffectLimiter::set_soft_clip_db(float p_soft_clip) {

    soft_clip = p_soft_clip;
}
float AudioEffectLimiter::get_soft_clip_db() const {

    return soft_clip;
}

void AudioEffectLimiter::set_soft_clip_ratio(float p_soft_clip) {

    soft_clip_ratio = p_soft_clip;
}
float AudioEffectLimiter::get_soft_clip_ratio() const {

    return soft_clip_ratio;
}

void AudioEffectLimiter::_bind_methods() {

    SE_BIND_METHOD(AudioEffectLimiter,set_ceiling_db);
    SE_BIND_METHOD(AudioEffectLimiter,get_ceiling_db);

    SE_BIND_METHOD(AudioEffectLimiter,set_threshold_db);
    SE_BIND_METHOD(AudioEffectLimiter,get_threshold_db);

    SE_BIND_METHOD(AudioEffectLimiter,set_soft_clip_db);
    SE_BIND_METHOD(AudioEffectLimiter,get_soft_clip_db);

    SE_BIND_METHOD(AudioEffectLimiter,set_soft_clip_ratio);
    SE_BIND_METHOD(AudioEffectLimiter,get_soft_clip_ratio);

    ADD_PROPERTY(PropertyInfo(VariantType::FLOAT, "ceiling_db", PropertyHint::Range, "-20,-0.1,0.1"), "set_ceiling_db", "get_ceiling_db");
    ADD_PROPERTY(PropertyInfo(VariantType::FLOAT, "threshold_db", PropertyHint::Range, "-30,0,0.1"), "set_threshold_db", "get_threshold_db");
    ADD_PROPERTY(PropertyInfo(VariantType::FLOAT, "soft_clip_db", PropertyHint::Range, "0,6,0.1"), "set_soft_clip_db", "get_soft_clip_db");
    ADD_PROPERTY(PropertyInfo(VariantType::FLOAT, "soft_clip_ratio", PropertyHint::Range, "3,20,0.1"), "set_soft_clip_ratio", "get_soft_clip_ratio");
}

AudioEffectLimiter::AudioEffectLimiter() {
    threshold = 0;
    ceiling = -0.1f;
    soft_clip = 2;
    soft_clip_ratio = 10;
}
