/*************************************************************************/
/*  broad_phase_2d_bvh.cpp                                               */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2021 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2021 Godot Engine contributors (cf. AUTHORS.md).   */
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

#include "broad_phase_2d_bvh.h"
#include "collision_object_2d_sw.h"
#include "core/project_settings.h"

BroadPhase2DSW::ID BroadPhase2DBVH::create(CollisionObject2DSW *p_object, int p_subindex, const Rect2 &p_aabb, bool p_static) {
    ID oid = bvh.create(p_object, true, p_aabb, p_subindex, !p_static, 1 << p_object->get_type(), p_static ? 0 : 0xFFFFF); // Pair everything, don't care?
    return oid + 1;
}

void BroadPhase2DBVH::move(ID p_id, const Rect2 &p_aabb) {
    bvh.move(p_id - 1, p_aabb);
}

void BroadPhase2DBVH::recheck_pairs(ID p_id) {
    bvh.recheck_pairs(p_id - 1);
}

void BroadPhase2DBVH::set_static(ID p_id, bool p_static) {
    CollisionObject2DSW *it = bvh.get(p_id - 1);
    bvh.set_pairable(p_id - 1, !p_static, 1 << it->get_type(), p_static ? 0 : 0xFFFFF, false); // Pair everything, don't care?
}

void BroadPhase2DBVH::remove(ID p_id) {
    bvh.erase(p_id - 1);
}

CollisionObject2DSW *BroadPhase2DBVH::get_object(ID p_id) const {
    CollisionObject2DSW *it = bvh.get(p_id - 1);
    ERR_FAIL_COND_V(!it, nullptr);
    return it;
}

bool BroadPhase2DBVH::is_static(ID p_id) const {
    return !bvh.is_pairable(p_id - 1);
}

int BroadPhase2DBVH::get_subindex(ID p_id) const {
    return bvh.get_subindex(p_id - 1);
}

int BroadPhase2DBVH::cull_segment(const Vector2 &p_from, const Vector2 &p_to, CollisionObject2DSW **p_results, int p_max_results, int *p_result_indices) {
    return bvh.cull_segment(p_from, p_to, p_results, p_max_results, p_result_indices);
}

int BroadPhase2DBVH::cull_aabb(const Rect2 &p_aabb, CollisionObject2DSW **p_results, int p_max_results, int *p_result_indices) {
    return bvh.cull_aabb(p_aabb, Span<CollisionObject2DSW *>(p_results,p_results+p_max_results), p_result_indices);
}

void *BroadPhase2DBVH::_pair_callback(void *p_self, uint32_t p_id_A, CollisionObject2DSW *p_object_A, int p_subindex_A, uint32_t p_id_B, CollisionObject2DSW *p_object_B, int p_subindex_B) {
    BroadPhase2DBVH *bpo = (BroadPhase2DBVH *)(p_self);
    if (!bpo->pair_callback) {
        return nullptr;
    }

    return bpo->pair_callback(p_object_A, p_subindex_A, p_object_B, p_subindex_B, nullptr, bpo->pair_userdata);
}

void BroadPhase2DBVH::_unpair_callback(void *p_self, uint32_t p_id_A, CollisionObject2DSW *p_object_A, int p_subindex_A, uint32_t p_id_B, CollisionObject2DSW *p_object_B, int p_subindex_B, void *p_pair_data) {
    BroadPhase2DBVH *bpo = (BroadPhase2DBVH *)(p_self);
    if (!bpo->unpair_callback) {
        return;
    }

    bpo->unpair_callback(p_object_A, p_subindex_A, p_object_B, p_subindex_B, p_pair_data, bpo->unpair_userdata);
}

void *BroadPhase2DBVH::_check_pair_callback(void *p_self, uint32_t p_id_A, CollisionObject2DSW *p_object_A, int p_subindex_A, uint32_t p_id_B, CollisionObject2DSW *p_object_B, int p_subindex_B, void *p_pair_data) {
    BroadPhase2DBVH *bpo = (BroadPhase2DBVH *)(p_self);
    if (!bpo->pair_callback) {
        return nullptr;
    }

    return bpo->pair_callback(p_object_A, p_subindex_A, p_object_B, p_subindex_B, p_pair_data, bpo->pair_userdata);
}

void BroadPhase2DBVH::set_pair_callback(PairCallback p_pair_callback, void *p_userdata) {
    pair_callback = p_pair_callback;
    pair_userdata = p_userdata;
}

void BroadPhase2DBVH::set_unpair_callback(UnpairCallback p_unpair_callback, void *p_userdata) {
    unpair_callback = p_unpair_callback;
    unpair_userdata = p_userdata;
}

void BroadPhase2DBVH::update() {
    bvh.update();
}

BroadPhase2DSW *BroadPhase2DBVH::_create() {
    return memnew(BroadPhase2DBVH);
}

BroadPhase2DBVH::BroadPhase2DBVH() {
    bvh.params_set_thread_safe(GLOBAL_GET("rendering/threads/thread_safe_bvh"));
    bvh.set_pair_callback(_pair_callback, this);
    bvh.set_unpair_callback(_unpair_callback, this);
    bvh.set_check_pair_callback(_check_pair_callback, this);
    pair_callback = nullptr;
    pair_userdata = nullptr;
    unpair_userdata = nullptr;
}
