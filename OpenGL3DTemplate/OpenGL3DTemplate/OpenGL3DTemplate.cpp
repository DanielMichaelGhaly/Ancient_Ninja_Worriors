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
static const float ARENA_MAX_X =  10.0f;
static const float ARENA_MIN_Z = -10.0f;
static const float ARENA_MAX_Z =  10.0f;

BouncingWall wallFront(0.0f, 0.5f, ARENA_MIN_Z, 20.0f, 3.0f, 0.4f);
BouncingWall wallBack (0.0f, 0.5f, ARENA_MAX_Z, 20.0f, 3.0f, 0.4f);
BouncingWall wallLeft (ARENA_MIN_X, 0.5f, 0.0f, 0.4f, 3.0f, 20.0f);
BouncingWall wallRight(ARENA_MAX_X, 0.5f, 0.0f, 0.4f, 3.0f, 20.0f);
BouncingWall walls[] = { wallFront, wallBack, wallLeft, wallRight };

NinjaPlayer ninja;

float camOffsetY = 2.5f;
float camOffsetZ = 6.5f;
float camYawOffset = 0.0f;
float camMinY = 1.5f, camMaxY = 5.0f;
float camMinDist = 4.0f, camMaxDist = 12.0f;

float freeCamX = 0.0f, freeCamY = 3.0f, freeCamZ = 15.0f;
float freeCamYaw = 0.0f, freeCamPitch = -10.0f;
float freeMoveSpeed = 7.0f;
float freeMouseSensitivity = 0.2f;
bool freeMouseRotate = false;

float ninjaSpeed = 3.0f;
float accel = 18.0f;
float decel = 14.0f;
float velX = 0.0f, velZ = 0.0f;
float targetYaw = 0.0f;

bool keyW=false, keyA=false, keyS=false, keyD=false;
bool keyUp=false, keyDown=false, keyLeft=false, keyRight=false;

int lastMouseX=0, lastMouseY=0; 
bool mouseOrbit=false; 
bool mousePan=false;
float mouseSensitivityOrbit = 0.35f;
float mouseSensitivityZoom = 0.025f;

float ninjaRadius = 0.35f;

enum CameraView { FREE_VIEW, FOLLOW_VIEW, TOP_VIEW, SIDE_VIEW, FRONT_VIEW };
CameraView currentView = FOLLOW_VIEW;

struct Coin { 
    float x, y, z; 
    bool collected=false; 
};

struct Platform { 
    float x, y, z; 
    float w, d; 
    float r, g, b; 
    std::vector<Coin> coins; 
    bool allCollected=false; 
    bool animPlaying=false; 
    float animT=0.0f; 
};
Platform platforms[4];
int totalCoins=0, totalCollected=0; 
bool gameWon=false, gameOver=false;
int timeLeftMs = 90 * 1000; 
int lastTickMs=0;

#ifdef _WIN32
bool audioInitialized = false;
bool winSoundPlayed = false;
bool loseSoundPlayed = false;
static void mciSafe(const char* cmd){ mciSendStringA(cmd, NULL, 0, NULL); }
void initAudio(){
    if(audioInitialized) return; 
    audioInitialized=true; 
    mciSafe("open \"background_music.wav\" type waveaudio alias bgm"); 
    mciSafe("open \"collect_sound.wav\" type waveaudio alias collect"); 
    mciSafe("open \"win_sound.wav\" type waveaudio alias win"); 
    mciSafe("open \"lose_sound.wav\" type waveaudio alias lose"); 
    mciSafe("play bgm repeat"); 
}
void playCollect(){
    if(!audioInitialized) return; 
    mciSafe("seek collect to start"); 
    mciSafe("play collect"); 
}
void playWin(){
    if(!audioInitialized || winSoundPlayed) return; 
    winSoundPlayed=true; 
    mciSafe("seek win to start"); 
    mciSafe("play win"); 
}
void playLose(){
    if(!audioInitialized || loseSoundPlayed) return; 
    loseSoundPlayed=true; 
    mciSafe("seek lose to start"); 
    mciSafe("play lose"); 
}
void shutdownAudio(){
    if(!audioInitialized) return; 
    mciSafe("stop bgm"); 
    mciSafe("close bgm"); 
    mciSafe("close collect"); 
    mciSafe("close win"); 
    mciSafe("close lose"); 
}
#else
void initAudio(){}
void playCollect(){}
void playWin(){}
void playLose(){}
void shutdownAudio(){}
#endif

float clampf(float v,float a,float b){ return v<a?a:(v>b?b:v); } 
float frand(float a,float b){ return a + (b - a) * (rand() / (float)RAND_MAX); } 

void drawText(float x,float y,const std::string& s, void* font=GLUT_BITMAP_HELVETICA_18){
    glRasterPos2f(x,y); 
    for(char c: s) glutBitmapCharacter(font,c); 
}

void drawHUD(){
    glMatrixMode(GL_PROJECTION); 
    glPushMatrix(); 
    glLoadIdentity(); 
    gluOrtho2D(0,1,0,1);
    glMatrixMode(GL_MODELVIEW); 
    glPushMatrix(); 
    glLoadIdentity();

    int secs = timeLeftMs / 1000; 
    int mm = secs / 60; 
    int ss = secs % 60; 
    char buf[128];

    glColor3f(0,0,0); 
    snprintf(buf,sizeof(buf),"Time %02d:%02d",mm,ss); 
    drawText(0.02f,0.96f,buf);

    snprintf(buf,sizeof(buf),"Coins %d/%d", totalCollected, totalCoins); 
    drawText(0.02f,0.92f,buf);

    glColor3f(0,0,0);
    drawText(0.02f,0.88f,"Views: C Free | Q Follow | 1 Top | 2 Side | 3 Front");
    drawText(0.02f,0.84f,"Free Cam: WASD move, QE up/down, Mouse drag rotate, Wheel forward/back");
    drawText(0.02f,0.80f,"Follow: WASD/Arrows move ninja, Mouse orbit/height, F/G/H/J pause anims");

    glColor3f(platforms[0].r,platforms[0].g,platforms[0].b); 
    drawText(0.02f,0.76f,"P1 Sushi F");
    glColor3f(platforms[1].r,platforms[1].g,platforms[1].b); 
    drawText(0.12f,0.76f,"P2 Star G");
    glColor3f(platforms[2].r,platforms[2].g,platforms[2].b); 
    drawText(0.22f,0.76f,"P3 Sword H");
    glColor3f(platforms[3].r,platforms[3].g,platforms[3].b); 
    drawText(0.34f,0.76f,"P4 Temple J");

    if(gameWon){
        glColor3f(0,0.7f,0); 
        drawText(0.40f,0.96f,"WIN"); 
    }
    if(gameOver && !gameWon){
        glColor3f(0.8f,0,0); 
        drawText(0.40f,0.96f,"LOSE"); 
    }

    glPopMatrix(); 
    glMatrixMode(GL_PROJECTION); 
    glPopMatrix(); 
    glMatrixMode(GL_MODELVIEW);
}

static void drawSolidCylinder(float radius,float height,int slices=24,int stacks=1){
    GLUquadric* q = gluNewQuadric(); 
    gluCylinder(q, radius, radius, height, slices, stacks); 
    glPushMatrix(); 
    glTranslatef(0,0,height); 
    gluDisk(q,0,radius,slices,1); 
    glPopMatrix(); 
    glPushMatrix(); 
    glRotatef(180,1,0,0); 
    gluDisk(q,0,radius,slices,1); 
    glPopMatrix(); 
    gluDeleteQuadric(q); 
}

void drawPlatformBase(const Platform& p){
    glPushMatrix(); 
    glTranslatef(p.x,p.y,p.z); 
    glColor3f(p.r,p.g,p.b);

    glPushMatrix(); 
    glScalef(p.w,0.18f,p.d); 
    glutSolidCube(1); 
    glPopMatrix();

    glColor3f(p.r*0.6f,p.g*0.6f,p.b*0.6f); 
    glPushMatrix(); 
    glTranslatef(0,0.10f,0); 
    glScalef(p.w*0.92f,0.02f,p.d*0.92f); 
    glutSolidCube(1); 
    glPopMatrix();

    glColor3f(p.r*0.4f,p.g*0.4f,p.b*0.4f); 
    float postH=0.22f; 
    for(int sx=-1; sx<=1; sx+=2){
        for(int sz=-1; sz<=1; sz+=2){ 
            glPushMatrix(); 
            glTranslatef(sx*(p.w*0.46f),0,sz*(p.d*0.46f)); 
            glRotatef(-90,1,0,0); 
            drawSolidCylinder(0.045f,postH); 
            glPopMatrix(); 
        }
    }
    glPopMatrix();
}

void drawSushi(const Platform& p){
    glPushMatrix(); 
    glTranslatef(p.x,p.y+0.22f,p.z); 
    glColor3f(0.92f,0.92f,0.98f); 
    glRotatef(-90,1,0,0); 
    drawSolidCylinder(0.50f,0.05f); 
    glRotatef(90,1,0,0); 

    glColor3f(0.97f,0.97f,0.97f); 
    glPushMatrix(); 
    glRotatef(-90,1,0,0); 
    drawSolidCylinder(0.40f,0.20f); 
    glPopMatrix();

    glColor3f(0.06f,0.12f,0.10f); 
    glPushMatrix(); 
    glScalef(0.85f,0.12f,0.50f); 
    glutSolidCube(0.6f); 
    glPopMatrix();

    glColor3f(0.90f,0.38f,0.22f); 
    glPushMatrix(); 
    glTranslatef(0,0.18f,0.05f); 
    glutSolidSphere(0.14f,16,16); 
    glPopMatrix();

    glColor3f(0.95f,0.52f,0.30f); 
    glPushMatrix(); 
    glTranslatef(0.07f,0.16f,-0.05f); 
    glutSolidSphere(0.10f,14,14); 
    glPopMatrix();
    glPopMatrix(); 
}

void drawNinjaStar(const Platform& p,float animT){
    glPushMatrix(); 
    glTranslatef(p.x,p.y+0.26f,p.z); 

    glColor3f(0.55f,0.55f,0.6f); 
    glutSolidTorus(0.04f,0.26f,18,30); 

    glColor3f(0.75f,0.75f,0.8f); 
    glutSolidSphere(0.085f,18,18); 

    glColor3f(0.85f,0.85f,0.9f); 
    for(int i=0;i<4;++i){ 
        glPushMatrix(); 
        glRotatef(90*i,0,1,0); 
        glTranslatef(0.28f,0,0); 
        glRotatef(90,0,0,1); 
        glutSolidCone(0.075f,0.40f,12,1); 
        glPopMatrix(); 
    }
    glColor3f(0.4f,0.4f,0.45f); 
    glutWireTorus(0.02f,0.33f,10,28); 
    glPopMatrix(); 
}

void drawSamuraiSword(const Platform& p){
    glPushMatrix(); 
    glTranslatef(p.x,p.y+0.27f,p.z); 
    glRotatef(18,0,1,0); 

    glColor3f(0.88f,0.88f,0.93f); 
    glPushMatrix(); 
    glScalef(0.10f,0.05f,1.25f); 
    glutSolidCube(1); 
    glPopMatrix();

    glColor3f(0.7f,0.7f,0.75f); 
    glPushMatrix(); 
    glScalef(0.05f,0.018f,1.20f); 
    glutSolidCube(1); 
    glPopMatrix();

    glColor3f(0.35f,0.28f,0.22f); 
    glPushMatrix(); 
    glTranslatef(0,0,-0.38f); 
    glRotatef(-90,1,0,0); 
    drawSolidCylinder(0.085f,0.03f); 
    glPopMatrix();

    glColor3f(0.22f,0.16f,0.12f); 
    glPushMatrix(); 
    glTranslatef(0,0,-0.56f); 
    glRotatef(-90,1,0,0); 
    drawSolidCylinder(0.052f,0.28f); 
    glPopMatrix();

    glColor3f(0.42f,0.33f,0.26f); 
    glPushMatrix(); 
    glTranslatef(0,0,-0.72f); 
    glutSolidSphere(0.05f,14,14); 
    glPopMatrix();
    glPopMatrix(); 
}

void drawSmallTemple(const Platform& p,float phase){
    glPushMatrix(); 
    glTranslatef(p.x,p.y+0.25f,p.z); 

    glColor3f(0.62f,0.57f,0.52f); 
    glPushMatrix(); 
    glScalef(0.80f,0.08f,0.80f); 
    glutSolidCube(1); 
    glPopMatrix();

    glColor3f(0.48f,0.42f,0.37f); 
    float s=0.32f,h=0.36f; 
    for(int i=-1;i<=1;i+=2){ 
        for(int j=-1;j<=1;j+=2){ 
            glPushMatrix(); 
            glTranslatef(i*s,0.04f,j*s); 
            glRotatef(-90,1,0,0); 
            drawSolidCylinder(0.05f,h); 
            glPopMatrix(); 
        }
    }

    float roofR=0.35f+0.25f*(0.5f*(sinf(phase*2.0f)+1)); 
    glColor3f(roofR,0.22f,0.22f); 
    glPushMatrix(); 
    glTranslatef(0,0.04f+h,0); 
    glRotatef(-90,1,0,0); 
    glutSolidCone(0.55f,0.34f,18,1); 
    glPopMatrix();

    glColor3f(0.88f,0.72f,0.45f); 
    glPushMatrix(); 
    glTranslatef(0,0.04f+h+0.37f,0); 
    glutSolidSphere(0.06f,14,14); 
    glPopMatrix();

    glColor3f(0.40f,0.30f,0.24f); 
    glPushMatrix(); 
    glTranslatef(0.0f,0.04f+h,0.22f); 
    glRotatef(-90,1,0,0); 
    drawSolidCylinder(0.02f,0.32f); 
    glPopMatrix();
    glPopMatrix(); 
}

void drawCoin(float x,float y,float z){
    glPushMatrix(); 
    glTranslatef(x,y,z); 
    glRotatef(-90,1,0,0); 
    glColor3f(0.95f,0.82f,0.15f); 
    drawSolidCylinder(0.12f,0.045f,24,1); 
    glPopMatrix(); 
}

void drawGround(){
    glPushMatrix(); 
    glTranslatef(0,-1.0f,0); 

    glColor3f(0.42f,0.45f,0.48f); 
    glPushMatrix(); 
    glScalef(60.0f,0.2f,60.0f); 
    glutSolidCube(1); 
    glPopMatrix();

    glColor3f(0.38f,0.40f,0.42f); 
    glBegin(GL_LINES); 
    for(int i=-30;i<=30;i++){ 
        glVertex3f(i,0.11f,-30); 
        glVertex3f(i,0.11f,30); 
        glVertex3f(-30,0.11f,i); 
        glVertex3f(30,0.11f,i); 
    } 
    glEnd();

    glPopMatrix(); 
}

void drawWalls(){
    for(int i=0;i<4;i++){ 
        glPushMatrix(); 
        float tint=0.20f+0.04f*i; 
        glColor3f(0.25f+tint,0.27f+tint,0.29f+tint); 
        walls[i].draw(); 
        glPopMatrix();

        glPushMatrix(); 
        glColor3f(0.18f,0.18f,0.19f); 
        if(i<2){ 
            glTranslatef(0,2.1f,(i==0?ARENA_MIN_Z:ARENA_MAX_Z)); 
            glScalef(20.2f,0.15f,0.45f); 
        } else { 
            glTranslatef((i==2?ARENA_MIN_X:ARENA_MAX_X),2.1f,0); 
            glScalef(0.45f,0.15f,20.2f); 
        } 
        glutSolidCube(1); 
        glPopMatrix(); 
    }
}

void updateCamera(){
    glMatrixMode(GL_MODELVIEW); 
    glLoadIdentity(); 

    if(currentView==FREE_VIEW){ 
        float yawRad=freeCamYaw*3.14159265f/180.0f; 
        float pitchRad=freeCamPitch*3.14159265f/180.0f; 
        float fx = sinf(yawRad)*cosf(pitchRad); 
        float fy = sinf(pitchRad); 
        float fz = cosf(yawRad)*cosf(pitchRad); 
        gluLookAt(freeCamX,freeCamY,freeCamZ, freeCamX+fx, freeCamY+fy, freeCamZ+fz, 0,1,0); 
        return; 
    }

    if(currentView==FOLLOW_VIEW){ 
        float yawRad=(ninja.yaw+camYawOffset)*3.14159265f/180.0f; 
        float dist=camOffsetZ; 
        float camX=ninja.x - sinf(yawRad)*dist; 
        float camZ=ninja.z - cosf(yawRad)*dist; 
        float camY=ninja.y + camOffsetY; 
        gluLookAt(camX,camY,camZ, ninja.x, ninja.y+0.6f, ninja.z, 0,1,0); 
        return; 
    }

    switch(currentView){ 
    case TOP_VIEW:
        gluLookAt(ninja.x, ninja.y+20.0f, ninja.z, ninja.x, ninja.y, ninja.z, 0,0,-1); 
        break; 
    case SIDE_VIEW:
        gluLookAt(ninja.x+18.0f, ninja.y+3.0f, ninja.z, ninja.x, ninja.y+1.0f, ninja.z,0,1,0); 
        break; 
    case FRONT_VIEW:
        gluLookAt(ninja.x, ninja.y+3.0f, ninja.z+18.0f, ninja.x, ninja.y+1.0f, ninja.z,0,1,0); 
        break; 
    default: 
        break; 
    }
}

void initScene(){
    srand((unsigned int)time(nullptr)); 
    float py=-0.8f; 
    platforms[0]={-5.0f,py,-5.0f,3.0f,2.4f,0.75f,0.25f,0.25f}; 
    platforms[1]={ 5.0f,py,-4.0f,2.4f,3.2f,0.25f,0.65f,0.85f}; 
    platforms[2]={-4.0f,py, 5.0f,3.2f,2.6f,0.30f,0.75f,0.35f}; 
    platforms[3]={ 5.0f,py, 5.0f,2.8f,2.8f,0.85f,0.72f,0.25f}; 

    totalCoins=0; 
    totalCollected=0; 

    for(int i=0;i<4;i++){ 
        platforms[i].coins.clear(); 
        int count=5; 
        for(int c=0;c<count;c++){ 
            float cx=frand(platforms[i].x-platforms[i].w*0.42f, platforms[i].x+platforms[i].w*0.42f); 
            float cz=frand(platforms[i].z-platforms[i].d*0.42f, platforms[i].z+platforms[i].d*0.42f); 
            float cy=platforms[i].y+0.16f; 
            platforms[i].coins.push_back({cx,cy,cz,false}); 
            totalCoins++; 
        } 
        platforms[i].allCollected=false; 
        platforms[i].animPlaying=false; 
        platforms[i].animT=0.0f; 
    }

    ninja.x=0; 
    ninja.z=0; 
    ninja.y=-0.5f; 
    ninja.scale=0.35f; 
    ninja.yaw=0; 
    targetYaw=0; 
    velX=velZ=0; 

    gameWon=false; 
    gameOver=false; 
    timeLeftMs=90*1000; 
    lastTickMs=glutGet(GLUT_ELAPSED_TIME); 
    for(int i=0;i<4;i++) walls[i].setColor(0.28f,0.30f,0.32f); 
    winSoundPlayed=false; 
    loseSoundPlayed=false; 
}

void updateCoinCollection(){
    for(int i=0;i<4;i++){
        for(auto& coin: platforms[i].coins){ 
            if(!coin.collected){ 
                float dx=ninja.x-coin.x; 
                float dz=ninja.z-coin.z; 
                if(dx*dx+dz*dz < (ninjaRadius+0.12f)*(ninjaRadius+0.12f)){ 
                    coin.collected=true; 
                    totalCollected++; 
                    playCollect(); 
                } 
            } 
        } 
        if(!platforms[i].allCollected){ 
            bool all=true; 
            for(auto& coin: platforms[i].coins){ 
                if(!coin.collected){ all=false; break; } 
            } 
            platforms[i].allCollected=all; 
            if(all) platforms[i].animPlaying=true; 
        } 
    }
    if(!gameWon && totalCollected==totalCoins){ 
        gameWon=true; 
        playWin(); 
    } 
}

void drawPlatformsAndObjects(float dt){
    for(int i=0;i<4;i++){ 
        Platform& p=platforms[i]; 
        drawPlatformBase(p); 

        for(auto& coin: p.coins){ 
            if(!coin.collected) drawCoin(coin.x,coin.y,coin.z); 
        }

        if(p.allCollected && p.animPlaying) p.animT += dt; 

        glPushMatrix(); 
        if(i==0){ 
            if(p.allCollected && p.animPlaying){ 
                glTranslatef(p.x,0,p.z); 
                glRotatef(p.animT*90.0f,0,1,0); 
                glTranslatef(-p.x,0,-p.z); 
            } 
            drawSushi(p); 
        } else if(i==1){ 
            glPushMatrix(); 
            glTranslatef(p.x,0,p.z); 
            float s=1.0f; 
            if(p.allCollected && p.animPlaying) s=1.0f+0.25f*sinf(p.animT*4.0f); 
            glScalef(s,s,s); 
            glTranslatef(-p.x,0,-p.z); 
            drawNinjaStar(p,p.animT); 
            glPopMatrix(); 
        } else if(i==2){ 
            glPushMatrix(); 
            float up=0.0f; 
            if(p.allCollected && p.animPlaying) up=0.25f*sinf(p.animT*3.0f); 
            glTranslatef(0,up,0); 
            drawSamuraiSword(p); 
            glPopMatrix(); 
        } else if(i==3){ 
            float phase=(p.allCollected && p.animPlaying)? p.animT:0.0f; 
            drawSmallTemple(p,phase); 
        } 
        glPopMatrix(); 
    } 
}

void integrateMovement(float dt){
    if(gameOver) return; 
    if(currentView!=FOLLOW_VIEW) return; 

    float dirX=0, dirZ=0; 
    if(keyW||keyUp) dirZ -= 1.0f; 
    if(keyS||keyDown) dirZ += 1.0f; 
    if(keyA||keyLeft) dirX -= 1.0f; 
    if(keyD||keyRight) dirX += 1.0f; 

    float len = sqrtf(dirX*dirX+dirZ*dirZ); 
    if(len>0){ 
        dirX/=len; 
        dirZ/=len; 
        velX += dirX*accel*dt; 
        velZ += dirZ*accel*dt; 
        float speed = sqrtf(velX*velX+velZ*velZ); 
        if(speed>ninjaSpeed){ 
            velX *= ninjaSpeed/speed; 
            velZ *= ninjaSpeed/speed; 
        } 
        targetYaw = atan2f(velX, velZ)*180.0f/3.14159265f; 
        ninja.movementMagnitude = speed / ninjaSpeed; 
    } else { 
        float speed = sqrtf(velX*velX+velZ*velZ); 
        if(speed>0){ 
            float drop = decel*dt; 
            float newSpeed = speed - drop; 
            if(newSpeed<0) newSpeed=0; 
            float scale = newSpeed/(speed+1e-6f); 
            velX*=scale; 
            velZ*=scale; 
        } 
        ninja.movementMagnitude = 0.0f; 
    }

    float newX = ninja.x + velX*dt; 
    float newZ = ninja.z + velZ*dt; 
    newX = clampf(newX, ARENA_MIN_X + ninjaRadius, ARENA_MAX_X - ninjaRadius); 
    newZ = clampf(newZ, ARENA_MIN_Z + ninjaRadius, ARENA_MAX_Z - ninjaRadius); 
    ninja.x=newX; 
    ninja.z=newZ; 
}

void updateFreeCamera(float dt){
    if(currentView!=FREE_VIEW) return; 

    float yawRad=freeCamYaw*3.14159265f/180.0f; 
    float pitchRad=freeCamPitch*3.14159265f/180.0f; 
    float forwardX = sinf(yawRad)*cosf(pitchRad); 
    float forwardY = sinf(pitchRad); 
    float forwardZ = cosf(yawRad)*cosf(pitchRad); 
    float rightX = cosf(yawRad); 
    float rightZ = -sinf(yawRad); 

    float moveX=0, moveY=0, moveZ=0; 
    if(keyW||keyUp){ moveX += forwardX; moveY += forwardY; moveZ += forwardZ; } 
    if(keyS||keyDown){ moveX -= forwardX; moveY -= forwardY; moveZ -= forwardZ; } 
    if(keyA||keyLeft){ moveX -= rightX; moveZ -= rightZ; } 
    if(keyD||keyRight){ moveX += rightX; moveZ += rightZ; } 
    if(keyUp) moveY += 1.0f; 
    if(keyDown) moveY -= 1.0f; 

    if(keyW||keyS||keyA||keyD||keyUp||keyDown||keyLeft||keyRight){ 
        float len = sqrtf(moveX*moveX+moveY*moveY+moveZ*moveZ); 
        if(len>0){ 
            moveX/=len; moveY/=len; moveZ/=len; 
            freeCamX += moveX*freeMoveSpeed*dt; 
            freeCamY += moveY*freeMoveSpeed*dt; 
            freeCamZ += moveZ*freeMoveSpeed*dt; 
        } 
    } 
}

void Display(void){
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 
    updateCamera(); 
    drawGround(); 
    drawWalls();

    static int prevMs=glutGet(GLUT_ELAPSED_TIME); 
    int now=glutGet(GLUT_ELAPSED_TIME); 
    float dt=(now-prevMs)/1000.0f; 
    prevMs=now; 

    integrateMovement(dt); 
    updateFreeCamera(dt); 

    float diff=targetYaw-ninja.yaw; 
    while(diff>180) diff-=360; 
    while(diff<-180) diff+=360; 
    if(currentView==FOLLOW_VIEW) ninja.yaw += diff*0.20f; 

    drawPlatformsAndObjects(dt); 
    if(currentView==FOLLOW_VIEW) ninja.t += dt; 

    ninja.draw(); 
    updateCoinCollection(); 
    drawHUD(); 
    glFlush(); 
}

void Keyboard(unsigned char key,int x,int y){
    if(gameOver) return; 
    switch(key){ 
    case 'w': case 'W': keyW=true; break; 
    case 's': case 'S': keyS=true; break; 
    case 'a': case 'A': keyA=true; break; 
    case 'd': case 'D': keyD=true; break; 
    case 'q': case 'Q': currentView=FOLLOW_VIEW; break; 
    case 'c': case 'C': currentView=FREE_VIEW; break; 
    case '1': currentView=TOP_VIEW; break; 
    case '2': currentView=SIDE_VIEW; break; 
    case '3': currentView=FRONT_VIEW; break; 
    case 'f': case 'F': if(platforms[0].allCollected) platforms[0].animPlaying=!platforms[0].animPlaying; break; 
    case 'g': case 'G': if(platforms[1].allCollected) platforms[1].animPlaying=!platforms[1].animPlaying; break; 
    case 'h': case 'H': if(platforms[2].allCollected) platforms[2].animPlaying=!platforms[2].animPlaying; break; 
    case 'j': case 'J': if(platforms[3].allCollected) platforms[3].animPlaying=!platforms[3].animPlaying; break; 
    case 'e': case 'E': if(currentView==FREE_VIEW){ keyUp=true; } break; 
    case 'z': case 'Z': if(currentView==FREE_VIEW){ keyDown=true; } break; 
    case 27: shutdownAudio(); exit(0); 
    }
    glutPostRedisplay(); 
}

void KeyboardUp(unsigned char key,int x,int y){
    switch(key){ 
    case 'w': case 'W': keyW=false; break; 
    case 's': case 'S': keyS=false; break; 
    case 'a': case 'A': keyA=false; break; 
    case 'd': case 'D': keyD=false; break; 
    case 'e': case 'E': keyUp=false; break; 
    case 'z': case 'Z': keyDown=false; break; 
    }
}

void SpecialKeys(int key,int x,int y){
    if(gameOver) return; 
    switch(key){ 
    case GLUT_KEY_UP: keyUp=true; break; 
    case GLUT_KEY_DOWN: keyDown=true; break; 
    case GLUT_KEY_LEFT: keyLeft=true; break; 
    case GLUT_KEY_RIGHT: keyRight=true; break; 
    }
}

void SpecialKeysUp(int key,int x,int y){
    switch(key){ 
    case GLUT_KEY_UP: keyUp=false; break; 
    case GLUT_KEY_DOWN: keyDown=false; break; 
    case GLUT_KEY_LEFT: keyLeft=false; break; 
    case GLUT_KEY_RIGHT: keyRight=false; break; 
    }
}

void Mouse(int button,int state,int x,int y){
    if(currentView==FOLLOW_VIEW){ 
        if(button==GLUT_LEFT_BUTTON) mouseOrbit = (state==GLUT_DOWN); 
        if(button==GLUT_MIDDLE_BUTTON) mousePan = (state==GLUT_DOWN); 
        if(state==GLUT_DOWN){ lastMouseX=x; lastMouseY=y; }
        if(button==3 && state==GLUT_DOWN){ 
            camOffsetZ -= 0.6f; 
            if(camOffsetZ<camMinDist) camOffsetZ=camMinDist; 
        }
        if(button==4 && state==GLUT_DOWN){ 
            camOffsetZ += 0.6f; 
            if(camOffsetZ>camMaxDist) camOffsetZ=camMaxDist; 
        } 
    } else if(currentView==FREE_VIEW){ 
        if(button==GLUT_LEFT_BUTTON) freeMouseRotate = (state==GLUT_DOWN); 
        if(state==GLUT_DOWN){ lastMouseX=x; lastMouseY=y; }
        if(button==3 && state==GLUT_DOWN){ 
            float yawRad=freeCamYaw*3.14159265f/180.0f; 
            float pitchRad=freeCamPitch*3.14159265f/180.0f; 
            freeCamX += sinf(yawRad)*cosf(pitchRad)*0.8f; 
            freeCamY += sinf(pitchRad)*0.8f; 
            freeCamZ += cosf(yawRad)*cosf(pitchRad)*0.8f; 
        }
        if(button==4 && state==GLUT_DOWN){ 
            float yawRad=freeCamYaw*3.14159265f/180.0f; 
            float pitchRad=freeCamPitch*3.14159265f/180.0f; 
            freeCamX -= sinf(yawRad)*cosf(pitchRad)*0.8f; 
            freeCamY -= sinf(pitchRad)*0.8f; 
            freeCamZ -= cosf(yawRad)*cosf(pitchRad)*0.8f; 
        } 
    }
}

void MouseMotion(int x,int y){
    int dx=x-lastMouseX; 
    int dy=y-lastMouseY; 
    if(currentView==FOLLOW_VIEW){ 
        if(mouseOrbit){ camYawOffset += dx*mouseSensitivityOrbit; } 
        if(mousePan){ 
            camOffsetY -= dy*mouseSensitivityZoom; 
            camOffsetY = clampf(camOffsetY, camMinY, camMaxY); 
        } 
    } else if(currentView==FREE_VIEW){ 
        if(freeMouseRotate){ 
            freeCamYaw += dx*freeMouseSensitivity; 
            freeCamPitch -= dy*freeMouseSensitivity; 
            if(freeCamPitch>89) freeCamPitch=89; 
            if(freeCamPitch<-89) freeCamPitch=-89; 
        } 
    }
    lastMouseX=x; 
    lastMouseY=y; 
    glutPostRedisplay(); 
}

void PassiveMouseMotion(int x,int y){
    lastMouseX=x; 
    lastMouseY=y; 
}

void Anim(){
    int now=glutGet(GLUT_ELAPSED_TIME); 
    int dt=now-lastTickMs; 
    lastTickMs=now; 
    if(!gameOver && !gameWon){ 
        timeLeftMs -= dt; 
        if(timeLeftMs<=0){ 
            timeLeftMs=0; 
            gameOver=true; 
            for(int i=0;i<4;i++) platforms[i].animPlaying=false; 
            playLose(); 
        } 
    } 
    rotAng+=0.01f; 
    glutPostRedisplay(); 
}

void initGL(){
    glEnable(GL_DEPTH_TEST); 
    glClearColor(1,1,1,0); 
    glMatrixMode(GL_PROJECTION); 
    glLoadIdentity(); 
    gluPerspective(45.0f,1.0f,0.1f,300.0f); 
    glMatrixMode(GL_MODELVIEW); 
}

void main(int argc,char** argv){
    glutInit(&argc,argv); 
    glutInitWindowSize(900,900); 
    glutInitWindowPosition(120,60); 
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
