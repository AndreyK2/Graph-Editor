#include "parsing.h"
#include <stack>

constexpr char upper(char c)
{
	if (c >= 'a' && c <= 'z') c -= ('a' - 'A');
	return c;
}

bool isBracketPacked(string str)
{
	size_t bracketStack = 0;
	for (char c : str)
	{
		if (c == '(') bracketStack++;
		else if (c == ')') bracketStack--;
		else if (bracketStack == 0) return false;
	}
	return true;
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
	str.erase(str.begin(), str.begin() + pos);
	pos = str.find_last_not_of(' ');
	str.erase(str.begin() + (pos + 1), str.end());
}

/*
Return the position of the first operator character from "operators" in string, skipping bracketed text
*/
size_t findOperator(string str, string operators, bool reverse = false)
{
	// TODO: use stack object to support all bracket types?
	size_t bracketStack = 0;

	if (reverse) std::reverse(str.begin(), str.end());

	char curr = 0;
	for (size_t i = 0; i < str.length(); i++)
	{
		curr = str[i];
		if (curr == '(')
		{
			bracketStack++;
		}
		else if (curr == ')')
		{
			bracketStack--;
		}
		else if (bracketStack == 0 && (operators.find(curr) != operators.npos))
		{
			if (reverse) return str.length() - 1 - i;
			return i;
		}
	}

	return str.npos;
}



pair<int, size_t> getMathFunction(string equation)
{
	void* func = nullptr;
	for (char& c : equation) c = upper(c);

	for (int i = 0; i < funcNames.size(); i++)
	{
		for (string funcName : funcNames[i])
		{
			if (equation.find(funcName) == 0)
			{
				return { i, funcName.length() };
			}
		}
	}

	return { NONE, equation.npos };
}

unique_ptr<EquationNode> GenerateEquationTree(string equation, vector<pair<char, double>>& vars, size_t substrIndex)
{
	unique_ptr<EquationNode> node(new EquationNode);
	size_t pos;

	// equation pre-processing

	if (substrIndex == 0) // perform once 
	{
		pos = unmatchedBracket(equation);
		if (pos != equation.npos)
			throw EquationError("Found unmatched bracket", substrIndex + pos);
	}

	pos = equation.find_first_not_of(" ");
	if (pos == equation.npos)
		throw EquationError("Missing parameter/empty input", substrIndex);
	substrIndex += pos;
	LRstripWhites(equation);

	// Unpack brackets
	if (isBracketPacked(equation))
	{
		equation.erase(equation.begin());
		equation.erase(equation.end() - 1);
		substrIndex++;
	}

	// End of pre-processing
	// -- 
	// Beginning of operator checks

	// Subtraction, division and power care about order, thus we sometimes find in reverse

	pos = findOperator(equation, "+-", true);
	if (pos != equation.npos)
	{
		node->_left = (GenerateEquationTree(equation.substr(0, pos), vars, substrIndex));
		node->_right = (GenerateEquationTree(equation.substr(pos + 1, equation.npos), vars, substrIndex + pos + 1));
		if (equation[pos] == '+')
			node->_evalFunc = [](EquationNode* curNode) {return curNode->_left->Evaluate() + curNode->_right->Evaluate(); };
		else
			node->_evalFunc = [](EquationNode* curNode) {return curNode->_left->Evaluate() - curNode->_right->Evaluate(); };
		return node;
	}

	pos = findOperator(equation, "*/", true);
	if (pos != equation.npos)
	{
		node->_left = (GenerateEquationTree(equation.substr(0, pos), vars, substrIndex));
		node->_right = (GenerateEquationTree(equation.substr(pos + 1, equation.npos), vars, substrIndex + pos + 1));
		if (equation[pos] == '*')
			node->_evalFunc = [](EquationNode* curNode) {return curNode->_left->Evaluate() * curNode->_right->Evaluate(); };
		else
			node->_evalFunc = [](EquationNode* curNode) {return curNode->_left->Evaluate() / curNode->_right->Evaluate(); };
		return node;
	}

	pos = findOperator(equation, "^");
	if (pos != equation.npos)
	{
		node->_left = (GenerateEquationTree(equation.substr(0, pos), vars, substrIndex));
		node->_right = (GenerateEquationTree(equation.substr(pos + 1, equation.npos), vars, substrIndex + pos + 1));
		node->_evalFunc = [](EquationNode* curNode) {return pow(curNode->_left->Evaluate(), curNode->_right->Evaluate()); };
		return node;
	}

	// End of operator checks
	// --
	// Beginning of function checks (Trig, log, ...)

	pair<int, size_t> mathFunction = getMathFunction(equation);
	int type = mathFunction.first;
	pos = mathFunction.second;
	if (pos != equation.npos)
	{
		node->_left = GenerateEquationTree(equation.substr(pos, equation.npos), vars, substrIndex + pos);

		std::function<double(EquationNode* curNode)> evalFunc;
		switch (type)
		{
		case COSINE:
		{
			evalFunc = [](EquationNode* curNode) {return cos(curNode->_left->Evaluate()); };
			break;
		}
		case SINE:
		{
			evalFunc = [](EquationNode* curNode) {return sin(curNode->_left->Evaluate()); };
			break;
		}
		case TANGENT:
		{
			evalFunc = [](EquationNode* curNode) {return tan(curNode->_left->Evaluate()); };
			break;
		}
		case ACOSINE:
		{
			evalFunc = [](EquationNode* curNode) {return acos(curNode->_left->Evaluate()); };
			break;
		}
		case ASINE:
		{
			evalFunc = [](EquationNode* curNode) {return asin(curNode->_left->Evaluate()); };
			break;
		}
		case ATANGENT:
		{
			evalFunc = [](EquationNode* curNode) {return atan(curNode->_left->Evaluate()); };
			break;
		}
		case LOG:
		{
			evalFunc = [](EquationNode* curNode) {return log(curNode->_left->Evaluate()); };
			break;
		}
		}
		node->_evalFunc = evalFunc;
		return node;
	}


	// End of function checks
	// -- 
	// Beginning of raw value checks


	for (std::pair<char, double>& var : vars)
	{
		if (equation.length() == 1 && upper(equation[0]) == upper(var.first))
		{
			double& var_ref = var.second;
			node->_evalFunc = [&var_ref](EquationNode* curNode) { return var_ref; };
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
		throw EquationError("Found invalid parameter, parameter must be a single number/existing variable name", substrIndex);
	}
	catch (std::out_of_range err)
	{
		throw EquationError("Parameter value too large", substrIndex);
	}

	// This covers the case of parameters with no operation between them, for example: "15 23.7" (stod() succeeds here)
	if (num_end < equation.length())
	{
		throw EquationError("Found invalid parameter, parameter must be a single number/existing variable name", substrIndex);
	}

	node->_evalFunc = [value](EquationNode* curNode) { return value; };
	return node;
}