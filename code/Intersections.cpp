#include "Intersections.h"

bool Intersections::Intersect(Body& a, Body& b,
const float dt, Contact& contact)
{
	contact.a = &a;
	contact.b = &b;
	const Vec3 ab = b.position - a.position;
	contact.normal = ab;
	contact.normal.Normalize();
	if (a.shape->GetType() == Shape::ShapeType::SHAPE_SPHERE
	&& b.shape->GetType() == Shape::ShapeType::SHAPE_SPHERE) {
		ShapeSphere* sphereA = static_cast<ShapeSphere*>(a.shape);
		ShapeSphere* sphereB = static_cast<ShapeSphere*>(b.shape);
		Vec3 posA = a.position;
		Vec3 posB = b.position;
		Vec3 valA = a.linearVelocity;
		Vec3 velB = b.linearVelocity;
		if (Intersections::SphereSphereDynamic(*sphereA, *sphereB,
		posA, posB, valA, velB, dt,
		contact.ptOnAWorldSpace, contact.ptOnBWorldSpace,
		contact.timeOfImpact))
		{
			a.Update(contact.timeOfImpact);
			b.Update(contact.timeOfImpact);
			contact.ptOnALocalSpace =
			a.WorldSpaceToBodySpace(contact.ptOnAWorldSpace);
			contact.ptOnBLocalSpace =
			b.WorldSpaceToBodySpace(contact.ptOnBWorldSpace);
			Vec3 ab = a.position - b.position;
			contact.normal = ab;
			contact.normal.Normalize();
			a.Update(-contact.timeOfImpact);
			b.Update(-contact.timeOfImpact);
			float r = ab.GetMagnitude()
			- (sphereA->radius + sphereB->radius);
			contact.separationDistance = r;
			return true;
		}
	}
	return false;
}

bool Intersections::RaySphere(const Vec3& rayStart, const Vec3& rayDir, const Vec3& sphereCenter, const float sphereRadius, float& t0, float& t1)
{
	const Vec3& s = sphereCenter - rayStart;
	const float a = rayDir.Dot(rayDir);
	const float b = s.Dot(rayDir);
	const float c = s.Dot(s) - sphereRadius * sphereRadius;
	const float delta = b * b - a * c;
	const float inverseA = 1.0f / a;
	if (delta < 0)
	{
		return false;
	}
	const float deltaRoot = sqrtf(delta);
	t0 = (b - deltaRoot) * inverseA;
	t1 = (b + deltaRoot) * inverseA;

	return true;
}

bool Intersections::SphereSphereDynamic(
const ShapeSphere& shapeA, const ShapeSphere& shapeB,
const Vec3& posA, const Vec3& posB, const Vec3& velA, const Vec3& velB,
const float dt, Vec3& ptOnA, Vec3& ptOnB, float& timeOfImpact)
{
	const Vec3 relativeVelocity = velA - velB;
	const Vec3 startPtA = posA;
	const Vec3 endPtA = startPtA + relativeVelocity * dt;
	const Vec3 rayDir = endPtA - startPtA;
	float t0 = 0;
	float t1 = 0;
	if (rayDir.GetLengthSqr() < 0.001f * 0.001f)
	{
		Vec3 ab = posB - posA;
		float radius = shapeA.radius + shapeB.radius + 0.001f;
		if (ab.GetLengthSqr() > radius * radius)
		{
			return false;
		}
	}
	else if (!RaySphere(startPtA, rayDir, posB,
	shapeA.radius + shapeB.radius, t0, t1))
	{
		return false;
	}
	t0 *= dt;
	t1 *= dt;
	if (t1 < 0) return false;
	timeOfImpact = t0 < 0.0f ? 0.0f : t0;
	if (timeOfImpact > dt) {
		return false;
	}
	Vec3 newPosA = posA + velA * timeOfImpact;
	Vec3 newPosB = posB + velB * timeOfImpact;
	Vec3 ab = newPosB - newPosA;
	ab.Normalize();
	ptOnA = newPosA + ab * shapeA.radius;
	ptOnB = newPosB - ab * shapeB.radius;
	return true;
}