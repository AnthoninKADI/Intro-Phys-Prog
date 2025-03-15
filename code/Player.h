#pragma once
#include <string>

enum class Name
{
	None,
	Player1,
	Player2
};

class Player
{
public:
	Player(Name name);
	~Player();

public:
	Name getName();
	std::string getStringName();
	int getShootLeft();
	int getScore();
	bool canShoot();
	void shoot();
	void setScore(int score);
	void setShootLeft(int number);

private:
	Name name;
	int shootLeft = 3;
	int score = 0;
};
