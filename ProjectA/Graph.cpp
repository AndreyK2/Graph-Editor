#include "Graph.h"
#include <stack>

EquationNode::EquationNode(EquationNode* left = nullptr, EquationNode* right = nullptr)
	: _left(left), _right(right)
{
	_evalFunc = [](EquationNode * curNode) { return 0.0; };
}

EquationNode::~EquationNode()
{
	if (_left != nullptr)
	{
		delete _left;
	}
	if (_right != nullptr)
	{
		delete _right;
	}
}

EquationNode* GenerateEquationTree(string equation, std::vector<std::pair<char, double&>> vars, size_t substrIndex)
{
	EquationNode* node = nullptr;
	size_t pos;

	// equation pre-processing

	if (substrIndex == 0) // perform once 
	{
		pos = unmatchedBracket(equation);
		if (pos != equation.npos)
			throw EquationError("Invalid Equation: Found unmatched bracket", substrIndex + pos);
	}
	
	pos = equation.find_first_not_of(" ");
	if (pos == equation.npos)
		throw EquationError("Invalid Equation: Missing parameter/empty input", substrIndex);
	substrIndex += pos;
	LRstripWhites(equation);

	// Unpack
	if (equation[0] == '(' && equation[equation.length() - 1] == ')')
	{
		equation.erase(equation.begin());
		equation.erase(equation.end()-1);
		return GenerateEquationTree(equation, vars, substrIndex + 1);
	}

	// End of pre-processing
	// -- 
	// Beginning of operator checks

	// Subtraction, division and power care about order! thus we sometimes find in reverse

	pos = findOperator(equation, "+-", true);
	if (pos != equation.npos) // TODO: put in func?
	{
		node = new EquationNode;
		node->setLeft(GenerateEquationTree(equation.substr(0, pos), vars));
		node->setRight(GenerateEquationTree(equation.substr(pos+1, equation.npos), vars, substrIndex + pos + 1));
		if(equation[pos] == '+')
			node->SetEvaluationFunc([](EquationNode * curNode) {return curNode->left()->Evaluate() + curNode->right()->Evaluate(); });
		else
			node->SetEvaluationFunc([](EquationNode * curNode) {return curNode->left()->Evaluate() - curNode->right()->Evaluate(); });
		return node;
	}

	pos = findOperator(equation, "*/", true);
	if (pos != equation.npos)
	{
		node = new EquationNode;
		node->setLeft(GenerateEquationTree(equation.substr(0, pos), vars));
		node->setRight(GenerateEquationTree(equation.substr(pos + 1, equation.npos), vars, substrIndex + pos + 1));
		if(equation[pos] == '*')
			node->SetEvaluationFunc([](EquationNode * curNode) {return curNode->left()->Evaluate() * curNode->right()->Evaluate(); });
		else
			node->SetEvaluationFunc([](EquationNode * curNode) {return curNode->left()->Evaluate() / curNode->right()->Evaluate(); });
		return node;
	}

	pos = findOperator(equation, "^", false);
	if (pos != equation.npos)
	{
		node = new EquationNode;
		node->setLeft(GenerateEquationTree(equation.substr(0, pos), vars));
		node->setRight(GenerateEquationTree(equation.substr(pos + 1, equation.npos), vars, substrIndex + pos + 1));
		node->SetEvaluationFunc([](EquationNode * curNode) {return pow(curNode->left()->Evaluate(),curNode->right()->Evaluate()); });
		return node;
	}
	// TODO: Implement for rest of operations/math functions

	// End of operator checks
	// --
	// Beginning of function checks (Trig, log, ...)



	// End of function checks
	// -- 
	// Beginning of raw value checks

	
	for (std::pair<char, double&> var : vars)
	{
		if (equation.length() == 1 && upper(equation[0]) == upper(var.first)) // TODO: self implement toupper to not include cctype?
		{
			node = new EquationNode;
			double& var_ref = var.second;
			node->SetEvaluationFunc([&var_ref](EquationNode * curNode) { return var_ref; });
			return node;
		}
	}

	size_t num_end = 0;
	double value;
	try
	{
		value = std::stod(equation, &num_end);
	}
	catch (std::invalid_argument err)
	{
		throw EquationError("Invalid Equation: Found invalid parameter, parameter must be a single number/existing variable name", substrIndex);
	}
	catch (std::out_of_range err)
	{
		throw EquationError("Invalid Equation: Parameter value too large", substrIndex);
	}

	// This covers the case of parameters with no operation between them, for example: "15 23.7" (stod succeeds here)
	if (num_end < equation.length())
	{
		throw EquationError("Invalid Equation: Found invalid parameter, parameter must be a single number/existing variable name", substrIndex);
	}
	
	node = new EquationNode;
	node->SetEvaluationFunc([value](EquationNode * curNode) { return value; });
	return node;
}

/*
Validates bracket stacking, purely for exception info
Returns position of unmatched bracket, or npos
*/
size_t unmatchedBracket(string str)
{
	// TODO: support all bracket types?
	std::stack<size_t> brackets;
	for (size_t pos = 0; pos < str.length(); pos++)
	{
		if (str[pos] == '(')
		{
			brackets.push(pos);
		}
		else if (str[pos] == ')')
		{
			if (brackets.empty()) return pos;
			else brackets.pop();
		}
	}
	if (brackets.empty()) return str.npos;
	else return brackets.top();
}

void LRstripWhites(string& str)
{
	size_t pos = str.find_first_not_of(' ');
	str.erase(str.begin(), str.begin()+pos);
	pos = str.find_last_not_of(' ');
	str.erase(str.begin() + (pos+1), str.end());
}

/*
Return the position of the first operator character from "operators" in string, skipping bracketed text
*/
size_t findOperator(string str, string operators, bool reverse)
{
	// TODO: use stack object to support all bracket types?
	int bracketStack = 0;
	size_t pos = 0;
	char curr = 0;
	bool found = false;

	for (unsigned int i = 0; i < str.length(); i++)
	{
		curr = str[i];
		if (curr == '(')
		{
			bracketStack += 1;
		}
		else if (curr == ')')
		{
			bracketStack -= 1;
		}
		else if (bracketStack == 0)
		{
			for (char op : operators)
			{
				if (curr == op)
				{
					pos = i;
					found = true;
					if (!reverse) return pos;
				}
			}
		}
	}

	if (found) return pos;
	else return str.npos;
}

char upper(char c)
{
	return c - (97 - 65);
}