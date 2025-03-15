#include "Ball.h"
#include "Player.h"

Ball::Ball(Type type, Player* player) : Body(), type(type), player(player)
{
}

Ball::~Ball()
{
}

Type Ball::getType()
{
	return Type();
}

Player* Ball::getPlayer()
{
	return player;
}
