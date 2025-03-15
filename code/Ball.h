#pragma once
#include "Body.h"

enum class Type
{
	None,
	jack,
	ball
};

class Ball : public Body
{
public:
	Ball(Type type, class Player* player);
	~Ball();
	
public:
	Type getType();
	class Player* getPlayer();
	
private:
	Type type;
	class Player* player = nullptr;
};

