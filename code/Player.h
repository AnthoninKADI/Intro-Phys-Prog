#pragma once
#include <string>

enum class Name
{
	None,
	FirstShooter,
	SecondShooter
};

class Player
{
public:
	Player(Name name);
	~Player();

public:
	int getShootLeft();
	bool canShoot();
	void shoot();
	void setShootLeft(int number);
	Name getName();
	std::string getStringName();

private:
	Name name;
	int shootLeft = 99;
};
