/*************************************************************************/
/*  audio_stream_preview.h                                               */
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

#include "core/hash_map.h"
#include "core/os/thread.h"
#include "scene/main/node.h"
#include "servers/audio/audio_stream.h"

class AudioStreamPreview : public RefCounted {
    GDCLASS(AudioStreamPreview,RefCounted)

    friend class AudioStream;
    Vector<uint8_t> preview;
    float length;

    friend class AudioStreamPreviewGenerator;

public:
    float get_length() const;
    float get_max(float p_time, float p_time_next) const;
    float get_min(float p_time, float p_time_next) const;

    AudioStreamPreview();
};

class GODOT_EXPORT AudioStreamPreviewGenerator : public Node {
    GDCLASS(AudioStreamPreviewGenerator,Node)

    static AudioStreamPreviewGenerator *singleton;

    struct Preview {
        Ref<AudioStreamPreview> preview;
        Ref<AudioStream> base_stream;
        Ref<AudioStreamPlayback> playback;
        SafeFlag generating;
        GameEntity id;
        Thread thread;
    };

    HashMap<GameEntity, Preview> previews;

    static void _preview_thread(void *p_preview);

    void _update_emit(GameEntity p_id);

protected:
    void _notification(int p_what);
    static void _bind_methods();

public:
    static AudioStreamPreviewGenerator *get_singleton() { return singleton; }

    Ref<AudioStreamPreview> generate_preview(const Ref<AudioStream> &p_stream);

    AudioStreamPreviewGenerator();
};
