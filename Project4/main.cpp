#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif
#include <vector>

std::vector<std::pair<int,int>> mouseList;

int window_width = 320;
int window_height = 320;

void renderScene(void) {

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0, window_width, window_height, 0);
	
	/*if (mouseList.size() > 1) {
	int x0 = mouseList[mouseList.size() - 2].first;
	int y0 = mouseList[mouseList.size() - 2].second;
	glBegin(GL_LINES);
	glVertex2f(100, 100);
	glVertex2f(100, 200);
	glEnd();
	}*/

	for (int i = 1; i < mouseList.size(); i++) {
		std::pair<int, int> pos1 = mouseList[i - 1];
		std::pair<int, int> pos2 = mouseList[i];
		glBegin(GL_LINES);
			glVertex2f(pos1.first, pos1.second);
			glVertex2f(pos2.first, pos2.second);
		glEnd();

		if (pos1.first == pos2.first && pos1.second == pos2.second) {
			std::pair<int, int> pos0 = mouseList[0];
			glBegin(GL_LINES);
				glVertex2f(pos1.first, pos1.second);
				glVertex2f(pos0.first, pos0.second);
			glEnd();
			break;
		}
	}

	/*glBegin(GL_TRIANGLES);
		glVertex3f(-0.5,-0.5,0.0);
		glVertex3f(0.5,0.0,0.0);
		glVertex3f(0.0,0.5,0.0);
	glEnd();*/

    glutSwapBuffers();
}

void detectMouse(int button, int state, int x, int y) {
	if (state != 1) return;
	if (button == GLUT_LEFT_BUTTON) {
		mouseList.push_back(std::make_pair(x, y));
	}
	else if (button == GLUT_RIGHT_BUTTON) {
		mouseList.clear();
	}	
	glutPostRedisplay();
}

void Resize(int width, int height)
{
	window_width = width;
	window_height = height;

	glViewport(0, 0, (GLsizei)window_width, (GLsizei)window_height);

	glutPostRedisplay();
}

int main(int argc, char **argv) {

	// init GLUT and create Window
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowPosition(100,100);
	glutInitWindowSize(window_width,window_height);
	glutCreateWindow("Lighthouse3D - GLUT Tutorial");

	// register callbacks
	glutDisplayFunc(renderScene);
	glutMouseFunc(detectMouse);
	glutReshapeFunc(Resize);

	// enter GLUT event processing cycle
	glutMainLoop();
	
	return 1;
}