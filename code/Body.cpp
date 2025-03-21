#include "Body.h"
#include "Shape.h"

void Body::Update(const float dt_sec)
{
	position += linearVelocity * dt_sec;
	Vec3 positionCM = GetCenterOfMassWorldSpace();
	Vec3 CMToPositon = position - positionCM;
	Mat3 orientationMat = orientation.ToMat3();
	Mat3 inertiaTensor = orientationMat * shape->InertiaTensor() * orientationMat.Transpose();
	Vec3 alpha = inertiaTensor.Inverse() * (angularVelocity.Cross(inertiaTensor * angularVelocity));
	angularVelocity += alpha * dt_sec;
	Vec3 dAngle = angularVelocity * dt_sec;
	Quat dq = Quat(dAngle, dAngle.GetMagnitude());
	orientation = dq * orientation;
	orientation.Normalize();
	position = positionCM + dq.RotatePoint(CMToPositon);
}

Vec3 Body::GetCenterOfMassWorldSpace() const
{
	const Vec3 centerOfMass = shape->GetCenterOfMass();
	const Vec3 pos = position + orientation.RotatePoint(centerOfMass);
	return pos;
}

Vec3 Body::GetCenterOfMassBodySpace() const
{
	return shape->GetCenterOfMass();
}

Vec3 Body::WorldSpaceToBodySpace(const Vec3& worldPoint)
{
	const Vec3 tmp = worldPoint - GetCenterOfMassWorldSpace();
	const Quat invertOrient = orientation.Inverse();
	Vec3 bodySpace = invertOrient.RotatePoint(tmp);
	return bodySpace;
}

Vec3 Body::BodySpaceToWorldSpace(const Vec3& bodyPoint)
{
	Vec3 worldSpace = GetCenterOfMassWorldSpace()

	+ orientation.RotatePoint(bodyPoint);

	return worldSpace;
}

void Body::ApplyImpulseLinear(const Vec3& impulse)
{
	if (inverseMass == 0.0f) return;
	linearVelocity += impulse * inverseMass;
}

Mat3 Body::GetInverseInertiaTensorBodySpace() const
{
	Mat3 inertiaTensor = shape->InertiaTensor();
	Mat3 inverseInertiaTensor = inertiaTensor.Inverse() * inverseMass;
	return inverseInertiaTensor;
}
Mat3 Body::GetInverseInertiaTensorWorldSpace() const
{
	Mat3 inertiaTensor = shape->InertiaTensor();
	Mat3 inverseInertiaTensor = inertiaTensor.Inverse() * inverseMass;
	Mat3 orient = orientation.ToMat3();
	inverseInertiaTensor = orient * inverseInertiaTensor
	* orient.Transpose();
	return inverseInertiaTensor;
}

void Body::ApplyImpulseAngular(const Vec3& impulse)
{
	if (inverseMass == 0.0f)
	{
		return;
	}
	angularVelocity += GetInverseInertiaTensorWorldSpace() * impulse;
	const float maxAngularSpeed = 30.0f;
	if (angularVelocity.GetLengthSqr() > maxAngularSpeed * maxAngularSpeed)
	{
		angularVelocity.Normalize();
		angularVelocity *= maxAngularSpeed;
	}
}

void Body::ApplyImpulse(const Vec3& impulsePoint, const Vec3& impulse)
{
	if (inverseMass == 0.0f) return;
	ApplyImpulseLinear(impulse);
	Vec3 position = GetCenterOfMassWorldSpace();
	Vec3 r = impulsePoint - position;
	Vec3 dL = r.Cross(impulse);
	ApplyImpulseAngular(dL);
}