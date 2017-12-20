#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif
#include <vector>

std::vector<std::pair<int,int>> mouseList;

int window_width = 320;
int window_height = 320;

typedef unsigned long	DWORD;
typedef unsigned short	WORD;
typedef unsigned char	BYTE;
#define EDGE_TABLE_SIZE		1025

#define  RGB(r,g,b) ((DWORD)(((BYTE)(r)|((WORD)((BYTE)(g))<<8))|(((DWORD)(BYTE)(b))<<16)))

typedef struct _Edge
{
	double	dbX;
	double	dbDelta;
	int		inMaxY;
	_Edge*	ptrNext;
}Edge;

Edge*	g_ptrEdgeTable[EDGE_TABLE_SIZE];
Edge*	g_ptrAELHead;

int		g_inMinY = 0x7fffffff;
int		g_inMaxY = -0x7fffffff;

typedef struct
{
	int		x;
	int		y;
}Vector;

#define GetMin(x,y) ((x)>(y)?(y):(x))
#define GetMax(x,y) ((x)>(y)?(x):(y))


void allocEdges()
{
	int i;
	for (i = 0; i<EDGE_TABLE_SIZE; i++)
	{
		g_ptrEdgeTable[i] = new Edge;
		g_ptrEdgeTable[i]->ptrNext = NULL;
	}
	g_ptrAELHead = new Edge;
	g_ptrAELHead->ptrNext = NULL;
}

void deallocEdges()
{
	delete g_ptrAELHead;
	g_ptrAELHead = NULL;
	int i;
	for (i = 0; i<EDGE_TABLE_SIZE; i++)
	{
		delete g_ptrEdgeTable[i];
		g_ptrEdgeTable[i] = NULL;
	}
}

#define  eps		0.00001

int ceilPixel(double xLeft)
{
	return (int)(xLeft + 1.0 - eps);
}

int floorPixel(double xRight)
{
	return (int)(xRight - eps);
}

void setPixel(int x, int y, DWORD color)
{
	glBegin(GL_POINTS);
	glVertex2i(x, y);
	glEnd();
}

void insertEdgeList(Edge* ptrHead, Edge* ptrEdge)
{

	Edge* current = ptrHead, *next = ptrHead->ptrNext;
	while (next && next->dbX > ptrEdge->dbX)
	{
		current = next;
		next = current->ptrNext;
	}
	current->ptrNext = ptrEdge;
	ptrEdge->ptrNext = next;

}

void insertAEL(Edge* ptrHeadList, Edge* ptrInsertList)
{
	if (!ptrInsertList->ptrNext)
		return;

	Edge* curInsert = ptrInsertList->ptrNext, *nextInsert = NULL;
	Edge* current = ptrHeadList, *next = current->ptrNext;
	while (curInsert != NULL)
	{
		nextInsert = curInsert->ptrNext;
		while (next && next->dbX < curInsert->dbX)
		{
			current = next;
			next = current->ptrNext;
		}
		current->ptrNext = curInsert;
		curInsert->ptrNext = next;
		next = curInsert;
		curInsert = nextInsert;
	}
}

void initEdgeTable(Vector* ptrPolygon, int inNumPoly)
{
	int current = inNumPoly - 1, next = 0;
	Edge* edge = NULL;
	for (; next<inNumPoly; current = next, next++)
	{
		if (ptrPolygon[current].y == ptrPolygon[next].y)
			continue;
		int minY, maxY, x;
		if (ptrPolygon[current].y > ptrPolygon[next].y)
		{
			maxY = ptrPolygon[current].y;
			minY = ptrPolygon[next].y;
			x = ptrPolygon[next].x;
		}
		else
		{
			maxY = ptrPolygon[next].y;
			minY = ptrPolygon[current].y;
			x = ptrPolygon[current].x;
		}
		if (minY<g_inMinY)
			g_inMinY = minY;
		if (maxY>g_inMaxY)
			g_inMaxY = maxY;

		edge = new Edge;
		edge->ptrNext = NULL;
		edge->dbX = x;
		edge->inMaxY = maxY;
		edge->dbDelta = (double)(ptrPolygon[current].x - ptrPolygon[next].x) / (double)(ptrPolygon[current].y - ptrPolygon[next].y);

		insertEdgeList(g_ptrEdgeTable[minY], edge);
	}
}

void updateAEL(Edge* ptrHead, int inCurrentY)
{
	Edge* remove = NULL;
	Edge* current = ptrHead, *next = ptrHead->ptrNext;
	while (next)
	{
		if (next->inMaxY <= inCurrentY)
		{
			remove = next;
			next = next->ptrNext;
			current->ptrNext = next;
			delete remove;
		}
		else
		{
			current = next;
			current->dbX += current->dbDelta;
			next = current->ptrNext;
		}
	}
}

void fillAELScanLine(Edge* ptrHead, int inY, DWORD inColor)
{
	Edge* current = NULL, *next = ptrHead->ptrNext;

	do
	{
		current = next;
		next = current->ptrNext;
		int x = ceilPixel(current->dbX);
		int endX = floorPixel(next->dbX);
		for (; x <= endX; x++)
			setPixel(x, inY, inColor);
		current = next;
		next = current->ptrNext;
	} while (next);
}

void scanLineFill(Vector* ptrPolygon, int inNumPoly, DWORD inColor)
{
	allocEdges();
	initEdgeTable(ptrPolygon, inNumPoly);
	for (int y = g_inMinY; y<g_inMaxY; y++)
	{
		insertAEL(g_ptrAELHead, g_ptrEdgeTable[y]);
		fillAELScanLine(g_ptrAELHead, y, inColor);
		updateAEL(g_ptrAELHead, y + 1);
	}
	deallocEdges();
}

void renderScene(void) {
	bool fill_flag = false;

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
		printf("(%d,%d)\n", pos1.first, pos1.second);
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
			fill_flag = true;
			break;
		}
	}

	if (fill_flag) {
		static Vector polygon[1000];
		int n = mouseList.size();
		for (int i = 0; i < n - 1; i++) {
			polygon[i].x = mouseList[i].first;
			polygon[i].y = mouseList[i].second;
		}
		polygon[n - 1].x = mouseList[0].first;
		polygon[n - 1].y = mouseList[0].second;
		scanLineFill(polygon, n, RGB(255, 0, 0));
	}

	//glPointSize(5.0f);
	glBegin(GL_POINTS);
	for (int i = 0; i < mouseList.size(); i++) {
		std::pair<int, int> pos = mouseList[i];
		glVertex2i(pos.first, pos.second);
	}
	glEnd();
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