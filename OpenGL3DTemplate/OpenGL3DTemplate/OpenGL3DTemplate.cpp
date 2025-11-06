#include <stdlib.h>
#include <glut.h>
#include <BouncingWall.h>

BouncingWall wall1(0.0f, 0.0f, -3.0f, 2.0f, 2.5f, 0.3f);
BouncingWall wall2(3.0f, 0.0f, -3.0f, 2.0f, 2.5f, 0.3f);
BouncingWall wall3(-3.0f, 0.0f, -3.0f, 2.0f, 2.5f, 0.3f);

BouncingWall walls[] = {wall1, wall2, wall3};

float rotAng;

void Display(void) {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

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

	glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB | GLUT_DEPTH);
	glClearColor(1.0f, 1.0f, 1.0f, 0.0f);

	glEnable(GL_DEPTH_TEST);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45.0f, 300 / 300, 0.1f, 700.0f);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	gluLookAt(0.0f, 2.0f, 5.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);

	glutMainLoop();
}
