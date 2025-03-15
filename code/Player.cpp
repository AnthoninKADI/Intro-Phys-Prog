#include "Player.h"

Player::Player(Name name) : name(name)
{
}

Player::~Player()
{
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
	case Name::FirstShooter:
		strName = "FirstShooter";
		break;
	case Name::SecondShooter:
		strName = "SecondShooter";
		break;
	default:
		strName = "Error";
		break;
	}
	return strName;
}


