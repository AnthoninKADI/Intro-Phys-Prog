//
//  Scene.cpp
//
#include "Scene.h"
#include "../Shape.h"
#include "Intersections.h"
#include "Broadphase.h"

Scene::~Scene()
{
	for (auto& bodie : bodies)
	{
		delete bodie.shape;
	}
	bodies.clear();
}

void Scene::Reset()
{
	for (auto& bodie : bodies)
	{
		delete bodie.shape;
	}
	bodies.clear();

	Initialize();
}

void Scene::Initialize()
{
	Body body;
	for (int i = 0; i < 1; ++i) // Number balls  (first spawn)
	{
		for (int j = 0; j < 3; ++j) // Number balls (first spawn)
		{
			float radius = 0.5f;
			float x = (i - 1) * radius * 1.5f;
			float y = (j - 1) * radius * 1.5f;
			body.position = Vec3(x, y, 10);
			body.orientation = Quat(0, 0, 0, 1);
			body.shape = new ShapeSphere(radius);
			body.inverseMass = 1.0f;
			body.elasticity = 0.5f;
			body.friction = 0.5f;
			body.linearVelocity.Zero();
			bodies.push_back(body);
		}
	}
	for (int i = 0; i < 1; ++i) // Number balls  (second spawn)
	{
		for (int j = 0; j < 1; ++j) // Number balls (second spawn)
		{
			float radius = 0.5f;
			float x = (i - 1) * radius * 1.5f;
			float y = (j - 1) * radius * 1.5f;
			body.position = Vec3(x + 2, y + 1, 10);
			body.orientation = Quat(0, 0, 0, 1);
			body.shape = new ShapeSphere(radius);
			body.inverseMass = 1.0f;
			body.elasticity = 0.5f;
			body.friction = 0.5f;
			body.linearVelocity.Zero();
			bodies.push_back(body);
		}
	}
	for (int i = 0; i < 3; ++i)    // Surface Size
	{
		for (int j = 0; j < 3; ++j)  // Surface Size
		{
			float radius = 80.0f;
			float x = (i - 1) * radius * 0.25f;
			float y = (j - 1) * radius * 0.25f;
			body.position = Vec3(x, y, -radius);
			body.orientation = Quat(0, 0, 0, 1);
			body.shape = new ShapeSphere(radius);
			body.inverseMass = 0.0f;
			body.elasticity = 0.99f;
			body.friction = 0.5f;
			bodies.push_back(body);
		}
	}
}

void Scene::Update(const float dt_sec)
{
	// Gravity
	for (int i = 0; i < bodies.size(); ++i)
	{
		Body& body = bodies[i];
		float mass = 1.0f / body.inverseMass;
		// Gravity needs to be an impulse I
		// I == dp, so F == dp/dt <=> dp = F * dt
		// <=> I = F * dt <=> I = m * g * dt
		Vec3 impulseGravity = Vec3(0, 0, -10) * mass * dt_sec;
		body.ApplyImpulseLinear(impulseGravity);
	}
	
	// Broadphase
	std::vector<CollisionPair> collisionPairs;
	BroadPhase(bodies.data(), bodies.size(), collisionPairs, dt_sec);
	// Collision checks (Narrow phase)
	int numContacts = 0;
	const int maxContacts = bodies.size() * bodies.size();
	Contact* contacts = (Contact*)alloca(sizeof(Contact) * maxContacts);
	for (int i = 0; i < collisionPairs.size(); ++i)
	{
		const CollisionPair& pair = collisionPairs[i];
		Body& bodyA = bodies[pair.a];
		Body& bodyB = bodies[pair.b];
		if (bodyA.inverseMass == 0.0f && bodyB.inverseMass == 0.0f)
			continue;
		Contact contact;
		if (Intersections::Intersect(bodyA, bodyB, dt_sec, contact))
		{
			contacts[numContacts] = contact;
			++numContacts;
		}
	}
	// Sort times of impact
	if (numContacts > 1)
	{
		qsort(contacts, numContacts, sizeof(Contact), Contact::CompareContact);
	}
	
	// Contact resolve in order
	float accumulatedTime = 0.0f;
	for (int i = 0; i < numContacts; ++i)
	{
		Contact& contact = contacts[i];
		const float dt = contact.timeOfImpact - accumulatedTime;
		
		Body* bodyA = contact.a;
		Body* bodyB = contact.b;
		
		// Skip body par with infinite mass
		if (bodyA->inverseMass == 0.0f && bodyB->inverseMass == 0.0f) continue;
		
		// Position update
		for (auto& bodie : bodies)
		{
			bodie.Update(dt);
		}
		Contact::ResolveContact(contact);
		accumulatedTime += dt;
	}
	
	// Other physics behaviours, outside collisions.
	// Update the positions for the rest of this frame's time.
	const float timeRemaining = dt_sec - accumulatedTime;
	if (timeRemaining > 0.0f)
	{
		for (auto& bodie : bodies)
		{
			bodie.Update(timeRemaining);
		}
	}
}
