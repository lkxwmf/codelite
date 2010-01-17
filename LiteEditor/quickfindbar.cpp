//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//
// copyright            : (C) 2008 by Eran Ifrah
// file name            : quickfindbar.cpp
//
// -------------------------------------------------------------------------
// A
//              _____           _      _     _ _
//             /  __ \         | |    | |   (_) |
//             | /  \/ ___   __| | ___| |    _| |_ ___
//             | |    / _ \ / _  |/ _ \ |   | | __/ _ )
//             | \__/\ (_) | (_| |  __/ |___| | ||  __/
//              \____/\___/ \__,_|\___\_____/_|\__\___|
//
//                                                  F i l e
//
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or
//    (at your option) any later version.
//
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
#include <wx/xrc/xmlres.h>
#include <wx/statline.h>
#include "manager.h"
#include <wx/textctrl.h>
#include <wx/wxscintilla.h>
#include "stringsearcher.h"
#include "quickfindbar.h"

#define CONTROL_ALIGN_STYLE wxLEFT|wxRIGHT|wxALIGN_CENTER_VERTICAL

BEGIN_EVENT_TABLE(QuickFindBar, wxPanel)
	EVT_BUTTON(XRCID("close_quickfind"), QuickFindBar::OnHide)
	EVT_BUTTON(XRCID("find_next_quick"), QuickFindBar::OnNext)
	EVT_BUTTON(XRCID("find_prev_quick"), QuickFindBar::OnPrev)
	EVT_BUTTON(XRCID("replace_quick"),   QuickFindBar::OnReplace)
	EVT_TEXT(XRCID("find_what_quick"),   QuickFindBar::OnText)

	EVT_CHECKBOX(wxID_ANY, QuickFindBar::OnCheckBox)

	EVT_UPDATE_UI(XRCID("find_next_quick"),      QuickFindBar::OnUpdateUI)
	EVT_UPDATE_UI(XRCID("find_prev_quick"),      QuickFindBar::OnUpdateUI)
	EVT_UPDATE_UI(XRCID("replace_quick"),        QuickFindBar::OnReplaceUI)
	EVT_UPDATE_UI(XRCID("replace_with_quick"),   QuickFindBar::OnReplaceUI)

END_EVENT_TABLE()


QuickFindBar::QuickFindBar(wxWindow* parent, wxWindowID id)
		: wxPanel(parent, id)
		, m_findWhat(NULL)
		, m_sci(NULL)
		, m_flags(0)
{
	Hide();
	wxBoxSizer *mainSizer = new wxBoxSizer(wxHORIZONTAL);
	SetSizer(mainSizer);

	wxButton *btn(NULL);
	m_closeButton = new wxBitmapButton(this, XRCID("close_quickfind"), wxXmlResource::Get()->LoadBitmap(wxT("page_close16")));
	mainSizer->Add(m_closeButton, 0, CONTROL_ALIGN_STYLE, 2);
	m_closeButton->SetToolTip(wxT("Close QuickFind Bar"));

	wxStaticText *text = new wxStaticText(this, wxID_ANY, wxT("Find:"));
	mainSizer->Add(text, 0, CONTROL_ALIGN_STYLE, 2);

	m_findWhat = new wxTextCtrl(this, XRCID("find_what_quick"), wxT(""), wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER|wxTE_RICH2);
	m_findWhat->SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));
	m_findWhat->SetMinSize(wxSize(200,-1));
	m_findWhat->Connect(wxEVT_KEY_DOWN, wxKeyEventHandler(QuickFindBar::OnKeyDown), NULL, this);
	m_findWhat->Connect(wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler(QuickFindBar::OnEnter), NULL, this);

	mainSizer->Add(m_findWhat, 1, CONTROL_ALIGN_STYLE, 5);

	// 'Replace with' controls
	m_replaceStaticText = new wxStaticText(this, wxID_ANY, wxT("Replace With:"));
	mainSizer->Add(m_replaceStaticText, 0, CONTROL_ALIGN_STYLE, 2);

	m_replaceWith = new wxTextCtrl(this, XRCID("replace_with_quick"), wxT(""), wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER|wxTE_RICH2);
	mainSizer->Add(m_replaceWith, 1, CONTROL_ALIGN_STYLE, 5);
	m_replaceWith->Connect(wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler(QuickFindBar::OnReplaceEnter), NULL, this);
	m_replaceWith->Connect(wxEVT_KEY_DOWN,           wxKeyEventHandler(QuickFindBar::OnKeyDown), NULL, this);

	btn = new wxButton(this, XRCID("find_next_quick"), wxT("Next"), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
	mainSizer->Add(btn, 0, CONTROL_ALIGN_STYLE, 5);
	btn->SetDefault();

	btn = new wxButton(this, XRCID("find_prev_quick"), wxT("Prev"), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
	mainSizer->Add(btn, 0, CONTROL_ALIGN_STYLE, 5);

	m_replaceButton = new wxButton(this, XRCID("replace_quick"), wxT("Replace"), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
	mainSizer->Add(m_replaceButton, 0, CONTROL_ALIGN_STYLE, 5);

	mainSizer->Add(new wxStaticLine(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_VERTICAL), 0, wxEXPAND);

	wxCheckBox *check = new wxCheckBox(this, XRCID("match_case_quick"), wxT("Case"));
	mainSizer->Add(check, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5);

	check = new wxCheckBox(this, XRCID("match_word_quick"), wxT("Word"));
	mainSizer->Add(check, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5);

	check = new wxCheckBox(this, XRCID("match_regexp_quick"), wxT("Regexp"));
	mainSizer->Add(check, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5);

	wxTheApp->Connect(wxID_COPY,      wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(QuickFindBar::OnCopy),      NULL, this);
	wxTheApp->Connect(wxID_PASTE,     wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(QuickFindBar::OnPaste),     NULL, this);
	wxTheApp->Connect(wxID_SELECTALL, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(QuickFindBar::OnSelectAll), NULL, this);

	wxTheApp->Connect(wxID_COPY,      wxEVT_UPDATE_UI, wxUpdateUIEventHandler(QuickFindBar::OnEditUI), NULL, this);
	wxTheApp->Connect(wxID_PASTE,     wxEVT_UPDATE_UI, wxUpdateUIEventHandler(QuickFindBar::OnEditUI), NULL, this);
	wxTheApp->Connect(wxID_SELECTALL, wxEVT_UPDATE_UI, wxUpdateUIEventHandler(QuickFindBar::OnEditUI), NULL, this);

	mainSizer->Layout();

}

bool QuickFindBar::Show(bool show)
{
	bool res = wxPanel::Show(show);
	if (res) {
		GetParent()->GetSizer()->Show(this, show, true);
		GetParent()->GetSizer()->Layout();
	}
	if (!m_sci) {
		// nothing to do
	} else if (!show) {
		m_sci->SetFocus();
	} else {
		wxString findWhat = m_sci->GetSelectedText().BeforeFirst(wxT('\n'));
		if (!findWhat.IsEmpty()) {
			m_findWhat->SetValue(m_sci->GetSelectedText().BeforeFirst(wxT('\n')));
		}
		m_findWhat->SelectAll();
		m_findWhat->SetFocus();
	}
	return res;
}

void QuickFindBar::DoSearch(bool fwd, bool incr)
{
	if (!m_sci || m_sci->GetLength() == 0 || m_findWhat->GetValue().IsEmpty())
		return;

	wxString find = m_findWhat->GetValue();
	wxString text = m_sci->GetText();

	int start = -1, stop = -1;
	m_sci->GetSelection(&start, &stop);

	int offset = !fwd || incr ? start : stop;
	int flags = m_flags | (fwd ? 0 : wxSD_SEARCH_BACKWARD);
	int pos = 0, len = 0;

	if (!StringFindReplacer::Search(text, offset, find, flags, pos, len)) {
		// wrap around and try again
		offset = fwd ? 0 : text.Len()-1;
		if (!StringFindReplacer::Search(text, offset, find, flags, pos, len)) {
			m_findWhat->SetBackgroundColour(wxT("PINK"));
			m_findWhat->Refresh();
			return;
		}
	}
	m_findWhat->SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));
	m_findWhat->Refresh();
	m_sci->SetSelection(pos, pos+len);
}

void QuickFindBar::OnHide(wxCommandEvent &e)
{
	wxUnusedVar(e);
	Show(false);
}

void QuickFindBar::OnNext(wxCommandEvent &e)
{
	wxUnusedVar(e);
	DoSearch(true, false);
}

void QuickFindBar::OnPrev(wxCommandEvent &e)
{
	wxUnusedVar(e);
	DoSearch(false, false);
}

void QuickFindBar::OnText(wxCommandEvent& e)
{
	wxUnusedVar(e);
	DoSearch(true, true);
}

void QuickFindBar::OnKeyDown(wxKeyEvent& e)
{
	switch (e.GetKeyCode()) {
	case WXK_ESCAPE: {
		wxCommandEvent cmd(wxEVT_COMMAND_BUTTON_CLICKED, m_closeButton->GetId());
		cmd.SetEventObject(m_closeButton);
		GetEventHandler()->AddPendingEvent(cmd);
		break;
	}
	default:
		e.Skip();
	}
}

void QuickFindBar::OnCheckBox(wxCommandEvent &e)
{
	size_t flag = 0;
	if (e.GetId() == XRCID("match_case_quick")) {
		flag = wxSD_MATCHCASE;
	} else if (e.GetId() == XRCID("match_word_quick")) {
		flag = wxSD_MATCHWHOLEWORD;
	} else if (e.GetId() == XRCID("match_regexp_quick")) {
		flag = wxSD_REGULAREXPRESSION;
	}
	if (e.IsChecked()) {
		m_flags |= flag;
	} else {
		m_flags &= ~flag;
	}
}

void QuickFindBar::OnUpdateUI(wxUpdateUIEvent &e)
{
	e.Enable(ManagerST::Get()->IsShutdownInProgress() == false && m_sci && m_sci->GetLength() > 0 && !m_findWhat->GetValue().IsEmpty());
}

void QuickFindBar::OnEnter(wxCommandEvent& e)
{
	wxUnusedVar(e);
	bool shift = wxGetKeyState(WXK_SHIFT);
	wxCommandEvent evt(wxEVT_COMMAND_BUTTON_CLICKED, shift ? XRCID("find_prev_quick") : XRCID("find_next_quick"));
	GetEventHandler()->AddPendingEvent(evt);
}

void QuickFindBar::OnCopy(wxCommandEvent& e)
{
	wxTextCtrl *ctrl = GetFocusedControl();
	if ( !ctrl ) {
		e.Skip();
		return;
	}

	if (ctrl->CanCopy())
		ctrl->Copy();
}

void QuickFindBar::OnPaste(wxCommandEvent& e)
{
	wxTextCtrl *ctrl = GetFocusedControl();
	if ( !ctrl ) {
		e.Skip();
		return;
	}

	if (ctrl->CanPaste())
		ctrl->Paste();
}

void QuickFindBar::OnSelectAll(wxCommandEvent& e)
{
	wxTextCtrl *ctrl = GetFocusedControl();
	if ( !ctrl ) {
		e.Skip();
	} else {
		ctrl->SelectAll();
	}
}

void QuickFindBar::OnEditUI(wxUpdateUIEvent& e)
{
	wxTextCtrl *ctrl = GetFocusedControl();
	if ( !ctrl ) {
		e.Skip();
		return;
	}

	switch (e.GetId()) {
	case wxID_SELECTALL:
		e.Enable(ctrl->GetValue().IsEmpty() == false);
		break;
	case wxID_COPY:
		e.Enable(ctrl->CanCopy());
		break;
	case wxID_PASTE:
		e.Enable(ctrl->CanPaste());
		break;
	default:
		e.Enable(false);
		break;
	}
}

void QuickFindBar::OnReplace(wxCommandEvent& e)
{
	if (!m_sci)
		return;

	// if there is no selection, invoke search
	wxString selectionText = m_sci->GetSelectedText();
	wxString find          = m_findWhat->GetValue();
	wxString replaceWith   = m_replaceWith->GetValue();

	bool caseSearch        = m_flags & wxSD_MATCHCASE;
	if ( !caseSearch ) {
		selectionText.MakeLower();
		find.MakeLower();
	}

	if (find.IsEmpty())
		return;

	// do we got a match?
	if (selectionText != find)
		DoSearch(true, true);

	else {
		m_sci->ReplaceSelection(replaceWith);
		// and search again
		DoSearch(true, true);
	}
}

void QuickFindBar::OnReplaceUI(wxUpdateUIEvent& e)
{
	e.Enable(   ManagerST::Get()->IsShutdownInProgress() == false &&
	            m_sci                                             &&
	            !m_sci->GetReadOnly()                             &&
	            m_sci->GetLength() > 0                            &&
	            !m_findWhat->GetValue().IsEmpty());
}

wxTextCtrl* QuickFindBar::GetFocusedControl()
{
	wxWindow *win = wxWindow::FindFocus();

	if (win == m_findWhat)
		return m_findWhat;

	else if (win == m_replaceWith)
		return m_replaceWith;

	return NULL;
}

void QuickFindBar::OnReplaceEnter(wxCommandEvent& e)
{
	wxUnusedVar(e);
	wxCommandEvent evt(wxEVT_COMMAND_BUTTON_CLICKED, XRCID("replace_quick"));
	GetEventHandler()->AddPendingEvent(evt);
}

void QuickFindBar::ShowReplaceControls(bool show)
{
	if ( show && !m_replaceWith->IsShown()) {
		m_replaceWith->Show();
		m_replaceButton->Show();
		m_replaceStaticText->Show();
		GetSizer()->Layout();

	} else if ( !show && m_replaceWith->IsShown()) {
		m_replaceWith->Show(false);
		m_replaceButton->Show(false);
		m_replaceStaticText->Show(false);
		GetSizer()->Layout();

	}
}

void QuickFindBar::SetEditor(wxScintilla* sci)
{
	m_sci = sci;
	ShowReplaceControls(m_sci && !m_sci->GetReadOnly());
}
