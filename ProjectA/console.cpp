#include "Console.h"
#include "misc/cpp/imgui_stdlib.h" // TODO: change all includes to this format?

Console::Console()
{
	_inputBuff.assign(_inputBuff.size(), 0);
	_historyPos = -1;

	// "CLASSIFY" is here to provide the test case where "C"+[tab] completes to "CL" and display multiple matches.
	_commands.push_back("HELP");
	_commands.push_back("HISTORY");
	_commands.push_back("CLEAR");
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
		if (entry.find("[error]") == 0) // user input will start with a prefix
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

// TODO: Parse and Exec Commands through some manager that has access to graphs.
void Console::ExecCommand(string raw)
{
}



