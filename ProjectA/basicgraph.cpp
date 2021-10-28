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
#include <iostream>

using std::vector;

#define WINDOWED NULL

GLFWwindow* window;

// setup -----------------------------------------------------------



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


//number of graph samples to make
int _graphSamples = 30;
//size of axis & marks
int graph_size = 100;
//scale y axis (because otherwise its between 0-1 while x is -30 to +30)
int graph_y_scale = 50;

//initial position + angle variables for shader
float xpos = 0;
float ypos = 0;
float zpos = -40;
float xang = 0;
float yang = 0;
double fov = 90;

//do we draw 2d, 3d?
int draw_2d = 1;
int draw_3d = 1;



//axis
struct position axis[6];
struct position axis_marks[600];

//Callbacks

static void glfw_error_callback(int error, const char* description)
{
	fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

void GLAPIENTRY // TODO: this is only in gl 4? also what is 8251
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

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);

void perspective(double fov);

void initBackends()
{
	glfwInit();
	glfwSetErrorCallback(glfw_error_callback);

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	window = glfwCreateWindow(1920, 1280, "Graph", WINDOWED, NULL);

	glfwMakeContextCurrent(window);
	glfwSetKeyCallback(window, key_callback);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSwapInterval(1); // Enable Vsync

	GLenum err = glewInit();

	glEnable(GL_DEBUG_OUTPUT);
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

	//vertex array object state
	glGenVertexArrays(1, &vertex_array_object);
	glBindVertexArray(vertex_array_object);

	//get uniform locations in shaders
	uniform_offset = glGetUniformLocation(program, "offset");
	uniform_angle = glGetUniformLocation(program, "angle");
	uniform_color = glGetUniformLocation(program, "color");
	uniform_perspective = glGetUniformLocation(program, "perspective");

	//setup perspective
	perspective(fov);

	//axis lines, per side
	//x
	axis[0].x = -graph_size;
	axis[0].y = 0;
	axis[0].z = 0;
	axis[1].x = graph_size;
	axis[1].y = 0;
	axis[1].z = 0;
	//y
	axis[2].x = 0;
	axis[2].y = -graph_size;
	axis[2].z = 0;
	axis[3].x = 0;
	axis[3].y = graph_size;
	axis[3].z = 0;
	//z
	axis[4].x = 0;
	axis[4].y = 0;
	axis[4].z = graph_size;
	axis[5].x = 0;
	axis[5].y = 0;
	axis[5].z = -graph_size;

	//buffer for axis
	glGenBuffers(1, &buffer_axis);
	glBindBuffer(GL_ARRAY_BUFFER, buffer_axis);
	//assign axis data
	glBufferData(GL_ARRAY_BUFFER, sizeof(axis), axis, GL_STATIC_DRAW);

	//axis marks
	int i;
	int index = 0;
	//x
	for (i = -graph_size; i < graph_size; ++i) {
		if (i % 2 != 0) continue; //only every 2 points
		axis_marks[index].x = i;
		axis_marks[index].y = 0;
		axis_marks[index].z = 0;
		index++;
		axis_marks[index].x = i;
		axis_marks[index].y = 1;
		axis_marks[index].z = 0;
		index++;
	}
	//y
	for (i = -graph_size; i < graph_size; ++i) {
		if (i % 2 != 0) continue;
		axis_marks[index].x = 0;
		axis_marks[index].y = i;
		axis_marks[index].z = 0;
		index++;
		axis_marks[index].x = 1;
		axis_marks[index].y = i;
		axis_marks[index].z = 0;
		index++;
	}
	//z
	for (i = -graph_size; i < graph_size; ++i) {
		if (i % 2 != 0) continue;
		axis_marks[index].x = 0;
		axis_marks[index].y = 0;
		axis_marks[index].z = i;
		index++;
		axis_marks[index].x = 0;
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

void perspective(double fov)
{
	//setup perspective
	float fzNear = 1.0f;
	float fzFar = 100.0f;
	//THE MATRIX
	float theMatrix[16];
	memset(theMatrix, 0, sizeof(float) * 16);
	//build perspective matrix
	float fFrustumScale = 1 / tan((fov / 2) * (3.141 / 180));
	theMatrix[0] = fFrustumScale;
	theMatrix[5] = fFrustumScale;
	theMatrix[10] = (fzFar + fzNear) / (fzNear - fzFar);
	theMatrix[14] = (2 * fzFar * fzNear) / (fzNear - fzFar);
	theMatrix[11] = -1.0f;

	//bind data to shader
	glUseProgram(program);
	glUniformMatrix4fv(uniform_perspective, 1, GL_TRUE, theMatrix); //perspective matrix
	glUniform3f(uniform_offset, xpos, ypos, zpos); //position offset
	glUniform2f(uniform_angle, xang, yang); //rotation angles
	glUniform4f(uniform_color, 1, 1, 1, 1); //color
	glUseProgram(0);
}


//draw function
void draw(GraphManager gm) {
	int i;

	//use our shader program
	glUseProgram(program);

	//setup offset + angle for everything
	glUniform3f(uniform_offset, xpos, ypos, zpos);
	glUniform2f(uniform_angle, xang, yang);

	//color for axis (green)
	glUniform4f(uniform_color, 0, 1, 0, 1);
	//draw axis
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
	if (glfwGetKey(window, 'P') == GLFW_PRESS && fov < 180) {
		fov += 0.2;
		perspective(fov);
	}
	if (glfwGetKey(window, 'O') == GLFW_PRESS && fov > 30) {
		fov -= 0.2;
		perspective(fov);
	}
}

// TODO: disable when focused on sub window (console)
//glfw keyboard callback (used for one-press actions)
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	if (action == GLFW_PRESS) {
		//decrease samples
		/* TODO: implement per graph via gui
		if (key == '-') {
			_graphSamples -= 2;
			if (_graphSamples == 0) _graphSamples = 1; //limit to minimum 1
			generate();
			//increase samples (keep going to crash)
		} 
		if (key == '=') {
			_graphSamples += 2;
			generate();
			//reset everything
		}*/
		if (key == 'R') {
			xang = 0;
			yang = 0;
			zpos = -40;
			xpos = 0;
			ypos = 0;
			draw_2d = 1;
			draw_3d = 1;
			//2d only
		}
		else if (key == '2') {
			//set perfect position
			xang = 0;
			yang = 0;
			zpos = -20;
			xpos = 0;
			ypos = -20;
			draw_2d = 1;
			draw_3d = 0;
		}
		//3d only
        else if( key == '3' ) {
            draw_3d = 1;
            draw_2d = 0;
        //both 2d + 3d
        } else if( key == '1' ) {
            draw_3d = 1;
            draw_2d = 1;
        //neither, axis only
        } else if( key == '0' ) {
            draw_3d = 0;
            draw_2d = 0;
		}
	}
}

//glfw resize
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
	glViewport(0, 0, width, height);
}


//main
int main(int argc, char const* argv[])
{

	//init glfw, opengl, imgui, shaders, axis buffer
	initBackends();
	initImGUI();
	initGraphEnvironment();

	//generate + buffer graph data
	//generate();

	
	GraphManager graphManager(program);
	Console console(&graphManager);
	bool show_console = true;

	bool show_demo_window = false; // DELETE

	

	//main loop
	//while window open + not escape key
	while (!glfwGetKey(window, GLFW_KEY_ESCAPE) && !glfwWindowShouldClose(window)) {
		//clear
		glClear(GL_COLOR_BUFFER_BIT);
		glClearColor(0.1, 0.1, 0.1, 1.0);
		//draw
		draw(graphManager);

		// Start the Dear ImGui frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		if (show_demo_window)
			ImGui::ShowDemoWindow(&show_demo_window);
		if (show_console)
			console.Draw(&show_console);
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(window);
		glfwPollEvents();
		//keyboard
		if(!console.IsFocused())
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










/*
void eqdebug()
{// temporary debugging
	EquationNode* head = nullptr;
	std::vector<std::pair<char, double&>> vars;
	double x = 0, y = 0;
	std::pair<char, double&> y_var('y', y), x_var('x', x);
	vars.push_back(y_var); vars.push_back(x_var);
	bool failed = false;
	try
	{
		head = GenerateEquationTree(" cos(3.14) ", vars);
	}
	catch (EquationError err)
	{
		std::cout << err.what() << '\n';
		failed = true;
	}
	if (!failed) std::cout << head->Evaluate() << '\n';
	delete head;
}*/