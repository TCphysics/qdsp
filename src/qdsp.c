#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <omp.h>

#include "glad/glad.h"
#include <GLFW/glfw3.h>
#include <SOIL/SOIL.h>

#include "qdsp.h"

static int makeShader(const char *filename, GLenum type);

static void resizeCallback(GLFWwindow *window, int width, int height);

static void keyCallback(GLFWwindow *window, int key, int code, int action, int mods);

QDSPplot *qdspInit(const char *title) {
	QDSPplot *plot = malloc(sizeof(QDSPplot));

	// create context
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	
	// make window, basic config
	plot->window = glfwCreateWindow(800, 600, title, NULL, NULL);
	if (plot->window == NULL) {
		fprintf(stderr, "Couldn't create window\n");
		glfwTerminate();
		free(plot);
		return NULL;
	}
	glfwMakeContextCurrent(plot->window);

	// we need to get the plot in the key hander
	glfwSetWindowUserPointer(plot->window, plot);
		
	glfwSetFramebufferSizeCallback(plot->window, resizeCallback);
	glfwSetKeyCallback(plot->window, keyCallback);
	
	// load extensions via GLAD
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		fprintf(stderr, "Couldn't initialize GLAD\n");
		glfwTerminate();
		free(plot);
		return NULL;
	}

	// create shaders and link program

	// for points
	int vertexShader = makeShader("vertex.glsl", GL_VERTEX_SHADER);
	int fragmentShader = makeShader("fragment.glsl", GL_FRAGMENT_SHADER);

	// for overlay
	int overVertexShader = makeShader("overlay-vertex.glsl", GL_VERTEX_SHADER);
	int overFragmentShader = makeShader("overlay-fragment.glsl", GL_FRAGMENT_SHADER);
	
	if (vertexShader == 0 || fragmentShader == 0 ||
	    overVertexShader == 0 || overFragmentShader == 0) {
		glfwTerminate();
		free(plot);
		return NULL;
	}

	plot->shaderProgram = glCreateProgram();
	glAttachShader(plot->shaderProgram, vertexShader);
	glAttachShader(plot->shaderProgram, fragmentShader);
	glLinkProgram(plot->shaderProgram);

	plot->overShaderProgram = glCreateProgram();
	glAttachShader(plot->overShaderProgram, overVertexShader);
	glAttachShader(plot->overShaderProgram, overFragmentShader);
	glLinkProgram(plot->overShaderProgram);

	int pointSuccess, overSuccess;
	glGetProgramiv(plot->shaderProgram, GL_LINK_STATUS, &pointSuccess);
	glGetProgramiv(plot->overShaderProgram, GL_LINK_STATUS, &overSuccess);
	if (!pointSuccess || !overSuccess) {
		char log[1024];
		glGetProgramInfoLog(plot->shaderProgram, 1024, NULL, log);
		fprintf(stderr, "Error linking program\n");
		fprintf(stderr, "%s\n", log);
		glfwTerminate();
		free(plot);
		return NULL;
	}

	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);
	glDeleteShader(overVertexShader);
	glDeleteShader(overFragmentShader);

	// buffer setup for points
	glGenVertexArrays(1, &plot->vertArrayObj);
	glGenBuffers(1, &plot->vertBufferObjX);
	glGenBuffers(1, &plot->vertBufferObjY);
	glGenBuffers(1, &plot->vertBufferObjCol);

	glBindVertexArray(plot->vertArrayObj);

	glBindBuffer(GL_ARRAY_BUFFER, plot->vertBufferObjX);
	glVertexAttribPointer(0, 1, GL_DOUBLE, GL_FALSE, 0, NULL);
	glEnableVertexAttribArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, plot->vertBufferObjY);
	glVertexAttribPointer(1, 1, GL_DOUBLE, GL_FALSE, 0, NULL);
	glEnableVertexAttribArray(1);

	glBindBuffer(GL_ARRAY_BUFFER, plot->vertBufferObjCol);
	glVertexAttribIPointer(2, 1, GL_INT, 0, NULL);
	glEnableVertexAttribArray(2);

	// buffer setup for overlay
	glGenVertexArrays(1, &plot->overVAO);
	glGenBuffers(1, &plot->overVBO);
	
	glBindVertexArray(plot->overVAO);

	glBindBuffer(GL_ARRAY_BUFFER, plot->overVBO);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), NULL);
	glEnableVertexAttribArray(0);

	// coords for overlay
	float overVertices[] = {
		// lower left triangle
		0.0f, 0.0f, 1.0f,
		0.0f, 1.0f, 1.0f,
		1.0f, 0.0f, 1.0f,
		// upper right triangle
		0.0f, 1.0f, 1.0f,
		1.0f, 0.0f, 1.0f,
		1.0f, 1.0f, 1.0f
	};
	glBufferData(GL_ARRAY_BUFFER, sizeof(overVertices), overVertices, GL_STATIC_DRAW);

	// overlay texture
	glGenTextures(1, &plot->overTex);
	glBindTexture(GL_TEXTURE_2D, plot->overTex);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// load image with SOIL
	int imgWidth, imgHeight;
	unsigned char *imgData =
		SOIL_load_image("/usr/local/share/qdsp/helpmessage.png",
		                &imgWidth, &imgHeight, NULL, SOIL_LOAD_RGB);

	if (imgData == NULL)
		// loading failed, try current dir
		imgData = SOIL_load_image("helpmessage.png", &imgWidth, &imgHeight,
		                          NULL, SOIL_LOAD_RGB);
		
	if (imgData == NULL) { // file is nowhere
		fprintf(stderr, "Error loading image file helpmessage.png\n");
		glfwTerminate();
		free(plot);
		return NULL;
	}

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, imgWidth, imgHeight, 0, GL_RGB,
	             GL_UNSIGNED_BYTE, imgData);
	
	SOIL_free_image_data(imgData);

	// dimensions
	glUseProgram(plot->overShaderProgram);
	glUniform2f(glGetUniformLocation(plot->overShaderProgram, "imgDims"),
	            imgWidth, imgHeight);
	resizeCallback(plot->window, 800, 600);
	
	// transparency
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); 
	
	// default bounds
	qdspSetBounds(plot, -1.0f, 1.0f, -1.0f, 1.0f);

	// default colors: yellow points, black background
	qdspSetPointColor(plot, 0xffff33);
	qdspSetBGColor(plot, 0x000000);

	plot->paused = 0;
	plot->overlay = 0;
	
	// framerate stuff
	clock_gettime(CLOCK_MONOTONIC, &plot->lastTime);

	return plot;
}

void qdspSetBounds(QDSPplot *plot, double xMin, double xMax, double yMin, double yMax) {
	glUseProgram(plot->shaderProgram);
	glUniform1f(glGetUniformLocation(plot->shaderProgram, "xMin"), xMin);
	glUniform1f(glGetUniformLocation(plot->shaderProgram, "xMax"), xMax);
	glUniform1f(glGetUniformLocation(plot->shaderProgram, "yMin"), yMin);
	glUniform1f(glGetUniformLocation(plot->shaderProgram, "yMax"), yMax);
}

void qdspSetPointColor(QDSPplot *plot, int rgb) {
	glUseProgram(plot->shaderProgram);
	glUniform1i(glGetUniformLocation(plot->shaderProgram, "defaultColor"), rgb);
}

void qdspSetBGColor(QDSPplot *plot, int rgb) {
	glClearColor((0xff & rgb >> 16) / 255.0,
	             (0xff & rgb >> 8) / 255.0,
	             (0xff & rgb) / 255.0,
	             1.0f);
}

int qdspUpdate(QDSPplot *plot, double *x, double *y, int *color, int numVerts) {
	// get ms since last full update (not last call)
	struct timespec lastTime = plot->lastTime;
	struct timespec newTime;
	clock_gettime(CLOCK_MONOTONIC, &newTime);
	double timeDiff = ((double)newTime.tv_sec*1.0e3 + newTime.tv_nsec*1.0e-6) - 
		((double)lastTime.tv_sec*1.0e3 + lastTime.tv_nsec*1.0e-6);

	while (plot->paused) {
		glfwWaitEvents();
	}
	
	// the rest of the function is a waste of time if no frame update is needed
	if (timeDiff >= 16)
		plot->lastTime = newTime;
	else
		return 2;

	// someone closed the window
	if (glfwWindowShouldClose(plot->window)) {
		glfwTerminate();
		return 0;
	}

	// copy all our vertex stuff
	glUseProgram(plot->shaderProgram);

	glBindBuffer(GL_ARRAY_BUFFER, plot->vertBufferObjX);
	glBufferData(GL_ARRAY_BUFFER, numVerts * sizeof(double), x, GL_STREAM_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, plot->vertBufferObjY);
	glBufferData(GL_ARRAY_BUFFER, numVerts * sizeof(double), y, GL_STREAM_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, plot->vertBufferObjCol);
	glBufferData(GL_ARRAY_BUFFER, numVerts * sizeof(int), color, GL_STREAM_DRAW);

	// should we use the default color?
	glUniform1i(glGetUniformLocation(plot->shaderProgram, "useCustom"), color != NULL);

	// drawing
	glClear(GL_COLOR_BUFFER_BIT);
	
	glBindVertexArray(plot->vertArrayObj);
	glDrawArrays(GL_POINTS, 0, numVerts);

	if (plot->overlay) {
		glUseProgram(plot->overShaderProgram);
		glBindVertexArray(plot->overVAO);
		glBindTexture(GL_TEXTURE_2D, plot->overTex);
		glDrawArrays(GL_TRIANGLES, 0, 6);
	}
	
	glfwSwapBuffers(plot->window);
	glfwPollEvents();

	return 1;
}

void qdspDelete(QDSPplot *plot) {
	free(plot);
}

static void resizeCallback(GLFWwindow *window, int width, int height) {
	QDSPplot *plot = glfwGetWindowUserPointer(window);
	glUseProgram(plot->overShaderProgram);
	glUniform2f(glGetUniformLocation(plot->overShaderProgram, "pixDims"),
	            width, height);
	
	glViewport(0, 0, width, height);
}

static void keyCallback(GLFWwindow *window, int key, int code, int action, int mods) {
	QDSPplot *plot = glfwGetWindowUserPointer(window);
	
	// ESC - close
	// q - close
	if ((key == GLFW_KEY_ESCAPE || key == GLFW_KEY_Q)
	    && action == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, 1);
		plot->paused = 0; // can't run cleanup code while paused
	}

	// p - pause
	if (key == GLFW_KEY_P && action == GLFW_PRESS) {
		plot->paused = !plot->paused;
	}

	// h - display help
	if (key == GLFW_KEY_H && action == GLFW_PRESS) {
		plot->overlay = !plot->overlay;
	}
}

static int makeShader(const char *filename, GLenum type) {
	// will fail with a crazy-long filename, but users can't call this anyway
	char fullpath[256] = "/usr/local/share/qdsp/shaders/";
	char localpath[256] = "./shaders/";
	strcat(fullpath, filename);
	FILE *file = fopen(fullpath, "r");

	// try current directory if it failed
	if (file == NULL) {
		strcat(localpath, filename);
		file = fopen(localpath, "r");
	}
	
	if (file == NULL) {
		fprintf(stderr, "Could not find file: %s\n", fullpath);
		fprintf(stderr, "Could not find file: ./%s\n", localpath);
		return 0;
	}

	// allocate memory
	int size;
	char *buf;
	fseek(file, 0L, SEEK_END);
	size = ftell(file); // file length
	rewind(file);
	buf = malloc(size * sizeof(char));

	fread(buf, 1, size, file);

	fclose(file);

	int shader = glCreateShader(type);
	glShaderSource(shader, 1, (const GLchar**)&buf, &size);
	glCompileShader(shader);
	
	// error checking
	int success;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (!success) {
		char log[1024];
		glGetShaderInfoLog(shader, 1024, NULL, log);
		fprintf(stderr, "Error compiling shader from file: %s\n", filename);
		fprintf(stderr, "%s\n", log);
		free(buf);
		return 0;
	}

	free(buf);
	return shader;
}