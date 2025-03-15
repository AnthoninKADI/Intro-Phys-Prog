#include "Player.h"

Player::Player(Name name) : name(name)
{
}

Player::~Player()
{
}

Name Player::getName()
{
	return Name();
}

std::string Player::getStringName()
{
	std::string strName = "";
	switch (name)
	{
	case Name::None:
		strName = "None";
		break;
	case Name::Player1:
		strName = "Player 1";
		break;
	case Name::Player2:
		strName = "Player 2";
		break;
	default:
		strName = "Error";
		break;
	}
	return strName;
}

int Player::getShootLeft()
{
	return shootLeft;
}

void Player::setShootLeft(int number)
{
	shootLeft = number;
}

bool Player::canShoot()
{
	return (shootLeft > 0);
}

void Player::shoot()
{
	shootLeft--;
}

int Player::getScore()
{
	return score;
}

void Player::setScore(int score)
{
	this->score = score;
}
