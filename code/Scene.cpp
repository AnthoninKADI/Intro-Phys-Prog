#include "Scene.h"
#include "Shape.h"
#include "Intersections.h"
#include "Broadphase.h"
#include "Player.h"

#include <algorithm>
#include <iostream>
#include <chrono>

Scene::~Scene()
{
	for ( int i = 0; i < bodies.size(); i++ )
	{
		delete bodies[ i ]->shape;
	}
	bodies.clear();
}

void Scene::Reset()
{
	for ( int i = 0; i < bodies.size(); i++ )
	{
		if (bodies[i]->shape != nullptr)
		{
			delete bodies[i]->shape;
		}
	}
	bodies.clear();
	Initialize();
}

void Scene::Initialize()
{
	float radius = 500.0f;
	earth = new Body();
	earth->position = Vec3(0, 0, -radius);
	earth->orientation = Quat(0, 0, 0, 1);
	earth->shape = new ShapeSphere(radius);
	earth->inverseMass = 0.0f;
	earth->elasticity = 0.2f;
	earth->friction = 0.5f;
	bodies.push_back(earth);

	if(firstShooter == nullptr)
	{
		firstShooter = new Player(Name::FirstShooter);
	}
	if(secondShooter == nullptr)
	{
		secondShooter = new Player(Name::SecondShooter);
	} 
	if (winner == nullptr)
	{
		currentShooter = firstShooter;
	}
	else
	{
		currentShooter = winner;
	} 
}

void Scene::Update(const float dt_sec)
{
	for (int i = 0; i < bodies.size(); ++i)
	{
		Body& body = *bodies[i];
		float mass = 1.0f / body.inverseMass;
		Vec3 centerOfGravity = earth->position - body.position;
		centerOfGravity.Normalize();
		centerOfGravity *= 9.8f;
		Vec3 impulseGravity = centerOfGravity * mass * dt_sec;
		body.ApplyImpulseLinear(impulseGravity);
		body.linearVelocity = Vec3::Lerp(body.linearVelocity, Vec3(0, 0, 0), 0.01f);
		body.angularVelocity = Vec3::Lerp(body.angularVelocity, Vec3(0, 0, 0), 0.01f);
	}
	std::vector<CollisionPair> collisionPairs;
	BroadPhase(bodies, collisionPairs, dt_sec);
	int numContacts = 0;
	const int maxContacts = bodies.size() * bodies.size();
	Contact* contacts = (Contact*)alloca(sizeof(Contact) * maxContacts);
	for (int i = 0; i < collisionPairs.size(); ++i)
	{
		const CollisionPair& pair = collisionPairs[i];
		Body& bodyA = *bodies[pair.a];
		Body& bodyB = *bodies[pair.b];
		if (bodyA.inverseMass == 0.0f && bodyB.inverseMass == 0.0f)
		{
			continue;
		}
		Contact contact;
		if (Intersections::Intersect(bodyA, bodyB, dt_sec, contact))
		{
			contacts[numContacts] = contact;
			++numContacts;
			bodyA.linearVelocity = Vec3::Lerp(bodyA.linearVelocity, Vec3(0, 0, 0), 0.015);
			bodyA.angularVelocity = Vec3::Lerp(bodyA.angularVelocity, Vec3(0, 0, 0), 0.015);
			bodyB.linearVelocity = Vec3::Lerp(bodyB.linearVelocity, Vec3(0, 0, 0), 0.015);
			bodyB.angularVelocity = Vec3::Lerp(bodyB.angularVelocity, Vec3(0, 0, 0), 0.015);
		}
	}
	if (numContacts > 1)
	{
		qsort(contacts, numContacts, sizeof(Contact),
		Contact::CompareContact);
	}
	float timeAccumulated = 0.0f;
	for (int i = 0; i < numContacts; ++i)
	{
		Contact& contact = contacts[i];
		const float dt = contact.timeOfImpact - timeAccumulated;
		Body* bodyA = contact.a;
		Body* bodyB = contact.b;
		if (bodyA->inverseMass == 0.0f && bodyB->inverseMass == 0.0f)
		{
			continue;
		}
		for (int j = 0; j < bodies.size(); ++j)
		{
			bodies[j]->Update(dt);
		}
		Contact::ResolveContact(contact);
		timeAccumulated += dt;
	}

	const float timeRemaining = dt_sec - timeAccumulated;
	if (timeRemaining > 0.0f)
	{
		for (int i = 0; i < bodies.size(); ++i)
		{
			bodies[i]->Update(timeRemaining);
		}
	}
	
	if (shoot_finished())
	{
		if (shootFirst)
		{
			shootFirst = false;
		}
		else if (turnFirst)
		{
			currentShooter == firstShooter ? currentShooter = secondShooter : currentShooter = secondShooter;
			turnFirst = false;
		}
		currentBall = nullptr;
		canShoot = true;
	}
}

bool Scene::EndUpdate()
{
	if (!std::empty(nextBodiesSpawn))
	{
		for (auto& body : nextBodiesSpawn)
		{
			bodies.push_back(body);
		}
		nextBodiesSpawn.clear(); 
		return true;
	}
	return false;
}

void Scene::BallSpawn(const Vec3& cameraPos, const Vec3& cameraFocusPoint, float strength)
{
	if (!canShoot)
	{
		return;
	}
	if (currentShooter == nullptr)
	{
		return;
	}
	if (!currentShooter->canShoot())
	{
		return;
	}
	Vec3 dir = cameraFocusPoint - cameraPos;
	dir.Normalize();
	Type type = Type::None;
	float radius = 0.0f;
	if (shootFirst)
	{
		type = Type::jack;
		radius = 0.3f;
	}
	else
	{
		currentShooter->shoot();
		type = Type::ball;
		radius = 0.8f;
	}
	start = std::chrono::system_clock::now();
	currentBall = new Ball(type, currentShooter);
	currentBall->position = cameraPos + dir * 20;
	dir.z += 0.1f;
	currentBall->linearVelocity = dir * 35 * std::min(std::max(0.8f,strength) , 1.5f);
	currentBall->orientation = Quat(0,0,0,1);
	currentBall->shape = new ShapeSphere(radius);
	currentBall->inverseMass = 3.1f;
	currentBall->elasticity = 0.2f;
	currentBall->friction = 0.4f;
	if (shootFirst)
	{
		jack = currentBall;
	}
	else
	{
		balls.push_back(currentBall);
	}
	nextBodiesSpawn.push_back(currentBall);
	canShoot = false;
}

bool Scene::shoot_finished()
{
	if (currentBall != nullptr)
	{
		std::chrono::time_point<std::chrono::system_clock> end = std::chrono::system_clock::now();
		std::chrono::duration<float> elapsed_seconds = end - start;
		if (elapsed_seconds.count() > 1.0f)
		{
			return true;
		}
	}
	return false;
}

float Scene::size = 0.4f;