#pragma once

#include <vector>
#include <string>
#include <imgui.h>
using std::vector; using std::string;

class Console
{
public:
	void Draw(bool* p_open);
	Console();
	//TODO: Check if this can be private if I pass "this*" to the forwarding lambda
	int TextEditCallback(ImGuiInputTextCallbackData* data);
	bool IsFocused();

private:
	void ExecCommand(string raw);

	string _inputBuff;
	vector<string> _log;
	vector<const char*> _commands;
	vector<string> _history;
	short _historyPos;    // -1: new line, 0..History.Size-1 browsing history.
	//ImGuiTextFilter       Filter;
	bool _autoScroll;
	bool _scrollToBottom;
	bool _focused;

	// some pointer to graph manager
};