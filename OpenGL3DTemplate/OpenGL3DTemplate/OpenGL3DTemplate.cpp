#include <stdlib.h>
#include <glut.h>
#include <BouncingWall.h>

BouncingWall wall1(0.0f, 0.0f, -3.0f, 2.0f, 2.5f, 0.3f);
BouncingWall wall2(3.0f, 0.0f, -3.0f, 2.0f, 2.5f, 0.3f);
BouncingWall wall3(-3.0f, 0.0f, -3.0f, 2.0f, 2.5f, 0.3f);

BouncingWall walls[] = {wall1, wall2, wall3};

float rotAng;

float cameraX = 0.0f;
float cameraY = 2.0f;
float cameraZ = 5.0f;

float lookAtX = 0.0f;
float lookAtY = 0.0f;
float lookAtZ = 0.0f;

float upX = 0.0f;
float upY = 1.0f;
float upZ = 0.0f;

float moveSpeed = 0.5f;

int lastMouseX = 0;
int lastMouseY = 0;
bool mouseLeftPressed = false;
bool mouseRightPressed = false;
bool mouseMiddlePressed = false;
float mouseSensitivity = 0.01f;

enum CameraView {
	FREE_VIEW,
	TOP_VIEW,
	SIDE_VIEW,
	FRONT_VIEW
};

CameraView currentView = FREE_VIEW;

const float DEFAULT_CAM_X = 0.0f;
const float DEFAULT_CAM_Y = 2.0f;
const float DEFAULT_CAM_Z = 5.0f;


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

	switch (currentView) {
	case TOP_VIEW:
		cameraX = 0.0f;
		cameraY = 20.0f;
		cameraZ = 0.0f;
		lookAtX = 0.0f;
		lookAtY = 0.0f;
		lookAtZ = 0.0f;
		upX = 0.0f;
		upY = 0.0f;
		upZ = -1.0f;  
		break;

	case SIDE_VIEW:
		cameraX = 20.0f;
		cameraY = 0.0f;
		cameraZ = 0.0f;
		lookAtX = 0.0f;
		lookAtY = 0.0f;
		lookAtZ = 0.0f;
		upX = 0.0f;
		upY = 1.0f;
		upZ = 0.0f;
		break;

	case FRONT_VIEW:
		cameraX = 0.0f;
		cameraY = 0.0f;
		cameraZ = 20.0f;
		lookAtX = 0.0f;
		lookAtY = 0.0f;
		lookAtZ = 0.0f;
		upX = 0.0f;
		upY = 1.0f;
		upZ = 0.0f;
		break;

	case FREE_VIEW:
		lookAtX = 0.0f;
		lookAtY = 0.0f;
		lookAtZ = 0.0f;
		upX = 0.0f;
		upY = 1.0f;
		upZ = 0.0f;
		break;
	}

	gluLookAt(cameraX, cameraY, cameraZ,
		lookAtX, lookAtY, lookAtZ,
		upX, upY, upZ);
}


void Display(void) {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	updateCamera();

	drawGround();

	for (BouncingWall wall : walls)
	{
		wall.draw();
	}
	//glPushMatrix();
	//glRotatef(rotAng, 0, 1, 0);
	//wall.draw();
	//glPopMatrix();

	//glPushMatrix();
	//glRotatef(-rotAng, 0, 1, 0);
	//glTranslatef(2, 0, 0);
	//glRotatef(rotAng, 1, 0, 0);
	//glColor3f(0.5f, 0.5f, 0.5f);
	//glutSolidSphere(0.5, 25, 25);
	//glPopMatrix();

	glFlush();
}

void Keyboard(unsigned char key, int x, int y) {
	switch (key) {
	case 'y': case 'Y':  
		if (currentView == FREE_VIEW) {
			cameraZ -= moveSpeed;
		}
		break;

	case 'u': case 'U':  
		if (currentView == FREE_VIEW) {
			cameraZ += moveSpeed;
		}
		break;

	case 'i': case 'I': 
		if (currentView == FREE_VIEW) {
			cameraX -= moveSpeed;
		}
		break;

	case 'o': case 'O':  
		if (currentView == FREE_VIEW) {
			cameraX += moveSpeed;
		}
		break;

	case 't': case 'T': 
		if (currentView == FREE_VIEW) {
			cameraY += moveSpeed;
		}
		break;

	case 'b': case 'B': 
		if (currentView == FREE_VIEW) {
			cameraY -= moveSpeed;
		}
		break;

	case '1':  
		currentView = TOP_VIEW;
		break;

	case '2':
		currentView = SIDE_VIEW;
		break;

	case '3':
		currentView = FRONT_VIEW;
		break;

	case 'q': case 'Q':  
		currentView = FREE_VIEW;
		cameraX = DEFAULT_CAM_X;
		cameraY = DEFAULT_CAM_Y;
		cameraZ = DEFAULT_CAM_Z;
		lookAtX = 0.0f;
		lookAtY = 0.0f;
		lookAtZ = 0.0f;
		upX = 0.0f;
		upY = 1.0f;
		upZ = 0.0f;
		break;
	}

	glutPostRedisplay();
}

void Mouse(int button, int state, int x, int y) {
	if (currentView != FREE_VIEW) return;

	if (button == GLUT_LEFT_BUTTON) {
		if (state == GLUT_DOWN) {
			mouseLeftPressed = true;
			lastMouseX = x;
			lastMouseY = y;
		}
		else if (state == GLUT_UP) {
			mouseLeftPressed = false;
		}
	}
	else if (button == GLUT_RIGHT_BUTTON) {
		if (state == GLUT_DOWN) {
			mouseRightPressed = true;
			lastMouseX = x;
			lastMouseY = y;
		}
		else if (state == GLUT_UP) {
			mouseRightPressed = false;
		}
	}
	else if (button == GLUT_MIDDLE_BUTTON) {
		if (state == GLUT_DOWN) {
			mouseMiddlePressed = true;
			lastMouseX = x;
			lastMouseY = y;
		}
		else if (state == GLUT_UP) {
			mouseMiddlePressed = false;
		}
	}
	else if (button == 3) { 
		cameraZ -= moveSpeed;
		glutPostRedisplay();
	}
	else if (button == 4) { 
		cameraZ += moveSpeed;
		glutPostRedisplay();
	}
}

void MouseMotion(int x, int y) {
	if (currentView != FREE_VIEW) return;

	int deltaX = x - lastMouseX;
	int deltaY = y - lastMouseY;

	if (mouseLeftPressed) {
		cameraX += deltaX * mouseSensitivity;
		cameraY -= deltaY * mouseSensitivity;
	}

	if (mouseRightPressed) {
		cameraX += deltaX * mouseSensitivity;
		cameraZ += deltaY * mouseSensitivity;
	}

	if (mouseMiddlePressed) {
		cameraZ += deltaX * mouseSensitivity;
		cameraY -= deltaY * mouseSensitivity;
	}

	lastMouseX = x;
	lastMouseY = y;

	glutPostRedisplay();
}

void PassiveMouseMotion(int x, int y) {
	lastMouseX = x;
	lastMouseY = y;
}


void Anim() {
	rotAng += 0.01;

	glutPostRedisplay();
}

void main(int argc, char** argv) {
	glutInit(&argc, argv);

	glutInitWindowSize(700, 700);
	glutInitWindowPosition(150, 150);

	glutCreateWindow("Ancient Ninja Warriors");
	glutDisplayFunc(Display);
	glutIdleFunc(Anim);
	glutKeyboardFunc(Keyboard);
	glutMouseFunc(Mouse);
	glutMotionFunc(MouseMotion);
	glutPassiveMotionFunc(PassiveMouseMotion);

	glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB | GLUT_DEPTH);
	glClearColor(1.0f, 1.0f, 1.0f, 0.0f);

	glEnable(GL_DEPTH_TEST);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45.0f, 300 / 300, 0.1f, 700.0f);

	glMatrixMode(GL_MODELVIEW);

	glutMainLoop();
}
