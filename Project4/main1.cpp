#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif
#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <cassert>
#include <cmath>

#include <list>
#include <vector>
#include <algorithm>

#define RGB(r, g, b) \
    ( ((r) & 0xff) | (((g) & 0xff) << 8) | (((b) & 0xff) << 16) )

#define for_iter(i, n) for (auto i = n.begin(); i != n.end(); ++i)

#define sgn(x) ((x) < 0 ? -1 : ((x) > 0 ? 1 : 0))

const size_t EDGE_TABLE_SIZE = 1080;
const double eps = 0.00001;
const int MAXH = 1080;
const int MAXW = 1920;
const int orange = RGB(255, 100, 0);
const int blue = RGB(100, 0, 255);
const int green = RGB(0, 255, 100);

void setPixel(int x, int y, int color);
void drawPixel(int x, int y, int color);

struct Point {
    Point() { }
    ~Point() { }

    explicit Point(const std::pair<int, int> &par):
        x(par.first), y(par.second) { }
    explicit Point(int x_, int y_):
        x(x_), y(y_) { }

    int x, y;
};

struct Edge {
    Edge() { }
    ~Edge() { }

    explicit Edge(double x_, double delta_, int y_):
        x(x_), delta(delta_), y(y_) { }

    bool operator < (const Edge &e) const {
        return x < e.x || (x == e.x && delta < e.delta);
    }

    double x;
    double delta;
    int y;
};

class EdgeTable {
public:
    EdgeTable() { }
    ~EdgeTable() { }

    void init(const std::vector<Point> &polygon) {
        int n = polygon.size();
        for (int i = 0; i < n; i++) {
            Point p1 = polygon[i];
            Point p2 = polygon[(i + 1) % n];

            if (p1.y == p2.y)
                continue;

            if (p1.y > p2.y)
                std::swap(p1, p2);
            
            minY = std::min(minY, p1.y);
            maxY = std::max(maxY, p2.y);
            double delta = 1.0 * (p1.x - p2.x) / (p1.y - p2.y);
            Edge e(p1.x, delta, p2.y);

            insert(p1.y, e);
        }
    }

    void insert(int y, const Edge &e) {
        std::list<Edge> &et = edgeTable[y];
        auto it = et.begin();
        for (; it != et.end() && *it < e; it++);
        et.insert(it, e);
    }

    int minY = ~0u>>1, maxY = ~minY;
    std::list<Edge> edgeTable[EDGE_TABLE_SIZE];
};

class ActiveEdgeList {
public:
    ActiveEdgeList() { }
    ~ActiveEdgeList() { }

    void insert(const std::list<Edge> &el) {
        auto it = ael.begin();
        for (auto e : el) {
            while (it != ael.end() && *it < e)
                it++;
            ael.insert(it, e);
        }
    }

    void update(int y) {
        for (auto it = ael.begin(); it != ael.end(); ) {
            if (it->y <= y) {
                ael.erase(it++);
            } else {
                it->x += it->delta;
                it++;
            }
        }
    }

    void fill(int y, int color) {
        for (auto next = ael.begin(); next != ael.end(); next++) {
            auto cur = next++;
            int x1 = ceil(cur->x);
            int x2 = floor(next->x);
            for (; x1 <= x2; x1++)
                setPixel(x1, y, color);
        }
    }

	void getSegments() {
		segments.clear();
        for (auto next = ael.begin(); next != ael.end(); next++) {
            auto cur = next++;
            int x1 = ceil(cur->x);
            int x2 = floor(next->x);
			segments.push_back(Point(x1, x2));
        }
	}

    std::list<Edge> ael;
	std::vector<Point> segments;
};

std::vector<std::pair<int,int>> mouseList;

int window_width = 320;
int window_height = 320;

char screenBuffer[MAXH][MAXW][3];
bool fill_flagA = false;
bool fill_flagB = false;
int cursorX, cursorY;
std::vector<Point> polygonA, polygonB;
int curColor = orange;

inline void setPixel(int x, int y, int color)
{
    glColor3f((color & 0xff) / 255.0,
              (color >> 8 & 0xff) / 255.0,
              (color >> 16 & 0xff) / 255.0);
    glBegin(GL_POINTS);
    glVertex2i(x, y);
    glEnd();
}

inline void drawPixel(int x, int y, int color)
{
    screenBuffer[x][y][0] = color & 0xff;
    screenBuffer[x][y][1] = (color >> 8) & 0xff;
    screenBuffer[x][y][2] = (color >> 16) & 0xff;
}

void clearScreen(int h, int w, int color = 0)
{
    for (int i = 0; i < h; i++)
        for (int j = 0; j < w; j++)
            drawPixel(i, j, color);
}

void drawLine(const Point &p1, const Point &p2, int color)
{
    int dx, dy, dxabs, dyabs, sdx, sdy, x, y, px, py;
    dx = p2.x - p1.x;
    dy = p2.y - p1.y;
    dxabs = abs(dx);
    dyabs = abs(dy);
    sdx = sgn(dx);
    sdy = sgn(dy);
    x = dxabs >> 1;
    y = dyabs >> 1;
    px = p1.x;
    py = p1.y;

    setPixel(px, py, color);

    if (dxabs >= dyabs) {
        for (int i = 0; i < dxabs; i++) {
            y += dyabs;
            if (y >= dxabs) {
                y -= dxabs;
                py += sdy;
            }
            px += sdx;
            setPixel(px, py, color);
        }
    } else {
        for (int i = 0; i < dyabs; i++) {
            x += dxabs;
            if (x >= dyabs) {
                x -= dyabs;
                px += sdx;
            }
            py += sdy;
            setPixel(px, py, color);
        }
    }
}

void scanLineFill(const std::vector<Point> &polygon, int color)
{
    EdgeTable et;
    ActiveEdgeList ael;
    et.init(polygon);
/*
	for (int i = 0; i < polygon.size() - 1; i++)
		drawLine(polygon[i], polygon[i + 1], color);
	drawLine(polygon[polygon.size() - 1], polygon[0], color);
*/
    for (int y = et.minY; y < et.maxY; y++) {
        ael.insert(et.edgeTable[y]);
        ael.fill(y, color);
        ael.update(y + 1);
    }
}
struct Event
{
	int x, id;
	Event(int _x, int _id): x(_x), id(_id) {}
	bool operator <(const Event &b) const {
		return x < b.x || (x == b.x && id > b.id);
	}
};
std::vector<Point> segmentsIntersection(const std::vector<Point> &seg1, const std::vector<Point> &seg2)
{
	std::vector<Point> ret;
	std::vector<Event> events;
	for (int i = 0; i < seg1.size(); i++) {
		events.push_back(Event(seg1[i].x, 0));
		events.push_back(Event(seg1[i].y, 1));
	}
	for (int i = 0; i < seg2.size(); i++) {
		events.push_back(Event(seg2[i].x, 0));
		events.push_back(Event(seg2[i].y, 1));
	}
	sort(events.begin(), events.end());
	int count = 0;
	int left = 0;
	for (int i = 0; i < events.size(); i++) {
		if (events[i].id == 0) {
			count++;
			if (count == 2)
				left = events[i].x;
		}
		else {
			count--;
			if (count == 1)
				ret.push_back(Point(left, events[i].x));
		}
	}
	return ret;
}

void scanLineFillIntersection(const std::vector<Point> &polygon1, const std::vector<Point> &polygon2, int color)
{
    EdgeTable et1, et2;
    ActiveEdgeList ael1, ael2;
    et1.init(polygon1);
	et2.init(polygon2);
	
	int minY = std::max(et1.minY, et2.minY);
	int maxY = std::min(et1.maxY, et2.maxY);
	for (int y = et1.minY; y < minY; y++) {
		ael1.insert(et1.edgeTable[y]);
		ael1.update(y + 1);
	}
	for (int y = et2.minY; y < minY; y++) {
		ael2.insert(et2.edgeTable[y]);
		ael2.update(y + 1);
	}
	for (int y = minY; y < maxY; y++) {
		ael1.insert(et1.edgeTable[y]);
		ael2.insert(et2.edgeTable[y]);
		ael1.getSegments();
		ael2.getSegments();
		std::vector<Point> seg;
		seg = segmentsIntersection(ael1.segments, ael2.segments);
		for (int i = 0; i < seg.size(); i++)
			for (int x = seg[i].x; x <= seg[i].y; x++)
				setPixel(x, y, color);
		ael1.update(y + 1);
		ael2.update(y + 1);
	}
}

void renderScene(void)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, window_width, window_height, 0);
    
	if (fill_flagA)
        scanLineFill(polygonA, orange);
	if (fill_flagB)
		scanLineFill(polygonB, blue);
	if (fill_flagA && fill_flagB)
		scanLineFillIntersection(polygonA, polygonB, green);

    for (size_t i = 1; i < mouseList.size(); i++) {
        auto pos1 = mouseList[i - 1];
        auto pos2 = mouseList[i];
        printf("(%d,%d)\n", pos1.first, pos1.second);
        drawLine((Point)pos1, (Point)pos2, curColor);

        if (pos1 == pos2) {
            auto pos0 = mouseList[0];
            drawLine((Point)pos1, (Point)pos0, curColor);
			if (!fill_flagA)
            	fill_flagA = true;
			else
				fill_flagB = true;
        }
    }

    if (fill_flagA && polygonA.size() == 0) {
        int n = mouseList.size();
        for (int i = 0; i < n - 1; i++) {
            Point p(mouseList[i]);
            polygonA.push_back(p);
        }
		mouseList.clear();
		curColor = blue;
        scanLineFill(polygonA, orange);
    }

    if (fill_flagB && polygonB.size() == 0) {
        int n = mouseList.size();
        for (int i = 0; i < n - 1; i++) {
            Point p(mouseList[i]);
            polygonB.push_back(p);
        }
		mouseList.clear();
        scanLineFill(polygonB, blue);
    }

    if (mouseList.size() > 0 && (!fill_flagA || !fill_flagB)) {
        Point p(mouseList[mouseList.size() - 1]);
		drawLine(Point(cursorX, cursorY), (Point)p, curColor);
    }
    glutSwapBuffers();
}

void readPolygon()
{
    FILE *fin = fopen("polygon.txt", "r");
    int n;
    fscanf(fin, "%d\n", &n);
    for (int i = 0; i < n; i++) {
        int x, y;
        fscanf(fin, "%d%d", &x, &y);
    }
}

void onClick(int button, int state, int x, int y)
{
    if (state != 1) return;
    if (button == GLUT_LEFT_BUTTON) {
        mouseList.push_back(std::make_pair(x, y));
    } else if (button == GLUT_RIGHT_BUTTON) {
        mouseList.clear();
        fill_flagA = false;
        fill_flagB = false;
		curColor = orange;
		polygonA.clear();
		polygonB.clear();
    }
    glutPostRedisplay();
}

void onMouseMove(int x, int y)
{
    static int jiffy = 0;
	if ((jiffy++ & 0x3) == 0) {
		cursorX = x, cursorY = y;
		renderScene();
	}
}

void onKeyPress(unsigned char key, int x, int y)
{
    switch (key) {
    case 'r':
    case 'R':
        readPolygon();
        break;
    }
    glutPostRedisplay();
}

void resize(int width, int height)
{
    window_width = width;
    window_height = height;

    glViewport(0, 0, (GLsizei)window_width, (GLsizei)window_height);

    glutPostRedisplay();
}

int main(int argc, char *argv[])
{

    // init GLUT and create Window
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowPosition(100, 100);
    glutInitWindowSize(window_width, window_height);
    glutCreateWindow("CG Project");

    // register callbacks
    glutDisplayFunc(renderScene);
    glutMouseFunc(onClick);
    glutReshapeFunc(resize);
    glutKeyboardFunc(onKeyPress);
    glutPassiveMotionFunc(onMouseMove);

    // enter GLUT event processing cycle
    glutMainLoop();
    
    return 1;
}
