#pragma once
#include "Math/Vector.h"
#include "Renderer/model.h"
#include "Math/Quat.h"

class Body
{
public:
	Vec3 position;
	Quat orientation;
	Vec3 linearVelocity;
	Vec3 angularVelocity;
	float inverseMass;
	float elasticity;
	float friction;
	Shape* shape;
	void Update(const float dt_sec);
	Vec3 GetCenterOfMassWorldSpace() const;
	Vec3 GetCenterOfMassBodySpace() const;
	Vec3 WorldSpaceToBodySpace(const Vec3& worldPoint);
	Vec3 BodySpaceToWorldSpace(const Vec3& bodyPoint);
	void ApplyImpulseLinear(const Vec3& impulse);
	void ApplyImpulseAngular(const Vec3& impulse);
	void ApplyImpulse(const Vec3& impulsePoint, const Vec3& impulse);
	Mat3 GetInverseInertiaTensorBodySpace() const;
	Mat3 GetInverseInertiaTensorWorldSpace() const;
};