#include "Graph.h"
#include <stack>


EquationNode::EquationNode(EquationNode* left, EquationNode* right)
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

EquationNode* GenerateEquationTree(string equation, vector<pair<char, double>>& vars, size_t substrIndex)
{
	EquationNode* node = nullptr;
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
		node->setLeft(GenerateEquationTree(equation.substr(0, pos), vars, substrIndex));
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
		node->setLeft(GenerateEquationTree(equation.substr(0, pos), vars, substrIndex));
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
		node->setLeft(GenerateEquationTree(equation.substr(0, pos), vars, substrIndex));
		node->setRight(GenerateEquationTree(equation.substr(pos + 1, equation.npos), vars, substrIndex + pos + 1));
		node->SetEvaluationFunc([](EquationNode * curNode) {return pow(curNode->left()->Evaluate(),curNode->right()->Evaluate()); });
		return node;
	}
	// TODO: Implement for rest of operations/math functions

	// End of operator checks
	// --
	// Beginning of function checks (Trig, log, ...)

	pair<int, size_t> mathFunction = getFunction(equation);
	int type = mathFunction.first;
	pos = mathFunction.second;
	if (pos != equation.npos)
	{
		node = new EquationNode;
		node->setLeft(GenerateEquationTree(equation.substr(pos, equation.npos), vars, substrIndex + pos));

		std::function<double(EquationNode * curNode)> evalFunc;
		switch (type)
		{
		case COSINE:
		{
			evalFunc = [](EquationNode * curNode) {return cos(curNode->left()->Evaluate()); };
			break;
		}
		case SINE:
		{
			evalFunc = [](EquationNode * curNode) {return sin(curNode->left()->Evaluate()); };
			break;
		}
		case TANGENT:
		{
			evalFunc = [](EquationNode * curNode) {return tan(curNode->left()->Evaluate()); };
			break;
		}
		case ACOSINE:
		{
			evalFunc = [](EquationNode * curNode) {return acos(curNode->left()->Evaluate()); };
			break;
		}
		case ASINE:
		{
			evalFunc = [](EquationNode * curNode) {return asin(curNode->left()->Evaluate()); };
			break;
		}
		case ATANGENT:
		{
			evalFunc = [](EquationNode * curNode) {return atan(curNode->left()->Evaluate()); };
			break;
		}
		case LOG:
		{
			evalFunc = [](EquationNode * curNode) {return log(curNode->left()->Evaluate()); };
			break;
		}
		}
		node->SetEvaluationFunc(evalFunc);
		return node;
	}


	// End of function checks
	// -- 
	// Beginning of raw value checks

	
	for (std::pair<char, double>& var : vars)
	{
		if (equation.length() == 1 && upper(equation[0]) == upper(var.first)) 
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
		throw EquationError("Found invalid parameter, parameter must be a single number/existing variable name", substrIndex);
	}
	catch (std::out_of_range err)
	{
		throw EquationError("Parameter value too large", substrIndex);
	}

	// This covers the case of parameters with no operation between them, for example: "15 23.7" (stod succeeds here)
	if (num_end < equation.length())
	{
		throw EquationError("Found invalid parameter, parameter must be a single number/existing variable name", substrIndex);
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
	if (c >= 97 && c <= 122) c -= (97 - 65);
	return c;
}



pair<int, size_t> getFunction(string equation)
{
	void* func = nullptr;
	for (int i = 0; i < equation.length(); i++)
	{
		equation[i] = upper(equation[i]);
	}

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

	return {NONE, equation.npos};
}

Graph::Graph(size_t id, GLuint program, double& x, double& z, EquationNode* graphEquation) : _id(id), _program(program), _x(x), _z(z)
{
	_draw3D = true; _draw2D = true;
	_graphEquation = graphEquation;
	_samples = 30; _sampleSize = 1;
	_buffer3D1 = NULL; _buffer3D2 = NULL; _buffer2D = NULL;
}

//
// ------ Graph ------
//

void Graph::Generate()
{
	//size of axis & marks
	int graph_size = 100;

	unsigned int estimatedPolys3D = pow(_samples * 2, 2);
	position* graph_3d_1 = (position*)malloc(estimatedPolys3D * sizeof(position));
	position* graph_3d_2 = (position*)malloc(estimatedPolys3D * sizeof(position));

	unsigned int estimatedPolys2D = _samples * 2;
	position* graph_2d = (position*)malloc(estimatedPolys2D * sizeof(position));



	//generate 3d graph
	int index = 0;
	double range = (_samples * _sampleSize);
	for (_x = -range; _x < range; _x += _sampleSize) {
		for (_z = -range; _z < range; _z += _sampleSize) {
			graph_3d_1[index].x = (GLfloat)_x;
			graph_3d_1[index].z = (GLfloat)_z;

			// TODO: incomplete function domain handling
			graph_3d_1[index].y = _graphEquation->Evaluate(); 

			index++;
		}
	}
	
	index = 0;
	for (_z = -range; _z < range; _z += _sampleSize) {
		for (_x = -range; _x < range; _x += _sampleSize) {
			graph_3d_2[index].x = (GLfloat)_x;
			graph_3d_2[index].z = (GLfloat)_z;

			// TODO: incomplete function domain handling
			graph_3d_2[index].y = _graphEquation->Evaluate();

			index++;
		}
	}

	//generate graph_2d
	index = 0;
	_z = 0;
	for (_x = -range; _x < range; _x += _sampleSize) {
		graph_2d[index].x = (GLfloat)_x;
		graph_2d[index].z = (GLfloat)_z;

		graph_2d[index].y = _graphEquation->Evaluate();

		index++;
	}

	//buffers for 3d graph
	glGenBuffers(1, &_buffer3D1);
	glBindBuffer(GL_ARRAY_BUFFER, _buffer3D1);
	glBufferData(GL_ARRAY_BUFFER, estimatedPolys3D * sizeof(position), graph_3d_1, GL_STATIC_DRAW);
	glGenBuffers(1, &_buffer3D2);
	glBindBuffer(GL_ARRAY_BUFFER, _buffer3D2);
	glBufferData(GL_ARRAY_BUFFER, estimatedPolys3D * sizeof(position), graph_3d_2, GL_STATIC_DRAW);

	//buffer for 2d graph
	glGenBuffers(1, &_buffer2D);
	glBindBuffer(GL_ARRAY_BUFFER, _buffer2D);
	glBufferData(GL_ARRAY_BUFFER, estimatedPolys2D * sizeof(position), graph_2d, GL_STATIC_DRAW);

	free(graph_3d_1);
	free(graph_3d_2);
	free(graph_2d);

	//console debug
	// TODO: Replace with logging
	printf("3D graph generated, samples used: %d\n", _samples * 2);
	printf("2D graph generated, samples used: %d\n", _samples * 2);
	int polyCount = (pow(_samples * 2, 2) + _samples * 2);
	printf("Total number of polygons: %d\n", polyCount);
}

void Graph::Draw()
{
	GLuint uniform_color = glGetUniformLocation(_program, "color");
	//allowed to draw?
	if (_draw3D) {
		//color for 3d graph pt 1
		glUniform4f(uniform_color, 1, 0.5, 0.1, 1);
		// graph
		glBindBuffer(GL_ARRAY_BUFFER, _buffer3D1);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
		for (int i = 0; i < _samples * 2; i++) {
			glDrawArrays(GL_LINE_STRIP, i * _samples * 2, _samples * 2);
		}
		glDisableVertexAttribArray(0);

		//color for 3d graph pt 2
		glUniform4f(uniform_color, 1, 0, 0, 1);
		glBindBuffer(GL_ARRAY_BUFFER, _buffer3D2);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
		for (int i = 0; i < _samples * 2; i++) {
			glDrawArrays(GL_LINE_STRIP, i * _samples * 2, _samples * 2);
		}
		glDisableVertexAttribArray(0);
	}

	//allowed to draw?
	if (_draw2D) {
		//color for 2d graph
		glUniform4f(uniform_color, 1, 0, 1, 1);
		glBindBuffer(GL_ARRAY_BUFFER, _buffer2D);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glDrawArrays(GL_LINE_STRIP, 0, _samples * 4);
		glDisableVertexAttribArray(0);
	}
}

void Graph::SetEquation(EquationNode* graphEquation)
{
}

GraphManager::GraphManager(GLuint program) : _program(program)
{
	_curId = 0;
	_vars.push_back({ 'x', 0 });
	_vars.push_back({ 'z', 0 });
}

void GraphManager::Draw()
{
	// TODO: Implement ordered layering?
	for (Graph g : _graphs)
	{
		g.Draw();
	}
}

size_t GraphManager::NewGraph(string equation)
{
	EquationNode* gEq = GenerateEquationTree(equation, _vars);

	size_t pos_x; size_t pos_z;
	for (size_t i = 0; i < _vars.size(); i++)
	{
		if (_vars[i].first == 'x') pos_x = i;
		else if (_vars[i].first == 'z') pos_z = i;
	}

	_curId++;
	Graph g(_curId, _program, _vars[pos_x].second, _vars[pos_z].second, gEq);
	g.Generate();
	_graphs.push_back(g);

	return _curId;
}
