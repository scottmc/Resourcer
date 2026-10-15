//------------------------------------------------------------------------------
//	EditingPanel.cpp
//
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------
#include <stdio.h>
#include <string.h>

// System Includes -------------------------------------------------------------
#include <interface/Box.h>
#include <interface/Button.h>
#include <interface/GroupLayout.h>
#include <interface/SpaceLayoutItem.h>
#include <interface/ColorControl.h>
#include <interface/MenuBar.h>
#include <interface/MenuField.h>
#include <interface/MenuItem.h>
#include <interface/PopUpMenu.h>
#include <interface/RadioButton.h>
#include <interface/ScrollBar.h>
#include <interface/Slider.h>
#include <interface/StringView.h>
#include <interface/TextControl.h>
#include <interface/Window.h>

#include <support/Autolock.h>
#include <support/Beep.h>
#include <support/ClassInfo.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "EditingPanel.h"
#include "MenuEditor.h"
//#include "ResImageView.h"
//#include "ResourceStringView.h"
#include "BViewEditor.h"
#include "BControlEditor.h"
#include "BButtonEditor.h"
#include "BBoxEditor.h"
#include "BStringViewEditor.h"
#include "BSliderEditor.h"
#include "BMenuFieldEditor.h"
#include "BMenuBarEditor.h"
#include "BCheckBoxEditor.h"
#include "BRadioButtonEditor.h"
#include "BColorControlEditor.h"
#include "BTextViewEditor.h"
#include "BTextControlEditor.h"
#include "WIND.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

//------------------------------------------------------------------------------
EditingPanel::EditingPanel(void)
	:	BView(BRect(0, 0, 300, 350), "editpanel", B_FOLLOW_ALL_SIDES, B_FRAME_EVENTS),
		fCurrentEditor(NULL),
		iscontrol(false),
		pointer(NULL)
{
	SetViewColor(216,216,216);
	AddChild(new BStringView(BRect(25,100,175,125),"noactive","No Focus View"));

	InitEditors();
}
//------------------------------------------------------------------------------
EditingPanel::~EditingPanel()
{
	CleanupEditors();
}
//------------------------------------------------------------------------------
// The text sized to fit, so it isn't cut off at a large font size
void EditingPanel::AddNoFocusText()
{
	BStringView* text = new BStringView(BRect(25, 100, 175, 125), "noactive",
										"No Focus View");
	text->ResizeToPreferred();
	AddChild(text);
}
//------------------------------------------------------------------------------
void EditingPanel::FrameResized(float width, float height)
{
	BView::FrameResized(width, height);
	UpdateScrollBar();
}
//------------------------------------------------------------------------------
// The panel sits in a scroll view when there is one.  The editor's controls can
// be taller than the panel (more rows, or a bigger font), so the vertical scroll
// bar covers the height they need.
void EditingPanel::UpdateScrollBar()
{
	BScrollBar* bar = ScrollBar(B_VERTICAL);
	if (bar == NULL)
		return;

	float needed = 0;
	if (GetLayout() != NULL)
	{
		needed = GetLayout()->MinSize().height;
	}
	else
	{
		for (int32 i = 0; i < CountChildren(); ++i)
		{
			float bottom = ChildAt(i)->Frame().bottom + 10;
			if (bottom > needed)
				needed = bottom;
		}
	}

	float visible = Bounds().Height();
	float range = needed - visible;
	if (range < 0)
		range = 0;
	bar->SetRange(0, range);
	if (needed > 0)
		bar->SetProportion(visible / needed);
	bar->SetSteps(be_plain_font->Size() * 1.5, visible > 20 ? visible - 20 : visible);
}
//------------------------------------------------------------------------------
void EditingPanel::AttachedToWindow()
{
	BView::AttachedToWindow();

	BControl* ctrl;
	for (int32 i = 0; i < CountChildren(); ++i)
	{
		ctrl = dynamic_cast<BControl*>(ChildAt(i));
		if (ctrl)
		{
			ctrl->SetTarget(this);
		}
	}
}
//------------------------------------------------------------------------------
void EditingPanel::MessageReceived(BMessage *msg)
{
	if (fCurrentEditor)
	{
		if (fCurrentEditor->Window())
		{
			BAutolock locker(fCurrentEditor->Window());
			if (fCurrentEditor->MessageReceived(msg))
			{
				return;
			}
		}
	}

	// The editors' controls send negative messages here.  When there is no
	// editor (the selection was let go while a field was still being edited)
	// or it did not take one, there is nothing to do: passed on, it would
	// travel up to the window being edited, which is not expecting it.
	if ((int32)msg->what < 0)
	{
		return;
	}

	switch(msg->what)
	{
		default:
			BView::MessageReceived(msg);
			break;
	}
}
//------------------------------------------------------------------------------
void EditingPanel::SetTo(BMessage* active)
{
	BAutolock locker(Window());

	// start at the top again for the new editor
	ScrollTo(0, 0);
	for (BView* dead = ChildAt(0); dead != NULL; dead = ChildAt(0))
	{
		RemoveChild(dead);
	}
	// An editor that is placed by a layout gets a new one each time, the
	// others place their controls by hand, so the panel starts without one.
	// (Taking the layout away also takes the glue that was added at the end.)
	if (GetLayout() != NULL)
	{
		SetLayout(NULL);
	}

	BView* view = NULL;
	if (active)
	{
		if (active->FindPointer("view", (void**)&view) == B_OK)
		{
			BMessage ctrl;
			if (active->FindMessage("ctrl", &ctrl) == B_OK)
			{
				const char* type;
				if (ctrl.FindString("class", &type) == B_OK)
				{
					EditorMap::iterator i = fEditorMap.find(type);
					if (i != fEditorMap.end())
					{
						fCurrentEditor = i->second;
						if (fCurrentEditor->UsesLayout())
						{
							BGroupLayout* group = new BGroupLayout(B_VERTICAL, 4);
							SetLayout(group);
							group->SetInsets(10, 10, 10, 10);
						}
						fCurrentEditor->SetParent(this);
						fCurrentEditor->SetTarget(view);
						fCurrentEditor->SetTo(&ctrl);
						if (GetLayout() != NULL)
						{
							// keeps the controls together at the top
							GetLayout()->AddItem(BSpaceLayoutItem::CreateGlue());
						}
					}
				}
			}
			else
			{
				// Houston, we have a problem ...
				debugger("Archive of currently selected control not found");
			}
		}
	}

	if (!active || !view)
	{
		AddNoFocusText();
		fCurrentEditor = NULL;
		UpdateScrollBar();
		return;
	}

	UpdateScrollBar();
}
//------------------------------------------------------------------------------
void EditingPanel::InitEditors()
{
	RegisterEditor(new BViewEditor);
	RegisterEditor(new BControlEditor);
	RegisterEditor(new BButtonEditor);
	RegisterEditor(new BBoxEditor);
	RegisterEditor(new BStringViewEditor);
	RegisterEditor(new BSliderEditor);
	RegisterEditor(new BMenuFieldEditor);
	RegisterEditor(new BMenuBarEditor);
	RegisterEditor(new BCheckBoxEditor);
	RegisterEditor(new BRadioButtonEditor);
	RegisterEditor(new BColorControlEditor);
	RegisterEditor(new BTextViewEditor);
	RegisterEditor(new BTextControlEditor);
}
//------------------------------------------------------------------------------
void EditingPanel::RegisterEditor(BViewEditor* editor)
{
	if (editor)
	{
		fEditorMap[editor->Supports()] = editor;
	}
}
//------------------------------------------------------------------------------
void EditingPanel::CleanupEditors()
{
	for (EditorMap::iterator i = fEditorMap.begin(); i != fEditorMap.end(); ++i)
	{
		delete i->second;
		i->second = NULL;
	}
}
//------------------------------------------------------------------------------

/*
 * $Log $
 *
 * $Id  $
 *
 */

