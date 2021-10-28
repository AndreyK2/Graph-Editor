#pragma once

#include <functional>
#include <string>
#include <stdexcept>
#include <vector>
#include <utility>
#include <glew.h>

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
	// TODO: Store equation string index for detailed exception handling

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
	Graph(size_t id, GLuint program, double& x, double& z, EquationNode* graphEquation = nullptr);

	void Generate();
	void Draw();

	void SetEquation(EquationNode* graphEquation);

private:
	EquationNode* _graphEquation;
	size_t _id;
	size_t _samples; 
	double _sampleSize;
	bool _draw2D; bool _draw3D;
	GLuint _buffer3D1; 
	GLuint _buffer3D2;
	GLuint _buffer2D;
	GLuint _program;
	double& _x; double& _z;
};


class GraphManager
{
public:
	GraphManager(GLuint program);

	size_t NewGraph(string equation = "0");
	void Draw();
private:
	vector<Graph> _graphs;
	vector<pair<char, double>> _vars;
	size_t _curId;
	GLuint _program;
};