/*************************************************************************/
/*  btRayShape.h                                                         */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2019 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2019 Godot Engine contributors (cf. AUTHORS.md)    */
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

/// IMPORTANT The class name and filename was created by following Bullet writing rules for an easy (eventually) porting to bullet
/// This shape is a custom shape that is not present to Bullet physics engine
#pragma once

#include <BulletCollision/CollisionShapes/btConvexInternalShape.h>

/**
	@author AndreaCatania
*/

/// Ray shape around z axis
ATTRIBUTE_ALIGNED16(class)
btRayShape : public btConvexInternalShape {

	btScalar m_length;
	bool slipsOnSlope;
	/// The default axis is the z
	btVector3 m_shapeAxis;

	btTransform m_cacheSupportPoint;
	btScalar m_cacheScaledLength;

public:
	BT_DECLARE_ALIGNED_ALLOCATOR();

	btRayShape(btScalar length);
	~btRayShape() override;

	void setLength(btScalar p_length);
	btScalar getLength() const { return m_length; }

	void setMargin(btScalar margin) override;

	void setSlipsOnSlope(bool p_slipOnSlope);
	bool getSlipsOnSlope() const { return slipsOnSlope; }

	const btTransform &getSupportPoint() const { return m_cacheSupportPoint; }
	const btScalar &getScaledLength() const { return m_cacheScaledLength; }

	btVector3 localGetSupportingVertex(const btVector3 &vec) const override;
#ifndef __SPU__
	btVector3 localGetSupportingVertexWithoutMargin(const btVector3 &vec) const override;
#endif //#ifndef __SPU__

	void batchedUnitVectorGetSupportingVertexWithoutMargin(const btVector3 *vectors, btVector3 *supportVerticesOut, int numVectors) const override;

	///getAabb returns the axis aligned bounding box in the coordinate frame of the given transform t.
	void getAabb(const btTransform &t, btVector3 &aabbMin, btVector3 &aabbMax) const override;

#ifndef __SPU__
	void calculateLocalInertia(btScalar mass, btVector3 & inertia) const override;

	const char *getName() const override {
		return "RayZ";
	}
#endif //__SPU__

	int getNumPreferredPenetrationDirections() const override;
	void getPreferredPenetrationDirection(int index, btVector3 &penetrationVector) const override;

private:
	void reload_cache();
};
