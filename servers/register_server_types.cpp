/*************************************************************************/
/*  register_server_types.cpp                                            */
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

#include "register_server_types.h"
#include "core/class_db.h"
#include "core/engine.h"
#include "core/project_settings.h"

#include "arvr/arvr_interface.h"
#include "arvr/arvr_positional_tracker.h"
#include "arvr_server.h"
#include "audio/audio_effect.h"
#include "audio/audio_stream.h"
#include "audio/effects/audio_effect_amplify.h"
#include "audio/effects/audio_effect_capture.h"
#include "audio/effects/audio_effect_chorus.h"
#include "audio/effects/audio_effect_compressor.h"
#include "audio/effects/audio_effect_delay.h"
#include "audio/effects/audio_effect_distortion.h"
#include "audio/effects/audio_effect_eq.h"
#include "audio/effects/audio_effect_filter.h"
#include "audio/effects/audio_effect_limiter.h"
#include "audio/effects/audio_effect_panner.h"
#include "audio/effects/audio_effect_phaser.h"
#include "audio/effects/audio_effect_pitch_shift.h"
#include "audio/effects/audio_effect_record.h"
#include "audio/effects/audio_effect_reverb.h"
#include "audio/effects/audio_effect_spectrum_analyzer.h"
#include "audio/effects/audio_effect_stereo_enhance.h"
#include "audio/effects/audio_stream_generator.h"
#include "audio_server.h"
#include "camera/camera_feed.h"
#include "camera_server.h"

#include "navigation_2d_server.h"
#include "navigation_server.h"
#include "physics_2d/physics_2d_server_sw.h"
#include "physics_2d/physics_2d_server_wrap_mt.h"
#include "physics_server_2d.h"
#include "physics_server_3d.h"
#include "scene/debugger/script_debugger_remote.h"
#include "rendering/shader_types.h"
#include "rendering_server.h"

static void _debugger_get_resource_usage(List<ScriptDebuggerRemote::ResourceUsage> *r_usage) {

    Vector<RenderingServer::TextureInfo> tinfo;
    RenderingServer::get_singleton()->texture_debug_usage(&tinfo);

    for (const RenderingServer::TextureInfo &E : tinfo) {

        ScriptDebuggerRemote::ResourceUsage usage;
        usage.path = E.path;
        usage.vram = E.bytes;
        usage.id = E.texture;
        usage.type = "Texture";
        if (E.depth == 0) {
            usage.format = itos(E.width) + "x" + itos(E.height) + " " + Image::get_format_name(E.format);
        } else {
            usage.format = itos(E.width) + "x" + itos(E.height) + "x" + itos(E.depth) + " " + Image::get_format_name(E.format);
        }
        r_usage->push_back(usage);
    }
}

ShaderTypes *shader_types = nullptr;

PhysicsServer2D *_createGodotPhysics2DCallback() {
    return Physics2DServerWrapMT::init_server<Physics2DServerSW>();
}

static bool has_server_feature_callback(StringView p_feature) {

    if (RenderingServer::get_singleton()) {
        if (RenderingServer::get_singleton()->has_os_feature(StringName(p_feature))) {
            return true;
        }
    }

    return false;
}

void register_server_types() {


    AudioStreamMicrophone::initialize_class();
    AudioStreamPlaybackMicrophone::initialize_class();
    AudioStreamRandomPitch::initialize_class();
    AudioStreamPlaybackRandomPitch::initialize_class();
    AudioEffectPitchShiftInstance::initialize_class();
    AudioEffectPitchShift::initialize_class();
    AudioEffectCompressorInstance::initialize_class();
    AudioEffectCompressor::initialize_class();
    AudioEffectStereoEnhanceInstance::initialize_class();
    AudioEffectStereoEnhance::initialize_class();
    AudioEffectDistortionInstance::initialize_class();
    AudioEffectDistortion::initialize_class();
    AudioEffectChorusInstance::initialize_class();
    AudioEffectChorus::initialize_class();
    AudioEffectReverbInstance::initialize_class();
    AudioEffectReverb::initialize_class();
    AudioEffectPannerInstance::initialize_class();
    AudioEffectPanner::initialize_class();
    AudioEffectLimiterInstance::initialize_class();
    AudioEffectLimiter::initialize_class();
    AudioEffectEQInstance::initialize_class();
    AudioEffectEQ::initialize_class();
    AudioEffectEQ6::initialize_class();
    AudioEffectEQ10::initialize_class();
    AudioEffectEQ21::initialize_class();
    AudioEffectPhaserInstance::initialize_class();
    AudioEffectPhaser::initialize_class();
    AudioEffectSpectrumAnalyzer::initialize_class();
    AudioEffectRecordInstance::initialize_class();
    AudioEffectRecord::initialize_class();
    AudioStreamGenerator::initialize_class();
    AudioEffectDelayInstance::initialize_class();
    AudioEffectDelay::initialize_class();
    AudioEffectAmplifyInstance::initialize_class();
    AudioEffectAmplify::initialize_class();
    AudioEffectFilterInstance::initialize_class();
    AudioEffectFilter::initialize_class();
    AudioEffectLowPassFilter::initialize_class();
    AudioEffectHighPassFilter::initialize_class();
    AudioEffectBandPassFilter::initialize_class();
    AudioEffectNotchFilter::initialize_class();
    AudioEffectBandLimitFilter::initialize_class();
    AudioEffectLowShelfFilter::initialize_class();
    AudioEffectHighShelfFilter::initialize_class();

    CameraServer::initialize_class();
    ARVRServer::initialize_class();
    ARVRPositionalTracker::initialize_class();
    CameraFeed::initialize_class();
    Physics2DDirectBodyStateSW::initialize_class();
    Physics2DServerSW::initialize_class();
    Physics2DDirectSpaceStateSW::initialize_class();
    Physics2DServerWrapMT::initialize_class();
    PhysicsShapeQueryParameters3D::initialize_class();
    PhysicsShapeQueryParameters2D::initialize_class();
    Physics2DTestMotionResult::initialize_class();
    AudioServer::initialize_class();
    AudioBusLayout::initialize_class();


    ClassDB::register_virtual_class<RenderingServer>();
    ClassDB::register_class<AudioServer>();
    ClassDB::register_virtual_class<PhysicsServer3D>();
    ClassDB::register_virtual_class<PhysicsServer2D>();
    ClassDB::register_virtual_class<NavigationServer>();
    ClassDB::register_virtual_class<Navigation2DServer>();
    ClassDB::register_class<ARVRServer>();
    ClassDB::register_class<CameraServer>();

    shader_types = memnew(ShaderTypes);

    ClassDB::register_virtual_class<ARVRInterface>();
    ClassDB::register_class<ARVRPositionalTracker>();

    ClassDB::register_virtual_class<AudioStream>();
    ClassDB::register_virtual_class<AudioStreamPlayback>();
    ClassDB::register_virtual_class<AudioStreamPlaybackResampled>();
    ClassDB::register_class<AudioStreamMicrophone>();
    ClassDB::register_class<AudioStreamRandomPitch>();
    ClassDB::register_virtual_class<AudioEffect>();
    ClassDB::register_virtual_class<AudioEffectInstance>();
    ClassDB::register_class<AudioEffectEQ>();
    ClassDB::register_class<AudioEffectFilter>();
    ClassDB::register_class<AudioBusLayout>();

    ClassDB::register_class<AudioStreamGenerator>();
    ClassDB::register_virtual_class<AudioStreamGeneratorPlayback>();

    {
        //audio effects
        ClassDB::register_class<AudioEffectAmplify>();

        ClassDB::register_class<AudioEffectReverb>();

        ClassDB::register_class<AudioEffectLowPassFilter>();
        ClassDB::register_class<AudioEffectHighPassFilter>();
        ClassDB::register_class<AudioEffectBandPassFilter>();
        ClassDB::register_class<AudioEffectNotchFilter>();
        ClassDB::register_class<AudioEffectBandLimitFilter>();
        ClassDB::register_class<AudioEffectLowShelfFilter>();
        ClassDB::register_class<AudioEffectHighShelfFilter>();

        ClassDB::register_class<AudioEffectEQ6>();
        ClassDB::register_class<AudioEffectEQ10>();
        ClassDB::register_class<AudioEffectEQ21>();

        ClassDB::register_class<AudioEffectDistortion>();

        ClassDB::register_class<AudioEffectStereoEnhance>();

        ClassDB::register_class<AudioEffectPanner>();
        ClassDB::register_class<AudioEffectChorus>();
        ClassDB::register_class<AudioEffectDelay>();
        ClassDB::register_class<AudioEffectCompressor>();
        ClassDB::register_class<AudioEffectLimiter>();
        ClassDB::register_class<AudioEffectPitchShift>();
        ClassDB::register_class<AudioEffectPhaser>();

        ClassDB::register_class<AudioEffectRecord>();
        ClassDB::register_class<AudioEffectSpectrumAnalyzer>();
        ClassDB::register_virtual_class<AudioEffectSpectrumAnalyzerInstance>();
        ClassDB::register_class<AudioEffectCapture>();
    }

    ClassDB::register_class<CameraFeed>();

    ClassDB::register_virtual_class<PhysicsDirectBodyState2D>();
    ClassDB::register_virtual_class<PhysicsDirectSpaceState2D>();
    ClassDB::register_class<Physics2DTestMotionResult>();
    ClassDB::register_class<PhysicsShapeQueryParameters2D>();

    ClassDB::register_class<PhysicsShapeQueryParameters3D>();
    ClassDB::register_virtual_class<PhysicsDirectBodyState3D>();
    ClassDB::register_virtual_class<PhysicsDirectSpaceState3D>();
    ClassDB::register_class<PhysicsTestMotionResult>();

    ScriptDebuggerRemote::resource_usage_func = _debugger_get_resource_usage;

}
void setup_server_defs()
{
    OS::get_singleton()->set_has_server_feature_callback(has_server_feature_callback);
    // Physics 2D
    GLOBAL_DEF(Physics2DServerManager::setting_property_name, "DEFAULT");
    ProjectSettings::get_singleton()->set_custom_property_info(Physics2DServerManager::setting_property_name,
            PropertyInfo(VariantType::STRING, StringName(Physics2DServerManager::setting_property_name), PropertyHint::Enum, "DEFAULT"));

    Physics2DServerManager::register_server(("GodotPhysics"), &_createGodotPhysics2DCallback);
    Physics2DServerManager::set_default_server(("GodotPhysics"));

    // Physics 3D
    GLOBAL_DEF(PhysicsServerManager::setting_property_name, "DEFAULT");
    ProjectSettings::get_singleton()->set_custom_property_info(PhysicsServerManager::setting_property_name,
        PropertyInfo(VariantType::STRING, StringName(PhysicsServerManager::setting_property_name), PropertyHint::Enum, "DEFAULT"));

}
void unregister_server_types() {

    memdelete(shader_types);
}

void register_server_singletons() {
    Engine::get_singleton()->add_singleton(Engine::Singleton("RenderingServer", RenderingServer::get_singleton()));
    Engine::get_singleton()->add_singleton(Engine::Singleton("AudioServer", AudioServer::get_singleton()));
    Engine::get_singleton()->add_singleton(Engine::Singleton("PhysicsServer3D", PhysicsServer3D::get_singleton()));
    Engine::get_singleton()->add_singleton(Engine::Singleton("PhysicsServer2D", PhysicsServer2D::get_singleton()));
    Engine::get_singleton()->add_singleton(Engine::Singleton("NavigationServer", NavigationServer::get_singleton_mut()));
    Engine::get_singleton()->add_singleton(Engine::Singleton("Navigation2DServer", Navigation2DServer::get_singleton_mut()));
    Engine::get_singleton()->add_singleton(Engine::Singleton("ARVRServer", ARVRServer::get_singleton()));
    Engine::get_singleton()->add_singleton(Engine::Singleton("CameraServer", CameraServer::get_singleton()));
}
