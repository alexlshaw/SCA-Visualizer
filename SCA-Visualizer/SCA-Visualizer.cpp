// SCA-Visualizer.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "glad/glad.h"
#include "GLFW/glfw3.h"
#include "glm/glm.hpp"
#include "glm\gtc\matrix_transform.hpp"
#include "time.h"
#include <iostream>
#include <vector>
#include "MapLayer.h"
#include "RoadNetwork.h"
#include "Shader.h"
#include "Vertex.h"
#include "Voronoi.h"

GLFWwindow* mainWindow = nullptr;
bool shouldExit = false;
int screenWidth = 1024, screenHeight = 1024;

Shader* basic = nullptr;
Shader* texturedUnlit = nullptr;
int uBProjMatrix, uBModelMatrix, uTProjMatrix, uTModelMatrix, uTex;

MapLayer* heightLayer;
MapLayer* streetLayer;
RoadNetwork* network;
Voronoi* voro;
bool showVoronoiOverlay = false;
bool showNetworkOverlay = true;
int layerToDraw = 0;

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
	if (key == GLFW_KEY_V && action == GLFW_PRESS)
	{
		//generate a voronoi diagram
		if (voro == nullptr)
		{
			voro = new Voronoi(0.1f, network->startingLocations, 0.0f, (float)screenWidth - 1.0f, 0.0f, (float)screenHeight - 1.0f);
			voro->BuildMesh();
			showVoronoiOverlay = true;
		}
		else
		{
			showVoronoiOverlay = !showVoronoiOverlay;
		}
	}
	if (key == GLFW_KEY_N && action == GLFW_PRESS)
	{
		showNetworkOverlay = !showNetworkOverlay;
	}
	if (key == GLFW_KEY_L && action == GLFW_PRESS)
	{
		layerToDraw = layerToDraw == 0 ? 1 : 0;
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
	glClearColor(0.0f, 0.0f, 1.0f, 1.0f);
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
	//load shaders
	basic = new Shader();
	basic->compileShaderFromFile("./Data/Shaders/basic.vert", VERTEX);
	basic->compileShaderFromFile("./Data/Shaders/basic.frag", FRAGMENT);
	basic->linkAndValidate();
	uBProjMatrix = basic->getUniformLocation("projectionViewMatrix");
	uBModelMatrix = basic->getUniformLocation("modelMatrix");
	texturedUnlit = new Shader();
	texturedUnlit->compileShaderFromFile("./Data/Shaders/TexturedUnlit.vert", VERTEX);
	texturedUnlit->compileShaderFromFile("./Data/Shaders/TexturedUnlit.frag", FRAGMENT);
	texturedUnlit->linkAndValidate();
	uTProjMatrix = texturedUnlit->getUniformLocation("projectionViewMatrix");
	uTModelMatrix = texturedUnlit->getUniformLocation("modelMatrix");
	uTex = texturedUnlit->getUniformLocation("tex");

	int width, height;
	glfwGetFramebufferSize(mainWindow, &width, &height);
	glViewport(0, 0, width, height);
}

void draw()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glm::mat4 projection = glm::ortho(0.0f, (float)screenWidth, 0.0f, (float)screenHeight, -1.0f, 1.0f);
	glm::mat4 modelview = glm::identity<glm::mat4>();
	
	texturedUnlit->use();
	texturedUnlit->setUniform(uTModelMatrix, modelview);
	texturedUnlit->setUniform(uTProjMatrix, projection);
	texturedUnlit->setUniform(uTex, 0);
	if (layerToDraw == 0)
	{
		heightLayer->Draw();
	}
	else
	{
		streetLayer->Draw();
	}
	

	basic->use();
	basic->setUniform(uBModelMatrix, modelview);
	basic->setUniform(uBProjMatrix, projection);
	if (showNetworkOverlay)
	{
		network->DrawMesh();
	}

	if (showVoronoiOverlay)
	{
		glLineWidth(5.0f);
		voro->DrawDiagram();
		glLineWidth(1.0f);
	}

	glfwSwapBuffers(mainWindow);
}

void exit()
{
	if (voro != nullptr)
	{
		delete voro;
		voro = nullptr;
	}
	if (network != nullptr)
	{
		delete network;
		network = nullptr;
	}
	if (heightLayer != nullptr)
	{
		delete heightLayer;
		heightLayer = nullptr;
	}
	if (streetLayer != nullptr)
	{
		delete streetLayer;
		streetLayer = nullptr;
	}
	if (basic != nullptr)
	{
		delete basic;
		basic = nullptr;
	}
	if (texturedUnlit != nullptr)
	{
		delete texturedUnlit;
		texturedUnlit = nullptr;
	}
	glfwTerminate();
}

void generateData()
{
	//load the map data
	heightLayer = new MapLayer("D:\\Data\\Topographical\\AucklandTest.tga", 1024, 1024, MAPTYPE_HEIGHT);
	streetLayer = new MapLayer("D:\\Data\\Topographical\\OSM Images\\AucklandOSMScale2.tga", 1024, 1024, MAPTYPE_ROADS);
	//streetLayer = new MapLayer("D:\\Data\\Topographical\\OSM Images\\AucklandOtherScale.tga", 1024, 1024, MAPTYPE_ROADS);
	//generate the network
	int sTime = (int)time(NULL);
	network = new RoadNetwork(heightLayer, streetLayer);
	int tTime = (int)time(NULL) - sTime;
	printf("Generation time: %i\n", tTime);
}

int main()
{
	if (!init_GLFW())
	{
		return -1;
	}
	init_GL();
	srand((unsigned int)(time(NULL)));
	generateData();
	/* Loop until the user closes the window */
	while (!shouldExit && !glfwWindowShouldClose(mainWindow))
	{
		draw();
		glfwPollEvents();
	}
	exit();
	return 0;
}