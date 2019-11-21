// SCA-Visualizer.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "glad/glad.h"
#include "GLFW/glfw3.h"
#include "glm/glm.hpp"
#include "glm\gtc\matrix_transform.hpp"
#include <iostream>
#include <vector>
#include "RoadNetwork.h"
#include "Shader.h"
#include "Vertex.h"

GLFWwindow* mainWindow = nullptr;
bool shouldExit = false;
int screenWidth = 1024, screenHeight = 768;

Shader* basic = nullptr;
int uProjMatrix, uModelMatrix;
GLuint vbo, vao, ibo;
int indexCount;

RoadNetwork* network;

void error_callback(int error, const char* description)
{
	fprintf(stderr, "Error: %s\n", description);
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
	{
		shouldExit = true;
	}
}

int init_GLFW()
{
	glfwSetErrorCallback(error_callback);
	if (!glfwInit())
	{
		printf("GLFW init error\n");
		return -1;
	}
	//Create a windowed mode window and its OpenGL context. Note: If I hint anything over 3.0 it doesn't like it, but testing shows I'm getting a 4.6 context
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
	mainWindow = glfwCreateWindow(screenWidth, screenHeight, "SCA Vis", NULL, NULL);
	if (!mainWindow)
	{
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(mainWindow);
	//input handlers
	glfwSetKeyCallback(mainWindow, key_callback);
	//glfwSetCursorPosCallback(mainWindow, cursor_position_callback);
	//set up glad
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		printf("GLAD loader initialization error!\n");
		return -1;
	}
	glfwSwapInterval(1);
	return 1;
}

void init_GL()
{
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glClearDepth(1.0);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_PROGRAM_POINT_SIZE);
	glActiveTexture(GL_TEXTURE0);
	if (glGetError() != GL_NO_ERROR)
	{
		printf("GL Initialisation error\n");
	}

	basic = new Shader();
	basic->compileShaderFromFile("./Data/Shaders/basic.vert", VERTEX);
	basic->compileShaderFromFile("./Data/Shaders/basic.frag", FRAGMENT);
	basic->linkAndValidate();
	uProjMatrix = basic->getUniformLocation("projectionViewMatrix");
	uModelMatrix = basic->getUniformLocation("modelMatrix");

	int width, height;
	glfwGetFramebufferSize(mainWindow, &width, &height);
	glViewport(0, 0, width, height);
}

void draw()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glm::mat4 projection = glm::ortho(0.0f, (float)screenWidth, 0.0f, (float)screenHeight, -1.0f, 1.0f);
	glm::mat4 modelview = glm::identity<glm::mat4>();
	basic->use();
	basic->setUniform(uModelMatrix, modelview);
	basic->setUniform(uProjMatrix, projection);

	network->DrawMesh();

	glfwSwapBuffers(mainWindow);
}

void createNetworkMesh()
{
	std::vector<Vertex> vertices;
	std::vector<int> indices;
	glm::vec4 nCol = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);		
	vertices.push_back(Vertex(glm::vec4(-100.0f, -100.0f, 0.0f, 1.0f), nCol));
	vertices.push_back(Vertex(glm::vec4(100.0f, 100.0f, 0.0f, 1.0f), nCol));
	//vertices.push_back(Vertex(glm::vec4(-100.0f, 100.0f, 0.0f, 1.0f), nCol));
	indices.push_back(0);
	indices.push_back(1);
	//indices.push_back(2);
	indexCount = indices.size();
	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vbo);
	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * vertices.size(), &vertices[0], GL_STATIC_DRAW);
	//position
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const GLvoid*)0);
	glEnableVertexAttribArray(0);
	//color
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const GLvoid*)16);
	glEnableVertexAttribArray(1);
	glGenBuffers(1, &ibo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * indices.size(), &indices[0], GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void exit()
{
	delete network;
	network = nullptr;
	delete basic;
	basic = nullptr;
	glfwTerminate();
}

int main()
{
	if (!init_GLFW())
	{
		return -1;
	}
	init_GL();
	//createNetworkMesh();
	network = new RoadNetwork();
	/* Loop until the user closes the window */
	while (!shouldExit && !glfwWindowShouldClose(mainWindow))
	{
		draw();
		glfwPollEvents();
	}
	exit();
	return 0;
}