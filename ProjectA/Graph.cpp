#include "Graph.h"
#include <stack>
#include "misc/cpp/imgui_stdlib.h"

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

	// Subtraction, division and power care about order, thus we sometimes find in reverse

	pos = findOperator(equation, "+-", true);
	if (pos != equation.npos) 
	{
		node->_left = (GenerateEquationTree(equation.substr(0, pos), vars, substrIndex));
		node->_right = (GenerateEquationTree(equation.substr(pos+1, equation.npos), vars, substrIndex + pos + 1));
		if(equation[pos] == '+')
			node->_evalFunc = [](EquationNode * curNode) {return curNode->_left->Evaluate() + curNode->_right->Evaluate(); };
		else
			node->_evalFunc = [](EquationNode * curNode) {return curNode->_left->Evaluate() - curNode->_right->Evaluate(); };
		return node;
	}

	pos = findOperator(equation, "*/", true);
	if (pos != equation.npos)
	{
		node->_left = (GenerateEquationTree(equation.substr(0, pos), vars, substrIndex));
		node->_right = (GenerateEquationTree(equation.substr(pos + 1, equation.npos), vars, substrIndex + pos + 1));
		if(equation[pos] == '*')
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
		node->_evalFunc = [](EquationNode * curNode) {return pow(curNode->_left->Evaluate(),curNode->_right->Evaluate()); };
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
		node->_left = GenerateEquationTree(equation.substr(pos, equation.npos), vars, substrIndex + pos);

		std::function<double(EquationNode * curNode)> evalFunc;
		switch (type)
		{
		case COSINE:
		{
			evalFunc = [](EquationNode * curNode) {return cos(curNode->_left->Evaluate()); };
			break;
		}
		case SINE:
		{
			evalFunc = [](EquationNode * curNode) {return sin(curNode->_left->Evaluate()); };
			break;
		}
		case TANGENT:
		{
			evalFunc = [](EquationNode * curNode) {return tan(curNode->_left->Evaluate()); };
			break;
		}
		case ACOSINE:
		{
			evalFunc = [](EquationNode * curNode) {return acos(curNode->_left->Evaluate()); };
			break;
		}
		case ASINE:
		{
			evalFunc = [](EquationNode * curNode) {return asin(curNode->_left->Evaluate()); };
			break;
		}
		case ATANGENT:
		{
			evalFunc = [](EquationNode * curNode) {return atan(curNode->_left->Evaluate()); };
			break;
		}
		case LOG:
		{
			evalFunc = [](EquationNode * curNode) {return log(curNode->_left->Evaluate()); };
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
			node->_evalFunc = [&var_ref](EquationNode * curNode) { return var_ref; };
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
	
	node->_evalFunc = [value](EquationNode * curNode) { return value; };
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

Graph::Graph(size_t id, GLuint program, double& x, double& z, unique_ptr<EquationNode> graphEquation = nullptr) : id(id), _program(program),
	_x(x), _z(z)
{
	show = true;
	_bufferHorizontalOutlineZupper = NULL; _bufferHorizontalOutlineXupper = NULL; _bufferGraphSurface = NULL;
	_bufferHorizontalOutlineZlower = NULL; _bufferHorizontalOutlineXlower = NULL;
	_graphEquation = std::move(graphEquation);
}


/*
Generates and binds the vertices of the graph surface and graph outlines
*/
void Graph::Generate(double sampleSize, size_t sampleCount, size_t resolution)
{
	unsigned int estimatedPolys3D = pow(sampleCount * resolution * graph_sides, 2);
	
	size_t bufferSize = estimatedPolys3D * sizeof(position);
	
	// TODO: change to new/delete?
	position* graphOutlineZupper = (position*)malloc(bufferSize);
	position* graphOutlineZlower = (position*)malloc(bufferSize);
	position* graphOutlineXupper = (position*)malloc(bufferSize);
	position* graphOutlineXlower = (position*)malloc(bufferSize);
	position* graphSurface = (position*)malloc(bufferSize);
	position* graphSurfaceNormals = (position*)malloc(bufferSize);

	int index = 0;
	int range = sampleCount;
	int smoothRange = sampleCount * resolution;

	for (int i = -smoothRange; i < smoothRange; i++)
	{
		_z = i * sampleSize / resolution;
		for (int j = -smoothRange; j < smoothRange; j++)
		{
			_x = j * sampleSize / resolution;

			graphSurface[index].x = (GLfloat)(_x / sampleSize);
			graphSurface[index].z = (GLfloat)(_z / sampleSize);

			graphSurface[index].y = _graphEquation->Evaluate();

			index++;
		}
	}

	const GLfloat zFightningFix = 0.1;

	index = 0;
	for (int i = -range; i < range; i++)
	{
		_x = i * sampleSize;
		for (int j = -smoothRange; j < smoothRange; j++)
		{
			_z = j * sampleSize / resolution;

			graphOutlineZupper[index].x = (GLfloat)(_x / sampleSize);
			graphOutlineZupper[index].z = (GLfloat)(_z / sampleSize);
			graphOutlineZlower[index].x = graphOutlineZupper[index].x;
			graphOutlineZlower[index].z = graphOutlineZupper[index].z;

			// The outlines can't be placed directly on the graph due to Z fightning.
			// TODO: scale the polygon seperation by the graph zoom?
			graphOutlineZupper[index].y = _graphEquation->Evaluate() + zFightningFix;
			graphOutlineZlower[index].y = graphOutlineZupper[index].y - (2 * zFightningFix);

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

			graphOutlineXupper[index].x = (GLfloat)(_x / sampleSize);
			graphOutlineXupper[index].z = (GLfloat)(_z / sampleSize);
			graphOutlineXlower[index].x = graphOutlineXupper[index].x;
			graphOutlineXlower[index].z = graphOutlineXupper[index].z;

			graphOutlineXupper[index].y = _graphEquation->Evaluate() + zFightningFix;
			graphOutlineXlower[index].y = graphOutlineXupper[index].y - (2 * zFightningFix);

			index++;
		}
	}

	//Bind buffers to OpenGL
	bindVertexBuffer(_bufferHorizontalOutlineZupper, graphOutlineZupper, bufferSize);
	bindVertexBuffer(_bufferHorizontalOutlineZlower, graphOutlineZlower, bufferSize);
	bindVertexBuffer(_bufferHorizontalOutlineXupper, graphOutlineXupper, bufferSize);
	bindVertexBuffer(_bufferHorizontalOutlineXlower, graphOutlineXlower, bufferSize);
	bindVertexBuffer(_bufferGraphSurface, graphSurface, bufferSize);
}

/*
Draws the graph surface and outlines
*/
void Graph::Draw(GLuint sampleCount, GLuint resolution, GLuint* indexBuffer, GraphProperties properties)
{
	GLuint uniform_color = glGetUniformLocation(_program, "color");
	size_t const vertexCount = sampleCount * resolution * graph_sides;
	size_t const vertexDimensions = 3;

	// Enable color grading for the graph
	GLuint uniform_isGradient = glGetUniformLocation(_program, "isGradient");
	glUniform1i(uniform_isGradient, true);

	size_t triangleVertexCount = (pow(vertexCount + duplicate_rowindecies, graph_sides) * index_repeats) - non_repeating_index_rows * vertexCount;
	glUniform4f(uniform_color, properties._sufColor.x, properties._sufColor.y, properties._sufColor.z, properties._sufColor.w);
	glBindBuffer(GL_ARRAY_BUFFER, _bufferGraphSurface);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, vertexDimensions, GL_FLOAT, GL_FALSE, 0, 0);
	glDrawElements(GL_TRIANGLE_STRIP, triangleVertexCount, GL_UNSIGNED_INT, (void*)indexBuffer);
	glDisableVertexAttribArray(0);

	vector<GLuint> outlineBuffers = { _bufferHorizontalOutlineZupper, _bufferHorizontalOutlineZlower,
		_bufferHorizontalOutlineXlower, _bufferHorizontalOutlineXupper };

	for (int i = 0; i < outlineBuffers.size(); i++)
	{
		// color for Z outlines
		if (i == 0) glUniform4f(uniform_color, properties._outlineColorZ.x, properties._outlineColorZ.y, properties._outlineColorZ.z, properties._outlineColorZ.w);
		// color for X outlines
		else if (i == 2) glUniform4f(uniform_color, properties._outlineColorX.x, properties._outlineColorX.y, properties._outlineColorX.z, properties._outlineColorX.w);

		glBindBuffer(GL_ARRAY_BUFFER, outlineBuffers[i]);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, vertexDimensions, GL_FLOAT, GL_FALSE, 0, 0);
		for (int j = 0; j < vertexCount; j++) {
			glDrawArrays(GL_LINE_STRIP, j * vertexCount, vertexCount);
		}
		glDisableVertexAttribArray(0);
	}

	// Revert shader changes
	glUniform1i(uniform_isGradient, false);
}

void Graph::SetEquation(unique_ptr<EquationNode> graphEquation)
{
	_graphEquation = std::move(graphEquation);
}

void Graph::bindVertexBuffer(GLuint& GLbuffer, position* vertexBuffer, size_t size)
{
	glGenBuffers(1, &GLbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, GLbuffer);
	glBufferData(GL_ARRAY_BUFFER, size, vertexBuffer, GL_STATIC_DRAW);
	free(vertexBuffer);
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

	_focused = false;

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

void GraphManager::Draw()
{
	_focused = false;
	for (GraphEditor& e : _graphEditors)
	{
		e.Draw();
	}

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
		GraphProperties prop;
		for (GraphEditor& editor : _graphEditors)
		{
			if (editor.id == g.id) prop = editor._prop;
		}
		if (g.show) g.Draw(_sampleCount, _resolution, _indexBuff, prop);
	}
}

size_t GraphManager::NewGraph(string equation)
{
	unique_ptr<EquationNode> eqHead = GenerateEquationTree(equation, _vars);

	size_t pos_x; size_t pos_z;
	for (size_t i = 0; i < _vars.size(); i++)
	{
		if (_vars[i].first == 'x') pos_x = i;
		else if (_vars[i].first == 'z') pos_z = i;
	}

	_graphs.push_back({ ++_curId, _program, _vars[pos_x].second, _vars[pos_z].second, std::move(eqHead) });
	
	double sampleSize = 1;
	if (_graphZoom != nullptr) sampleSize = exp(*_graphZoom);
	_graphs.back().Generate(sampleSize, _sampleCount, _resolution);

	// Create new editor window
	GraphEditor e(_curId, this, equation);
	_graphEditors.push_back(e);

	return _curId;
}

size_t GraphManager::RemoveGraph(size_t graphId)
{
	auto pos = std::find_if(_graphEditors.begin(), _graphEditors.end(), [graphId](GraphEditor& e) { return e.id == graphId; });
	if (pos != _graphEditors.end()) _graphEditors.erase(pos);
	// TODO: figure out the copy constructor
	auto g = std::find_if(_graphs.begin(), _graphs.end(), [graphId](Graph& g) { return g.id == graphId; });
	g->show = false; 

	return 0;
}

void GraphManager::UpdateEquation(size_t graphId, string equation)
{
	unique_ptr<EquationNode> eqHead = GenerateEquationTree(equation, _vars);
	for (vector<Graph>::iterator it = _graphs.begin(); it != _graphs.end(); ++it)
	{
		if (it->id == graphId)
		{
			it->SetEquation(std::move(eqHead));
			it->Generate(exp(_curGraphZoom), _sampleCount, _resolution);
		}
	}
}

/*
Generates triangle indicies for the vertices of the graph surface
This is in GraphManager because the resolution and sample count are uniform for all graphs, and thus so are the indecies
*/
void GraphManager::generateIndecies()
{
	// The Indecies are a bit wider than the graph. This is because we need to generate "degenerate" triangles (with area 0) which
	// won't be drawn by opengl, so that we can render all the triangles in one strip that is cut at the edges of the graph.
	// We do this by adding duplicate indecies at the edges.
	// Note: We add 2 duplicate indecies per row, to perserve the orientation of the triangles by keeping the index count even.

	delete _indexBuff;

	GLuint graphWidth = _sampleCount * _resolution * Graph::graph_sides;
	GLuint estimatedIndecies = pow((graphWidth) + Graph::duplicate_rowindecies, 2) * Graph::index_repeats;
	_indexBuff = (GLuint*)malloc(estimatedIndecies * sizeof(GLuint));

	//Index buffer for triangle elements
	unsigned int index = 0;
	int j = 0;
	for (int i = 0; i < graphWidth - 1; i++)
	{
		j = 0;
		// First duplicate index ahead of the row to create a degenrate triangle
		_indexBuff[index] = (i * graphWidth) + j;
		index++;

		// Real triangle indecies
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

GraphEditor::GraphEditor(size_t graphId, GraphManager* graphManager, string equation) : \
	id(graphId), _graphManager(graphManager), _equation(equation)
{
	_prop._equation = "";
	_prop._gradingIntensity = 1;
	_prop._sufColor = ImVec4(0.5f, 0.5f, 0.9f, 0.7f);
	_prop._outlineColorX = ImVec4(0.8f, 0.8f, 1.0f, 1.0f);
	_prop._outlineColorZ = ImVec4(1.0f, 0.8f, 0.8f, 1.0f);
	_open = true;
	
}

void GraphEditor::Draw()
{
	// TODO: Set to the right of the screen
	ImGui::SetNextWindowPos(ImVec2(1000, 200), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(300, 300), ImGuiCond_FirstUseEver);

	string windowName = "Graph " + std::to_string((unsigned int)id);
	const char* wName = windowName.c_str();
	if (!ImGui::Begin(wName, &_open))
	{
		ImGui::End();
		return;
	}

	ImGuiColorEditFlags flags = 0;
	flags |= ImGuiColorEditFlags_AlphaBar;
	//flags |= ImGuiColorEditFlags_NoSidePreview;
	flags |= ImGuiColorEditFlags_PickerHueBar;
	//flags |= ImGuiColorEditFlags_PickerHueWheel;
	flags |= ImGuiColorEditFlags_NoInputs;       // Disable all RGB/HSV/Hex displays


	bool open_popup = ImGui::ColorButton("MyColor##3b", _prop._sufColor, flags);
	ImGui::SameLine(0, ImGui::GetStyle().ItemInnerSpacing.x);
	open_popup |= ImGui::Button("Surface Color");
	if (open_popup)
	{
		ImGui::OpenPopup("color picker 1");
	}
	if (ImGui::BeginPopup("color picker 1"))
	{
		ImGui::Separator();
		ImGui::ColorPicker4("Surface Color##4", (float*)&_prop._sufColor, flags); // TODO: implement for outlines aswell
		ImGui::EndPopup();
	}

	open_popup = ImGui::ColorButton("MyColor##3b", _prop._outlineColorX, flags);
	ImGui::SameLine(0, ImGui::GetStyle().ItemInnerSpacing.x);
	open_popup |= ImGui::Button("X Outline Color");
	if (open_popup)
	{
		ImGui::OpenPopup("color picker 2");
	}
	if (ImGui::BeginPopup("color picker 2"))
	{
		ImGui::Separator();
		ImGui::ColorPicker4("X Outline##4", (float*)&_prop._outlineColorX, flags); // TODO: implement for outlines aswell
		ImGui::EndPopup();
	}

	open_popup = ImGui::ColorButton("MyColor##3b", _prop._outlineColorZ, flags);
	ImGui::SameLine(0, ImGui::GetStyle().ItemInnerSpacing.x);
	open_popup |= ImGui::Button("Z Outline Color");
	if (open_popup)
	{
		ImGui::OpenPopup("color picker 3");
	}
	if (ImGui::BeginPopup("color picker 3"))
	{
		ImGui::Separator();
		ImGui::ColorPicker4("Z Outline##4", (float*)&_prop._outlineColorZ, flags); // TODO: implement for outlines aswell
		ImGui::EndPopup();
	}

	ImGui::TextWrapped("Function");
	ImGui::SameLine();

	auto callbackForwarder = [](ImGuiInputTextCallbackData* data) {GraphEditor* ge = (GraphEditor*)data->UserData; return ge->TextEditCallback(data); };
	if (ImGui::InputText("", &_equation, NULL, (ImGuiInputTextCallback)callbackForwarder, (void*)this))
	{
		try
		{
			_graphManager->UpdateEquation(id, _equation);
		}
		catch(EquationError err){} // TODO: indicate that the equation is invalid, maybe with a red outline
	}

	if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows))
	{
		_graphManager->_focused = true;
	}

	ImGui::End();
}

int GraphEditor::TextEditCallback(ImGuiInputTextCallbackData* data)
{
	return 0;
}
