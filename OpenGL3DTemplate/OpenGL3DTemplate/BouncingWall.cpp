#include "BouncingWall.h"
#include <cmath>

BouncingWall::BouncingWall(float x, float y, float z, float w, float h, float d, float r, float g, float b, float bounce):
	posX(x), posY(y), posZ(z), width(w), height(h), depth(d),
	colorR(r), colorG(g), colorB(b), bounceFactor(bounce) {
}

void BouncingWall::drawSolidCylinder(float radius, float height, int slices, int stacks)
{
	GLUquadric* quad = gluNewQuadric();

	gluCylinder(quad, radius, radius, height, slices, stacks);

	glPushMatrix();
	glTranslatef(0.0f, 0.0f, height);
	gluDisk(quad, 0.0f, radius, slices, 1);
	glPopMatrix();

	glPushMatrix();
	glRotatef(180.0f, 1.0f, 0.0f, 0.0f);
	gluDisk(quad, 0.0f, radius, slices, 1);
	glPopMatrix();

	gluDeleteQuadric(quad);
}

void BouncingWall::draw()
{
    auto drawLantern = [&](float w, float h, float d) {
        glColor3f(0.10f, 0.10f, 0.12f);
        glPushMatrix(); glScalef(w, h, d); glutSolidCube(1.f); glPopMatrix();

        glColor3f(1.0f, 0.95f, 0.75f);
        glPushMatrix(); glTranslatef(0, 0, d * 0.51f); glScalef(w * 0.80f, h * 0.80f, d * 0.04f); glutSolidCube(1.f); glPopMatrix();

        glColor3f(0.05f, 0.05f, 0.06f);
        float t = h * 0.06f;
        for (int i = -1; i <= 1; ++i) {
            glPushMatrix();
            glTranslatef(i * (w * 0.22f), 0, d * 0.52f);
            glScalef(t, h * 0.78f, d * 0.03f);
            glutSolidCube(1.f);
            glPopMatrix();
        }
        for (int i = -1; i <= 1; i += 2) {
            glPushMatrix();
            glTranslatef(0, i * (h * 0.22f), d * 0.52f);
            glScalef(w * 0.78f, t, d * 0.03f);
            glutSolidCube(1.f);
            glPopMatrix();
        }

        glColor3f(0.08f, 0.08f, 0.09f);
        glPushMatrix();
        glTranslatef(0, h * 0.65f, 0);
        glRotatef(90, 1, 0, 0);
        drawSolidCylinder(w * 0.06f, d * 0.7f, 12, 1);
        glPopMatrix();
        };

    auto drawShojiGridPanel = [&](float W, float H, float Z) {
        glColor3f(0.20f, 0.22f, 0.24f); 
        glPushMatrix();
        glTranslatef(0, 0, Z);
        glScalef(W, H, depth * 0.02f);
        glutSolidCube(1.f);
        glPopMatrix();

        glColor3f(0.10f, 0.10f, 0.12f);
        int cols = 6, rows = 4;
        float marginX = W * 0.46f, marginY = H * 0.46f;
        float thick = 0.02f * fminf(W, H);

        auto frameStrip = [&](float sx, float sy, float sz, float tx, float ty, float tz) {
            glPushMatrix(); glTranslatef(sx, sy, sz); glScalef(tx, ty, tz); glutSolidCube(1.f); glPopMatrix();
            };
        frameStrip(0, marginY, Z + 0.001f, W * 0.92f, thick, depth * 0.02f);
        frameStrip(0, -marginY, Z + 0.001f, W * 0.92f, thick, depth * 0.02f);
        frameStrip(marginX, 0, Z + 0.001f, thick, H * 0.92f, depth * 0.02f);
        frameStrip(-marginX, 0, Z + 0.001f, thick, H * 0.92f, depth * 0.02f);

        for (int c = 1; c < cols; ++c) {
            float x = -W * 0.5f + (W / cols) * c;
            glPushMatrix();
            glTranslatef(x, 0, Z + 0.001f);
            glScalef(thick, H * 0.9f, depth * 0.02f);
            glutSolidCube(1.f);
            glPopMatrix();
        }
        for (int r = 1; r < rows; ++r) {
            float y = -H * 0.5f + (H / rows) * r;
            glPushMatrix();
            glTranslatef(0, y, Z + 0.001f);
            glScalef(W * 0.9f, thick, depth * 0.02f);
            glutSolidCube(1.f);
            glPopMatrix();
        }
        };

    auto drawClanCrest = [&](float radius, float z) {
        glColor3f(0.12f, 0.12f, 0.14f);
        glPushMatrix();
        glTranslatef(0, 0, z);
        drawSolidCylinder(radius, depth * 0.01f, 40, 1);
        glPopMatrix();

        glColor3f(0.9f, 0.1f, 0.1f); 
        glPushMatrix();
        glTranslatef(0, 0, z + depth * 0.015f);
        glRotatef(45, 0, 0, 1);
        glScalef(radius * 0.9f, radius * 0.9f, depth * 0.03f);
        glutSolidOctahedron();
        glPopMatrix();
        };

    glPushMatrix();
    glTranslatef(posX, posY, posZ);

    glPushMatrix();
    glColor3f(0.08f, 0.08f, 0.09f); 
    glScalef(width, height, depth);
    glutSolidCube(1.0f);
    glPopMatrix();

    float zFront = depth * 0.51f;
    float t = 0.05f;

    glColor3f(0.03f, 0.03f, 0.035f);
    glPushMatrix(); glTranslatef(0, 0.5f * height - t * height, zFront); glScalef(width * 0.98f, t * height, depth * 0.04f); glutSolidCube(1.f); glPopMatrix();
    glPushMatrix(); glTranslatef(0, -0.5f * height + t * height, zFront); glScalef(width * 0.98f, t * height, depth * 0.04f); glutSolidCube(1.f); glPopMatrix();
    glPushMatrix(); glTranslatef(-0.5f * width + t * width, 0, zFront); glScalef(t * width, height * 0.98f, depth * 0.04f); glutSolidCube(1.f); glPopMatrix();
    glPushMatrix(); glTranslatef(0.5f * width - t * width, 0, zFront); glScalef(t * width, height * 0.98f, depth * 0.04f); glutSolidCube(1.f); glPopMatrix();

    drawShojiGridPanel(width * 0.88f, height * 0.88f, depth * 0.49f);

    glPushMatrix();
    glTranslatef(0, height * 0.18f, 0);
    drawClanCrest(fminf(width, height) * 0.12f, depth * 0.505f);
    glPopMatrix();

    float lanternY = height * 0.40f;
    float lanternZ = depth * 0.51f;
    float lanternW = width * 0.18f;
    float lanternH = height * 0.28f;
    float lanternD = depth * 0.10f;
    float lanternX = width * 0.35f;

    glPushMatrix();
    glTranslatef(-lanternX, lanternY, lanternZ);
    drawLantern(lanternW, lanternH, lanternD);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(lanternX, lanternY, lanternZ);
    drawLantern(lanternW, lanternH, lanternD);
    glPopMatrix();

    glColor3f(0.8f, 0.1f, 0.1f);
    float tasselH = lanternH * 0.25f;
    float tasselR = lanternW * 0.04f;

    glPushMatrix();
    glTranslatef(-lanternX, lanternY - lanternH * 0.65f, lanternZ);
    glRotatef(90, 1, 0, 0);
    drawSolidCylinder(tasselR, tasselH, 10, 1);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(lanternX, lanternY - lanternH * 0.65f, lanternZ);
    glRotatef(90, 1, 0, 0);
    drawSolidCylinder(tasselR, tasselH, 10, 1);
    glPopMatrix();

    glPushAttrib(GL_ENABLE_BIT | GL_POLYGON_BIT | GL_LINE_BIT);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glLineWidth(1.0f);
    glColor3f(0.f, 0.f, 0.f);
    glPushMatrix();
    glScalef(width * 1.001f, height * 1.001f, depth * 1.001f);
    glutWireCube(1.0f);
    glPopMatrix();
    glPopAttrib();

    glPopMatrix(); 
}




bool BouncingWall::checkCollisionAndBounce(float& playerX, float& playerY, float& playerZ,
	float& velocityX, float& velocityY, float& velocityZ,
	float playerRadius) {

	float minX = posX - width * 0.5f;
	float maxX = posX + width * 0.5f;
	float minY = posY - height * 0.5f;
	float maxY = posY + height * 0.5f;
	float minZ = posZ - depth * 0.5f;
	float maxZ = posZ + depth * 0.5f;

	float closestX = (playerX < minX) ? minX : (playerX > maxX) ? maxX : playerX;
	float closestY = (playerY < minY) ? minY : (playerY > maxY) ? maxY : playerY;
	float closestZ = (playerZ < minZ) ? minZ : (playerZ > maxZ) ? maxZ : playerZ;

	float distX = playerX - closestX;
	float distY = playerY - closestY;
	float distZ = playerZ - closestZ;
	float distance = sqrt(distX * distX + distY * distY + distZ * distZ);

	if (distance < playerRadius) {
		float penetration = playerRadius - distance;

		float normalX = distX / distance;
		float normalY = distY / distance;
		float normalZ = distZ / distance;

		playerX += normalX * penetration;
		playerY += normalY * penetration;
		playerZ += normalZ * penetration;

		float velocityAlongNormal = velocityX * normalX + velocityY * normalY + velocityZ * normalZ;

		if (velocityAlongNormal < 0) {
			velocityX -= (1.0f + bounceFactor) * velocityAlongNormal * normalX;
			velocityY -= (1.0f + bounceFactor) * velocityAlongNormal * normalY;
			velocityZ -= (1.0f + bounceFactor) * velocityAlongNormal * normalZ;
		}

		return true;
	}

	return false;
}

void BouncingWall::setPosition(float x, float y, float z) {
	posX = x;
	posY = y;
	posZ = z;
}

void BouncingWall::setColor(float r, float g, float b) {
	colorR = r;
	colorG = g;
	colorB = b;
}

void BouncingWall::setBounceFactor(float bounce) {
	bounceFactor = bounce;
}