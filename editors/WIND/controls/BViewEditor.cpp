//------------------------------------------------------------------------------
//	BViewEditor.cpp
//
// TODO: Add view/high/low color editing
// TODO: Add font editing
// TODO: Add view/overlay bitmap editing
// TODO: Add cursor editing
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------
#include <algorithm>
#include <math.h>
#include <stdio.h>

// System Includes -------------------------------------------------------------
#include <interface/StringView.h>
#include <interface/TextControl.h>
#include <interface/View.h>
#include <interface/Window.h>
#include <support/ClassInfo.h>
#include <support/String.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "BViewEditor.h"
#include "../EditingPanel.h"
#include "../EditingControl.h"
#include "../WIND.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

BViewEditor::ControlKeeper BViewEditor::fControlKeeper;

//------------------------------------------------------------------------------
BViewEditor::BViewEditor()
	:	fClassName(NULL),
		fViewName(NULL),
		fPosition(NULL),
		fBottom(10)
{
}
//------------------------------------------------------------------------------
BViewEditor::~BViewEditor()
{
}
//------------------------------------------------------------------------------
void BViewEditor::SetParent(BView* parent)
{
	fParent = parent;
}
//------------------------------------------------------------------------------
void BViewEditor::SetTarget(BView* target)
{
	fTarget = target;
}
//------------------------------------------------------------------------------
void BViewEditor::SetTo(BMessage* archive)
{
	Init();

	if (archive->FindMessenger("Resourcer:messenger", &fControl))
		debugger("Couldn't find messenger in archive");

	BString ClassName;
	if (archive->FindString("class", &ClassName))
		debugger("Couldn't find class name in archive");

	BString ViewName;
	if (archive->FindString("_name", &ViewName))
		debugger("Couldn't find view name in archive");

	ClassName.Prepend("Element Type: ");
	fClassName->SetText(ClassName.String());

	fViewName->SetText(ViewName.String());

	fViewName->SetTarget(Parent());

	// Where the element is in the window (while it is selected it is a child
	// of the window, so its frame is in window coordinates)
	BRect frame;
	if (archive->FindRect("_frame", &frame) == B_OK)
	{
		char position[64];
		sprintf(position, "%g, %g", frame.left, frame.top);
		fPosition->SetText(position);
	}
	else
	{
		fPosition->SetText("");
	}
	fPosition->SetTarget(Parent());

	Parent()->AddChild(fClassName);
	Parent()->AddChild(fViewName);
	Parent()->AddChild(fPosition);
}
//------------------------------------------------------------------------------
bool BViewEditor::MessageReceived(BMessage* msg)
{
	switch(msg->what)
	{
		case MSG_VIEW_SET_NAME:
		{
			Target()->SetName(fViewName->Text());
			return true;
		}

		case MSG_VIEW_SET_POSITION:
		{
			float x, y;
			// "20, 30" or "20 30"
			if (sscanf(fPosition->Text(), " %f , %f", &x, &y) == 2 ||
				sscanf(fPosition->Text(), " %f %f", &x, &y) == 2)
			{
				Target()->MoveTo(x, y);
				// the white frame around the selected element goes with it
				EditingControl* handle = NULL;
				if (Window() != NULL)
					handle = dynamic_cast<EditingControl*>(Window()->FindView("edithandle"));
				if (handle != NULL && handle->controlled == Target())
				{
					handle->MoveTo(x - 5, y - 5);
					handle->Invalidate();
				}
			}
			else
			{
				// not two numbers: put the real position back
				BPoint now = Target()->Frame().LeftTop();
				char position[64];
				sprintf(position, "%g, %g", now.x, now.y);
				fPosition->SetText(position);
			}
			return true;
		}
	}

	return false;
}
//------------------------------------------------------------------------------
const char* BViewEditor::Supports()
{
	return "BView";
}
//------------------------------------------------------------------------------
bool BViewEditor::UsesLayout() const
{
	return false;
}
//------------------------------------------------------------------------------
float BViewEditor::DividerFor(const char* label, float padding)
{
	return ceilf(be_plain_font->StringWidth(label)) + padding;
}
//------------------------------------------------------------------------------
float BViewEditor::LabelDivider()
{
	float widest = be_plain_font->StringWidth("View Name: ");
	float other = be_plain_font->StringWidth("Message: ");
	if (other > widest)
		widest = other;
	other = be_plain_font->StringWidth("Label: ");
	if (other > widest)
		widest = other;
	// a label is cut short when it fills the whole divider
	return ceilf(widest) + 10;
}
//------------------------------------------------------------------------------
BWindow* BViewEditor::Window()
{
	if (Target())
	{
		return Target()->Window();
	}

	return NULL;
}
//------------------------------------------------------------------------------
BMessenger& BViewEditor::Messenger()
{
	return fControl;
}
//------------------------------------------------------------------------------
int BViewEditor::Bottom()
{
	return fBottom;
}
//------------------------------------------------------------------------------
void BViewEditor::AddControl(BView* control)
{
	fBottom += control->Frame().IntegerHeight();
	fControlKeeper.AddControl(control);
}
//------------------------------------------------------------------------------
BView* BViewEditor::Parent()
{
	return fParent;
}
//------------------------------------------------------------------------------
BView* BViewEditor::Target()
{
	return fTarget;
}
//------------------------------------------------------------------------------
void BViewEditor::Init()
{
	fBottom = 10;
	int bottom;
	if (!fClassName)
	{
		bottom = Bottom();
		fClassName = new BStringView(BRect(10, bottom, 190, bottom + 20),
									 "classname", "Element Type: ");
		AddControl(fClassName);
	}

	if (!fViewName)
	{
		bottom = Bottom();
		fViewName = new BTextControl(BRect(10, bottom, 190, bottom + 20),
									 "name", "View Name: ", "",
									 new BMessage(MSG_VIEW_SET_NAME));
		fViewName->SetDivider(LabelDivider());
		AddControl(fViewName);
	}

	if (!fPosition)
	{
		bottom = Bottom();
		fPosition = new BTextControl(BRect(10, bottom, 190, bottom + 20),
									 "position", "Position: ", "",
									 new BMessage(MSG_VIEW_SET_POSITION));
		fPosition->SetDivider(LabelDivider());
		AddControl(fPosition);
	}
}
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
//	#pragma mark -
//------------------------------------------------------------------------------
BViewEditor::ControlKeeper::ControlKeeper()
{
	;
}
//------------------------------------------------------------------------------
BViewEditor::ControlKeeper::~ControlKeeper()
{
	for (uint32 i = 0; i < fControls.size(); ++i)
	{
		if (fControls[i]->Parent() == NULL)
		{
			delete fControls[i];
			fControls[i] = NULL;
		}
	}
}
//------------------------------------------------------------------------------
void BViewEditor::ControlKeeper::AddControl(BView* control)
{
	std::vector<BView*>::iterator i = std::find(fControls.begin(),
												fControls.end(),
												control);
	if (i != fControls.end())
	{
		// control hasn't been previously added
		fControls.push_back(control);
	}
}
//------------------------------------------------------------------------------

/*
 * $Log $
 *
 * $Id  $
 *
 */

