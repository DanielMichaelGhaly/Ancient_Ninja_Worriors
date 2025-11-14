#include <stdlib.h>
#include <glut.h>
#include <BouncingWall.h>
#include <NinjaPlayer.cpp>
#include <cmath>
#include <vector>
#include <string>
#include <ctime>


static const float ARENA_MIN_X = -6.0f;
static const float ARENA_MAX_X =  6.0f;
static const float ARENA_MIN_Z = -6.0f;
static const float ARENA_MAX_Z =  6.0f;

BouncingWall wallFront(0.0f, 0.5f, ARENA_MIN_Z, 12.0f, 1.0f, 0.3f);  
BouncingWall wallBack (0.0f, 0.5f, ARENA_MAX_Z, 12.0f, 1.0f, 0.3f);   
BouncingWall wallLeft (ARENA_MIN_X, 0.5f, 0.0f, 0.3f, 1.0f, 12.0f);   
BouncingWall wallRight(ARENA_MAX_X, 0.5f, 0.0f, 0.3f, 1.0f, 12.0f);   

BouncingWall walls[] = {wallFront, wallBack, wallLeft, wallRight};

NinjaPlayer ninja;

float rotAng;

float camOffsetY = 2.0f;   
float camOffsetZ = 5.0f;   

float moveSpeed = 0.5f; 

float ninjaSpeed = 0.18f; 
float targetYaw = 0.0f;   

int lastMouseX = 0;
int lastMouseY = 0;
bool mouseLeftPressed = false;
bool mouseRightPressed = false;
bool mouseMiddlePressed = false;
float mouseSensitivity = 0.01f;

float ninjaRadius = 0.35f;

enum CameraView {
    FOLLOW_VIEW,
    TOP_VIEW,
    SIDE_VIEW,
    FRONT_VIEW
};

CameraView currentView = FOLLOW_VIEW;

struct Coin {
    float x, y, z;
    bool collected = false;
};

struct Platform {
    float x, y, z; 
    float w, d;    
    float r, g, b; 
    std::vector<Coin> coins;
    bool allCollected = false;
    bool animPlaying = false;
    float animT = 0.0f;       
};

Platform platforms[4];


int totalCoins = 0;
int totalCollected = 0;
bool gameWon = false;
bool gameOver = false;

int timeLeftMs = 90 * 1000; 
int lastTickMs = 0;

float clampf(float v, float a, float b) { return v < a ? a : (v > b ? b : v); }

float frand(float a, float b) { return a + (b - a) * (rand() / (float)RAND_MAX); }

void drawText(float x, float y, const std::string& s, void* font = GLUT_BITMAP_HELVETICA_18) {
    glRasterPos2f(x, y);
    for (char c : s) glutBitmapCharacter(font, c);
}

void drawHUD() {
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, 1, 0, 1);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    int secs = timeLeftMs / 1000;
    int mm = secs / 60;
    int ss = secs % 60;
    char buf[128];
    snprintf(buf, sizeof(buf), "Time: %02d:%02d", mm, ss);
    glColor3f(0, 0, 0);
    drawText(0.02f, 0.95f, buf);

    snprintf(buf, sizeof(buf), "Collected: %d / %d", totalCollected, totalCoins);
    drawText(0.02f, 0.90f, buf);

    drawText(0.02f, 0.85f, "Move: WASD or Arrows  |  View: 1 Top, 2 Side, 3 Front, Q Follow");
    drawText(0.02f, 0.80f, "Animations (after collecting platform coins): P1:F rotate, P2:G scale, P3:H move, P4:J color");

    if (gameWon) {
        glColor3f(0.0f, 0.7f, 0.0f);
        drawText(0.40f, 0.95f, "YOU WIN!");
    }
    if (gameOver && !gameWon) {
        glColor3f(0.8f, 0.0f, 0.0f);
        drawText(0.40f, 0.95f, "GAME OVER");
        drawText(0.35f, 0.90f, "Controls disabled");
    }

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

static void drawSolidCylinder(float radius, float height, int slices = 24, int stacks = 1) {
    GLUquadric* q = gluNewQuadric();
    gluCylinder(q, radius, radius, height, slices, stacks);
    glPushMatrix(); glTranslatef(0, 0, height); gluDisk(q, 0, radius, slices, 1); glPopMatrix();
    glPushMatrix(); glRotatef(180, 1, 0, 0);  gluDisk(q, 0, radius, slices, 1); glPopMatrix();
    gluDeleteQuadric(q);
}

void drawPlatformBase(const Platform& p) {
    glPushMatrix();
    glTranslatef(p.x, p.y, p.z);
    glColor3f(p.r, p.g, p.b);
    glPushMatrix(); glScalef(p.w, 0.2f, p.d); glutSolidCube(1.0f); glPopMatrix();
    glColor3f(p.r * 0.8f, p.g * 0.8f, p.b * 0.8f);
    glPushMatrix(); glTranslatef(0, 0.12f, 0); glScalef(p.w * 0.95f, 0.02f, p.d * 0.95f); glutSolidCube(1.0f); glPopMatrix();
    glPopMatrix();
}

void drawSushi(const Platform& p) {
    glPushMatrix();
    glTranslatef(p.x, p.y + 0.2f, p.z);
    glColor3f(0.9f, 0.9f, 0.95f);
    glPushMatrix(); glRotatef(-90, 1, 0, 0); drawSolidCylinder(0.6f, 0.05f); glPopMatrix();
    glColor3f(0.95f, 0.95f, 0.95f);
    glPushMatrix(); glRotatef(-90, 1, 0, 0); drawSolidCylinder(0.45f, 0.25f); glPopMatrix();
    glColor3f(0.05f, 0.1f, 0.08f);
    glPushMatrix(); glScalef(0.9f, 0.15f, 0.5f); glutSolidCube(0.6f); glPopMatrix();
    glColor3f(0.9f, 0.3f, 0.2f);
    glTranslatef(0, 0.22f, 0);
    glutSolidSphere(0.18f, 18, 18);
    glPopMatrix();
}

void drawNinjaStar(const Platform& p, float animT) {
    glPushMatrix();
    glTranslatef(p.x, p.y + 0.25f, p.z);

    glColor3f(0.6f, 0.6f, 0.65f);
    glutSolidTorus(0.03f, 0.25f, 16, 24);
    glColor3f(0.7f, 0.7f, 0.75f);
    glutSolidSphere(0.08f, 16, 16);
    glColor3f(0.8f, 0.8f, 0.85f);
    for (int i = 0; i < 4; ++i) {
        glPushMatrix();
        glRotatef(90.0f * i, 0, 1, 0);
        glTranslatef(0.25f, 0, 0);
        glRotatef(90, 0, 0, 1);
        glutSolidCone(0.07f, 0.38f, 10, 1);
        glPopMatrix();
    }
    glPopMatrix();
}

void drawSamuraiSword(const Platform& p) {
    glPushMatrix();
    glTranslatef(p.x, p.y + 0.28f, p.z);
    glRotatef(15, 0, 1, 0);
    glColor3f(0.85f, 0.85f, 0.9f);
    glPushMatrix(); glScalef(0.1f, 0.05f, 1.2f); glutSolidCube(1.0f); glPopMatrix();
    glColor3f(0.3f, 0.25f, 0.2f);
    glPushMatrix(); glTranslatef(0, 0, -0.35f); glRotatef(-90, 1, 0, 0); drawSolidCylinder(0.08f, 0.02f); glPopMatrix();
    glColor3f(0.15f, 0.12f, 0.1f);
    glPushMatrix(); glTranslatef(0, 0, -0.55f); glRotatef(-90, 1, 0, 0); drawSolidCylinder(0.05f, 0.25f); glPopMatrix();
    glColor3f(0.25f, 0.2f, 0.15f);
    glPushMatrix(); glTranslatef(0, 0, -0.7f); glutSolidSphere(0.05f, 12, 12); glPopMatrix();
    glPopMatrix();
}

void drawSmallTemple(const Platform& p, float colorPhase) {
    glPushMatrix();
    glTranslatef(p.x, p.y + 0.25f, p.z);
    glColor3f(0.6f, 0.55f, 0.5f);
    glPushMatrix(); glScalef(0.8f, 0.08f, 0.8f); glutSolidCube(1.0f); glPopMatrix();
    glColor3f(0.45f, 0.4f, 0.35f);
    float s = 0.32f; float h = 0.35f;
    for (int i = -1; i <= 1; i += 2) {
        for (int j = -1; j <= 1; j += 2) {
            glPushMatrix(); glTranslatef(i * s, 0.04f, j * s); glRotatef(-90, 1, 0, 0); drawSolidCylinder(0.05f, h); glPopMatrix();
        }
    }
    float r = 0.3f + 0.2f * (0.5f * (sinf(colorPhase) + 1.0f)); 
    glColor3f(r, 0.2f, 0.2f);
    glPushMatrix(); glTranslatef(0, 0.04f + h, 0); glRotatef(-90, 1, 0, 0); glutSolidCone(0.55f, 0.35f, 18, 1); glPopMatrix();
    glColor3f(0.85f, 0.7f, 0.4f);
    glPushMatrix(); glTranslatef(0, 0.04f + h + 0.35f, 0); glutSolidSphere(0.06f, 12, 12); glPopMatrix();
    glPopMatrix();
}

void drawCoin(float x, float y, float z) {
    glPushMatrix();
    glTranslatef(x, y, z);
    glRotatef(-90, 1, 0, 0);
    glColor3f(0.95f, 0.8f, 0.1f);
    drawSolidCylinder(0.12f, 0.04f, 20, 1);
    glPopMatrix();
}

void drawGround() {
    glPushMatrix();
    glTranslatef(0.0f, -1.0f, 0.0f);

    glColor3f(0.5f, 0.5f, 0.5f);
    glPushMatrix();
    glScalef(700.0f, 0.2f, 700.0f);
    glutSolidCube(1.0f);
    glPopMatrix();

    glColor3f(0.4f, 0.4f, 0.4f);
    for (int i = -100; i <= 100; i++) {
        for (int j = -100; j <= 100; j++) {
            if ((i + j) % 2 == 0) {
                glPushMatrix();
                glTranslatef(i * 1.8f, 0.11f, j * 1.8f);
                glScalef(1.7f, 0.02f, 1.7f);
                glutSolidCube(1.0f);
                glPopMatrix();
            }
        }
    }

    glLineWidth(1.0f);
    glColor3f(0.35f, 0.35f, 0.35f);
    glBegin(GL_LINES);

    for (int j = -100; j <= 100; j++) {
        glVertex3f(-90.0f, 0.12f, j * 1.8f);
        glVertex3f(90.0f, 0.12f, j * 1.8f);
    }

    for (int i = -100; i <= 100; i++) {
        glVertex3f(i * 1.8f, 0.12f, -90.0f);
        glVertex3f(i * 1.8f, 0.12f, 90.0f);
    }

    glEnd();

    glPopMatrix();
}

void updateCamera() {
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    if (currentView == FOLLOW_VIEW) {
        float yawRad = ninja.yaw * 3.14159265f / 180.0f;
        float camX = ninja.x - sinf(yawRad) * camOffsetZ;
        float camY = ninja.y + camOffsetY;
        float camZ = ninja.z - cosf(yawRad) * camOffsetZ;
        gluLookAt(camX, camY, camZ, ninja.x, ninja.y + 0.5f, ninja.z, 0.0f, 1.0f, 0.0f);
        return;
    }

    switch (currentView) {
    case TOP_VIEW:
        gluLookAt(ninja.x, ninja.y + 15.0f, ninja.z, ninja.x, ninja.y, ninja.z, 0.0f, 0.0f, -1.0f);
        break;
    case SIDE_VIEW:
        gluLookAt(ninja.x + 15.0f, ninja.y + 2.0f, ninja.z, ninja.x, ninja.y + 1.0f, ninja.z, 0.0f, 1.0f, 0.0f);
        break;
    case FRONT_VIEW:
        gluLookAt(ninja.x, ninja.y + 2.0f, ninja.z + 15.0f, ninja.x, ninja.y + 1.0f, ninja.z, 0.0f, 1.0f, 0.0f);
        break;
    default:
        break;
    }
}

void initScene() {
    srand((unsigned int)time(nullptr));

    float py = -0.8f; 
    platforms[0] = { -3.0f, py, -3.0f, 2.2f, 2.2f, 0.75f, 0.2f, 0.2f };
    platforms[1] = {  3.0f, py, -3.0f, 2.2f, 2.2f, 0.2f, 0.6f, 0.8f };
    platforms[2] = { -3.0f, py,  3.0f, 2.2f, 2.2f, 0.3f, 0.7f, 0.3f };
    platforms[3] = {  3.0f, py,  3.0f, 2.2f, 2.2f, 0.8f, 0.7f, 0.2f };

    totalCoins = 0; totalCollected = 0;
    for (int i = 0; i < 4; ++i) {
        platforms[i].coins.clear();
        int count = 4; 
        for (int c = 0; c < count; ++c) {
            float cx = frand(platforms[i].x - platforms[i].w * 0.4f, platforms[i].x + platforms[i].w * 0.4f);
            float cz = frand(platforms[i].z - platforms[i].d * 0.4f, platforms[i].z + platforms[i].d * 0.4f);
            float cy = platforms[i].y + 0.16f; 
            platforms[i].coins.push_back({ cx, cy, cz, false });
            totalCoins++;
        }
        platforms[i].allCollected = false;
        platforms[i].animPlaying = false;
        platforms[i].animT = 0.0f;
    }

    ninja.x = 0.0f; ninja.z = 0.0f; ninja.y = -0.5f; ninja.scale = 0.4f; ninja.yaw = 0.0f; targetYaw = 0.0f;

    gameWon = false; gameOver = false; timeLeftMs = 90 * 1000; lastTickMs = glutGet(GLUT_ELAPSED_TIME);
}

void moveNinja(float moveX, float moveZ) {
    if (gameOver) return;

    float newX = ninja.x + moveX;
    float newZ = ninja.z + moveZ;

    newX = clampf(newX, ARENA_MIN_X + ninjaRadius, ARENA_MAX_X - ninjaRadius);
    newZ = clampf(newZ, ARENA_MIN_Z + ninjaRadius, ARENA_MAX_Z - ninjaRadius);

    ninja.x = newX;
    ninja.z = newZ;

    if (moveX != 0.0f || moveZ != 0.0f) {
        targetYaw = atan2f(moveX, moveZ) * 180.0f / 3.14159265f;
    }
}

void drawPlatformsAndObjects(float dt) {
    for (int i = 0; i < 4; ++i) {
        Platform& p = platforms[i];
        drawPlatformBase(p);

        for (auto& coin : p.coins) {
            if (!coin.collected) drawCoin(coin.x, coin.y, coin.z);
        }

        if (!p.allCollected) {
            bool all = true;
            for (auto& coin : p.coins) if (!coin.collected) { all = false; break; }
            p.allCollected = all;
        }

        if (p.allCollected && p.animPlaying) p.animT += dt;

        glPushMatrix();
        if (i == 0) {
            if (p.allCollected && p.animPlaying) { glTranslatef(p.x, 0, p.z); glRotatef(p.animT * 90.0f, 0, 1, 0); glTranslatef(-p.x, 0, -p.z); }
            drawSushi(p);
        }
        else if (i == 1) {
            glPushMatrix();
            glTranslatef(p.x, 0, p.z);
            float s = 1.0f;
            if (p.allCollected && p.animPlaying) s = 1.0f + 0.2f * sinf(p.animT * 4.0f);
            glScalef(s, s, s);
            glTranslatef(-p.x, 0, -p.z);
            drawNinjaStar(p, p.animT);
            glPopMatrix();
        }
        else if (i == 2) {
            glPushMatrix();
            float up = 0.0f;
            if (p.allCollected && p.animPlaying) up = 0.2f * sinf(p.animT * 3.0f);
            glTranslatef(0, up, 0);
            drawSamuraiSword(p);
            glPopMatrix();
        }
        else if (i == 3) {
            float phase = (p.allCollected && p.animPlaying) ? p.animT : 0.0f;
            drawSmallTemple(p, phase);
        }
        glPopMatrix();
    }
}

void drawWalls() {
    for (BouncingWall& w : walls) w.draw();
}

void updateCoinCollection() {
    for (int i = 0; i < 4; ++i) {
        for (auto& coin : platforms[i].coins) {
            if (!coin.collected) {
                float dx = ninja.x - coin.x;
                float dz = ninja.z - coin.z;
                float dist2 = dx * dx + dz * dz;
                if (dist2 < (ninjaRadius + 0.12f) * (ninjaRadius + 0.12f)) {
                    coin.collected = true;
                    totalCollected++;
                }
            }
        }
    }

    if (!gameWon && totalCollected == totalCoins) {
        gameWon = true; 
    }
}

void Display(void) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    updateCamera();

    drawGround();

    drawWalls();

    static int prevMs = glutGet(GLUT_ELAPSED_TIME);
    int now = glutGet(GLUT_ELAPSED_TIME);
    float dt = (now - prevMs) / 1000.0f;
    prevMs = now;

    float diff = targetYaw - ninja.yaw;
    while (diff > 180.0f) diff -= 360.0f;
    while (diff < -180.0f) diff += 360.0f;
    ninja.yaw += diff * 0.2f; 

    drawPlatformsAndObjects(dt);

    ninja.draw();

    drawHUD();

    glFlush();
}

void Keyboard(unsigned char key, int x, int y) {
    if (gameOver) return; 

    float moveX = 0.0f;
    float moveZ = 0.0f;

    switch (key) {

    case 'w': case 'W': moveZ = -ninjaSpeed; break;
    case 's': case 'S': moveZ =  ninjaSpeed; break;
    case 'a': case 'A': moveX = -ninjaSpeed; break;
    case 'd': case 'D': moveX =  ninjaSpeed; break;

    case '1': currentView = TOP_VIEW; break;
    case '2': currentView = SIDE_VIEW; break;
    case '3': currentView = FRONT_VIEW; break;
    case 'q': case 'Q': currentView = FOLLOW_VIEW; break;

    case 'f': case 'F': if (platforms[0].allCollected) platforms[0].animPlaying = !platforms[0].animPlaying; break;
    case 'g': case 'G': if (platforms[1].allCollected) platforms[1].animPlaying = !platforms[1].animPlaying; break;
    case 'h': case 'H': if (platforms[2].allCollected) platforms[2].animPlaying = !platforms[2].animPlaying; break;
    case 'j': case 'J': if (platforms[3].allCollected) platforms[3].animPlaying = !platforms[3].animPlaying; break;
    }

    if (moveX != 0.0f || moveZ != 0.0f) {
        moveNinja(moveX, moveZ);
        updateCoinCollection();
    }

    glutPostRedisplay();
}

void SpecialKeys(int key, int x, int y) {
    if (gameOver) return; 

    float moveX = 0.0f;
    float moveZ = 0.0f;

    switch (key) {
    case GLUT_KEY_UP:    moveZ = -ninjaSpeed; break;
    case GLUT_KEY_DOWN:  moveZ =  ninjaSpeed; break;
    case GLUT_KEY_LEFT:  moveX = -ninjaSpeed; break;
    case GLUT_KEY_RIGHT: moveX =  ninjaSpeed; break;
    }

    if (moveX != 0.0f || moveZ != 0.0f) {
        moveNinja(moveX, moveZ);
        updateCoinCollection();
    }

    glutPostRedisplay();
}

void Mouse(int button, int state, int x, int y) {
    // Mouse camera controls disabled in follow mode intentionally
}

void MouseMotion(int x, int y) { }

void PassiveMouseMotion(int x, int y) { }

void Anim() {
    int now = glutGet(GLUT_ELAPSED_TIME);
    int dt = now - lastTickMs;
    lastTickMs = now;
    if (!gameOver && !gameWon) {
        timeLeftMs -= dt;
        if (timeLeftMs <= 0) {
            timeLeftMs = 0;
            gameOver = true; 
            for (int i = 0; i < 4; ++i) platforms[i].animPlaying = false;
        }
    }

    rotAng += 0.01f;
    glutPostRedisplay();
}

void drawWalls(); 

void initGL() {
    glEnable(GL_DEPTH_TEST);
    glClearColor(1.0f, 1.0f, 1.0f, 0.0f);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0f, 1.0f, 0.1f, 700.0f);

    glMatrixMode(GL_MODELVIEW);
}

void main(int argc, char** argv) {
    glutInit(&argc, argv);

    glutInitWindowSize(700, 700);
    glutInitWindowPosition(150, 150);

    glutCreateWindow("Ancient Ninja Warriors");

    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB | GLUT_DEPTH);

    initGL();
    initScene();

    glutDisplayFunc(Display);
    glutIdleFunc(Anim);
    glutKeyboardFunc(Keyboard);
    glutSpecialFunc(SpecialKeys);
    glutMouseFunc(Mouse);
    glutMotionFunc(MouseMotion);
    glutPassiveMotionFunc(PassiveMouseMotion);

    glutMainLoop();
}
