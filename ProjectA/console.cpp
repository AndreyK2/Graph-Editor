#include "Console.h"
#include "misc/cpp/imgui_stdlib.h" // TODO: change all includes to this format?

Console::Console(GraphManager* gm) : _graphManager(gm)
{
	_inputBuff.assign(_inputBuff.size(), 0);
	_historyPos = -1;

	// "CLASSIFY" is here to provide the test case where "C"+[tab] completes to "CL" and display multiple matches.
	_commands.push_back("HELP");
	_commands.push_back("HISTORY");
	_commands.push_back("CLEAR");
	_commands.push_back("GRAPH");
	_commands.push_back("REMOVE");
	_autoScroll = true;
	_scrollToBottom = false;
	_focused = false;
	//AddLog("Welcome to Dear ImGui!");
}

/*
TODO: Handles cmdline input callbacks such as auto-completion and cmdline history
*/
int Console::TextEditCallback(ImGuiInputTextCallbackData* data)
{
	switch (data->EventFlag)
	{

	}
	return 0;
}

bool Console::IsFocused()
{
	return _focused;
}

void Console::Draw(bool* p_open)
{
	ImGui::SetNextWindowPos(ImVec2(0, 200), ImGuiCond_FirstUseEver); // TODO: Set below top bar
	ImGui::SetNextWindowSize(ImVec2(520, 600), ImGuiCond_FirstUseEver);
	if (!ImGui::Begin("Graph Editing Console", p_open))
	{
		ImGui::End();
		return;
	}

	// Options menu
	if (ImGui::BeginPopup("Options"))
	{
		ImGui::Checkbox("Auto-scroll", &_autoScroll);
		ImGui::EndPopup();
	}
	// Options
	if (ImGui::Button("Options"))
		ImGui::OpenPopup("Options");

	ImGui::SameLine();
	if (ImGui::Button("Clear")) { _log.clear(); }
	ImGui::TextWrapped("Enter 'HELP' for help.");
	ImGui::Separator();

	const float footer_height_to_reserve = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
	ImGui::BeginChild("ScrollingRegion", ImVec2(0, -footer_height_to_reserve), false, ImGuiWindowFlags_HorizontalScrollbar);

	// TODO: if you have thousands of entries this approach may be too inefficient and may require user-side clipping
	// to only process visible items. The clipper will automatically measure the height of your first item and then

	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 1)); // Tighten spacing
	// TODO: copy log?
	for (string entry : _log)
	{
		ImVec4 color;
		bool has_color = false;
		if (entry.find("[error]") == 0)
		{
			color = ImVec4(1.0f, 0.4f, 0.4f, 1.0f);
			has_color = true;
		}
		// TODO: Additional colors
		if (has_color)
		{
			ImGui::PushStyleColor(ImGuiCol_Text, color);
			ImGui::TextUnformatted(entry.c_str());
			ImGui::PopStyleColor();
		}
		else
			ImGui::TextUnformatted(entry.c_str());

		if (_scrollToBottom || (_autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()))
			ImGui::SetScrollHereY(1.0f);
		_scrollToBottom = false;
	}

	ImGui::PopStyleVar();
	// Ending of logged text area
	ImGui::EndChild();
	ImGui::Separator();

	bool reclaim_keyboard_focus = false;

	ImGuiInputTextFlags input_text_flags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackCompletion | ImGuiInputTextFlags_CallbackHistory;
	auto callbackForwarder = [](ImGuiInputTextCallbackData * data) {Console* console = (Console*)data->UserData; return console->TextEditCallback(data); };
	if (ImGui::InputText("Input", &_inputBuff, input_text_flags, (ImGuiInputTextCallback)callbackForwarder, (void*)this))
	{
		string userInputPrefix("> ");
		_log.push_back(userInputPrefix + _inputBuff);
		ExecCommand(_inputBuff);
		reclaim_keyboard_focus = true;
		_inputBuff.clear();
	}

	_focused = false;
	if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows))
	{
		_focused = true;
	}

	// Auto-focus on window apparition
	ImGui::SetItemDefaultFocus();
	if (reclaim_keyboard_focus)
		ImGui::SetKeyboardFocusHere(-1); // Auto focus previous widget
	

	ImGui::End();
}

void Console::ExecCommand(string raw)
{
	// lstrip
	raw.erase(0, raw.find_first_not_of(' '));

	string cmd = "";
	bool found = false;
	for (string command : _commands)
	{
		if (toUpper(raw).find(command + " ") == 0 || (toUpper(raw).find(command) == 0 && raw.length() == command.length()))
		{
			cmd = command;
			raw.erase(0, cmd.size() + 1);
			found = true;
		}
	}

	if (!found)
	{
		_log.push_back("Unrecognized command");
		return;
	}
		
	// if command matched
	vector<string> args;

	bool validArgs = true;
	size_t pos_start = 0; size_t pos_end = 0;
	while (pos_end != raw.npos) // todo
	{
		pos_start = raw.find_first_not_of(' ', pos_end);
		if (pos_start == raw.npos) break;

		if (raw[pos_start] == '"') // For spaced arguments in quatations
		{
			pos_end = raw.find_first_of('"', pos_start + 1);
			if (pos_end == raw.npos)
			{
				validArgs = false;
				IndexedError("Unmatched quatation", raw, pos_start);
				return;
			}
			else if (pos_end < raw.size() - 1 && raw[pos_end+1] != ' ')
			{
				validArgs = false;
				IndexedError("Invalid quatation argument", raw, pos_start);
				return;
			}
			pos_end++;
		}
		else
		{
			pos_end = raw.find_first_of(' ', pos_start);
		}
		args.push_back(raw.substr(pos_start, pos_end - pos_start));
	}

	// strip quoted args
	for (string& arg : args)
	{
		if (arg[0] == '"')
		{
			arg.erase(0, 1);
			arg.erase(arg.size() - 1, 1);
		}
	}

	// handle commands
	size_t cargs = args.size();
	if (cmd == "GRAPH")
	{
		if (cargs != 1)
		{
			_log.push_back("Invalid usage, try: graph [equation]");
			return;
		}
		
		try
		{
			size_t id = _graphManager->NewGraph(args[0]);
			_log.push_back("Generated graph with id: " + std::to_string(id)); 
		}
		catch (EquationError err)
		{
			IndexedError(err.what(), args[0], err.index());
		}
	}
	else if(cmd == "HELP")
	{
		if (cargs == 0)
		{
			string result = "Available commands:";
			for (string command : _commands)
			{
				result.append(" " + command + ",");
			}
			result.erase(result.size() - 1);
			_log.push_back(result);
		}
		else if (cargs == 1)
		{
			// todo(?): move command descriptions to type?
			string cmdName = toUpper(args[0]);
			if (cmdName == "GRAPH")
			{
				_log.push_back("graph [eq]\nGenerates a new 3D graph with equation [eq]");
			}
			else if (cmdName == "REMOVE")
			{
				_log.push_back("remove [id]\nRemoves a graph with id [id]");
			}
			else
			{
				_log.push_back("Unrecognized command/No Description exists");
			}
		}
		else
		{
			_log.push_back("Invalid usage, try: \"help\" for available commands, \"help [command name]\" for command description");
		}
	}
	else if (cmd == "REMOVE")
	{
		if (cargs != 1)
		{
			_log.push_back("Invalid usage, try: remove [graph id]");
			return;
		}

		size_t id = stoi(args[0]);
		size_t result = _graphManager->RemoveGraph(id);
		if (result == NOT_FOUND)
		{
			_log.push_back("[error] Could not find graph with id " + args[0]);
		}
	}
	else
	{
		_log.push_back("Not implemented");
	}
}

void Console::IndexedError(string err, string input, size_t index)
{
	_log.push_back("[error] " + err);
	_log.push_back("[error] " + input);
	string marker = "^";
	marker.insert(0, index, ' ');
	_log.push_back("[error] " + marker);
}

string toUpper(string s)
{
	for (char& c : s)
	{
		c = upper(c);
	}
	return s;
}

