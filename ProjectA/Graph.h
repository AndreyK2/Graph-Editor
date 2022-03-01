#pragma once

#include <functional>
#include <string>
#include <stdexcept>
#include <vector>
#include <utility>
#include <glew.h>

#define NO_ERR 0
#define NOT_FOUND 1
#define HIDE 999999

using std::string; using std::vector; using std::pair;

//a point in 3d space (used on 2d & 3d graphs)
struct position {
	GLfloat x;
	GLfloat y;
	GLfloat z;
};

/*
Graph equations are parsed as binary trees composed of EquationNodes 
*/
class EquationNode
{
public:
	EquationNode(EquationNode* left = nullptr, EquationNode* right = nullptr);
	~EquationNode();
	double Evaluate() { return _evalFunc(this); };
	void SetEvaluationFunc(std::function<double(EquationNode* curNode)> func) { _evalFunc = func; };
	void setLeft(EquationNode* left) { _left = left; };
	void setRight(EquationNode* right) { _right = right; };
	EquationNode* left() { return _left; };
	EquationNode* right() { return _right; };

private:
	EquationNode* _left;
	EquationNode* _right;

	std::function<double(EquationNode* curNode)> _evalFunc;
};


EquationNode* GenerateEquationTree(string equation, vector<pair<char,double>>& vars, size_t substrIndex = 0); // TODO: static in EquationNode class?
size_t unmatchedBracket(string equation);
void LRstripWhites(string& str);
size_t findOperator(string str, string operators, bool reverse = false);
char upper(char c);
pair<int, size_t> getFunction(string equation);


// TODO: move func and func names to a structure?
enum mathFunctions
{
	COSINE = 0, SINE, TANGENT,
	ACOSINE, ASINE, ATANGENT,
	LOG,
	NONE
};

static vector<vector<string>> funcNames{
	{"COS", "COSINE"},
	{"SIN", "SINE"},
	{"TAN", "TANGENT"},
	{"ACOS", "ARCCOS", "ACOSINE", "ARCCOSINE"},
	{"ASIN", "ARCSIN", "ASINE", "ARCSINE"},
	{"ATAN", "ARCTAN", "ATANGENT", "ARCTANGENT"},
	{"LOG"}
};

class EquationError : public std::exception
{
public:
	explicit EquationError(const char* message, size_t index = 0) : _msg(message), _index(index) {};
	explicit EquationError(const string& message, size_t index = 0) : _msg(message), _index(index) {};
	virtual ~EquationError() noexcept {};

	virtual const char* what() const noexcept { return _msg.c_str(); };
	size_t index() const { return _index; };

protected:
	string _msg;
	size_t _index;
};

class Graph
{
public:
	Graph(size_t id, GLuint program, double& x, double& z, EquationNode* graphEquation);

	void Generate(double sampleSize, size_t samples, size_t resolution);
	void Draw(GLuint sampleCount, GLuint resolution, GLuint* indexBuffer);

	void SetEquation(EquationNode* graphEquation);

	bool show;
	const size_t id;

private:
	EquationNode* _graphEquation;
	GLuint _bufferHorizontalOutlineX; 
	GLuint _bufferHorizontalOutlineZ;
	GLuint _bufferGraphSurface;
	GLuint _program;
	double& _x; double& _z;
};

class GraphManager
{
public:
	GraphManager(GLuint program, vector<pair<string, void*>>* windowVars = nullptr);
	~GraphManager();

	size_t NewGraph(string equation = "0");
	size_t RemoveGraph(size_t graphId);
	void generateIndecies();
	void Draw();
private:
	void _DeleteEquation(size_t graphId);

	vector<Graph> _graphs;
	vector<pair<size_t, EquationNode*>> _graphEquations;
	vector<pair<char, double>> _vars; // x,z,...
	size_t _sampleCount;
	size_t _resolution;
	double* _graphZoom; // The graph zoom is ideally global for all graphs
	double _curGraphZoom; // for forcing graph updates, might make an array of forced varaibles if needed
	GLuint* _indexBuff;
	size_t _curId;
	GLuint _program;
};