#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif
#include <vector>
#include <stdlib.h>
#include <algorithm>
#include <iostream>

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
// Color for the user-defined polygon and clipping window
float polygonColor[3] = { 0.0, 0.0, 1.0 }; // Default to blue

// Define the clipping window as a rectangle
Point clippingWindow[4] = {
    {100, 100}, // Bottom left
    {100, 300}, // Top left
    {300, 300}, // Top right
    {300, 100}  // Bottom right
};

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
    Point d = { line.p2.x - line.p1.x, line.p2.y - line.p1.y }; // Direction vector of the line

    for (const auto& edge : edges) {
        Point edgeDirection = { edge.end.x - edge.start.x, edge.end.y - edge.start.y };
        Point startToP1 = { line.p1.x - edge.start.x, line.p1.y - edge.start.y };
        float nDotD = dot(edge.normal, d);
        float nDotW = dot(edge.normal, startToP1);

        if (nDotD != 0.0f) {
            float t = -nDotW / nDotD;
            if (nDotD > 0.0f) {
                // Potential entry point, update tE
                if (t > tE) tE = t;
            }
            else {
                // Potential leaving point, update tL
                if (t < tL) tL = t;
            }
        }
        else if (nDotW < 0) {
            return Line{ {0, 0}, {0, 0} }; // Line is outside and parallel to edge
        }
    }

    if (tE > tL) {
        return Line{ {0, 0}, {0, 0} }; // No part of line within window
    }

    Line clippedLine;
    clippedLine.p1 = { line.p1.x + tE * d.x, line.p1.y + tE * d.y };
    clippedLine.p2 = { line.p1.x + tL * d.x, line.p1.y + tL * d.y };
    return clippedLine;
}


// Returns x-value of point of intersection of two lines
int x_intersect(int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4)
{
    int num = (x1 * y2 - y1 * x2) * (x3 - x4) -
        (x1 - x2) * (x3 * y4 - y3 * x4);
    int den = (x1 - x2) * (y3 - y4) - (y1 - y2) * (x3 - x4);
    return num / den;
}

// Returns y-value of point of intersection of
// two lines
int y_intersect(int x1, int y1, int x2, int y2,
    int x3, int y3, int x4, int y4)
{
    int num = (x1 * y2 - y1 * x2) * (y3 - y4) -
        (y1 - y2) * (x3 * y4 - y3 * x4);
    int den = (x1 - x2) * (y3 - y4) - (y1 - y2) * (x3 - x4);
    return num / den;
}



// Clip function for Sutherland-Hodgman
void clip(const std::vector<Point>& polyPoints, int poly_size,
    int x1, int y1, int x2, int y2, std::vector<Point>& clippedPoints) {
    clippedPoints.clear(); // Limpiar el vector antes de agregar nuevos puntos

    for (int i = 0; i < poly_size; i++) {
        int k = (i + 1) % poly_size;
        int ix = polyPoints[i].x, iy = polyPoints[i].y;
        int kx = polyPoints[k].x, ky = polyPoints[k].y;

        int i_pos = (x2 - x1) * (iy - y1) - (y2 - y1) * (ix - x1);
        int k_pos = (x2 - x1) * (ky - y1) - (y2 - y1) * (kx - x1);

        if (i_pos < 0 && k_pos < 0) {
            // Solo se agrega el segundo punto
            clippedPoints.push_back({ static_cast<float>(kx), static_cast<float>(ky) });
        }
        else if (i_pos >= 0 && k_pos < 0) {
            // Punto de intersección con el borde y se agrega el segundo punto

            Point newPoint1 = { static_cast<float>(x_intersect(x1, y1, x2, y2, ix, iy, kx, ky)),
                                      static_cast<float>(y_intersect(x1, y1, x2, y2, ix, iy, kx, ky)) };
            clippedPoints.push_back(newPoint1);

            Point newPoint2 = { static_cast<float>(kx), static_cast<float>(ky) };
            clippedPoints.push_back(newPoint2);
        }
        else if (i_pos < 0 && k_pos >= 0) {
            // Solo se agrega el punto de intersección con el borde

            Point newPoint3 = { static_cast<float>(x_intersect(x1, y1, x2, y2, ix, iy, kx, ky)),
                                      static_cast<float>(y_intersect(x1, y1, x2, y2, ix, iy, kx, ky)) };
            clippedPoints.push_back(newPoint3);
        }
        // Caso 4: Cuando ambos puntos están fuera, no se agregan puntos

        
    }
}
// Sutherland-Hodgman line-clipping algorithm
void clipLineSutherlandHodgman(const std::vector<Point>& polyPoints, std::vector<Point>& clippedPoints,
    const std::vector<Point>& clipperPoints) {
    int poly_size = polyPoints.size();    

    for (size_t i = 0; i < clipperPoints.size(); i++) {
        size_t k = (i + 1) % clipperPoints.size();

        // Clip the polygon against the current clipper edge
        clip(polyPoints, poly_size, clipperPoints[i].x, clipperPoints[i].y,
            clipperPoints[k].x, clipperPoints[k].y, clippedPoints);
    }
    
}
void performSutherlandHodgmanClipping(const std::vector<Point>& polyPoints, const std::vector<Edge>& edges) {
    std::vector<Point> clippedPoints = polyPoints; // Inicializamos con el polígono original
   
    for (size_t i = 0; i < edges.size(); ++i) {
        std::vector<Point> inputPoints = clippedPoints;
        clippedPoints.clear();
        clip(inputPoints, inputPoints.size(), edges[i].start.x, edges[i].start.y, edges[i].end.x, edges[i].end.y, clippedPoints);

        if (clippedPoints.empty()) {
            std::cout << "Polygon outside the window after clipping with edge " << i << std::endl;
            break;
        }
    }

    // Draw the clipped polygon
    if (!clippedPoints.empty()) {
        std::cout << "Drawing clipped polygon with " << clippedPoints.size() << " points." << std::endl;

        glColor3f(1.0, 0.0, 0.0); // Red color for the clipped polygon
        glBegin(GL_LINE_LOOP);
        for (const auto& point : clippedPoints) {
            glVertex2f(point.x, point.y);
        }
        glEnd();
    }


}



void mouse(int button, int state, int x, int y) {
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        if (userPolygon.size() < maxPoints) {
            // Convert screen coordinates to world coordinates if necessary
            Point newPoint = { static_cast<float>(x), static_cast<float>(windowHeight - y) };
            userPolygon.push_back(newPoint);

            // Redraw the window to show the updated polygon
            glutPostRedisplay();
        }
    }
}

// Function to draw a line
void drawLine(Point p1, Point p2) {
    glBegin(GL_LINES);
    glVertex2f(p1.x, p1.y);
    glVertex2f(p2.x, p2.y);
    glEnd();
}

// Function to draw the clipping window
void drawClippingWindow() {
    glColor3f(1.0, 0.0, 0.0); // Red color for the clipping window
    for (int i = 0; i < 4; ++i) {
        drawLine(clippingWindow[i], clippingWindow[(i + 1) % 4]);
    }
}

// Menu action identifiers
enum MenuActions {
    DRAW_POLYGON,
    DRAW_WINDOW,
    PERFORM_CLIPPING,
    SUTHERLAND_HODGMAN_CLIPPING,
    CLEAR_ALL,
    COLOR_RED,
    COLOR_GREEN,
    COLOR_BLUE,
    COLOR_YELLOW,
    COLOR_PURPLE
};



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
bool performClippingSH=false;

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
        break;
    case PERFORM_CLIPPING:
        if (userPolygon.size() >= 3) {
            performClippingMode = true;
            performClippingSH = false;
        }
        break;
    case SUTHERLAND_HODGMAN_CLIPPING:
        if (userPolygon.size() >= 3) {
            performClippingMode = false;
            performClippingSH = true;
        }
        break;
    case CLEAR_ALL:
        drawPolygonMode = false;
        drawWindowMode = false;
        performClippingMode = false;
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
    glutAddMenuEntry("Perform Clipping SH", SUTHERLAND_HODGMAN_CLIPPING);
    glutAddMenuEntry("Clear All", CLEAR_ALL);
    glutAddSubMenu("Colors", colorMenu);
    glutAttachMenu(GLUT_RIGHT_BUTTON);
}




void display() {
    glClear(GL_COLOR_BUFFER_BIT);

    if (drawWindowMode || performClippingMode || drawPolygonMode) {
        // Draw the clipping window in red
       // glColor3f(1.0, 0.0, 0.0); // Red color for the clipping window
        glColor3fv(polygonColor); // Set color for the clipping window

        glBegin(GL_LINE_LOOP);
        for (int i = 0; i < 4; ++i) {
            glVertex2f(clippingWindow[i].x, clippingWindow[i].y);
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
        std::vector<Edge> edges = buildEdges(std::vector<Point>(std::begin(clippingWindow), std::end(clippingWindow)));
        std::vector<Point> clippedPoints; // To store the endpoints of clipped segments

        for (size_t i = 0; i < userPolygon.size(); ++i) {
            Line lineSegment = { userPolygon[i], userPolygon[(i + 1) % userPolygon.size()] };
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
    if (performClippingSH)
    {

        performSutherlandHodgmanClipping(userPolygon, buildEdges(std::vector<Point>(std::begin(clippingWindow), std::end(clippingWindow))));
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
