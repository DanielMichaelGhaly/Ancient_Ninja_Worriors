#include <stdlib.h>
#include <glut.h>
#include <BouncingWall.h>
#include <NinjaPlayer.cpp>
#include <cmath>
#include <vector>
#include <string>
#include <ctime>
#ifdef _WIN32
#include <windows.h>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")
#endif

float rotAng = 0.0f;
static const float ARENA_MIN_X = -10.0f;
static const float ARENA_MAX_X = 10.0f;
static const float ARENA_MIN_Z = -10.0f;
static const float ARENA_MAX_Z = 10.0f;

BouncingWall wallFront(0.0f, 0.5f, ARENA_MIN_Z, 20.0f, 3.0f, 0.4f);
BouncingWall wallBack(0.0f, 0.5f, ARENA_MAX_Z, 20.0f, 3.0f, 0.4f);
BouncingWall wallLeft(ARENA_MIN_X, 0.5f, 0.0f, 0.4f, 3.0f, 20.0f);
BouncingWall wallRight(ARENA_MAX_X, 0.5f, 0.0f, 0.4f, 3.0f, 20.0f);
BouncingWall walls[] = { wallFront, wallBack, wallLeft, wallRight };

NinjaPlayer ninja;

float camOffsetY = 2.5f;
float camOffsetZ = 5.0f;
float camYawOffset = 0.0f;
float camMinY = 1.5f, camMaxY = 5.0f;
float camMinDist = 4.0f, camMaxDist = 12.0f;
float camFollowStep = 0.3f;

float freeCamX = 0.0f, freeCamY = 3.0f, freeCamZ = 15.0f;
float freeCamYaw = 0.0f, freeCamPitch = -10.0f;
float freeMoveSpeed = 7.0f;
float freeMouseSensitivity = 0.2f;
bool freeMouseRotate = false;
float freeMouseDollyFactor = 0.06f; 
float freeMouseStrafeFactor = 0.04f; 

float ninjaSpeed = 3.0f;
float accel = 18.0f;
float decel = 14.0f;
float velX = 0.0f, velZ = 0.0f;
float targetYaw = 0.0f;

bool keyW = false, keyA = false, keyS = false, keyD = false;
bool keyUp = false, keyDown = false, keyLeft = false, keyRight = false;
bool camMoveUp = false, camMoveDown = false, camMoveLeft = false, camMoveRight = false, camMoveForward = false, camMoveBack = false;

int lastMouseX = 0, lastMouseY = 0;
bool mouseOrbit = false;
bool mousePan = false;
float mouseSensitivityOrbit = 0.35f;
float mouseSensitivityZoom = 0.025f;
float ninjaRadius = 0.35f;

enum CameraView { FREE_VIEW, FOLLOW_VIEW, TOP_VIEW, SIDE_VIEW, FRONT_VIEW };
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
static float platformTopY(const Platform& p) { return p.y + 0.11f; }

Platform platforms[4];
int totalCoins = 0, totalCollected = 0;
bool gameWon = false, gameOver = false;
int timeLeftMs = 90 * 1000;
int lastTickMs = 0;
float objectRadii[4] = { 0.65f,0.70f,0.60f,0.85f };

#ifdef _WIN32
bool audioInitialized = false;
bool winSoundPlayed = false;
bool loseSoundPlayed = false;
bool audioMCI = false;
static void mciSafe(const char* cmd) {
    mciSendStringA(cmd, NULL, 0, NULL);
}
static bool mciTryOpen(const char* file, const char* alias) {
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "open \"%s\" type waveaudio alias %s", file, alias);
    MCIERROR e = mciSendStringA(cmd, NULL, 0, NULL);
    return e == 0;
}
void initAudio() {
    if (audioInitialized) return;
    audioInitialized = true;
    audioMCI = mciTryOpen("background_music.wav", "bgm") &&
        mciTryOpen("collect_sound.wav", "collect") &&
        mciTryOpen("win_sound.wav", "win") &&
        mciTryOpen("lose_sound.wav", "lose");
    if (audioMCI) {
        mciSafe("play bgm repeat");
    }
    else {
        PlaySound(L"background_music.wav", NULL, SND_FILENAME | SND_ASYNC | SND_LOOP);
    }
}
void playCollect() {
    if (!audioInitialized) return;
    if (audioMCI) {
        mciSafe("seek collect to start");
        mciSafe("play collect");
    }
    else {
        PlaySound(L"collect_sound.wav", NULL, SND_FILENAME | SND_ASYNC);
        PlaySound(L"background_music.wav", NULL, SND_FILENAME | SND_ASYNC | SND_LOOP);
    }
}
void playWin() {
    if (!audioInitialized || winSoundPlayed) return;
    winSoundPlayed = true;
    if (audioMCI) {
        mciSafe("seek win to start");
        mciSafe("play win");
    }
    else {
        PlaySound(L"win_sound.wav", NULL, SND_FILENAME | SND_ASYNC);
    }
}
void playLose() {
    if (!audioInitialized || loseSoundPlayed) return;
    loseSoundPlayed = true;
    if (audioMCI) {
        mciSafe("seek lose to start");
        mciSafe("play lose");
    }
    else {
        PlaySound(L"lose_sound.wav", NULL, SND_FILENAME | SND_ASYNC);
    }
}
void shutdownAudio() {
    if (!audioInitialized) return;
    if (audioMCI) {
        mciSafe("stop bgm");
        mciSafe("close bgm");
        mciSafe("close collect");
        mciSafe("close win");
        mciSafe("close lose");
    }
    audioInitialized = false;
}
#else
void initAudio() {}
void playCollect() {}
void playWin() {}
void playLose() {}
void shutdownAudio() {}
#endif

float clampf(float v, float a, float b) {
    return v < a ? a : (v > b ? b : v);
}
float frand(float a, float b) {
    return a + (b - a) * (rand() / (float)RAND_MAX);
}

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

    glColor3f(0, 1, 1);
    snprintf(buf, sizeof(buf), "Time %02d:%02d", mm, ss);
    drawText(0.02f, 0.96f, buf);
    snprintf(buf, sizeof(buf), "Coins %d/%d", totalCollected, totalCoins);
    drawText(0.02f, 0.92f, buf);

    drawText(0.02f, 0.88f, "Views: C Free | Q Follow | 1 Top | 2 Side | 3 Front (Mouse disabled)");
    drawText(0.02f, 0.84f, "Free Cam: y/u fwd/back i/o left/right t/b up/down");
    drawText(0.02f, 0.80f, "Ninja: WASD / Arrows (relative) – collides with center objects");

    glColor3f(platforms[0].r, platforms[0].g, platforms[0].b);
    drawText(0.02f, 0.76f, "P1 Sushi F");
    glColor3f(platforms[1].r, platforms[1].g, platforms[1].b);
    drawText(0.12f, 0.76f, "P2 Star G");
    glColor3f(platforms[2].r, platforms[2].g, platforms[2].b);
    drawText(0.22f, 0.76f, "P3 Sword H");
    glColor3f(platforms[3].r, platforms[3].g, platforms[3].b);
    drawText(0.34f, 0.76f, "P4 Temple J");

    if (gameWon) {
        glColor3f(0, 0.7f, 0);
        drawText(0.40f, 0.96f, "WIN");
    }
    if (gameOver && !gameWon) {
        glColor3f(0.8f, 0, 0);
        drawText(0.40f, 0.96f, "LOSE");
    }

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

static void drawSolidCylinder(float radius, float height, int slices = 24, int stacks = 1) {
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

void drawPlatformBase(const Platform& p) {
    glPushMatrix();
    glTranslatef(p.x, p.y, p.z);
    glColor3f(p.r, p.g, p.b);

    glPushMatrix();
    glScalef(p.w, 0.18f, p.d);
    glutSolidCube(1);
    glPopMatrix();

    glColor3f(p.r * 0.6f, p.g * 0.6f, p.b * 0.6f);
    glPushMatrix();
    glTranslatef(0, 0.10f, 0);
    glScalef(p.w * 0.92f, 0.02f, p.d * 0.92f);
    glutSolidCube(1);
    glPopMatrix();

    glColor3f(p.r * 0.4f, p.g * 0.4f, p.b * 0.4f);
    float postH = 0.22f;
    for (int sx = -1; sx <= 1; sx += 2) {
        for (int sz = -1; sz <= 1; sz += 2) {
            glPushMatrix();
            glTranslatef(sx * (p.w * 0.46f), 0, sz * (p.d * 0.46f));
            glRotatef(-90, 1, 0, 0);
            drawSolidCylinder(0.045f, postH);
            glPopMatrix();
        }
    }
    glPopMatrix();
}

// Helper (place near other statics if you want reuse)
static void drawLantern(float r, float g, float b) {
    glPushMatrix();
    glColor3f(r * 0.85f, g * 0.85f, b * 0.85f);
    glRotatef(-90, 1, 0, 0);
    drawSolidCylinder(0.045f, 0.11f);
    glColor3f(r, g, b);
    glutSolidSphere(0.05f, 16, 16);
    glTranslatef(0, 0, 0.11f);
    glColor3f(r * 0.65f, g * 0.65f, b * 0.65f);
    glutSolidSphere(0.038f, 12, 12);
    glPopMatrix();
}

// Replace drawSushi
void drawSushi(const Platform& p) {
    float top = platformTopY(p);
    glPushMatrix();
    glTranslatef(p.x, top + 0.02f, p.z);

    // Plate
    glPushMatrix();
    glColor3f(0.93f, 0.93f, 0.95f);
    glRotatef(-90, 1, 0, 0);
    drawSolidCylinder(0.70f, 0.04f);
    glPopMatrix();

    // Outer rice ring (slight roughness via alternating tiny cubes)
    glPushMatrix();
    glColor3f(0.95f, 0.95f, 0.96f);
    for (int i = 0; i < 24; ++i) {
        float ang = (3.14159265f * 2.0f / 24) * i;
        float r = 0.46f;
        glPushMatrix();
        glTranslatef(sinf(ang) * r, 0.09f, cosf(ang) * r);
        float s = 0.14f + 0.02f * ((i % 2) ? 1 : -1);
        glScalef(s, 0.12f, s);
        glutSolidCube(1);
        glPopMatrix();
    }
    glPopMatrix();

    // Seaweed wrap
    glColor3f(0.06f, 0.12f, 0.11f);
    glPushMatrix();
    glRotatef(-90, 1, 0, 0);
    drawSolidCylinder(0.42f, 0.22f);
    glPopMatrix();

    // Inner rice core
    glColor3f(0.97f, 0.97f, 0.97f);
    glPushMatrix();
    glRotatef(-90, 1, 0, 0);
    drawSolidCylinder(0.33f, 0.18f);
    glPopMatrix();

    // Topping assortment (fish slices + roe + wasabi dab)
    glPushMatrix();
    glTranslatef(0, 0.24f, 0);
    // Fish slice stack
    for (int i = 0; i < 3; ++i) {
        glPushMatrix();
        glTranslatef(-0.10f + i * 0.10f, 0.0f, -0.03f + 0.025f * i);
        glRotatef(12.0f * i, 0, 1, 0);
        glColor3f(0.92f - 0.05f * i, 0.38f + 0.04f * i, 0.26f);
        glScalef(0.22f, 0.05f, 0.10f);
        glutSolidCube(1);
        glPopMatrix();
    }
    // Roe cluster
    for (int r = 0; r < 12; ++r) {
        float ang = (2.0f * 3.14159265f / 12) * r;
        glPushMatrix();
        glTranslatef(sinf(ang) * 0.16f, 0.015f, cosf(ang) * 0.12f);
        glColor3f(0.98f, 0.45f, 0.20f);
        glutSolidSphere(0.035f, 14, 14);
        glPopMatrix();
    }
    // Wasabi
    glPushMatrix();
    glTranslatef(0.18f, 0.01f, 0.10f);
    glColor3f(0.36f, 0.75f, 0.32f);
    glutSolidSphere(0.06f, 14, 14);
    glPopMatrix();
    glPopMatrix();

    // Chopsticks on plate edge
    glPushMatrix();
    glTranslatef(0.0f, 0.05f, 0.58f);
    glRotatef(6, 0, 1, 0);
    for (int c = 0; c < 2; ++c) {
        glPushMatrix();
        glTranslatef(0.06f * c, 0, 0);
        glColor3f(0.55f, 0.42f, 0.25f);
        glRotatef(-90, 1, 0, 0);
        drawSolidCylinder(0.015f, 0.80f);
        glPopMatrix();
    }
    glPopMatrix();

    glPopMatrix();
}

// Replace drawSmallTemple
void drawSmallTemple(const Platform& p, float phase) {
    float top = platformTopY(p);
    glPushMatrix();
    glTranslatef(p.x, top + 0.02f, p.z);

    // Animated subtle lift if phase>0
    float animLift = (phase > 0.0f) ? 0.06f * sinf(phase * 2.2f) : 0.0f;

    // Base foundation & stairs
    glPushMatrix();
    glColor3f(0.52f, 0.47f, 0.42f);
    glScalef(0.90f, 0.08f, 0.90f);
    glutSolidCube(1);
    glPopMatrix();

    for (int s = 0; s < 3; ++s) {
        glPushMatrix();
        glTranslatef(0, 0.04f + s * 0.025f, 0.42f - s * 0.05f);
        glColor3f(0.45f - s * 0.03f, 0.40f - s * 0.03f, 0.35f - s * 0.03f);
        glScalef(0.50f + 0.10f * s, 0.025f, 0.26f);
        glutSolidCube(1);
        glPopMatrix();
    }

    // Pillars
    float pillarH = 0.40f;
    float pillarR = 0.055f;
    for (int sx = -1; sx <= 1; sx += 2) {
        for (int sz = -1; sz <= 1; sz += 2) {
            glPushMatrix();
            glTranslatef(sx * 0.32f, 0.04f, sz * 0.32f);
            glColor3f(0.50f, 0.44f, 0.38f);
            glRotatef(-90, 1, 0, 0);
            drawSolidCylinder(pillarR, pillarH);
            glPopMatrix();
        }
    }

    // Mid platform
    glPushMatrix();
    glTranslatef(0, 0.04f + pillarH, 0);
    glColor3f(0.46f, 0.41f, 0.36f);
    glScalef(0.70f, 0.05f, 0.70f);
    glutSolidCube(1);
    glPopMatrix();

    // Roof layered (with animated vertical breathing)
    float roofBaseY = 0.04f + pillarH + animLift;
    int layers = 3;
    for (int L = 0; L < layers; ++L) {
        float y = roofBaseY + 0.10f * L;
        float scale = 0.78f - 0.16f * L;
        glPushMatrix();
        glTranslatef(0, y, 0);
        glColor3f(0.35f + 0.05f * L, 0.18f + 0.02f * L, 0.18f);
        glRotatef(-90, 1, 0, 0);
        glutSolidCone(scale, 0.18f, 20, 1);
        glPopMatrix();
    }

    // Roof finial
    glPushMatrix();
    glTranslatef(0, roofBaseY + 0.10f * layers + 0.04f, 0);
    glColor3f(0.88f, 0.70f, 0.42f);
    glutSolidSphere(0.06f, 16, 16);
    glPopMatrix();

    // Hanging lanterns (pulse brightness when collecting)
    float glowPhase = (phase > 0.0f) ? (0.5f + 0.5f * sinf(phase * 4.0f)) : 0.4f;
    for (int side = -1; side <= 1; side += 2) {
        glPushMatrix();
        glTranslatef(side * 0.48f, 0.04f + pillarH - 0.06f, 0.10f);
        glColor3f(0.40f, 0.30f, 0.25f);
        glRotatef(-90, 1, 0, 0);
        drawSolidCylinder(0.012f, 0.06f);
        glPopMatrix();

        glPushMatrix();
        glTranslatef(side * 0.48f, 0.04f + pillarH - 0.08f, 0.10f);
        float cr = 0.90f * glowPhase;
        float cg = 0.78f * glowPhase;
        float cb = 0.55f * glowPhase;
        drawLantern(cr, cg, cb);
        glPopMatrix();
    }

    // Banners
    for (int s = -1; s <= 1; s += 2) {
        glPushMatrix();
        glTranslatef(s * 0.60f, 0.04f + 0.12f, -0.35f);
        glColor3f(0.55f, 0.10f + 0.25f * (phase > 0.0f), 0.10f);
        glScalef(0.10f, 0.28f, 0.01f);
        glutSolidCube(1);
        glPopMatrix();
    }

    // Central incense stand
    glPushMatrix();
    glTranslatef(0, 0.04f + pillarH + 0.02f, 0.18f);
    glColor3f(0.42f, 0.33f, 0.26f);
    glRotatef(-90, 1, 0, 0);
    drawSolidCylinder(0.025f, 0.10f);
    glPopMatrix();

    glPopMatrix();
}

void drawNinjaStar(const Platform& p, float animT) {
    float top = platformTopY(p);
    glPushMatrix(); glTranslatef(p.x, top + 1.0f, p.z);
    glColor3f(0.55f, 0.55f, 0.6f); glutSolidTorus(0.04f, 0.26f, 18, 30);
    glColor3f(0.75f, 0.75f, 0.8f); glutSolidSphere(0.085f, 18, 18);
    glColor3f(0.85f, 0.85f, 0.9f); for (int i = 0; i < 4; ++i) { glPushMatrix(); glRotatef(90 * i, 0, 1, 0); glTranslatef(0.28f, 0, 0); glRotatef(90, 0, 0, 1); glutSolidCone(0.075f, 0.40f, 12, 1); glPopMatrix(); }
    glColor3f(0.4f, 0.4f, 0.45f); glutWireTorus(0.02f, 0.33f, 10, 28);
    glColor3f(0.6f, 0.6f, 0.65f); for (int i = 0; i < 8; i++) { glPushMatrix(); glRotatef(45 * i, 0, 1, 0); glTranslatef(0.18f, 0, 0); glRotatef(90, 0, 0, 1); glutSolidCone(0.04f, 0.18f, 10, 1); glPopMatrix(); }
    glPopMatrix();
}

void drawSamuraiSword(const Platform& p) {
    float top = platformTopY(p);
    glPushMatrix(); glTranslatef(p.x, top + 0.5f, p.z); glRotatef(18, 0, 1, 0);
    glColor3f(0.88f, 0.88f, 0.93f); glPushMatrix(); glScalef(0.10f, 0.05f, 1.25f); glutSolidCube(1); glPopMatrix();
    glColor3f(0.7f, 0.7f, 0.75f); glPushMatrix(); glScalef(0.05f, 0.018f, 1.20f); glutSolidCube(1); glPopMatrix();
    glColor3f(0.35f, 0.28f, 0.22f); glPushMatrix(); glTranslatef(0, 0, -0.38f); glRotatef(-90, 1, 0, 0); drawSolidCylinder(0.085f, 0.03f); glPopMatrix();
    glColor3f(0.22f, 0.16f, 0.12f); glPushMatrix(); glTranslatef(0, 0, -0.56f); glRotatef(-90, 1, 0, 0); drawSolidCylinder(0.052f, 0.28f); glPopMatrix();
    glColor3f(0.42f, 0.33f, 0.26f); glPushMatrix(); glTranslatef(0, 0, -0.72f); glutSolidSphere(0.05f, 14, 14); glPopMatrix();
    glColor3f(0.3f, 0.25f, 0.2f); glPushMatrix(); glTranslatef(0, 0, -0.30f); glRotatef(90, 1, 0, 0); glutSolidTorus(0.015f, 0.11f, 12, 20); glPopMatrix();
    glColor3f(0.5f, 0.4f, 0.3f); for (int s = -1; s <= 1; s += 2) { glPushMatrix(); glTranslatef(0.07f * s, 0.015f, -0.30f); glScalef(0.04f, 0.02f, 0.16f); glutSolidCube(1); glPopMatrix(); }
    glPopMatrix();
}



void drawCoin(float x, float y, float z) {
    glPushMatrix();
    glTranslatef(x, y, z);
    glRotatef(-90, 1, 0, 0);
    glColor3f(0.95f, 0.82f, 0.15f);
    drawSolidCylinder(0.12f, 0.045f, 24, 1);
    glPopMatrix();
}

void drawGround() {
    glPushMatrix();
    glTranslatef(0, -1.0f, 0);

    glColor3f(0.42f, 0.45f, 0.48f);
    glPushMatrix();
    glScalef(60.0f, 0.2f, 60.0f);
    glutSolidCube(1);
    glPopMatrix();

    glColor3f(0.38f, 0.40f, 0.42f);
    glBegin(GL_LINES);
    for (int i = -30; i <= 30; i++) {
        glVertex3f(i, 0.11f, -30);
        glVertex3f(i, 0.11f, 30);
        glVertex3f(-30, 0.11f, i);
        glVertex3f(30, 0.11f, i);
    }
    glEnd();

    glPopMatrix();
}

void drawWalls() {
    for (int i = 0; i < 4; i++) {
        glPushMatrix();
        float tint = 0.20f + 0.04f * i;
        glColor3f(0.25f + tint, 0.27f + tint, 0.29f + tint);
        walls[i].draw();
        glPopMatrix();

        glPushMatrix();
        glColor3f(0.18f, 0.18f, 0.19f);
        if (i < 2) {
            glTranslatef(0, 2.1f, (i == 0 ? ARENA_MIN_Z : ARENA_MAX_Z));
            glScalef(20.2f, 0.15f, 0.45f);
        }
        else {
            glTranslatef((i == 2 ? ARENA_MIN_X : ARENA_MAX_X), 2.1f, 0);
            glScalef(0.45f, 0.15f, 20.2f);
        }
        glutSolidCube(1);
        glPopMatrix();
    }
}

void updateCamera() {
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    if (currentView == FREE_VIEW) {
        float yawRad = freeCamYaw * 3.14159265f / 180.0f;
        float pitchRad = freeCamPitch * 3.14159265f / 180.0f;
        float fx = sinf(yawRad) * cosf(pitchRad);
        float fy = sinf(pitchRad);
        float fz = cosf(yawRad) * cosf(pitchRad);
        gluLookAt(freeCamX, freeCamY, freeCamZ, freeCamX + fx, freeCamY + fy, freeCamZ + fz, 0, 1, 0);
        return;
    }

    if (currentView == FOLLOW_VIEW) {
        float yawRad = (ninja.yaw + camYawOffset) * 3.14159265f / 180.0f;
        float dist = camOffsetZ;
        float camX = ninja.x - sinf(yawRad) * dist;
        float camZ = ninja.z - cosf(yawRad) * dist;
        float camY = ninja.y + camOffsetY;
        gluLookAt(camX, camY, camZ, ninja.x, ninja.y + 0.6f, ninja.z, 0, 1, 0);
        return;
    }

    switch (currentView) {
    case TOP_VIEW:
        gluLookAt(ninja.x, ninja.y + 20.0f, ninja.z, ninja.x, ninja.y, ninja.z, 0, 0, -1);
        break;
    case SIDE_VIEW:
        gluLookAt(ninja.x + 18.0f, ninja.y + 3.0f, ninja.z, ninja.x, ninja.y + 1.0f, ninja.z, 0, 1, 0);
        break;
    case FRONT_VIEW:
        gluLookAt(ninja.x, ninja.y + 3.0f, ninja.z + 18.0f, ninja.x, ninja.y + 1.0f, ninja.z, 0, 1, 0);
        break;
    default:
        break;
    }
}

static float groundTopY() { return -0.9f; } // based on ground cube centered at -1.0 with half-height 0.1
static bool pointOnPlatform(const Platform& p, float x, float z) {
    return x >= p.x - p.w * 0.5f && x <= p.x + p.w * 0.5f && z >= p.z - p.d * 0.5f && z <= p.z + p.d * 0.5f;
}
static float surfaceTopAt(float x, float z) {
    for (int i = 0; i < 4; i++) { if (pointOnPlatform(platforms[i], x, z)) return platformTopY(platforms[i]); }
    return groundTopY();
}
static float ninjaFootOffset() { return 0.85f * ninja.scale; } // derived from internal model leg chain length

void initScene() {
    srand((unsigned int)time(nullptr));
    float py = -0.8f;
    platforms[0] = { -5.0f,py,-5.0f,4.5f,3.6f,0.75f,0.25f,0.25f };
    platforms[1] = { 5.0f,py,-4.0f,3.6f,4.4f,0.25f,0.65f,0.85f };
    platforms[2] = { -4.0f,py, 5.0f,4.4f,3.6f,0.30f,0.75f,0.35f };
    platforms[3] = { 5.0f,py, 5.0f,4.0f,4.0f,0.85f,0.72f,0.25f };

    totalCoins = 0;
    totalCollected = 0;

    const float coinRadius = 0.12f; // matches drawCoin
    const float buffer = 0.28f;     // extra space from central object

    for (int i = 0; i < 4; i++) {
        platforms[i].coins.clear();
        int count = 5;
        for (int c = 0; c < count; c++) {
            float cx, cz; int attempts = 0; const int maxAttempts = 200;
            while (true) {
                cx = frand(platforms[i].x - platforms[i].w * 0.42f, platforms[i].x + platforms[i].w * 0.42f);
                cz = frand(platforms[i].z - platforms[i].d * 0.42f, platforms[i].z + platforms[i].d * 0.42f);
                float dx = cx - platforms[i].x; float dz = cz - platforms[i].z;
                float minDist = objectRadii[i] + coinRadius + buffer; // keep away from center objects
                if (dx * dx + dz * dz >= minDist * minDist) break;
                attempts++;
                if (attempts > maxAttempts) { // fallback: push coin to edge
                    float angle = frand(0.0f, 6.2831853f);
                    float rEdge = objectRadii[i] + coinRadius + buffer;
                    cx = platforms[i].x + cosf(angle) * rEdge;
                    cz = platforms[i].z + sinf(angle) * rEdge;
                    break;
                }
            }
            float cy = platforms[i].y + 0.16f;
            platforms[i].coins.push_back({ cx,cy,cz,false });
            totalCoins++;
        }
        platforms[i].allCollected = false;
        platforms[i].animPlaying = false;
        platforms[i].animT = 0.0f;
    }

    ninja.x = 0; ninja.z = 0; ninja.y = -0.5f; ninja.scale = 0.35f; ninja.yaw = 0; targetYaw = 0; velX = velZ = 0;
    gameWon = false; gameOver = false; timeLeftMs = 90 * 1000; lastTickMs = glutGet(GLUT_ELAPSED_TIME);
    for (int i = 0; i < 4; i++) walls[i].setColor(0.28f, 0.30f, 0.32f);
    winSoundPlayed = false; loseSoundPlayed = false;
    //postInitAdjustNinjaY();
}

void updateCoinCollection() {
    for (int i = 0; i < 4; i++) {
        for (auto& coin : platforms[i].coins) {
            if (!coin.collected) {
                float dx = ninja.x - coin.x;
                float dz = ninja.z - coin.z;
                if (dx * dx + dz * dz < (ninjaRadius + 0.12f) * (ninjaRadius + 0.12f)) {
                    coin.collected = true;
                    totalCollected++;
                    playCollect();
                }
            }
        }
        if (!platforms[i].allCollected) {
            bool all = true;
            for (auto& coin : platforms[i].coins) {
                if (!coin.collected) { all = false; break; }
            }
            platforms[i].allCollected = all;
            if (all) platforms[i].animPlaying = true;
        }
    }
    if (!gameWon && totalCollected == totalCoins) {
        gameWon = true;
        playWin();
    }
}

void drawPlatformsAndObjects(float dt) {
    for (int i = 0; i < 4; i++) {
        Platform& p = platforms[i];
        drawPlatformBase(p);

        for (auto& coin : p.coins) {
            if (!coin.collected) drawCoin(coin.x, coin.y, coin.z);
        }

        if (p.allCollected && p.animPlaying) p.animT += dt;

        glPushMatrix();
        if (i == 0) {
            if (p.allCollected && p.animPlaying) {
                glTranslatef(p.x, 0, p.z);
                glRotatef(p.animT * 90.0f, 0, 1, 0);
                glTranslatef(-p.x, 0, -p.z);
            }
            drawSushi(p);
        }
        else if (i == 1) {
            glPushMatrix();
            glTranslatef(p.x, 0, p.z);
            float s = 1.0f;
            if (p.allCollected && p.animPlaying) s = 1.0f + 0.25f * sinf(p.animT * 4.0f);
            glScalef(s, s, s);
            glTranslatef(-p.x, 0, -p.z);
            drawNinjaStar(p, p.animT);
            glPopMatrix();
        }
        else if (i == 2) {
            glPushMatrix();
            float up = 0.0f;
            if (p.allCollected && p.animPlaying) up = 0.25f * sinf(p.animT * 3.0f);
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

void integrateMovement(float dt) {
    if (gameOver) return;

    float localF = 0.0f, localR = 0.0f;
    if (keyW || keyUp) localF += 1.0f;
    if (keyS || keyDown) localF -= 1.0f;
    if (keyD || keyRight) localR -= 1.0f;
    if (keyA || keyLeft) localR += 1.0f;

    float dirX = 0.0f, dirZ = 0.0f;
    if (localF != 0.0f || localR != 0.0f) {
        float yawRad = ninja.yaw * 3.14159265f / 180.0f;
        dirX = sinf(yawRad) * localF + cosf(yawRad) * localR;
        dirZ = cosf(yawRad) * localF - sinf(yawRad) * localR;
        float len = sqrtf(dirX * dirX + dirZ * dirZ);
        if (len > 0) { dirX /= len; dirZ /= len; }
        velX += dirX * accel * dt;
        velZ += dirZ * accel * dt;
        float speed = sqrtf(velX * velX + velZ * velZ);
        if (speed > ninjaSpeed) { velX *= ninjaSpeed / speed; velZ *= ninjaSpeed / speed; }
        if (!(localF < 0 && fabs(localR) < 1e-6f))
            targetYaw = atan2f(velX, velZ) * 180.0f / 3.14159265f;
        ninja.movementMagnitude = speed / ninjaSpeed;
    }
    else {
        float speed = sqrtf(velX * velX + velZ * velZ);
        if (speed > 0) {
            float drop = decel * dt;
            float newSpeed = speed - drop;
            if (newSpeed < 0)newSpeed = 0;
            float scale = newSpeed / (speed + 1e-6f);
            velX *= scale; velZ *= scale;
        }
        ninja.movementMagnitude = 0.0f;
    }

    float newX = ninja.x + velX * dt;
    float newZ = ninja.z + velZ * dt;
    newX = clampf(newX, ARENA_MIN_X + ninjaRadius, ARENA_MAX_X - ninjaRadius);
    newZ = clampf(newZ, ARENA_MIN_Z + ninjaRadius, ARENA_MAX_Z - ninjaRadius);

    bool blocked = false;
    for (int i = 0; i < 4; i++) {
        float dx = newX - platforms[i].x;
        float dz = newZ - platforms[i].z;
        float rr = (ninjaRadius + objectRadii[i]);
        if (dx * dx + dz * dz < rr * rr) { blocked = true; break; }
    }
    if (!blocked) {
        ninja.x = newX; ninja.z = newZ;
    }
    else {
        velX *= 0.3f; velZ *= 0.3f;
    }
    float top = surfaceTopAt(ninja.x, ninja.z);
    float desiredY = top + ninjaFootOffset();
    if (ninja.y < desiredY) ninja.y = desiredY;
    else ninja.y = desiredY;
}
void updateFreeCamera(float dt) {
    if (currentView != FREE_VIEW) return;

    float yawRad = freeCamYaw * 3.14159265f / 180.0f;
    float pitchRad = freeCamPitch * 3.14159265f / 180.0f;
    float forwardX = sinf(yawRad) * cosf(pitchRad);
    float forwardY = sinf(pitchRad);
    float forwardZ = cosf(yawRad) * cosf(pitchRad);
    float rightX = cosf(yawRad);
    float rightZ = -sinf(yawRad);

    float moveX = 0, moveY = 0, moveZ = 0;
    if (camMoveForward) { moveX += forwardX; moveY += forwardY; moveZ += forwardZ; }
    if (camMoveBack) { moveX -= forwardX; moveY -= forwardY; moveZ -= forwardZ; }
    if (camMoveLeft) { moveX -= rightX; moveZ -= rightZ; }
    if (camMoveRight) { moveX += rightX; moveZ += rightZ; }
    if (camMoveUp) moveY += 1.0f;
    if (camMoveDown) moveY -= 1.0f;

    if (camMoveForward || camMoveBack || camMoveLeft || camMoveRight || camMoveUp || camMoveDown) {
        float len = sqrtf(moveX * moveX + moveY * moveY + moveZ * moveZ);
        if (len > 0) {
            moveX /= len; moveY /= len; moveZ /= len;
            freeCamX += moveX * freeMoveSpeed * dt;
            freeCamY += moveY * freeMoveSpeed * dt;
            freeCamZ += moveZ * freeMoveSpeed * dt;
        }
    }
}

void Display(void) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    if (gameOver && !gameWon) {
        glDisable(GL_DEPTH_TEST);

        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();
        gluOrtho2D(0, 1, 0, 1);

        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();

        glBegin(GL_QUADS);
        glColor3f(0.05f, 0.05f, 0.08f);
        glVertex2f(0, 0);
        glVertex2f(1, 0);
        glVertex2f(1, 1);
        glVertex2f(0, 1);
        glEnd();

        glColor3f(0.9f, 0.1f, 0.1f);
        drawText(0.40f, 0.55f, "GAME OVER", GLUT_BITMAP_HELVETICA_18);
        drawText(0.40f, 0.75f, "LOSSEEERRRRRRRR", GLUT_BITMAP_HELVETICA_18);


        glPopMatrix();
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glMatrixMode(GL_MODELVIEW);

        glEnable(GL_DEPTH_TEST);
        glFlush();
        return;
    }
    updateCamera();
    drawGround();
    drawWalls();

    static int prevMs = glutGet(GLUT_ELAPSED_TIME);
    int now = glutGet(GLUT_ELAPSED_TIME);
    float dt = (now - prevMs) / 1000.0f;
    prevMs = now;

    integrateMovement(dt);
    updateFreeCamera(dt);

    float diff = targetYaw - ninja.yaw;
    while (diff > 180) diff -= 360;
    while (diff < -180) diff += 360;
    ninja.yaw += diff * 0.20f;

    drawPlatformsAndObjects(dt);
    ninja.t += dt;

    ninja.draw();
    updateCoinCollection();
    drawHUD();
    glFlush();
}

void Keyboard(unsigned char key, int x, int y) {
    if (gameOver) return;
    switch (key) {
    case 'w': case 'W': keyW = true; break;
    case 's': case 'S': keyS = true; break;
    case 'a': case 'A': keyA = true; break;
    case 'd': case 'D': keyD = true; break;
    case 'q': case 'Q': currentView = FOLLOW_VIEW; break;
    case 'c': case 'C': currentView = FREE_VIEW; break;
    case '1': currentView = TOP_VIEW; break;
    case '2': currentView = SIDE_VIEW; break;
    case '3': currentView = FRONT_VIEW; break;
    case 'f': case 'F': if (platforms[0].allCollected) platforms[0].animPlaying = !platforms[0].animPlaying; break;
    case 'g': case 'G': if (platforms[1].allCollected) platforms[1].animPlaying = !platforms[1].animPlaying; break;
    case 'h': case 'H': if (platforms[2].allCollected) platforms[2].animPlaying = !platforms[2].animPlaying; break;
    case 'j': case 'J': if (platforms[3].allCollected) platforms[3].animPlaying = !platforms[3].animPlaying; break;
    case 't': case 'T': if (currentView == FREE_VIEW) camMoveUp = true; break;
    case 'b': case 'B': if (currentView == FREE_VIEW) camMoveDown = true; break;
    case 'i': case 'I': if (currentView == FREE_VIEW) camMoveLeft = true; break;
    case 'o': case 'O': if (currentView == FREE_VIEW) camMoveRight = true; break;
    case 'y': case 'Y': if (currentView == FREE_VIEW) camMoveForward = true; break;
    case 'u': case 'U': if (currentView == FREE_VIEW) camMoveBack = true; break;
    case '+':
        if (currentView == FOLLOW_VIEW) camOffsetZ = clampf(camOffsetZ - camFollowStep, 2.5f, 10.0f);
        break;
    case '-': 
        if (currentView == FOLLOW_VIEW) camOffsetZ = clampf(camOffsetZ + camFollowStep, 2.5f, 10.0f);
        break;
    case 27: shutdownAudio(); exit(0); break;
    }
    glutPostRedisplay();
}

void KeyboardUp(unsigned char key, int x, int y) {
    switch (key) {
    case 'w': case 'W': keyW = false; break;
    case 's': case 'S': keyS = false; break;
    case 'a': case 'A': keyA = false; break;
    case 'd': case 'D': keyD = false; break;
    case 't': case 'T': camMoveUp = false; break;
    case 'b': case 'B': camMoveDown = false; break;
    case 'i': case 'I': camMoveLeft = false; break;
    case 'o': case 'O': camMoveRight = false; break;
    case 'y': case 'Y': camMoveForward = false; break;
    case 'u': case 'U': camMoveBack = false; break;
    }
}

void SpecialKeys(int key, int x, int y) {
    if (gameOver) return;
    switch (key) {
    case GLUT_KEY_UP: keyUp = true; break;
    case GLUT_KEY_DOWN: keyDown = true; break;
    case GLUT_KEY_LEFT: keyLeft = true; break;
    case GLUT_KEY_RIGHT: keyRight = true; break;
    }
}

void SpecialKeysUp(int key, int x, int y) {
    switch (key) {
    case GLUT_KEY_UP: keyUp = false; break;
    case GLUT_KEY_DOWN: keyDown = false; break;
    case GLUT_KEY_LEFT: keyLeft = false; break;
    case GLUT_KEY_RIGHT: keyRight = false; break;
    }
}

void Mouse(int button, int state, int x, int y) {
    if (gameOver) return; // disable after losing
    if (currentView == FREE_VIEW) {
        if (button == GLUT_LEFT_BUTTON) {
            if (state == GLUT_DOWN) {
                freeMouseRotate = true;
                lastMouseX = x; lastMouseY = y;
            }
            else if (state == GLUT_UP) {
                freeMouseRotate = false;
            }
        }
        // scroll forward/back (buttons 3/4 on some GLUT implementations)
        if (state == GLUT_DOWN) {
            if (button == 3) { // wheel up moves forward
                float yawRad = freeCamYaw * 3.14159265f / 180.0f;
                float pitchRad = freeCamPitch * 3.14159265f / 180.0f;
                freeCamX += sinf(yawRad) * cosf(pitchRad) * 0.8f;
                freeCamY += sinf(pitchRad) * 0.8f;
                freeCamZ += cosf(yawRad) * cosf(pitchRad) * 0.8f;
            }
            else if (button == 4) { // wheel down moves backward
                float yawRad = freeCamYaw * 3.14159265f / 180.0f;
                float pitchRad = freeCamPitch * 3.14159265f / 180.0f;
                freeCamX -= sinf(yawRad) * cosf(pitchRad) * 0.8f;
                freeCamY -= sinf(pitchRad) * 0.8f;
                freeCamZ -= cosf(yawRad) * cosf(pitchRad) * 0.8f;
            }
        }
    }
}
void MouseMotion(int x, int y) {
    if (gameOver) return;

    int dx = x - lastMouseX;
    int dy = y - lastMouseY;

    bool shiftDown = false;
#ifdef _WIN32
    // More reliable on Windows
    shiftDown = ((GetKeyState(VK_LSHIFT) & 0x8000) != 0) || ((GetKeyState(VK_RSHIFT) & 0x8000) != 0);
#endif
    // Also check GLUT modifiers (works on other platforms/impls)
    shiftDown = shiftDown || ((glutGetModifiers() & GLUT_ACTIVE_SHIFT) != 0);

    if (currentView == FREE_VIEW) {
        if (shiftDown) {
            // Dolly (vertical drag) and strafe (horizontal drag)
            float yawRad = freeCamYaw * 3.14159265f / 180.0f;
            float pitchRad = freeCamPitch * 3.14159265f / 180.0f;

            float fx = sinf(yawRad) * cosf(pitchRad);
            float fy = sinf(pitchRad);
            float fz = cosf(yawRad) * cosf(pitchRad);

            float rightX = cosf(yawRad);
            float rightZ = -sinf(yawRad);

            float dolly = -dy * freeMouseDollyFactor;   // up = forward
            float strafe = dx * freeMouseStrafeFactor;  // right

            freeCamX += fx * dolly + rightX * strafe;
            freeCamY += fy * dolly;
            freeCamZ += fz * dolly + rightZ * strafe;
        }
        else if (freeMouseRotate) {
            // Normal rotation when a button is down (e.g., left button)
            freeCamYaw += dx * freeMouseSensitivity;
            freeCamPitch -= dy * freeMouseSensitivity;
            if (freeCamPitch > 89) freeCamPitch = 89;
            if (freeCamPitch < -89) freeCamPitch = -89;
        }
    }

    lastMouseX = x; lastMouseY = y;
    glutPostRedisplay();
}
void PassiveMouseMotion(int x, int y) {
    if (gameOver) return; // track only if not lost
    lastMouseX = x; lastMouseY = y;
}

void Anim() {
    int now = glutGet(GLUT_ELAPSED_TIME);
    int dt = now - lastTickMs;
    lastTickMs = now;
    if (!gameOver && !gameWon) {
        timeLeftMs -= dt;
        if (timeLeftMs <= 0) {
            timeLeftMs = 0;
            gameOver = true;
            for (int i = 0; i < 4; i++) platforms[i].animPlaying = false;
            playLose();
        }
    }
    rotAng += 0.01f;
    glutPostRedisplay();
}

void initGL() {
    glEnable(GL_DEPTH_TEST);
    glClearColor(1, 1, 1, 0);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0f, 1.0f, 0.1f, 300.0f);
    glMatrixMode(GL_MODELVIEW);
}

void main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitWindowSize(900, 900);
    glutInitWindowPosition(120, 60);
    glutCreateWindow("Ancient Ninja Warriors");
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB | GLUT_DEPTH);
    initGL();
    initScene();
    initAudio();
    atexit(shutdownAudio);
    glutDisplayFunc(Display);
    glutIdleFunc(Anim);
    glutKeyboardFunc(Keyboard);
    glutKeyboardUpFunc(KeyboardUp);
    glutSpecialFunc(SpecialKeys);
    glutSpecialUpFunc(SpecialKeysUp);
    glutMouseFunc(Mouse);
    glutMotionFunc(MouseMotion);
    glutPassiveMotionFunc(PassiveMouseMotion);
    glutMainLoop();
}