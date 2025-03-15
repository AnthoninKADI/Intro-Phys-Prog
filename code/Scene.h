#pragma once
#include "application.h"
#include <vector>
#include <chrono>
#include <string>
#include "Ball.h"

class Scene
{
public:
	Scene() { bodies.reserve( 128 ); nextBodiesSpawn.reserve(128);}
	~Scene();

	void Reset();
	void Initialize();
	void Update( const float dt_sec );
	bool EndUpdate();
	void BallSpawn(const Vec3& cameraPos, const Vec3& cameraFocusPoint, float strength);
	std::vector<Body*> bodies;
	std::vector<Body*> nextBodiesSpawn;
	bool shoot_finished();

private:
	class Body* earth = nullptr;
	class Player* player1 = nullptr;
	class Player* player2 = nullptr;
	class Player* currentPlayer = nullptr;
	class Player* winner = nullptr;
	class Ball* jack = nullptr;
	std::vector<class Ball*> balls;
	class Ball* currentBall = nullptr;
	bool canShoot = true;
	bool shootFirst = true;
	bool turnFirst = true;
	std::chrono::time_point<std::chrono::system_clock> start;
	static float size;
};

