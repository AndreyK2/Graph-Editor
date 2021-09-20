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

EquationNode* GenerateEquationTree(string equation, double& x, double& y)
{
	EquationNode* node = nullptr;
	size_t pos;

	// equation pre-processing

	pos = equation.find_first_not_of(' ');
	if (pos == equation.npos)
	{
		throw EquationError("Invalid Argument: Empty/incomplete argument");
	}
	equation = equation.substr(pos, equation.find_last_not_of(' '));
	
	// TODO: makes this prettier?
	if (equation[0] == '(' && equation[equation.npos - 1] == ')')
		return GenerateEquationTree(equation.substr(1,equation.npos - 2),x,y);

	// Node Generation

	// For subtraction and division, we need to search in reverse in order to
	// build the tree nodes in the correct arithmetic order.
	// TODO: Need to skip brackets!
	pos = equation.find_last_of("+-");
	if (pos != equation.npos) // TODO: put in func?
	{
		node = new EquationNode;
		node->setLeft(GenerateEquationTree(equation.substr(0, pos),x,y));
		node->setRight(GenerateEquationTree(equation.substr(pos+1, equation.npos),x,y));
		if(equation[pos] == '+')
			node->SetEvaluationFunc([](EquationNode * curNode) {return curNode->left()->Evaluate() + curNode->right()->Evaluate(); });
		else
			node->SetEvaluationFunc([](EquationNode * curNode) {return curNode->left()->Evaluate() - curNode->right()->Evaluate(); });
		return node;
	}
	// TODO: Implement for rest of operations/math functions

	// End of operator checks
	// --
	// Begging of function checks (Trig, log, ...)



	// End of function checks
	// -- 
	// Beginning of raw value checks

	
	// TODO: Handle custom variables  
	if (!equation.compare("y") || !equation.compare("Y"))
	{
		node = new EquationNode;
		node->SetEvaluationFunc([&y](EquationNode * curNode) { return y; });
		return node;
	}
	else if (!equation.compare("x") || !equation.compare("X"))
	{
		node = new EquationNode;
		node->SetEvaluationFunc([&x](EquationNode * curNode) { return x; });
		return node;
	}
	// TODO: 

	size_t* num_end = nullptr;
	double value;
	try
	{
		value = std::stod(equation, num_end);
	}
	catch (std::invalid_argument err)
	{
		throw EquationError("Invalid Equation: Found invalid parameter, parameter must be a single number/existing variable name");
	}
	catch (std::out_of_range err)
	{
		throw EquationError("Invalid Equation: Parameter value too large");
	}

	// This covers the case of parameters with no operation between them, for example: "15 23.7"
	if (*num_end != equation.npos)
	{
		throw EquationError("Invalid Equation: Found invalid parameter, parameter must be a single number/existing variable name");
	}
	
	node = new EquationNode;
	node->SetEvaluationFunc([value](EquationNode * curNode) { return value; });
	return node;
}

