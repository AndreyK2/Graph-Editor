#pragma once

#include <functional>
#include <string>
using std::string;

class Graph
{
public:
	Graph();

	void Draw();

private:

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

	std::function<double(EquationNode* curNode)> _evalFunc;
};

EquationNode* GenerateEquationTree(string equation);