#include <stdlib.h>
#include <glut.h>
#include <BouncingWall.h>
#include <NinjaPlayer.cpp>
#include <cmath>
#include <vector>
#include <string>
#include <ctime>

// ================= Walls: rectangular boundary =================
// Define a rectangular arena centered at origin in XZ plane
static const float ARENA_MIN_X = -6.0f;
static const float ARENA_MAX_X =  6.0f;
static const float ARENA_MIN_Z = -6.0f;
static const float ARENA_MAX_Z =  6.0f;

BouncingWall wallFront(0.0f, 0.5f, ARENA_MIN_Z, 12.0f, 1.0f, 0.3f);   // along X axis, front wall (towards -Z)
BouncingWall wallBack (0.0f, 0.5f, ARENA_MAX_Z, 12.0f, 1.0f, 0.3f);   // along X axis, back wall
BouncingWall wallLeft (ARENA_MIN_X, 0.5f, 0.0f, 0.3f, 1.0f, 12.0f);   // along Z axis, left wall
BouncingWall wallRight(ARENA_MAX_X, 0.5f, 0.0f, 0.3f, 1.0f, 12.0f);   // along Z axis, right wall

BouncingWall walls[] = {wallFront, wallBack, wallLeft, wallRight};

// ================= Player and camera =================
NinjaPlayer ninja;

float rotAng;

// Third-person follow camera offsets
float camOffsetY = 2.0f;   // height above player
float camOffsetZ = 5.0f;   // distance behind player

float moveSpeed = 0.5f; // legacy camera free move speed (unused in follow)

float ninjaSpeed = 0.18f; // faster player
float targetYaw = 0.0f;   // for smooth turning

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

// ================= Platforms, objects, coins, game state =================
struct Coin {
    float x, y, z;
    bool collected = false;
};

struct Platform {
    float x, y, z; // center
    float w, d;    // size in X and Z
    float r, g, b; // color
    std::vector<Coin> coins;
    bool allCollected = false;
    bool animPlaying = false; // toggled by key
    float animT = 0.0f;       // animation timer
};

// Four platforms inside arena (quadrants)
Platform platforms[4];

// Per-platform object animation unlocked flag is platforms[i].allCollected
// Keys: F,G,H,J toggle playing for P1..P4

// Game state
int totalCoins = 0;
int totalCollected = 0;
bool gameWon = false;
bool gameOver = false;

// Countdown 1:30
int timeLeftMs = 90 * 1000; // 90 seconds
int lastTickMs = 0;

// ================= Utility =================
float clampf(float v, float a, float b) { return v < a ? a : (v > b ? b : v); }

float frand(float a, float b) { return a + (b - a) * (rand() / (float)RAND_MAX); }

void drawText(float x, float y, const std::string& s, void* font = GLUT_BITMAP_HELVETICA_18) {
    glRasterPos2f(x, y);
    for (char c : s) glutBitmapCharacter(font, c);
}

void drawHUD() {
    // Setup 2D orthographic projection
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, 1, 0, 1);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    // Time text
    int secs = timeLeftMs / 1000;
    int mm = secs / 60;
    int ss = secs % 60;
    char buf[128];
    snprintf(buf, sizeof(buf), "Time: %02d:%02d", mm, ss);
    glColor3f(0, 0, 0);
    drawText(0.02f, 0.95f, buf);

    // Collected text
    snprintf(buf, sizeof(buf), "Collected: %d / %d", totalCollected, totalCoins);
    drawText(0.02f, 0.90f, buf);

    // Help overlay
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

    // Restore
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

// ================= Drawing helpers =================
static void drawSolidCylinder(float radius, float height, int slices = 24, int stacks = 1) {
    GLUquadric* q = gluNewQuadric();
    gluCylinder(q, radius, radius, height, slices, stacks);
    glPushMatrix(); glTranslatef(0, 0, height); gluDisk(q, 0, radius, slices, 1); glPopMatrix();
    glPushMatrix(); glRotatef(180, 1, 0, 0);  gluDisk(q, 0, radius, slices, 1); glPopMatrix();
    gluDeleteQuadric(q);
}

// Platforms
void drawPlatformBase(const Platform& p) {
    glPushMatrix();
    glTranslatef(p.x, p.y, p.z);
    glColor3f(p.r, p.g, p.b);
    // Base slab
    glPushMatrix(); glScalef(p.w, 0.2f, p.d); glutSolidCube(1.0f); glPopMatrix();
    // Trim using a thin top layer (second primitive)
    glColor3f(p.r * 0.8f, p.g * 0.8f, p.b * 0.8f);
    glPushMatrix(); glTranslatef(0, 0.12f, 0); glScalef(p.w * 0.95f, 0.02f, p.d * 0.95f); glutSolidCube(1.0f); glPopMatrix();
    glPopMatrix();
}

// Sushi (>=4 primitives): rice(cylinder), nori(cube band), topping(sphere), plate(thin cylinder)
void drawSushi(const Platform& p) {
    glPushMatrix();
    glTranslatef(p.x, p.y + 0.2f, p.z);
    // plate
    glColor3f(0.9f, 0.9f, 0.95f);
    glPushMatrix(); glRotatef(-90, 1, 0, 0); drawSolidCylinder(0.6f, 0.05f); glPopMatrix();
    // rice
    glColor3f(0.95f, 0.95f, 0.95f);
    glPushMatrix(); glRotatef(-90, 1, 0, 0); drawSolidCylinder(0.45f, 0.25f); glPopMatrix();
    // nori band
    glColor3f(0.05f, 0.1f, 0.08f);
    glPushMatrix(); glScalef(0.9f, 0.15f, 0.5f); glutSolidCube(0.6f); glPopMatrix();
    // topping
    glColor3f(0.9f, 0.3f, 0.2f);
    glTranslatef(0, 0.22f, 0);
    glutSolidSphere(0.18f, 18, 18);
    glPopMatrix();
}

// Ninja star (>=4 primitives): hub(sphere), 4 blades(cones), ring(torus)
void drawNinjaStar(const Platform& p, float animT) {
    glPushMatrix();
    glTranslatef(p.x, p.y + 0.25f, p.z);
    // optional rotation animation handled outside
    // ring
    glColor3f(0.6f, 0.6f, 0.65f);
    glutSolidTorus(0.03f, 0.25f, 16, 24);
    // hub
    glColor3f(0.7f, 0.7f, 0.75f);
    glutSolidSphere(0.08f, 16, 16);
    // blades
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

// Samurai sword (>=4 primitives): blade(cube), guard(disk), handle(cylinder), pommel(sphere)
void drawSamuraiSword(const Platform& p) {
    glPushMatrix();
    glTranslatef(p.x, p.y + 0.28f, p.z);
    glRotatef(15, 0, 1, 0);
    // blade
    glColor3f(0.85f, 0.85f, 0.9f);
    glPushMatrix(); glScalef(0.1f, 0.05f, 1.2f); glutSolidCube(1.0f); glPopMatrix();
    // guard
    glColor3f(0.3f, 0.25f, 0.2f);
    glPushMatrix(); glTranslatef(0, 0, -0.35f); glRotatef(-90, 1, 0, 0); drawSolidCylinder(0.08f, 0.02f); glPopMatrix();
    // handle
    glColor3f(0.15f, 0.12f, 0.1f);
    glPushMatrix(); glTranslatef(0, 0, -0.55f); glRotatef(-90, 1, 0, 0); drawSolidCylinder(0.05f, 0.25f); glPopMatrix();
    // pommel
    glColor3f(0.25f, 0.2f, 0.15f);
    glPushMatrix(); glTranslatef(0, 0, -0.7f); glutSolidSphere(0.05f, 12, 12); glPopMatrix();
    glPopMatrix();
}

// Small temple (>=4 primitives): base(cube), 4 pillars(cylinders), roof(cone), cap(sphere)
void drawSmallTemple(const Platform& p, float colorPhase) {
    glPushMatrix();
    glTranslatef(p.x, p.y + 0.25f, p.z);
    // base
    glColor3f(0.6f, 0.55f, 0.5f);
    glPushMatrix(); glScalef(0.8f, 0.08f, 0.8f); glutSolidCube(1.0f); glPopMatrix();
    // pillars
    glColor3f(0.45f, 0.4f, 0.35f);
    float s = 0.32f; float h = 0.35f;
    for (int i = -1; i <= 1; i += 2) {
        for (int j = -1; j <= 1; j += 2) {
            glPushMatrix(); glTranslatef(i * s, 0.04f, j * s); glRotatef(-90, 1, 0, 0); drawSolidCylinder(0.05f, h); glPopMatrix();
        }
    }
    // roof
    float r = 0.3f + 0.2f * (0.5f * (sinf(colorPhase) + 1.0f)); // for color animation
    glColor3f(r, 0.2f, 0.2f);
    glPushMatrix(); glTranslatef(0, 0.04f + h, 0); glRotatef(-90, 1, 0, 0); glutSolidCone(0.55f, 0.35f, 18, 1); glPopMatrix();
    // cap
    glColor3f(0.85f, 0.7f, 0.4f);
    glPushMatrix(); glTranslatef(0, 0.04f + h + 0.35f, 0); glutSolidSphere(0.06f, 12, 12); glPopMatrix();
    glPopMatrix();
}

// Coin (>=3 primitives): rim(cylinder) + 2 faces(disks)
void drawCoin(float x, float y, float z) {
    glPushMatrix();
    glTranslatef(x, y, z);
    glRotatef(-90, 1, 0, 0);
    glColor3f(0.95f, 0.8f, 0.1f);
    drawSolidCylinder(0.12f, 0.04f, 20, 1);
    glPopMatrix();
}

// ================= Scene: ground + walls =================
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

// ================= Camera =================
void updateCamera() {
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    if (currentView == FOLLOW_VIEW) {
        // Smooth yaw for camera to trail behind the ninja
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

// ================= Init platforms and coins =================
void initScene() {
    srand((unsigned int)time(nullptr));

    // Place platforms in quadrants inside the arena
    float py = -0.8f; // slightly above ground (-1)
    platforms[0] = { -3.0f, py, -3.0f, 2.2f, 2.2f, 0.75f, 0.2f, 0.2f };
    platforms[1] = {  3.0f, py, -3.0f, 2.2f, 2.2f, 0.2f, 0.6f, 0.8f };
    platforms[2] = { -3.0f, py,  3.0f, 2.2f, 2.2f, 0.3f, 0.7f, 0.3f };
    platforms[3] = {  3.0f, py,  3.0f, 2.2f, 2.2f, 0.8f, 0.7f, 0.2f };

    // Create 3-5 coins per platform (we choose 4) at random positions on platform top
    totalCoins = 0; totalCollected = 0;
    for (int i = 0; i < 4; ++i) {
        platforms[i].coins.clear();
        int count = 4; // can vary
        for (int c = 0; c < count; ++c) {
            float cx = frand(platforms[i].x - platforms[i].w * 0.4f, platforms[i].x + platforms[i].w * 0.4f);
            float cz = frand(platforms[i].z - platforms[i].d * 0.4f, platforms[i].z + platforms[i].d * 0.4f);
            float cy = platforms[i].y + 0.16f; // slightly above platform
            platforms[i].coins.push_back({ cx, cy, cz, false });
            totalCoins++;
        }
        platforms[i].allCollected = false;
        platforms[i].animPlaying = false;
        platforms[i].animT = 0.0f;
    }

    // Spawn player at arena center
    ninja.x = 0.0f; ninja.z = 0.0f; ninja.y = -0.5f; ninja.scale = 0.4f; ninja.yaw = 0.0f; targetYaw = 0.0f;

    // Reset timer and state
    gameWon = false; gameOver = false; timeLeftMs = 90 * 1000; lastTickMs = glutGet(GLUT_ELAPSED_TIME);
}

// ================= Player movement (with clamp, unlock smooth yaw) =================
void moveNinja(float moveX, float moveZ) {
    if (gameOver) return;

    float newX = ninja.x + moveX;
    float newZ = ninja.z + moveZ;

    // Clamp within arena (consider radius)
    newX = clampf(newX, ARENA_MIN_X + ninjaRadius, ARENA_MAX_X - ninjaRadius);
    newZ = clampf(newZ, ARENA_MIN_Z + ninjaRadius, ARENA_MAX_Z - ninjaRadius);

    ninja.x = newX;
    ninja.z = newZ;

    if (moveX != 0.0f || moveZ != 0.0f) {
        targetYaw = atan2f(moveX, moveZ) * 180.0f / 3.14159265f;
    }
}

// ================= Draw scene =================
void drawPlatformsAndObjects(float dt) {
    for (int i = 0; i < 4; ++i) {
        Platform& p = platforms[i];
        // Base
        drawPlatformBase(p);

        // Coins
        for (auto& coin : p.coins) {
            if (!coin.collected) drawCoin(coin.x, coin.y, coin.z);
        }

        // Progress unlock check
        if (!p.allCollected) {
            bool all = true;
            for (auto& coin : p.coins) if (!coin.collected) { all = false; break; }
            p.allCollected = all;
        }

        // Animate object only if unlocked and playing, advance timer
        if (p.allCollected && p.animPlaying) p.animT += dt;

        // Draw platform-specific object with per-platform animation behavior
        glPushMatrix();
        // Platform 0: rotating object (sushi rotates)
        if (i == 0) {
            if (p.allCollected && p.animPlaying) { glTranslatef(p.x, 0, p.z); glRotatef(p.animT * 90.0f, 0, 1, 0); glTranslatef(-p.x, 0, -p.z); }
            drawSushi(p);
        }
        // Platform 1: scaling object (ninja star pulses)
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
        // Platform 2: translation (sword bobs up/down)
        else if (i == 2) {
            glPushMatrix();
            float up = 0.0f;
            if (p.allCollected && p.animPlaying) up = 0.2f * sinf(p.animT * 3.0f);
            glTranslatef(0, up, 0);
            drawSamuraiSword(p);
            glPopMatrix();
        }
        // Platform 3: color changing (temple roof color varies with time)
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

// ================= Collectibles logic =================
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
        gameWon = true; // continue running per requirements
    }
}

// ================= GLUT callbacks =================
void Display(void) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    updateCamera();

    drawGround();

    drawWalls();

    // Delta time for animations
    static int prevMs = glutGet(GLUT_ELAPSED_TIME);
    int now = glutGet(GLUT_ELAPSED_TIME);
    float dt = (now - prevMs) / 1000.0f;
    prevMs = now;

    // Smoothly interpolate ninja yaw toward target
    float diff = targetYaw - ninja.yaw;
    while (diff > 180.0f) diff -= 360.0f;
    while (diff < -180.0f) diff += 360.0f;
    ninja.yaw += diff * 0.2f; // smoothing factor

    // Platforms, coins, and objects
    drawPlatformsAndObjects(dt);

    // Player
    ninja.draw();

    // HUD overlay
    drawHUD();

    glFlush();
}

void Keyboard(unsigned char key, int x, int y) {
    if (gameOver) return; // controls disabled when game over

    float moveX = 0.0f;
    float moveZ = 0.0f;

    switch (key) {
    // Player movement
    case 'w': case 'W': moveZ = -ninjaSpeed; break;
    case 's': case 'S': moveZ =  ninjaSpeed; break;
    case 'a': case 'A': moveX = -ninjaSpeed; break;
    case 'd': case 'D': moveX =  ninjaSpeed; break;

    // View modes centered on player
    case '1': currentView = TOP_VIEW; break;
    case '2': currentView = SIDE_VIEW; break;
    case '3': currentView = FRONT_VIEW; break;
    case 'q': case 'Q': currentView = FOLLOW_VIEW; break;

    // Toggle animations when unlocked: F,G,H,J for P1..P4
    case 'f': case 'F': if (platforms[0].allCollected) platforms[0].animPlaying = !platforms[0].animPlaying; break;
    case 'g': case 'G': if (platforms[1].allCollected) platforms[1].animPlaying = !platforms[1].animPlaying; break;
    case 'h': case 'H': if (platforms[2].allCollected) platforms[2].animPlaying = !platforms[2].animPlaying; break;
    case 'j': case 'J': if (platforms[3].allCollected) platforms[3].animPlaying = !platforms[3].animPlaying; break;

    case 27: exit(0); break; // ESC
    }

    if (moveX != 0.0f || moveZ != 0.0f) {
        moveNinja(moveX, moveZ);
        updateCoinCollection();
    }

    glutPostRedisplay();
}

void SpecialKeys(int key, int x, int y) {
    if (gameOver) return; // disabled on game over

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
    // Countdown timer
    int now = glutGet(GLUT_ELAPSED_TIME);
    int dt = now - lastTickMs;
    lastTickMs = now;
    if (!gameOver && !gameWon) {
        timeLeftMs -= dt;
        if (timeLeftMs <= 0) {
            timeLeftMs = 0;
            gameOver = true; // freeze controls and pause objects
            // Pause all animations
            for (int i = 0; i < 4; ++i) platforms[i].animPlaying = false;
        }
    }

    rotAng += 0.01f;
    glutPostRedisplay();
}

void drawWalls(); // fwd

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
