// Bryan Duong
// Assn-2-RotateLetter.cpp
// This program uses an array of vertices of a triangle to produce a 
// letter D that can be rotated through the mouse. Additionally,
// it also produces two letters that can be rotated on both sides of
// the screen.
// 4/9/24

#include <glad.h>
#include <glfw3.h>
#include "GLXtras.h"
#include "VecMat.h"
#include <cmath>

GLuint vBuffer = 0; // GPU buffer ID
GLuint program = 0; // GLSL shader program ID
GLfloat zRotation = 0.0f; // used for rotation around the z-axis

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

vec2 mouseNow; // in pixels
vec2 mouseWas;
mat4 standardizeMat;  // matrix used to standardize

// Define initial positions for letters
vec2 leftLetterPos = { -0.5f, 0.0f }; // Left letter initial position
vec2 rightLetterPos = { 0.5f, 0.0f }; // Right letter initial position

// Define rotation angles for each letter
float leftRotationAngle = 0.0f; // Initial rotation angle for left letter
float rightRotationAngle = 0.0f; // Initial rotation angle for right letter

const char *vertexShader = R"(
	#version 130
	in vec2 point;
	in vec3 color;
	out vec3 vColor;
	uniform mat4 view;	
	void main() {
		gl_Position = view * vec4(point, 0, 1);
		vColor = color;
	}
)";

const char *pixelShader = R"(
	#version 130
	in vec3 vColor;
	out vec4 pColor;
	void main() {
		pColor = vec4(vColor, 1); // 1: fully opaque
	}
)";

void Display() {
	// clear background
	glClearColor(1, 1, 1, 1);
	glClear(GL_COLOR_BUFFER_BIT);
	// run shader program, enable GPU vertex buffer
	glUseProgram(program);
	glBindBuffer(GL_ARRAY_BUFFER, vBuffer);
	// connect GPU point and color buffers to shader inputs
	VertexAttribPointer(program, "point", 2, 0, (void *) 0);
	VertexAttribPointer(program, "color", 3, 0, (void *) sizeof(points));
	// create compound transform and send to shader
	mat4 view = RotateY(mouseNow.x) * RotateX(mouseNow.y) * RotateZ(zRotation);
	// RotateX, RotateY are in VecMat.h
	SetUniform(program, "view", view*standardizeMat);
	// render three vertices as one triangle
	int nVertices = sizeof(triangles) / sizeof(int);

	// Drawing left letter
	mat4 leftView = Translate(leftLetterPos.x, leftLetterPos.y, 0.0f) * RotateZ(leftRotationAngle) * standardizeMat * view;
	SetUniform(program, "view", leftView);
	glDrawElements(GL_TRIANGLES, nVertices, GL_UNSIGNED_INT, triangles);

	// Drawing right letter
	mat4 rightView = Translate(rightLetterPos.x, rightLetterPos.y, 0.0f) * RotateZ(rightRotationAngle) * standardizeMat * view;
	SetUniform(program, "view", rightView);
	glDrawElements(GL_TRIANGLES, nVertices, GL_UNSIGNED_INT, triangles);
	glFlush();
}

void BufferVertices() {
	// assign GPU buffer for points and colors, set it active
	glGenBuffers(1, &vBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vBuffer);
	// allocate
	int sPoints = sizeof(points), sColors = sizeof(colors);
	glBufferData(GL_ARRAY_BUFFER, sPoints+sColors, NULL, GL_STATIC_DRAW);
	// copy points to beginning of buffer, for length of points array
	glBufferSubData(GL_ARRAY_BUFFER, 0, sPoints, points);
	// copy colors, starting at end of points buffer, for length of colors array
	glBufferSubData(GL_ARRAY_BUFFER, sPoints, sColors, colors);
}

void StandardizePoints(float s = 1) {
	// scale and offset so points are in range +/-s, centered at origin
	vec2 min, max;
	float range = Bounds(points, nPoints, min, max); // in VecMat.h
	float scale = 2 * s / range;
	vec2 center = (min + max) / 2;
	for (int i = 0; i < nPoints; i++)
		points[i] = scale * (points[i] - center);
}

void MouseMove(float x, float y, bool leftDown, bool rightDown) {
	if (leftDown) {
		// change in mouse position since last frame
		vec2 mouseDelta = vec2(x, y) - mouseWas;
		mouseNow += mouseDelta;
		// current mouse position as the last position for next frame
		mouseWas = vec2(x, y);
	}
}

void MouseButton(float x, float y, bool left, bool down) {
	// called upon mouse-button press or releaSe
	if (left && down)
		mouseWas = vec2(x, y);
}

void MouseWheel(float spin) {
	// updating the rotation based on the mouse wheel spin
	zRotation += spin;
	leftRotationAngle += spin;
	rightRotationAngle += spin;
}

int main() {
	// init window
	GLFWwindow *w = InitGLFW(100, 100, 800, 800, "Colorful Triangle");
	// build shader program
	program = LinkProgramViaCode(&vertexShader, &pixelShader);
	// using a matrix to standardize rather than using StandardizePoints
	vec2 min, max;
	float f = 2 * 0.8 / Bounds(points, nPoints, min, max);
	standardizeMat = Scale(f) * Translate(-(min + max) / 2);
	// allocate GPU vertex memory
	BufferVertices();
	RegisterMouseMove(MouseMove);
	RegisterMouseButton(MouseButton);
	RegisterMouseWheel(MouseWheel);
	// event loop
	while (!glfwWindowShouldClose(w)) {
		Display();
		glfwSwapBuffers(w);
		glfwPollEvents();
	}
	// unbind vertex buffer, free GPU memory
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glDeleteBuffers(1, &vBuffer);
	glfwDestroyWindow(w);
	glfwTerminate();
}
