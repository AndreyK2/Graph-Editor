#include "Graph.h"

EquationNode::EquationNode(EquationNode* left = nullptr, EquationNode* right = nullptr)
	: _left(left), _right(right)
{
	_evalFunc = [](EquationNode * curNode) { return 0.0; };
}

EquationNode::~EquationNode()
{
	if (_left != nullptr)
	{
		_left->~EquationNode();
		delete _left;
	}
	if (_right != nullptr)
	{
		_right->~EquationNode();
		delete _right;
	}
}

EquationNode* GenerateEquationTree(std::string equation)
{
	EquationNode* node = new EquationNode;

	// For subtraction and division, we need to search in reverse in order to
	// build the tree nodes in the correct arithmetic order.
	size_t pos = equation.find_last_of("+-");
	if (pos != equation.npos) // TODO: put in func?
	{
		node->setLeft(GenerateEquationTree(equation.substr(0, pos)));
		node->setRight(GenerateEquationTree(equation.substr(pos+1, equation.npos))); // TODO: check correct skip of pos
		if(equation.at(pos) == '+')
			node->SetEvaluationFunc([](EquationNode * curNode) {return curNode->left()->Evaluate() + curNode->right()->Evaluate(); });
		else
			node->SetEvaluationFunc([](EquationNode * curNode) {return curNode->left()->Evaluate() - curNode->right()->Evaluate(); });
	}
	// TODO: Implement for rest of operations/math functions

	return node;
}
