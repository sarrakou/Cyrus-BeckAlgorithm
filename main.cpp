#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif
#include <vector>
#include <stdlib.h>
#include <algorithm>

struct Point {
    float x, y;
};

struct Line {
    Point p1, p2;
};

struct Edge {
    Point start, end;
    Point normal;
};

std::vector<Point> userPolygon; // To store user-defined polygon vertices
std::vector<Point> ClippingWindowPolygon; // To store user-defined ClippingWindowPolygon vertices

// Define the clipping window as a rectangle
/* Point clippingWindow[4] = {
    {100, 100}, // Bottom left
    {100, 300}, // Top left
    {300, 300}, // Top right
    {300, 100}  // Bottom right
}; */

// Limit for the number of points
const int maxPoints = 5;

int windowHeight = 400;

// Function to compute dot product
float dot(const Point& a, const Point& b) {
    return a.x * b.x + a.y * b.y;
}

// Function to compute the normal to an edge
Point computeNormal(const Point& start, const Point& end) {
    Point normal;
    normal.x = end.y - start.y;
    normal.y = start.x - end.x;
    return normal;
}

// Function to build edges from vertices
std::vector<Edge> buildEdges(const std::vector<Point>& vertices) {
    std::vector<Edge> edges;
    for (size_t i = 0; i < vertices.size(); i++) {
        Edge edge;
        edge.start = vertices[i];
        edge.end = vertices[(i + 1) % vertices.size()];
        edge.normal = computeNormal(edge.start, edge.end);
        edges.push_back(edge);
    }
    return edges;
}

// Cyrus-Beck line-clipping algorithm
Line clipLine(const Line& line, const std::vector<Edge>& edges) {
    float tE = 0.0f; // Initialize to start of the line
    float tL = 1.0f; // Initialize to end of the line
    Point d = {line.p2.x - line.p1.x, line.p2.y - line.p1.y}; // Direction vector of the line

    for (const auto& edge : edges) {
        Point edgeDirection = {edge.end.x - edge.start.x, edge.end.y - edge.start.y};
        Point startToP1 = {line.p1.x - edge.start.x, line.p1.y - edge.start.y};
        float nDotD = dot(edge.normal, d);
        float nDotW = dot(edge.normal, startToP1);

        if (nDotD != 0.0f) {
            float t = -nDotW / nDotD;
            if (nDotD > 0.0f) {
                // Potential entry point, update tE
                if (t > tE) tE = t;
            } else {
                // Potential leaving point, update tL
                if (t < tL) tL = t;
            }
        } else if (nDotW < 0) {
            return Line{{0, 0}, {0, 0}}; // Line is outside and parallel to edge
        }
    }

    if (tE > tL) {
        return Line{{0, 0}, {0, 0}}; // No part of line within window
    }

    Line clippedLine;
    clippedLine.p1 = {line.p1.x + tE * d.x, line.p1.y + tE * d.y};
    clippedLine.p2 = {line.p1.x + tL * d.x, line.p1.y + tL * d.y};
    return clippedLine;
}

// Function to draw a line
void drawLine(Point p1, Point p2) {
    glBegin(GL_LINES);
        glVertex2f(p1.x, p1.y);
        glVertex2f(p2.x, p2.y);
    glEnd();
}

// Function to draw the clipping window
/* void drawClippingWindow() {
    glColor3f(1.0, 0.0, 0.0); // Red color for the clipping window
    for (int i = 0; i < 4; ++i) {
        drawLine(clippingWindow[i], clippingWindow[(i + 1) % 4]);
    }
} */

// Menu action identifiers
enum MenuActions {
    DRAW_POLYGON,
    DRAW_WINDOW,
    PERFORM_CLIPPING,
    CLEAR_ALL,
    COLOR_RED,
    COLOR_GREEN,
    COLOR_BLUE,
    COLOR_YELLOW,
    COLOR_PURPLE
};

// Color for the user-defined polygon and clipping window
float polygonColor[3] = {0.0, 0.0, 1.0}; // Default to blue

void changeColor(float r, float g, float b) {
    polygonColor[0] = r;
    polygonColor[1] = g;
    polygonColor[2] = b;
}

void colorMenu(int item) {
    switch (item) {
        case COLOR_RED:
            changeColor(1.0, 0.0, 0.0); // Red
            break;
        case COLOR_GREEN:
            changeColor(0.0, 1.0, 0.0); // Green
            break;
        case COLOR_BLUE:
            changeColor(0.0, 0.0, 1.0); // Blue
            break;
        case COLOR_YELLOW:
            changeColor(1.0, 1.0, 0.0); // Yellow
            break;
        case COLOR_PURPLE:
            changeColor(1.0, 0.0, 1.0); // Purple
            break;
    }
    glutPostRedisplay();
}

int createColorMenu() {
    int submenu = glutCreateMenu(colorMenu);
    glutAddMenuEntry("Red", COLOR_RED);
    glutAddMenuEntry("Green", COLOR_GREEN);
    glutAddMenuEntry("Blue", COLOR_BLUE);
    glutAddMenuEntry("Yellow", COLOR_YELLOW);
    glutAddMenuEntry("Purple", COLOR_PURPLE);

    return submenu;
}

// Global state variables
bool drawPolygonMode = false;
bool drawWindowMode = false;
bool performClippingMode = false;

void menu(int item) {
    switch (item) {
        case DRAW_POLYGON:
            drawPolygonMode = true;
            drawWindowMode = false;
            performClippingMode = false;
            userPolygon.clear();
            break;
        case DRAW_WINDOW:
            drawPolygonMode = false;
            drawWindowMode = true;
            performClippingMode = false;
            ClippingWindowPolygon.clear();
            break;
        case PERFORM_CLIPPING:
            if (userPolygon.size() >= 3) {
                performClippingMode = true;
            }
            break;
        case CLEAR_ALL:
            drawPolygonMode = false;
            drawWindowMode = false;
            performClippingMode = false;
            ClippingWindowPolygon.clear();
            userPolygon.clear();
            break;
    }
    glutPostRedisplay();
}

void createMenu() {
    int colorMenu = createColorMenu();

    glutCreateMenu(menu);
    glutAddMenuEntry("Draw Polygon", DRAW_POLYGON);
    glutAddMenuEntry("Draw Clipping Window", DRAW_WINDOW);
    glutAddMenuEntry("Perform Clipping", PERFORM_CLIPPING);
    glutAddMenuEntry("Clear All", CLEAR_ALL);
    glutAddSubMenu("Colors", colorMenu);
    glutAttachMenu(GLUT_RIGHT_BUTTON);
}


void mouse(int button, int state, int x, int y) {
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        if (drawWindowMode)
        {
            if (ClippingWindowPolygon.size() < 4) {
                // Convert screen coordinates to world coordinates if necessary
                Point newPoint = {static_cast<float>(x), static_cast<float>(windowHeight - y)};
                ClippingWindowPolygon.push_back(newPoint);

                // Redraw the window to show the updated clipping window
                glutPostRedisplay();
            }
        } else if (drawPolygonMode)
        {
             if (userPolygon.size() < maxPoints) {
            // Convert screen coordinates to world coordinates if necessary
            Point newPoint = {static_cast<float>(x), static_cast<float>(windowHeight - y)};
            userPolygon.push_back(newPoint);

            // Redraw the window to show the updated polygon
            glutPostRedisplay();
            }
        }
    }
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT);

    if (drawWindowMode || performClippingMode || drawPolygonMode) {
         // Draw the clipping window in red
        glColor3f(1.0, 0.0, 0.0); // Red color for the clipping window
        // glColor3fv(polygonColor); // Set color for the clipping window

        glBegin(GL_LINE_LOOP);
       /* for (int i = 0; i < 4; ++i) {
            glVertex2f(clippingWindow[i].x, clippingWindow[i].y);
        } */
        for (const auto& point : ClippingWindowPolygon) {
            glVertex2f(point.x, point.y);
        }
        glEnd();
    }

     // Draw the user-defined polygon in blue if enough points are selected
    if (drawPolygonMode || performClippingMode || userPolygon.size() >= 2) {
        //glColor3f(0.0, 0.0, 1.0); // Blue color for the original polygon
        glColor3fv(polygonColor); // Set color for the user-defined polygon

        glBegin(GL_LINE_LOOP);
        for (const auto& point : userPolygon) {
            glVertex2f(point.x, point.y);
        }
        glEnd();
    }

    // Check if the user has finished selecting points for the original polygon
    if (performClippingMode && userPolygon.size() >= 3 && userPolygon.size() <= maxPoints) {
        // Apply Cyrus-Beck algorithm to each line segment of the polygon
        std::vector<Edge> edges = buildEdges(std::vector<Point>(std::begin(ClippingWindowPolygon), std::end(ClippingWindowPolygon)));
        std::vector<Point> clippedPoints; // To store the endpoints of clipped segments

        for (size_t i = 0; i < userPolygon.size(); ++i) {
            Line lineSegment = {userPolygon[i], userPolygon[(i + 1) % userPolygon.size()]};
            Line clippedSegment = clipLine(lineSegment, edges);

            // Store the endpoints of the clipped line segment
            if (!(clippedSegment.p1.x == 0 && clippedSegment.p1.y == 0 &&
                  clippedSegment.p2.x == 0 && clippedSegment.p2.y == 0)) {
                clippedPoints.push_back(clippedSegment.p1);
                clippedPoints.push_back(clippedSegment.p2);
            }
        }

        // Draw the connected clipped segments
        glColor3f(0.0, 1.0, 0.0); // Green color for the clipped polygon
        if (!clippedPoints.empty()) {
            glBegin(GL_LINE_LOOP);
            for (const auto& point : clippedPoints) {
                glVertex2f(point.x, point.y);
            }
            glEnd();
        }
    }
    glFlush();
}

// GLUT main function
int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
    glutInitWindowSize(400, 400);
    glutInitWindowPosition(100, 100);
    glutCreateWindow("Clipping Window");

    glClearColor(1.0, 1.0, 1.0, 0.0); // Set background to white
    gluOrtho2D(0, 400, 0, 400); // Set the coordinate system

    createMenu(); // Create the right-click menu

    glutMouseFunc(mouse); // Register mouse callback
    glutDisplayFunc(display);
    glutMainLoop();
    return 0;
}
