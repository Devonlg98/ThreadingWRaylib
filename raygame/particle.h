#pragma once

#include "raylib.h"

struct particle
{
	float startTime;
	float lifeTime;

	float size;
	Color color;

	Vector2 pos;
	Vector2 vel;

	bool isAlive(float currentTime) const
	{
		return currentTime < startTime + lifeTime;


	}

	void draw() const
	{
		DrawEllipseLines(pos.x, pos.y, size, size, color);
	}




};