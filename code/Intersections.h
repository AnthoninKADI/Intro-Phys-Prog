#pragma once
#include "Body.h"
#include "Contact.h"
#include "Shape.h"

class Intersections
{
public:
	static bool Intersect(Body& a, Body& b, const float dt, Contact& contact);
	static bool RaySphere(const Vec3& rayStart, const Vec3& rayDir, const Vec3& sphereCenter, float sphereRadius, float& t0,
	               float& t1);
	static bool SphereSphereDynamic(const ShapeSphere& shapeA, const ShapeSphere& shapeB, const Vec3& posA, const Vec3& posB,
	                         const Vec3& velA, const Vec3& velB, float dt, Vec3& ptOnA, Vec3& ptOnB,
	                         float& timeOfImpact);
};