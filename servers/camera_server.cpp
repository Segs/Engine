/*************************************************************************/
/*  camera_server.cpp                                                    */
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

#include "camera_server.h"

#include "camera_server_enum_casters.h"
#include "servers/camera/camera_feed.h"
#include "rendering_server.h"

#include "core/method_bind.h"
#include "core/method_enum_caster.h"
#include "core/os/mutex.h"
#include "core/print_string.h"

IMPL_GDCLASS(CameraServer)

////////////////////////////////////////////////////////
// CameraServer

CameraServer::CreateFunc CameraServer::create_func = nullptr;

void CameraServer::_bind_methods() {
    SE_BIND_METHOD(CameraServer,get_feed);
    SE_BIND_METHOD(CameraServer,get_feed_count);
    SE_BIND_METHOD(CameraServer,get_feeds);

    SE_BIND_METHOD(CameraServer,add_feed);
    SE_BIND_METHOD(CameraServer,remove_feed);

    ADD_SIGNAL(MethodInfo("camera_feed_added", PropertyInfo(VariantType::INT, "id")));
    ADD_SIGNAL(MethodInfo("camera_feed_removed", PropertyInfo(VariantType::INT, "id")));

    BIND_ENUM_CONSTANT(FEED_RGBA_IMAGE);
    BIND_ENUM_CONSTANT(FEED_YCBCR_IMAGE);
    BIND_ENUM_CONSTANT(FEED_Y_IMAGE);
    BIND_ENUM_CONSTANT(FEED_CBCR_IMAGE);
}

CameraServer *CameraServer::singleton = nullptr;

CameraServer *CameraServer::get_singleton() {
    return singleton;
}

int CameraServer::get_free_id() {
    bool id_exists = true;
    int newid = 0;

    // find a free id
    while (id_exists) {
        newid++;
        id_exists = false;
        for (int i = 0; i < feeds.size() && !id_exists; i++) {
            if (feeds[i]->get_id() == newid) {
                id_exists = true;
            }
        }
    }

    return newid;
}

int CameraServer::get_feed_index(int p_id) {
    for (int i = 0; i < feeds.size(); i++) {
        if (feeds[i]->get_id() == p_id) {
            return i;
        }
    }

    return -1;
}

Ref<CameraFeed> CameraServer::get_feed_by_id(int p_id) {
    int index = get_feed_index(p_id);

    if (index == -1) {
        return Ref<CameraFeed>();
    } else {
        return feeds[index];
    }
}

void CameraServer::add_feed(const Ref<CameraFeed> &p_feed) {
    ERR_FAIL_COND(!p_feed);
    // add our feed
    feeds.push_back(p_feed);

// record for debugging
#ifdef DEBUG_ENABLED
    print_line("Registered camera " + p_feed->get_name() + " with id " + itos(p_feed->get_id()) + " position " + itos(p_feed->get_position()) + " at index " + itos(feeds.size() - 1));
#endif

    // let whomever is interested know
    emit_signal("camera_feed_added", p_feed->get_id());
}

void CameraServer::remove_feed(const Ref<CameraFeed> &p_feed) {
    for (int i = 0; i < feeds.size(); i++) {
        if (feeds[i] == p_feed) {
            int feed_id = p_feed->get_id();

// record for debugging
#ifdef DEBUG_ENABLED
            print_line("Removed camera " + p_feed->get_name() + " with id " + itos(feed_id) + " position " + itos(p_feed->get_position()));
#endif

            // remove it from our array, if this results in our feed being unreferenced it will be destroyed
            feeds.erase_at(i);

            // let whomever is interested know
            emit_signal("camera_feed_removed", feed_id);
            return;
        }
    }
}

Ref<CameraFeed> CameraServer::get_feed(int p_index) {
    ERR_FAIL_INDEX_V(p_index, feeds.size(), Ref<CameraFeed>());

    return feeds[p_index];
}

int CameraServer::get_feed_count() {
    return feeds.size();
}

Array CameraServer::get_feeds() {
    Array return_feeds;
    int cc = get_feed_count();
    return_feeds.resize(cc);

    for (int i = 0; i < feeds.size(); i++) {
        return_feeds[i] = get_feed(i);
    }

    return return_feeds;
}

RenderingEntity CameraServer::feed_texture(int p_id, CameraServer::FeedImage p_texture) {
    int index = get_feed_index(p_id);
    ERR_FAIL_COND_V(index == -1, entt::null);

    Ref<CameraFeed> feed = get_feed(index);

    return feed->get_texture(p_texture);
}

CameraServer::CameraServer() {
    singleton = this;
}

CameraServer::~CameraServer() {
    singleton = nullptr;
}
