// Bryan Duong
// Assignment-5.cpp
// 4/30/24
// This program reads in a OBJ file with
// texture image and uses the keyboard
// to toggle between the shading and highlighting.

#include <glad.h>
#include <GLFW/glfw3.h>
#include "Camera.h"
#include "Draw.h"
#include "GLXtras.h"
#include "IO.h"
#include "VecMat.h"
#include "Widgets.h"
#include <vector>

// display
int winWidth = 800, winHeight = 800;
Camera camera(0, 0, winWidth, winHeight, vec3(15, -15, 0), vec3(0, 0, -5), 30);


vector<vec3> points; // vertex locations
vector<vec3> normals; // surface normals
vector<vec2> uvs; // texture coordinates
vector<int3> triangles; // triplets of vertex indices

const int nPoints = sizeof(points)/sizeof(vec3);

// OpenGL IDs for vertex buffer, shader program
GLuint vBuffer = 0, program = 0;

// texture image
const char *textFilename = "C:/Users/duong/Graphics/Apps/donutTextureImage.jpg";
GLuint textureName = 0;
int textureUnit = 0;

// movable lights       
vec3 lights[] = { {.5, 0, 1}, {1, 1, 0} };
const int nLights = sizeof(lights)/sizeof(vec3);

// interaction
void *picked = NULL;	// if non-null: light or camera
Mover mover;

bool useFacetedNormal = true;
float ambientValue = 0.1f;
float diffuseValue = 0.5f;
float specularValue = 0.8f;
float shininessValue = 32.0f;

// Shaders

const char *vertexShader = R"(
	#version 130
	in vec3 vPoint;
	in vec2 uv;
	in vec3 normal;
	out vec2 vUv;
	out vec3 vNormal;
	uniform mat4 modelview, persp;
	void main() {
		gl_Position = persp * modelview * vec4(vPoint, 1);
		vUv = uv;
		vNormal = normalize((modelview * vec4(normal, 0)).xyz);
	}
)";

const char* pixelShader = R"(
	#version 130
	in vec2 vUv; // Receive texture coordinates from vertex shader
	in vec3 vNormal;
	out vec4 pColor;
	uniform sampler2D textureImage;
	uniform bool useFacetedNormal;
	uniform float ambientValue;
    uniform float diffuseValue;
    uniform float specularValue;
    uniform float shininessValue;

	void main() {
		vec3 N;
		if (useFacetedNormal) {
			N = normalize(vNormal); // Use normal from the vertex shader (faceted shading)
		} else {
			N = normalize(cross(dFdx(vPosition), dFdy(vPosition))); // Use normal from the rasterizer (smooth shading)
		}
		vec4 textColor = texture(textureImage, vUv); // Sample the texture using texture coordinates
		pColor = textColor; // Set the fragment color to the sampled texture color
	}

)";

// Display

void Display(GLFWwindow *w) {
	// clear screen, enable blend, z-buffer
	glClearColor(1, 1, 1, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	// init shader program, connect GPU buffer to vertex shader
	glUseProgram(program);
	glBindBuffer(GL_ARRAY_BUFFER, vBuffer);
	VertexAttribPointer(program, "point", 3, 0, (void *) 0);
	VertexAttribPointer(program, "uv", 2, 0, (void *) sizeof(points));
	VertexAttribPointer(program, "normal", 3, 0, (void*)(sizeof(points) + sizeof(uvs))); // Add normals
	// update matrices
	SetUniform(program, "modelview", camera.modelview);
	SetUniform(program, "persp", camera.persp);
	// update/transform lights
	SetUniform(program, "nLights", nLights);
	SetUniform3v(program, "lights", nLights, (float *) lights, camera.modelview);
	// bind textureName to textureUnit
	glBindTexture(GL_TEXTURE_2D, textureName);
	glActiveTexture(GL_TEXTURE0+textureUnit);
	SetUniform(program, "textureImage", textureUnit);
	// render
	int nVertices = triangles.size() * 3;
	glDrawElements(GL_TRIANGLES, nVertices, GL_UNSIGNED_INT, triangles.data());
	// annotation
	glDisable(GL_DEPTH_TEST);
	UseDrawShader(camera.fullview);
	for (int i = 0; i < nLights; i++)
		Star(lights[i], 8, vec3(1, .8f, 0), vec3(0, 0, 1));
	if (picked == &camera && !Shift())
		camera.arcball.Draw(Control());
	glFlush();
}

// Mouse Callbacks

void MouseButton(float x, float y, bool left, bool down) {
	picked = NULL;
	if (left && down) {
		// light picked?
		for (int i = 0; i < nLights; i++)
			if (MouseOver(x, y, lights[i], camera.fullview)) {
				picked = &mover;
				mover.Down(&lights[i], (int) x, (int) y, camera.modelview, camera.persp);
			}
		if (picked == NULL) {
			picked = &camera;
			camera.Down(x, y, Shift(), Control());
		}
	}
	else camera.Up();
}

void MouseMove(float x, float y, bool leftDown, bool rightDown) {
	if (leftDown) {
		if (picked == &mover)
			mover.Drag((int) x, (int) y, camera.modelview, camera.persp);
		if (picked == &camera)
			camera.Drag(x, y);
	}
}

void MouseWheel(float spin) {
	camera.Wheel(spin, Shift());
}

// Initialization

void BufferVertices() {
	// create GPU buffer, make it active
	glGenBuffers(1, &vBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vBuffer);
	// allocate/load memory for points and uvs
	int sPoints = sizeof(points), sUvs = sizeof(uvs), sNormals = sizeof(normals);
	glBufferData(GL_ARRAY_BUFFER, sPoints+sUvs, NULL, GL_STATIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, points.size() * sizeof(vec3), points.data());
	glBufferSubData(GL_ARRAY_BUFFER, points.size() * sizeof(vec3), uvs.size() * sizeof(vec2), uvs.data());
	glBufferSubData(GL_ARRAY_BUFFER, points.size() * sizeof(vec3) + uvs.size() * sizeof(vec2), normals.size() * sizeof(vec3), normals.data());

}

// Application

void WriteObjFile(const char *filename) {
	FILE *file = fopen(filename, "w");
	if (!file)
		printf("can't save %s\n", filename);
	else {
		int nVertices = sizeof(triangles)/sizeof(int), nTriangles = nVertices/3;
		fprintf(file, "\n# %i vertices\n", nPoints);
		for (int i = 0; i < nPoints; i++)
			fprintf(file, "v %f %f %f \n", points[i].x, points[i].y, points[i].z);
		fprintf(file, "\n# %i textures\n", nPoints);
		for (int i = 0; i < nPoints; i++)
			fprintf(file, "vt %f %f \n", uvs[i].x, uvs[i].y);
		fprintf(file, "\n# %i triangles\n", nTriangles);
		for (int i = 0; i < nTriangles; i++)
			fprintf(file, "f %i %i %i \n", 1+triangles[i][0], 1+triangles[i][1], 1+triangles[i][2]); // OBJ format
		fclose(file);
		printf("%s written\n", filename);
	}
}

void Keyboard(int key, bool press, bool shift, bool control) {
	if (press && key == 'S')
		WriteObjFile("C:/Users/Duong/Graphics/Apps/Doughnut_OBJ.obj");
	if (press && key == 'F') { // Toggle between faceted and smooth shading when the 'F' key is pressed
		useFacetedNormal = !useFacetedNormal;
		glUseProgram(program);
		glUniform1i(glGetUniformLocation(program, "useFacetedNormal"), useFacetedNormal);
	}

	// Varying the pixel shader values of amb, dif, spc
	if (press) {
		switch (key) {
		case GLFW_KEY_A:
			ambientValue += 0.1f;
			break;
		case GLFW_KEY_D:
			diffuseValue += 0.1f;
			break;
		case GLFW_KEY_S:
			specularValue += 0.1f;
			break;
		case GLFW_KEY_E:
			shininessValue += 10.0f;
			break;
		case GLFW_KEY_R:
			shininessValue -= 10.0f;
			if (shininessValue < 0.0f) shininessValue = 0.0f;
			break;
		}
		// Update shader program with new values
		glUseProgram(program);
		glUniform1f(glGetUniformLocation(program, "ambientValue"), ambientValue);
		glUniform1f(glGetUniformLocation(program, "diffuseValue"), diffuseValue);
		glUniform1f(glGetUniformLocation(program, "specularValue"), specularValue);
		glUniform1f(glGetUniformLocation(program, "shininessValue"), shininessValue);
	}
}

void Resize(int width, int height) {
	camera.Resize(width, height);
	glViewport(0, 0, width, height);
}

int main(int ac, char **av) {
	// enable anti-alias, init app window and GL context
	GLFWwindow *w = InitGLFW(100, 100, winWidth, winHeight, "Textured Letter");
	// init shader program, set GPU buffer, read texture image
	program = LinkProgramViaCode(&vertexShader, &pixelShader);
	Standardize(points.data(), points.size(), .8f);   // fit points to +/- .8 space
	BufferVertices();
	textureName = ReadTexture(textFilename);
	// callbacks
	RegisterMouseMove(MouseMove);
	RegisterMouseButton(MouseButton);
	RegisterMouseWheel(MouseWheel);
	RegisterResize(Resize);
	RegisterKeyboard(Keyboard);
	printf("Usage: S to save as OBJ file\n");

	if (!ReadAsciiObj("Doughnut_OBJ.obj", points, triangles, &normals, &uvs))
	{
		printf("can’t read %s\n", "Doughnut_OBJ.obj");
		return 1;
	}
	if (!normals.size())
		SetVertexNormals(points, triangles, normals);
	// event loop
	while (!glfwWindowShouldClose(w)) {
		glfwPollEvents();
		Display(w);
		glfwSwapBuffers(w);
	}
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glDeleteBuffers(1, &vBuffer);
	glfwDestroyWindow(w);
	glfwTerminate();

	return 0;
}
