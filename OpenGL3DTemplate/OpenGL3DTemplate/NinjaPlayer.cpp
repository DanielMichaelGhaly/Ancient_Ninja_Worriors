#include <stdlib.h>
#include <glut.h>
#include <cmath>

class NinjaPlayer {
public:
    float x = 0, y = 0, z = 0;
    float yaw = 0;
    float scale = 0.35f;
    float t = 0.0f;
    float movementMagnitude = 0.0f; 

    static void drawSolidCylinder(float radius, float height, int slices = 18, int stacks = 1) {
        GLUquadric* q = gluNewQuadric();
        gluCylinder(q, radius, radius, height, slices, stacks);
        glPushMatrix(); glTranslatef(0, 0, height); gluDisk(q, 0, radius, slices, 1); glPopMatrix();
        glPushMatrix(); glRotatef(180, 1, 0, 0); gluDisk(q, 0, radius, slices, 1); glPopMatrix();
        gluDeleteQuadric(q);
    }

    void draw() const {
        glPushMatrix();
        glTranslatef(x, y, z);
        glRotatef(yaw, 0, 1, 0);
        glScalef(scale, scale, scale);

        const float BODY_DARK[3] = { 0.07f, 0.07f, 0.09f };
        const float BODY_MID[3] = { 0.12f, 0.13f, 0.16f };
        const float ACCENT_RED[3] = { 0.85f, 0.15f, 0.18f };
        const float SKIN[3] = { 0.82f, 0.68f, 0.52f };
        const float ACCENT_GOLD[3] = { 0.78f, 0.62f, 0.22f };
        const float GRAY_TRIM[3] = { 0.28f, 0.29f, 0.30f };

        auto setc = [&](const float c[3]) { glColor3f(c[0], c[1], c[2]); };

        float breathe = 0.015f * sinf(t * 2.0f);
        float walkPhase = t * 6.0f;
        float swing = movementMagnitude * 35.0f * sinf(walkPhase);
        float legLift = movementMagnitude * 8.0f * sinf(walkPhase + 3.14159f * 0.5f);
        float armSwing = movementMagnitude * 30.0f * sinf(walkPhase + 3.14159f);
        float tailWave = 5.0f * sinf(t * 4.0f);

        setc(BODY_DARK);
        glPushMatrix(); glTranslatef(0, 0.9f + breathe, 0); glScalef(0.9f, 1.25f, 0.55f); glutSolidCube(1.0f); glPopMatrix();

        setc(BODY_MID);
        glPushMatrix(); glTranslatef(0, 1.05f + breathe, 0.28f); glScalef(0.95f, 0.28f, 0.10f); glutSolidCube(1.0f); glPopMatrix();

        setc(ACCENT_RED);
        glPushMatrix(); glTranslatef(0, 0.35f + breathe, 0); glRotatef(90, 1, 0, 0); glutSolidTorus(0.05f, 0.40f, 14, 32); glPopMatrix();

        glPushMatrix();
        glTranslatef(0, 1.95f + breathe, 0);
        setc(BODY_MID); glutSolidSphere(0.36f, 26, 26);
        setc(GRAY_TRIM);
        glPushMatrix(); glRotatef(90, 1, 0, 0); drawSolidCylinder(0.37f, 0.05f, 28, 1); glPopMatrix();
        setc(SKIN); glPushMatrix(); glTranslatef(0.13f, 0.05f, 0.33f); glutSolidSphere(0.045f, 14, 14); glPopMatrix();
        glPushMatrix(); glTranslatef(-0.13f, 0.05f, 0.33f); glutSolidSphere(0.045f, 14, 14); glPopMatrix();
        setc(ACCENT_RED); glPushMatrix(); glTranslatef(-0.11f, 0.12f, -0.30f); glutSolidSphere(0.065f, 14, 14); glPopMatrix();
        glPushMatrix(); glTranslatef(-0.11f, 0.12f, -0.30f); glRotatef(-25 + tailWave, 0, 1, 0); glRotatef(32, 1, 0, 0);
        drawSolidCylinder(0.038f, 0.38f, 14, 1); glTranslatef(0, 0, 0.38f); glRotatef(-18, 1, 0, 0); drawSolidCylinder(0.032f, 0.28f, 14, 1); glPopMatrix();
        glPopMatrix();

        auto drawArm = [&](float side) {
            glPushMatrix();
            glTranslatef(side * 0.60f, 1.40f + breathe, 0);
            glRotatef(armSwing * side, 1, 0, 0);
            setc(BODY_MID); glutSolidSphere(0.17f, 18, 18);
            glPushMatrix(); glRotatef(90, 1, 0, 0); drawSolidCylinder(0.12f, 0.48f, 16, 1); glPopMatrix();
            glTranslatef(0, -0.48f, 0); glutSolidSphere(0.14f, 18, 18);
            glPushMatrix(); glRotatef(90, 1, 0, 0); drawSolidCylinder(0.11f, 0.44f, 16, 1); glPopMatrix();
            glTranslatef(0, -0.44f, 0.02f); setc(ACCENT_RED); glutSolidSphere(0.13f, 18, 18);
            glPopMatrix();
        };
        drawArm(+1.f); drawArm(-1.f);

        for (int s = -1; s <= 1; s += 2) {
            setc(ACCENT_GOLD);
            glPushMatrix(); glTranslatef(s * 0.60f, 0.80f + breathe, 0.03f); glRotatef(90, 1, 0, 0); glutSolidTorus(0.035f, 0.13f, 12, 20); glPopMatrix();
        }

        auto drawLeg = [&](float side) {
            glPushMatrix();
            glTranslatef(side * 0.35f, 0.22f, 0);
            glRotatef(swing * side, 1, 0, 0);
            setc(BODY_MID); glPushMatrix(); glRotatef(90, 1, 0, 0); drawSolidCylinder(0.15f, 0.60f, 18, 1); glPopMatrix();
            glTranslatef(0, -0.60f, 0); glutSolidSphere(0.14f, 18, 18);
            glPushMatrix(); glRotatef(90, 1, 0, 0); drawSolidCylinder(0.13f, 0.60f, 18, 1); glPopMatrix();
            setc(BODY_DARK); glTranslatef(0, -0.40f, 0.12f + legLift * 0.01f); glScalef(0.30f, 0.14f, 0.55f); glutSolidCube(1.0f);
            glPopMatrix();
        };
        drawLeg(+1.f); drawLeg(-1.f);

        glPopMatrix();
    }
};
