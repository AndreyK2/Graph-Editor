#pragma once

#include <functional>
#include <string>
#include <vector>
#include <utility>

#include <memory>

#include <glew.h>
#include <imgui.h>

#include "parsing.h"

using std::string; 
using std::vector; 
using std::pair;
using std::unique_ptr;

struct position {
	GLfloat x, y, z;
};


struct GraphProperties
{
	ImVec4 _sufColor;
	ImVec4 _outlineColorX;
	ImVec4 _outlineColorZ;
	double _gradingIntensity;
	string _equation;
};

class Graph
{
public:
	Graph(size_t id, GLuint program, double& x, double& z, unique_ptr<EquationNode> graphEquation);
	//Graph& operator=(const Graph& other);

	void Generate(double sampleSize, size_t samples, size_t resolution);
	void Draw(GLuint sampleCount, GLuint resolution, GLuint* indexBuffer, GraphProperties properties);
	void SetEquation(unique_ptr<EquationNode> graphEquation);

	bool show;
	size_t id;

	constexpr static size_t graph_sides = 2, duplicate_rowindecies = 2,
		non_repeating_index_rows = 2, index_repeats = 2;

private:
	void bindVertexBuffer(GLuint& GLbuffer, position* vertexBuffer, size_t size);

	unique_ptr<EquationNode> _graphEquation;
	GLuint _bufferHorizontalOutlineXupper;
	GLuint _bufferHorizontalOutlineXlower;
	GLuint _bufferHorizontalOutlineZupper;
	GLuint _bufferHorizontalOutlineZlower;
	GLuint _bufferGraphSurface;
	GLuint _program;
	// TODO: Shared pointers?
	double& _x; double& _z;
};

class GraphEditor;

class GraphManager
{
public:
	GraphManager(GLuint program, vector<pair<string, void*>>* windowVars = nullptr);

	size_t NewGraph(string equation = "0");
	size_t RemoveGraph(size_t graphId);
	void UpdateEquation(size_t graphId, string equation);
	void generateIndecies();
	void Draw();

	bool _focused;

private:
	vector<Graph> _graphs;
	vector<GraphEditor> _graphEditors;
	vector<pair<char, double>> _vars; // x,z,...
	size_t _sampleCount;
	size_t _resolution;
	double* _graphZoom; // The graph zoom is ideally global for all graphs
	double _curGraphZoom; // for forcing graph updates, might make an array of forced varaibles if needed
	GLuint* _indexBuff;
	size_t _curId;
	GLuint _program;
};


class GraphEditor
{
public:
	GraphEditor(size_t graphId, GraphManager* graphManager, string equation = "");
	void Draw();
	int TextEditCallback(ImGuiInputTextCallbackData* data);
	

	size_t id;
	GraphProperties _prop;

private:
	bool _open;
	string _equation;
	GraphManager* _graphManager;
};

constexpr char upper(char c);

