#pragma once

#include <functional>
#include <string>
#include <stdexcept>
#include <vector>
#include <utility>

using std::string; using std::vector; using std::pair;

class Graph
{
public:
	Graph();

	void Draw();

private:
	//EquationNode* _graphEquation;
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


EquationNode* GenerateEquationTree(string equation, vector<pair<char,double&>> vars, size_t substrIndex = 0); // TODO: static in EquationNode class?
size_t unmatchedBracket(string equation);
void LRstripWhites(string& str);
size_t findOperator(string str, string operators, bool reverse = false);
char upper(char c);
pair<int, size_t> getFunction(string equation);

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

protected:
	string _msg;
	size_t _index;
};