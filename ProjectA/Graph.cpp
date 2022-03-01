#include "Graph.h"
#include <stack>

#define GRAPH_SIDES 2
#define INDEX_REPEATS 2
#define NON_REPEATING_INDEX_ROWS 2
#define DUPLICATE_INDECIES 2
#define VERTEX_DIMENSIONS 3

EquationNode::EquationNode(EquationNode* left, EquationNode* right)
	: _left(left), _right(right)
{
	_evalFunc = [](EquationNode * curNode) { return 0.0; };
}

EquationNode::~EquationNode()
{
	delete _left;
	delete _right;
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
	if (pos != equation.npos) 
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
	if (c >= 'a' && c <= 'z') c -= ('a' - 'A');
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

//
// ------ Graph Section ------
//

Graph::Graph(size_t id, GLuint program, double& x, double& z, EquationNode* graphEquation = nullptr) : id(id), _program(program),
	_x(x), _z(z), _graphEquation(graphEquation)
{
	show = true;
	_bufferHorizontalOutlineZ = NULL; _bufferHorizontalOutlineX = NULL; _bufferGraphSurface = NULL;
}

void Graph::Generate(double sampleSize, size_t sampleCount, size_t resolution)
{
	unsigned int estimatedPolys3D = pow(sampleCount * resolution * GRAPH_SIDES, 2);
	// TODO: change to new/delete?
	position* graph_3d_1 = (position*)malloc(estimatedPolys3D * sizeof(position));
	position* graph_3d_2 = (position*)malloc(estimatedPolys3D * sizeof(position));
	position* graph_3d = (position*)malloc(estimatedPolys3D * sizeof(position));

	int index = 0;
	int range = sampleCount;
	// The vertices are generated more densly in one axis to make for a smoother graph
	int smoothRange = sampleCount * resolution;

	for (int i = -range; i < range; i++)
	{
		_x = i * sampleSize;
		for (int j = -smoothRange; j < smoothRange; j++)
		{
			_z = j * sampleSize / resolution;

			graph_3d_1[index].x = (GLfloat)(_x / sampleSize);
			graph_3d_1[index].z = (GLfloat)(_z / sampleSize);

			graph_3d_1[index].y = _graphEquation->Evaluate();

			index++;
		}
	}

	index = 0;
	for (int i = -range; i < range; i++)
	{
		_z = i * sampleSize;
		for (int j = -smoothRange; j < smoothRange; j++)
		{
			_x = j * sampleSize / resolution;

			graph_3d_2[index].x = (GLfloat)(_x / sampleSize);
			graph_3d_2[index].z = (GLfloat)(_z / sampleSize);

			graph_3d_2[index].y = _graphEquation->Evaluate();

			index++;
		}
	}

	// Temporary buffer for triangles
	index = 0;
	for (int i = -smoothRange; i < smoothRange; i++)
	{
		_z = i * sampleSize / resolution;
		for (int j = -smoothRange; j < smoothRange; j++)
		{
			_x = j * sampleSize / resolution;

			graph_3d[index].x = (GLfloat)(_x / sampleSize);
			graph_3d[index].z = (GLfloat)(_z / sampleSize);

			graph_3d[index].y = _graphEquation->Evaluate();

			index++;
		}
	}

	

	//buffers for 3d graph
	glGenBuffers(1, &_bufferHorizontalOutlineZ);
	glBindBuffer(GL_ARRAY_BUFFER, _bufferHorizontalOutlineZ);
	glBufferData(GL_ARRAY_BUFFER, estimatedPolys3D * sizeof(position), graph_3d_1, GL_STATIC_DRAW);
	glGenBuffers(1, &_bufferHorizontalOutlineX);
	glBindBuffer(GL_ARRAY_BUFFER, _bufferHorizontalOutlineX);
	glBufferData(GL_ARRAY_BUFFER, estimatedPolys3D * sizeof(position), graph_3d_2, GL_STATIC_DRAW);
	glGenBuffers(1, &_bufferGraphSurface);
	glBindBuffer(GL_ARRAY_BUFFER, _bufferGraphSurface);
	glBufferData(GL_ARRAY_BUFFER, estimatedPolys3D * sizeof(position), graph_3d, GL_STATIC_DRAW);

	free(graph_3d_1);
	free(graph_3d_2);
	free(graph_3d);

}

void Graph::Draw(GLuint sampleCount, GLuint resolution, GLuint* indexBuffer)
{
	GLuint uniform_color = glGetUniformLocation(_program, "color");
	size_t vertexCount = sampleCount * resolution * GRAPH_SIDES;


	size_t triangleVertexCount = (pow(vertexCount + DUPLICATE_INDECIES, 2) * INDEX_REPEATS) - NON_REPEATING_INDEX_ROWS * vertexCount;
	glUniform4f(uniform_color, 0, 0, 0.3, 0.1);
	glBindBuffer(GL_ARRAY_BUFFER, _bufferGraphSurface);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, VERTEX_DIMENSIONS, GL_FLOAT, GL_FALSE, 0, 0);
	glDrawElements(GL_TRIANGLE_STRIP, triangleVertexCount, GL_UNSIGNED_INT, (void*)indexBuffer);
	glDisableVertexAttribArray(0);


	//color for 3d graph pt 1
	glUniform4f(uniform_color, 1, 0.5, 0.1, 1);
	// graph
	glBindBuffer(GL_ARRAY_BUFFER, _bufferHorizontalOutlineZ);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, VERTEX_DIMENSIONS, GL_FLOAT, GL_FALSE, 0, 0);
	for (int i = 0; i < vertexCount; i++) {
		glDrawArrays(GL_LINE_STRIP, i * vertexCount, vertexCount);
	}
	glDisableVertexAttribArray(0);

	//color for 3d graph pt 2
	glUniform4f(uniform_color, 1, 0, 0, 1);
	glBindBuffer(GL_ARRAY_BUFFER, _bufferHorizontalOutlineX);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, VERTEX_DIMENSIONS, GL_FLOAT, GL_FALSE, 0, 0);
	for (int i = 0; i < vertexCount; i++) {
		glDrawArrays(GL_LINE_STRIP, i * vertexCount, vertexCount);
	}
	glDisableVertexAttribArray(0);

}

void Graph::SetEquation(EquationNode* graphEquation)
{
	_graphEquation = graphEquation;
}

//
// ------ GraphManager Section ------
//

GraphManager::GraphManager(GLuint program, vector<pair<string, void*>>* windowVars) : _program(program)
{
	_curId = 0;
	_vars.push_back({ 'x', 0 });
	_vars.push_back({ 'z', 0 });

	_sampleCount = 30;
	_resolution = 4;

	_graphZoom = nullptr;
	_indexBuff = nullptr;
	_curGraphZoom = 1;

	if (windowVars == nullptr) return;
	for (auto var : *windowVars)
	{
		if (var.first == "graphZoom")
		{
			_graphZoom = (double*)var.second;
			_curGraphZoom = *_graphZoom;
		}
	}

	generateIndecies();
}

GraphManager::~GraphManager()
{
	// Free equation memory
	for (pair<size_t, EquationNode*> equation : _graphEquations)
	{
		delete equation.second;
	}
}

void GraphManager::Draw()
{
	// Force zoom updates
	// TODO: Might want to delete this and instead handle the zoom callback itself in GraphManager
	if (_curGraphZoom != *_graphZoom)
	{
		_curGraphZoom = *_graphZoom;

		for (Graph& g : _graphs)
		{
			if(g.show) g.Generate(exp(_curGraphZoom), _sampleCount, _resolution);
		}
	}
	// TODO: Implement ordered layering?
	for (Graph& g : _graphs)
	{
		if (g.show) g.Draw(_sampleCount, _resolution, _indexBuff);
	}
}

void GraphManager::_DeleteEquation(size_t graphId)
{
	for (vector<pair<size_t, EquationNode*>>::iterator eq = _graphEquations.begin(); eq != _graphEquations.end(); ++eq)
	{
		if (eq->first == graphId)
		{
			delete eq->second;
			_graphEquations.erase(eq);
			return;
		}
	}
}

size_t GraphManager::NewGraph(string equation)
{
	EquationNode* eqHead = GenerateEquationTree(equation, _vars);
	// It's a bit more efficient and safe to manage equation memory in here over Graph
	_graphEquations.push_back({ ++_curId, eqHead });

	size_t pos_x; size_t pos_z;
	for (size_t i = 0; i < _vars.size(); i++)
	{
		if (_vars[i].first == 'x') pos_x = i;
		else if (_vars[i].first == 'z') pos_z = i;
	}

	_graphs.push_back({ _curId, _program, _vars[pos_x].second, _vars[pos_z].second, eqHead });
	
	double sampleSize = 1;
	if (_graphZoom != nullptr) sampleSize = exp(*_graphZoom);
	_graphs.back().Generate(sampleSize, _sampleCount, _resolution);

	return _curId;
}

size_t GraphManager::RemoveGraph(size_t graphId)
{
	for (vector<Graph>::iterator it = _graphs.begin(); it != _graphs.end(); ++it)
	{
		if (it->id == graphId)
		{
			it->show = false;
			_DeleteEquation(graphId);
			return NO_ERR;
		}
	}
	return NOT_FOUND;
}

void GraphManager::generateIndecies()
{
	// The Indecies are a bit wider than the graph. This is because we need to generate "degenerate" triangles (with area 0) which
	// won't be drawn by opengl, so that we can render all the triangles in one strip that has cuts at the edges of the graph.
	// We do this by adding duplicate indecies at the edges.
	// Note: We add 2 duplicate indecies per row, to perserve the orientation of the triangles by keeping the index count even.
	delete _indexBuff;
	GLuint estimatedIndecies = pow((_sampleCount * _resolution * GRAPH_SIDES) + DUPLICATE_INDECIES, 2) * INDEX_REPEATS;
	_indexBuff = (GLuint*)malloc(estimatedIndecies * sizeof(GLuint));

	//Index buffer for triangle elements
	GLuint graphWidth = _sampleCount * _resolution * 2;
	unsigned int index = 0;
	int j = 0;
	for (int i = 0; i < graphWidth - 1; i++)
	{
		j = 0;
		// First duplicate index ahead of the row to create a degenrate triangle
		_indexBuff[index] = (i * graphWidth) + j;
		index++;

		// Real triangle indexies
		for (; j < graphWidth; j++)
		{
			_indexBuff[index] = (i * graphWidth) + j;
			index++;

			_indexBuff[index] = ((i + 1) * graphWidth) + j;
			index++;
		}

		// Second duplicate index infront of the row
		_indexBuff[index] = ((i + 1) * graphWidth) + (j - 1);
		index++;
	}
}

