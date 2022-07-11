#pragma once

#include <functional>
#include <string>
#include <stdexcept>
#include <vector>
#include <utility>

#include <memory>

using std::string;
using std::vector;
using std::unique_ptr;
using std::pair;

struct EquationNode
{
	unique_ptr<EquationNode> _left;
	unique_ptr<EquationNode> _right;
	std::function<double(EquationNode* curNode)> _evalFunc;

	double Evaluate() { return _evalFunc(this); };
};

unique_ptr<EquationNode> GenerateEquationTree(string equation, vector<pair<char, double>>& vars, size_t substrIndex = 0); // TODO: static in EquationNode?

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