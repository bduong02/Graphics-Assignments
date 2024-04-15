// RotateLetter.cpp: rotate letter with mouse

#include <glad.h>
#include <GLFW/glfw3.h>
#include "Draw.h"
#include "Text.h"
#include "GLXtras.h"
#include "VecMat.h"

// a triangle (3 2D locations, 3 RGB colors)
// for letter D
vec2 points[] = {
{150, 230}, {50, 50}, {150, 75}, {220, 110}, {250, 150},
{250, 270}, {230, 310}, {170, 350}, {50, 350}, {50, 230} };


const int nPoints = sizeof(points) / sizeof(vec2);
vec3 colors[] = { {0, 0, 1}, {1, 0, 0}, {0, 1, 0} };

// vertex indices of triangles
int triangles[][3] = { {0,1,2}, {0,2,3}, {0,3,4}, {0,4,5},
{0,5,6}, {0,6,7}, {0,7,8}, {0,8,9}, {0, 9, 1} };

// OpenGL IDs for vertex buffer and shader program
GLuint vBuffer = 0, program = 0;

// display
int winWidth = 400, winHeight = 400;					// window size, in pixels
vec2 mouseWas, mouseNow;								// rotation control
mat4 standardizeMat;									// scale/center letter

// Shaders

const char *vertexShader = R"(
	#version 130
	in vec2 point;
	in vec3 color;
	out vec4 vColor;
	uniform mat4 view;
	void main() {
		gl_Position = view*vec4(point, 0, 1);			// promote point to 4D, transform by view
		vColor = vec4(color, 1);
	}
)";

const char *pixelShader = R"(
	#version 130
	in vec4 vColor;
	out vec4 pColor;
	void main() {
		pColor = vColor;
	}
)";

// Display

void Display() {
	glClearColor(0,0,0,1);//1, 1, 1, 1);
	glClear(GL_COLOR_BUFFER_BIT);
	glUseProgram(program);
	// set transform
	mat4 view = RotateY(mouseNow.x)*RotateX(mouseNow.y)*standardizeMat;
	SetUniform(program, "view", view);
	// connect GPU buffer to vertex shader
	glBindBuffer(GL_ARRAY_BUFFER, vBuffer);
	VertexAttribPointer(program, "point", 2, 0, (void *) 0);
	VertexAttribPointer(program, "color", 3, 0, (void *) sizeof(points));
	// render
	glDrawElements(GL_TRIANGLES, sizeof(triangles)/sizeof(int), GL_UNSIGNED_INT, triangles);
	// test connectivity
	UseDrawShader(view);
	for (int i = 0; i < nPoints; i++) {
		vec2 p = points[i];
		Disk(p, 8, vec3(1, 1, 0));
		Text(vec3(p.x, p.y, 0), view, vec3(1, 1, 0), 10, " v%i", i);
	}
	int nTriangles = sizeof(triangles)/(3*sizeof(int));
	for (int i = 0; i < nTriangles; i++) {
		int3 t = triangles[i];
		vec2 p1 = points[t.i1], p2 = points[t.i2], p3 = points[t.i3], c = (p1+p2+p3)/3;
		Line(p1, p2, 1, vec3(0, 1, 1));
		Line(p2, p2, 1, vec3(0, 1, 1));
		Line(p3, p1, 1, vec3(0, 1, 1));
		Text(vec3(c.x, c.y, 0), view, vec3(0, 1, 1), 10, "t%i", i);
	}
	glFlush();
}

// Mouse Callbacks

void MouseButton(float x, float y, bool left, bool down) {
	if (left && down)
		mouseWas = vec2(x, y);
}

void MouseMove(float x, float y, bool leftDown, bool rightDown) {
	if (leftDown) {
		vec2 m(x, y);
		mouseNow += (m-mouseWas);
		mouseWas = m;
	}
}

// Initialization

void StandardizePoints(float s = 1) {
	// scale and offset so points in +/-s, centered at origin
	vec2 min, max;
	float range = Bounds(points, nPoints, min, max);
	float scale = 2*s/range;
	vec2 center = (min+max)/2;
	for (int i = 0; i < nPoints; i++)
		points[i] = scale*(points[i]-center);
}

mat4 StandardizeMatrix(float s = 1) {
	vec2 min, max;
	float f = 2*s/Bounds(points, nPoints, min, max);
	return Scale(f)*Translate(-vec3((min+max)/2));
}

void BufferVertices(vec2 *points, vec3 *colors, int npoints) {
	// create GPU buffer, make it active
	glGenBuffers(1, &vBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vBuffer);
	// allocate memory
	int sPoints = npoints*sizeof(vec2), sColors = npoints*sizeof(vec3);
	glBufferData(GL_ARRAY_BUFFER, sPoints+sColors, NULL, GL_STATIC_DRAW);
	// copy to sub-buffers
	glBufferSubData(GL_ARRAY_BUFFER, 0, sPoints, points);
	glBufferSubData(GL_ARRAY_BUFFER, sPoints, sColors, colors);
}

// Application

int main() {
	// init window
	GLFWwindow *w = InitGLFW(100, 100, winWidth, winHeight, "Rotate Letter");
	program = LinkProgramViaCode(&vertexShader, &pixelShader);
	// fit letter to window
//	StandardizePoints(.8f);						// option: normalize size, center letter
	standardizeMat = StandardizeMatrix(.8f);	// option: use matrix to normalize and center
	// copy vertices to GPU memory
	BufferVertices(points, colors, nPoints);
	// callbacks and event loop
	RegisterMouseButton(MouseButton);
	RegisterMouseMove(MouseMove);
	while (!glfwWindowShouldClose(w)) {
		Display();
		glfwSwapBuffers(w);
		glfwPollEvents();
	}
	// finish
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glDeleteBuffers(1, &vBuffer);
	glfwDestroyWindow(w);
	glfwTerminate();
}
