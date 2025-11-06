#include <stdlib.h>
#include <glut.h>;
#include <cmath>;

class NinjaPlayer {
public:
    float x = 0, y = 0, z = 0;
    float yaw = 0;  
    float scale = 1.0f;

    float t = 0.0f; 

    static void drawSolidCylinder(float radius, float height, int slices = 18, int stacks = 1)
    {
        GLUquadric* q = gluNewQuadric();
        gluCylinder(q, radius, radius, height, slices, stacks);

        glPushMatrix();
        glTranslatef(0, 0, height);
        gluDisk(q, 0, radius, slices, 1);
        glPopMatrix();

        glPushMatrix();
        glRotatef(180, 1, 0, 0);
        gluDisk(q, 0, radius, slices, 1);
        glPopMatrix();

        gluDeleteQuadric(q);
    }

    void draw() const
    {
        glPushMatrix();
        glTranslatef(x, y, z);
        glRotatef(yaw, 0, 1, 0);
        glScalef(scale, scale, scale);

        const float BLACK[3] = { 0.05f, 0.05f, 0.06f };
        const float DARK[3] = { 0.10f, 0.10f, 0.12f };
        const float CLOTH[3] = { 0.12f, 0.12f, 0.15f };
        const float RED[3] = { 0.85f, 0.12f, 0.12f };
        const float SKIN[3] = { 0.85f, 0.70f, 0.55f };
        const float GRAY[3] = { 0.25f, 0.26f, 0.28f };

        auto setc = [&](const float c[3]) { glColor3f(c[0], c[1], c[2]); };

        float breathe = 0.02f * sinf(t * 2.2f);
        float armSwing = 10.0f * sinf(t * 2.8f);
        float tailWave = 6.0f * sinf(t * 3.6f);

        glPushMatrix();
        setc(BLACK);
        glTranslatef(0, 0.9f + breathe, 0);
        glScalef(0.9f, 1.2f, 0.5f);
        glutSolidCube(1.0f);
        glPopMatrix();

        glPushMatrix();
        setc(CLOTH);
        glTranslatef(0, 1.0f + breathe, 0.26f);
        glScalef(0.92f, 0.25f, 0.06f);
        glutSolidCube(1.0f);
        glPopMatrix();

        glPushMatrix();
        setc(RED);
        glTranslatef(0, 0.3f + breathe, 0);
        glRotatef(90, 1, 0, 0);
        glutSolidTorus(0.05f, 0.38f, 12, 28);
        glPopMatrix();

        glPushMatrix();
        glTranslatef(0, 1.9f + breathe, 0);
        setc(DARK);
        glutSolidSphere(0.35f, 24, 24);
        setc(GRAY);
        glPushMatrix();
        glRotatef(90, 1, 0, 0);
        drawSolidCylinder(0.36f, 0.05f, 28, 1);
        glPopMatrix();

        setc(SKIN);
        glPushMatrix(); glTranslatef(0.12f, 0.04f, 0.31f); glutSolidSphere(0.04f, 12, 12); glPopMatrix();
        glPushMatrix(); glTranslatef(-0.12f, 0.04f, 0.31f); glutSolidSphere(0.04f, 12, 12); glPopMatrix();

        setc(RED);
        glPushMatrix();
        glTranslatef(-0.10f, 0.10f, -0.28f);
        glutSolidSphere(0.06f, 12, 12);
        glPopMatrix();

        glPushMatrix();
        glTranslatef(-0.10f, 0.10f, -0.28f);
        glRotatef(-25 + tailWave, 0, 1, 0);
        glRotatef(30, 1, 0, 0);
        drawSolidCylinder(0.035f, 0.35f, 12, 1);
        glTranslatef(0, 0, 0.35f);
        glRotatef(-15, 1, 0, 0);
        drawSolidCylinder(0.03f, 0.25f, 12, 1);
        glPopMatrix();
        glPopMatrix();

        auto drawArm = [&](float side) {  
            glPushMatrix();
            glTranslatef(side * 0.55f, 1.35f + breathe, 0);
            glRotatef(armSwing * side, 1, 0, 0);   
            setc(DARK);
            glutSolidSphere(0.16f, 16, 16);

            glPushMatrix();
            glRotatef(90, 1, 0, 0);
            drawSolidCylinder(0.11f, 0.45f, 16, 1);
            glPopMatrix();

            glTranslatef(0, -0.45f, 0);
            glutSolidSphere(0.13f, 16, 16);

            glPushMatrix();
            glRotatef(90, 1, 0, 0);
            drawSolidCylinder(0.10f, 0.42f, 16, 1);
            glPopMatrix();

            glTranslatef(0, -0.42f, 0.02f);
            setc(CLOTH);
            glutSolidSphere(0.12f, 16, 16);
            glPopMatrix();
            };
        drawArm(+1.f);
        drawArm(-1.f);

        for (int s = -1; s <= 1; s += 2) {
            setc(RED);
            glPushMatrix(); glTranslatef(s * 0.55f, 0.78f + breathe, 0.02f); glRotatef(90, 1, 0, 0); glutSolidTorus(0.03f, 0.12f, 10, 18); glPopMatrix();
        }

        auto drawLeg = [&](float side) {
            glPushMatrix();
            glTranslatef(side * 0.32f, 0.20f, 0);
            setc(DARK);
            glPushMatrix();
            glRotatef(90, 1, 0, 0);
            drawSolidCylinder(0.14f, 0.55f, 16, 1);
            glPopMatrix();

            glTranslatef(0, -0.55f, 0);
            glutSolidSphere(0.13f, 16, 16);

            glPushMatrix();
            glRotatef(90, 1, 0, 0);
            drawSolidCylinder(0.12f, 0.55f, 16, 1);
            glPopMatrix();

            setc(BLACK);
            glTranslatef(0, -0.35f, 0.10f);
            glScalef(0.28f, 0.12f, 0.5f);
            glutSolidCube(1.0f);
            glPopMatrix();
            };
        drawLeg(+1.f);
        drawLeg(-1.f);
        glPopMatrix();
    }
};
