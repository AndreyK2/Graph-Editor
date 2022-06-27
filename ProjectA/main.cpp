// opengl wrangler
#include <glew.h>
// window handling backend
#include <glfw3.h>
// imgui
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

// self implements
#include "Console.h"
#include "Graph.h"
// shaders
#include "shaders.h"

#include <vector>
#include <string>
#include <iostream>

using std::vector;
using std::string;

GLFWwindow* window;

//init gl vars: buffers
GLuint vertex_array_object;
GLuint buffer_3d_1;
GLuint buffer_3d_2;
GLuint buffer_2d;
GLuint buffer_axis;
GLuint buffer_axis_marks;
//shader program
GLuint program;
//uniform locations
GLuint uniform_offset;
GLuint uniform_angle;
GLuint uniform_color;
GLuint uniform_rotation;
GLuint uniform_perspective;
GLuint uniform_isGradient;

//size of axis & marks
int graph_size = 100;
//scale y axis
int graph_y_scale = 50;

//initial position + angle variables for shader
float xpos = 0;
float ypos = 0;
float zpos = -40;
float xang = 0;
float yang = 0;
float fov = 90;



//Callbacks

static void glfw_error_callback(int error, const char* description)
{
	fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

void GLAPIENTRY 
MessageCallback(GLenum source,
	GLenum type,
	GLuint id,
	GLenum severity,
	GLsizei length,
	const GLchar* message,
	const void* userParam)
{

	if(type != 0x8251) fprintf(stderr, "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
		(type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""),
		type, severity, message);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);

void perspective(float fov);

void* getWindowVar(GLFWwindow* window, string varName);

void initBackends()
{
	glfwInit();
	glfwSetErrorCallback(glfw_error_callback);

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	window = glfwCreateWindow(1920, 1280, "Graph", NULL, NULL);

	glfwMakeContextCurrent(window);
	//glfwSetKeyCallback(window, nullptr);
	glfwSetScrollCallback(window, scroll_callback);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSwapInterval(1); // Enable Vsync

	GLenum err = glewInit();

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_DEBUG_OUTPUT);

	// Enable alpha channel for the graphs
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glDebugMessageCallback(MessageCallback, 0);

}

void initImGUI()
{
	const char* glsl_version = "#version 330";
	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	// Setup Platform/Renderer backends
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init(glsl_version);
}

void initGraphEnvironment() {
	//compile vertex shader
	GLuint v_shader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(v_shader, 1, &vertex_shader, 0); //glShaderSource( v_shader, 1, (const GLchar**)&vertex_shader, 0 );
	glCompileShader(v_shader);
	//compile frag shader
	GLuint f_shader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(f_shader, 1, &fragment_shader, 0);
	glCompileShader(f_shader);
	//create the shader program
	program = glCreateProgram();
	glAttachShader(program, v_shader);
	glAttachShader(program, f_shader);
	glLinkProgram(program);

	glDeleteShader(v_shader);
	glDeleteShader(f_shader);

	//vertex array object state
	glGenVertexArrays(1, &vertex_array_object);
	glBindVertexArray(vertex_array_object);

	//get uniform locations in shaders
	uniform_offset = glGetUniformLocation(program, "offset");
	uniform_angle = glGetUniformLocation(program, "angle");
	uniform_color = glGetUniformLocation(program, "color");
	uniform_perspective = glGetUniformLocation(program, "perspective");
	uniform_isGradient = glGetUniformLocation(program, "isGradient");

	glUniform1i(uniform_isGradient, false);

	//setup perspective
	perspective(fov);

	const size_t count_sides = 6;
	const size_t count_marks = 100;
	position axis[count_sides] = {0};
	position axis_marks[count_sides * count_marks] = {0};

	//axis lines, per side
	//x
	axis[0].x = -graph_size;
	axis[1].x = graph_size;
	//y
	axis[2].y = -graph_size;
	axis[3].y = graph_size;
	//z
	axis[4].z = graph_size;
	axis[5].z = -graph_size;

	//buffer for axis
	glGenBuffers(1, &buffer_axis);
	glBindBuffer(GL_ARRAY_BUFFER, buffer_axis);
	//assign axis data
	glBufferData(GL_ARRAY_BUFFER, sizeof(axis), axis, GL_STATIC_DRAW);

	//axis marks
	size_t index = 0;
	//x
	for (int i = -graph_size; i < graph_size; ++i) {
		if (i % 2 != 0) continue; //only every 2 points
		axis_marks[index].x = i;
		index++;
		axis_marks[index].x = i;
		axis_marks[index].y = 1;
		index++;
	}
	//y
	
	for (int i = -graph_size; i < graph_size; ++i) {
		if (i % 2 != 0) continue;
		axis_marks[index].y = i;
		index++;
		axis_marks[index].x = 1;
		axis_marks[index].y = i;
		index++;
	}
	//z
	for (int i = -graph_size; i < graph_size; ++i) {
		if (i % 2 != 0) continue;
		axis_marks[index].z = i;
		index++;
		axis_marks[index].y = 1;
		axis_marks[index].z = i;
		index++;
	}

	//buffer for axis marks
	glGenBuffers(1, &buffer_axis_marks);
	glBindBuffer(GL_ARRAY_BUFFER, buffer_axis_marks);
	//assign axis_marks data
	glBufferData(GL_ARRAY_BUFFER, sizeof(axis_marks), axis_marks, GL_STATIC_DRAW);
}

void perspective(float fov)
{
	//setup perspective
	const float fzNear = 1;
	const float fzFar = 100;

	const float pi = 4 * atan(1);
	const float fFrustumScale = 1 / tan((fov / 2) * (pi / 180));

	float perspectiveMatrix[16] = {
		fFrustumScale,	0,	0,	0,
		0,	fFrustumScale,  0,	0,
		0,	0,	(fzFar + fzNear) / (fzNear - fzFar),  -1,
		0,	0,	(2 * fzFar * fzNear) / (fzNear - fzFar),  0
	};

	//bind data to shader
	glUseProgram(program);
	glUniformMatrix4fv(uniform_perspective, 1, GL_TRUE, perspectiveMatrix); //perspective matrix
	glUniform3f(uniform_offset, xpos, ypos, zpos); //position offset
	glUniform2f(uniform_angle, xang, yang); //rotation angles
	glUniform4f(uniform_color, 1, 1, 1, 1); //color
	glUseProgram(0);
}


//draw function
void draw(GraphManager& gm) {

	//use our shader program
	glUseProgram(program);

	//setup offset + angle for everything
	glUniform3f(uniform_offset, xpos, ypos, zpos);
	glUniform2f(uniform_angle, xang, yang);

	//color for axis
	glUniform4f(uniform_color, 0.3f, 0.4f, 0.7f, 1.0f);
	//draw each axis
	glBindBuffer(GL_ARRAY_BUFFER, buffer_axis);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glDrawArrays(GL_LINES, 0, 9);
	glDisableVertexAttribArray(0);

	//axis marks
	glBindBuffer(GL_ARRAY_BUFFER, buffer_axis_marks);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glDrawArrays(GL_LINES, 0, 600);
	glDisableVertexAttribArray(0);

	// Draw each graph
	gm.Draw();

	//stop using latest buffer + shader
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glUseProgram(0);
}


//keyboard handling (used for continuous movement of angles/position)
void keyboard() {
	//keyboard management
	if (glfwGetKey(window, 'A') == GLFW_PRESS) {
		xpos = xpos - 0.1;
	}
	if (glfwGetKey(window, 'D') == GLFW_PRESS) {
		xpos = xpos + 0.1;
	}
	if (glfwGetKey(window, 'W') == GLFW_PRESS) {
		ypos = ypos + 0.1;
	}
	if (glfwGetKey(window, 'S') == GLFW_PRESS) {
		ypos = ypos - 0.1;
	}
	if (glfwGetKey(window, 'E') == GLFW_PRESS) {
		zpos = zpos - 0.1;
	}
	if (glfwGetKey(window, 'Q') == GLFW_PRESS) {
		zpos = zpos + 0.1;
	}
	if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
		xang = xang - 0.025;
	}
	if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
		xang = xang + 0.025;
	}
	if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
		yang = yang - 0.025;
	}
	if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
		yang = yang + 0.025;
	}
	// fov, is it needed?
	/*
	if (glfwGetKey(window, 'P') == GLFW_PRESS && fov < 180) {
		fov += 0.2;
		perspective(fov);
	}
	if (glfwGetKey(window, 'O') == GLFW_PRESS && fov > 30) {
		fov -= 0.2;
		perspective(fov);
	}*/
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	double* zoom = (double*)getWindowVar(window, "graphZoom");
	*zoom += 0.1 * -yoffset;
}

//glfw resize
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
	glViewport(0, 0, width, height);
}

void* getWindowVar(GLFWwindow* window, string varName)
{
	auto vars = (vector<std::pair<string, void*>>*)glfwGetWindowUserPointer(window);
	for (std::pair<string, void*> var : *vars)
	{
		if (var.first == varName) return var.second;
	}
	return nullptr;
}

int main(int argc, char const* argv[])
{

	//init glfw, opengl, imgui, shaders, axis buffer
	initBackends();
	initImGUI();
	initGraphEnvironment();
	
	bool show_console = true;
	bool show_demo = false;

	vector<std::pair<string, void*>> windowVars;
	glfwSetWindowUserPointer(window, &windowVars);
	bool graphFocused;
	windowVars.push_back({ "graphFocused", (void*)&graphFocused });

	double graphZoom = 0;
	windowVars.push_back({ "graphZoom", (void*)&graphZoom });

	GraphManager graphManager(program, &windowVars);
	Console console(&graphManager);


	//main loop
	while (!glfwGetKey(window, GLFW_KEY_ESCAPE) && !glfwWindowShouldClose(window)) {
		//clear the screen
		glClear(GL_COLOR_BUFFER_BIT);
		glClearColor(0.1, 0.1, 0.1, 1.0);
		glClear(GL_DEPTH_BUFFER_BIT);

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
		//draw
		draw(graphManager);

		//ImGui::ShowDemoWindow(&show_demo);

		if (show_console)
			console.Draw(&show_console);
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(window);
		glfwPollEvents();

		graphFocused = !(console.IsFocused() || graphManager._focused);
		//keyboard
		if(graphFocused)
			keyboard();
	} //end of loop

	// Cleanup
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
	//end
	glfwDestroyWindow(window);
	glfwTerminate();
}
