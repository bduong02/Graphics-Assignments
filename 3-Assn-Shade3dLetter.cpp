// Bryan Duong
// 3-Assn-Shade3dLetter.cpp
// This program produces a shaded image of the
// letter D using 3D vertices, lighting and the functionality
// from the Camera class. Additionally, it also allows
// users to enable/disable the highlights.
// 4/16/24

#include <glad.h>
#include <GLFW/glfw3.h>
#include "Draw.h"
#include "Text.h"
#include "GLXtras.h"
#include "VecMat.h"
#include "IO.h"
#include "Camera.h"
#include <iostream>

// display
int winWidth = 400, winHeight = 400;					// window size, in pixels
vec3 mouseWas, mouseNow;								// rotation control
mat4 standardizeMat;									// scale/center letter

vec3 points[] = { {150, 230}, {50, 50}, {150, 75}, {220, 110}, {250, 150},
{250, 270}, {230, 310}, {170, 350}, {50, 350}, {50, 230}, 

{150, 230, -50}, {50, 50, -50}, {150, 75, -50}, {220, 110, -50}, {250, 150, -50},
{250, 270, -50}, {230, 310, -50}, {170, 350, -50}, {50, 350, -50}, {50, 230, -50} }; // 3D vertices for the letter D

vec3 colors[] = { {0, 0, 1}, {1, 0, 0}, {0, 1, 0},
	{0, 0, 1}, {1, 0, 0}, {0, 1, 0}, {0.5, 0.5, 0.5} };	// colors for vertices

const int nPoints = sizeof(points) / sizeof(vec3);

// vertex indices of triangles
int triangles[][3] = { {0,1,2}, {0,2,3}, {0,3,4}, {0,4,5},
{0,5,6}, {0,6,7}, {0,7,8}, {0,8,9}, {0, 9, 1},

{10, 12, 11}, {10, 13, 12}, {10, 14, 13}, {10, 15, 14},
	{10, 16, 15}, {10, 17, 16}, {10, 18, 17}, {10, 19, 18}, {10, 11, 19},

	{1, 9, 11}, {11, 9, 19}, {2, 1, 11}, {12, 2, 11}, {3, 2, 12}, {13, 3, 12},
	{4, 3, 13}, {14, 4, 13}, {5, 4, 14}, {15, 5, 14}, {6, 5, 15}, {16, 6, 15},
	{7, 6, 16}, {17, 7, 16}, {8, 7, 17}, {18, 8, 17}, {9, 8, 18}, {19, 9, 18}
};

// OpenGL IDs for vertex buffer and shader program
GLuint vBuffer = 0, program = 0;

// Cameras used to view
Camera camera(0, 0, winWidth, winHeight, vec3(15, -30, 0), vec3(0, 0, -5), 30);

// Shaders
// bool variable to track whether highlights are on or not
bool onHighlights = true;

// Vertex shader
const char* vertexShader = R"(
	#version 130
	in vec3 point;
	in vec3 color;
	out vec4 vColor;
	out vec3 vPoint;
	uniform mat4 modelview, persp;
	void main() {
		// transforming vertex to world space
		vPoint = (modelview*vec4(point, 1.0)).xyz; // transformed to world space
		// transforming vertex to perspective space
		gl_Position = persp*vec4(vPoint, 1.0); // transformed to perspective space
		// passing color to pixel shader
		vColor = vec4(color, 1.0);
	}
)";

const char* pixelShader = R"(
	#version 130
	in vec4 vColor;
	in vec3 vPoint;
	out vec4 pColor;

	uniform vec3 light = vec3(1.0, 1.0, 1.0);
	uniform float amb = .1, dif = .8, spc =.7; // ambient, diffuse, specular weights
	uniform bool onHighlights;
	void main() {
		vec3 dx = dFdx(vPoint); // vPoint change along hor/vert raster
		vec3 dy = dFdy(vPoint); // vPoint change along hor/vert raster
		vec3 N = normalize(cross(dx, dy)); // unit-length surface normal
		vec3 L = normalize(light - vPoint); // unit-length light vector
		float d = abs(dot(N, L)); // diffuse term

		vec3 E = normalize(vPoint);

		vec3 R = reflect(L, N); // reflection vector
		float h = max(0, dot(R, E)); // highlight term
		float s = pow(h, 100); // specular term

		float intensity = min(1, amb+ dif * d) + spc * s; // weighted sum
		if(onHighlights) {
			intensity += spc * s;
		}
		pColor = vec4(intensity * vColor.rgb, 1); // opaque
	}
)";

// Display

void Display() {
	glClear(GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	glClearColor(0, 0, 0, 1);//1, 1, 1, 1);
	glClear(GL_COLOR_BUFFER_BIT);
	glUseProgram(program);

	// set transform
	mat4 view = RotateY(mouseNow.x) * RotateX(mouseNow.y) * standardizeMat;
	SetUniform(program, "modelview", camera.modelview);
	SetUniform(program, "persp", camera.persp);

	// connect GPU buffer to vertex shader
	glBindBuffer(GL_ARRAY_BUFFER, vBuffer);
	VertexAttribPointer(program, "point", 3, 0, (void*)0);
	VertexAttribPointer(program, "color", 3, 0, (void*)sizeof(points));

	// render
	glDrawElements(GL_TRIANGLES, sizeof(triangles) / sizeof(int), GL_UNSIGNED_INT, triangles);

	// test connectivity
	UseDrawShader(view);
	glDisable(GL_DEPTH_TEST);
	if (!Shift() && camera.down)
		camera.arcball.Draw(Control());
	glFlush();
}

// Mouse Callbacks
void MouseButton(float x, float y, bool left, bool down) {
	if (left && down)
		camera.Down(x, y, Shift(), Control());
	else camera.Up();
}

void MouseMove(float x, float y, bool leftDown, bool rightDown) {
	if (leftDown) {
		camera.Drag(x, y);
	}
}

void MouseWheel(float spin) {
	camera.Wheel(spin, Shift());
}

// Initialization

void StandardizePoints(float s = 1) {
	// scale and offset so points in +/-s, centered at origin
	vec3 min, max;
	float range = Bounds(points, nPoints, min, max);
	float scale = 2 * s / range;
	vec3 center = (min + max) / 2;
	for (int i = 0; i < nPoints; i++)
		points[i] = scale * (points[i] - center);
}

mat4 StandardizeMatrix(float s = 1) {
	vec3 min, max;
	float f = 2 * s / Bounds(points, nPoints, min, max);
	return Scale(f) * Translate(-vec3((min + max) / 2));
}

void BufferVertices(vec3* points, vec3* colors, int npoints) {
	// create GPU buffer, make it active
	glGenBuffers(1, &vBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vBuffer);
	// allocate memory
	int sPoints = npoints * sizeof(vec3), sColors = npoints * sizeof(vec3);
	glBufferData(GL_ARRAY_BUFFER, sPoints + sColors, NULL, GL_STATIC_DRAW);
	// copy to sub-buffers
	glBufferSubData(GL_ARRAY_BUFFER, 0, sPoints, points);
	glBufferSubData(GL_ARRAY_BUFFER, sPoints, sColors, colors);
}

// resize callback
void Resize(int width, int height) {
	glViewport(0, 0, width, height);
	camera.Resize(width, height);
}

// switches the mode of the highlights
void ToggleHighlights() {
	onHighlights = !onHighlights;
	if (onHighlights) {
		std::cout << "Highlights are on." << endl;
	}
	else {
		std::cout << "Highlights are off." << endl;
	}
}

void KeyCallback(GLFWwindow* w, int key, int scancode, int action, int mods) {
	// Highlights will be toggled on/off when the user presses H on the keyboard
	if (key == GLFW_KEY_H && action == GLFW_PRESS) {
		ToggleHighlights();
	}
}

// Application

int main() {
	// init window
	GLFWwindow* w = InitGLFW(100, 100, winWidth, winHeight, "Rotate Letter");
	program = LinkProgramViaCode(&vertexShader, &pixelShader);
	// fit letter to window
	Standardize(points, nPoints, .8f);
	standardizeMat = StandardizeMatrix(.8f);	// option: use matrix to normalize and center
	
	// copy vertices to GPU memory
	BufferVertices(points, colors, nPoints);
	// register keyboard callback
	glfwSetKeyCallback(w, KeyCallback);
	// callbacks and event loop
	
	RegisterMouseButton(MouseButton);
	RegisterMouseMove(MouseMove);
	RegisterMouseWheel(MouseWheel);
	RegisterResize(Resize);
	
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
