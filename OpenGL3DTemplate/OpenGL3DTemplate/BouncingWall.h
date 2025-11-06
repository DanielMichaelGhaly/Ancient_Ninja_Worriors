#pragma once

#include <stdlib.h>
#include <glut.h>

class BouncingWall {
private:
	float posX, posY, posZ;
	float width, height, depth;
	float colorR, colorG, colorB;
	float bounceFactor;

public:
	BouncingWall(float x, float y, float z, float w, float h, float d, float r = 0.7f, float g = 0.5f, float b = 0.3f, float bounce = 0.3f);

	void drawSolidCylinder(float radius, float height, int slices, int stacks);

	void draw();

	bool checkCollisionAndBounce(float& playerX, float& playerY, float& playerZ, float& velocityX, float& velocityY, float& velocityZ, float playerRadius);

	float getX() const { return posX; }
	float getY() const { return posY; }
	float getZ() const { return posZ; }
	float getWidth() const { return width; }
	float getHeight() const { return height; }
	float getDepth() const { return depth; }

	void setPosition(float x, float y, float z);
	void setColor(float r, float g, float b);
	void setBounceFactor(float bounce);
};