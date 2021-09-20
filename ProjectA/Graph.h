#pragma once

#include <functional>
#include <string>
#include <stdexcept>

using std::string;

class Graph
{
public:
	Graph();

	void Draw();

private:
	EquationNode* _graphEquation;
};

/*
Graph equations are parsed as binary trees composed of EquationNodes 
*/
class EquationNode
{
public:
	EquationNode(EquationNode* left, EquationNode* right);
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

EquationNode* GenerateEquationTree(string equation, double& x, double& y); // TODO: static in EquationNode class?

class EquationError : public std::exception
{
public:
	// The constructor forces you to release memory when you throw
	explicit EquationError(const char* message) : msg_(message) {};
	explicit EquationError(const std::string& message) : msg_(message) {};
	virtual ~EquationError() noexcept {};

	virtual const char* what() const noexcept { return msg_.c_str(); };

protected:
	string msg_;
};