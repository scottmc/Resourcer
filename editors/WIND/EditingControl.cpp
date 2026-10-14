//------------------------------------------------------------------------------
//	EditingControl.cpp
//
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------
#include <interface/Box.h>
#include <interface/ColorControl.h>
#include <interface/MenuBar.h>
#include <interface/MenuField.h>
#include <interface/TextControl.h>
#include <interface/TextView.h>
#include <interface/Window.h>

#include <support/Autolock.h>
#include <support/ClassInfo.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "EditingControl.h"
#include "EditingWindow.h"

// TODO: create a real solution for this
#include "EditingPanel.h"
extern EditingPanel* editpanel;

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

//------------------------------------------------------------------------------
EditingControl::EditingControl(void)
	:	BView(BRect(0, 0, 1, 1), "edithandle", B_FOLLOW_TOP | B_FOLLOW_LEFT,
			  B_WILL_DRAW | B_FRAME_EVENTS | B_FULL_UPDATE_ON_RESIZE |
			  B_PULSE_NEEDED)
{
	SetViewColor(255,255,255);
	//SetLowColor(B_TRANSPARENT_32_BIT);
	controlled = NULL;
	alreadyDrawing = false;
	notInit = true;
	/*Window()->Lock();
	Window()->Unlock();*/
}
//------------------------------------------------------------------------------
void EditingControl::Draw(BRect area) {
	BRect edge = Bounds();
	if (alreadyDrawing == true) {
		FillRect(BRect(edge.left,edge.top,controlled->Bounds().left,edge.bottom),B_SOLID_LOW);
		FillRect(BRect(edge.left,controlled->Bounds().bottom,edge.right,edge.bottom),B_SOLID_LOW);
		FillRect(BRect(controlled->Bounds().right,edge.top,edge.right,edge.bottom),B_SOLID_LOW);
		FillRect(BRect(edge.left,edge.top,edge.right,controlled->Bounds().top),B_SOLID_LOW);
		StrokeRect(Bounds(),B_MIXED_COLORS);
		FillRect(BRect(edge.right-5,edge.bottom-5,edge.right,edge.bottom));
		alreadyDrawing = false;
		return;
	}
	alreadyDrawing = true;
	if (controlled != NULL) {
		controlled->Draw(controlled->Bounds());
		Draw(area);
	} else
		alreadyDrawing = false;
}
//------------------------------------------------------------------------------
void EditingControl::KeyDown(const char *bytes,int32 numBytes)
{
	if (controlled == NULL)
		return;

	if (numBytes != 1)
		return;

	if ((*bytes == B_DELETE) || (*bytes == B_BACKSPACE))
	{
		BAutolock locker(Window());
		controlled->RemoveSelf();
		if (is_kind_of(controlled, BMenuBar))
		{
			// TODO: add message for removing menubar
		}
		RepairMenuFields(controlled);
		delete controlled;
		controlled = NULL;
		notInit = true;
		editpanel->SetTo(NULL);
		ResizeTo(1,1);
		MoveTo(0,0);
		Invalidate(Bounds());
	}
}
//------------------------------------------------------------------------------
// True for the window's own top view.  Parent() is no help here, because it
// is NULL for every view that sits directly in the window too, so look for
// the view among the window's children instead: the top view is the only
// view without a parent that is not one of them.
bool EditingControl::IsTopView(BView *view)
{
	if (view->Parent() != NULL)
		return false;
	BWindow *window = view->Window();
	if (window == NULL)
		return false;
	for (int32 i = 0; i < window->CountChildren(); i++) {
		if (window->ChildAt(i) == view)
			return false;
	}
	return true;
}
//------------------------------------------------------------------------------
// Takes the controlled view back out from beside the handle and puts it into
// the view it lies in (a box, or the window).  The handle has to be out of the
// way first, or it is the view found at that place.
void EditingControl::PutControlledBack(void)
{
	// controlled may have been left without a window by an earlier
	// reparenting; put it back so the conversions below are valid.
	if (controlled != NULL && controlled->Window() == NULL) {
		// It may still hang off a parent that is itself detached, in which
		// case AddChild would refuse it, so take it off that parent first.
		controlled->RemoveSelf();
		Window()->AddChild(controlled);
		if (controlled->Window() == NULL)
			controlled = NULL;
	}
	if (controlled != NULL) {
		BRect empty = controlled->ConvertToScreen(controlled->Bounds());
		empty = Window()->ConvertFromScreen(empty);
		BPoint empt = empty.LeftTop();
		empt.x -= 1;empt.y -= 1;
		BView *posparent = Window()->FindView(empt);
		if (posparent != NULL) {
			// Menus and pop-up menu fields cannot hold child views; adding
			// to them silently fails and leaves the view without a window.
			bool ismenu = is_kind_of(posparent,BMenu) || is_kind_of(posparent,BMenuField) ||
				is_kind_of(posparent->Parent(),BMenuField);
			if (!ismenu && posparent->ConvertToScreen(posparent->Bounds()).Contains(controlled->ConvertToScreen(controlled->Bounds()))) {
				BRect wait = controlled->ConvertToScreen(controlled->Bounds());
				wait = posparent->ConvertFromScreen(wait);
				controlled->RemoveSelf();
				controlled->MoveTo(wait.LeftTop());
				posparent->AddChild(controlled);
				if (controlled->Window() == NULL) {
					controlled->RemoveSelf();
					Window()->AddChild(controlled);
				}
			}
		}
	}
}
//------------------------------------------------------------------------------
// Lets go of the selection, leaving the controlled view where it is.
void EditingControl::Deselect(void)
{
	if (controlled == NULL)
		return;

	MoveTo(0,0);
	ResizeTo(1,1);
	PutControlledBack();
	if (controlled != NULL)
		controlled->SetViewColor(controlled->ViewColor());
	controlled = NULL;
	notInit = true;
	editpanel->SetTo(NULL);
	Invalidate(Bounds());
}
//------------------------------------------------------------------------------
void EditingControl::Pulse(void)
{
	if (Window() != NULL) {
	if ((Window()->CurrentFocus() != controlled) && (Window()->CurrentFocus() != this)) {
		BView *cur = Window()->CurrentFocus();
		if (cur == NULL) {
			return;
		}
		if (IsTopView(cur)) {
			// A click on the bare window gives the focus to the window's own top
			// view.  That is nothing to edit (and taking it out of the window,
			// as is done with the views below, detaches every view in it), so
			// just let go of the selection.
			Deselect();
			return;
		}
		if (is_kind_of(cur,BTextView)) {
			if (is_kind_of(cur->Parent(),BTextControl))
				cur = cur->Parent();
		}
		if (is_kind_of(cur,BMenu)) {
			if (is_kind_of(cur->Parent(),BMenuField))
				cur = cur->Parent();
		}
		if (is_kind_of(cur,BTextControl)) {
			if (is_kind_of(cur->Parent(),BColorControl))
				cur = cur->Parent();
		}
		if (cur == controlled)
			return;
		//if (Parent() != NULL) {
			//MakeFocus();
			BRect tempy = cur->ConvertToScreen(cur->Bounds());
			tempy = Window()->ConvertFromScreen(tempy);
			cur->RemoveSelf();
			cur->MoveTo(tempy.LeftTop());
			Window()->AddChild(cur);
			//cur->MakeFocus();
		//}
		if (controlled != NULL) {
			if (!(is_kind_of(controlled,BBox))) {
				if (controlled->Parent() == NULL)
					controlled->SetViewColor(255,255,255);
			}
		}
		BRect edge = cur->Frame();
		edge.left -= 5;
		edge.top -= 5;
		MoveTo(edge.left,edge.top);
		edge = cur->Bounds();
		edge.bottom += 10;
		edge.right += 10;
		ResizeTo(edge.right,edge.bottom);
		PutControlledBack();
		if (controlled != NULL)
			//controlled->Invalidate(controlled->Bounds());
			controlled->SetViewColor(controlled->ViewColor());
		controlled = cur;
		if (controlled != NULL)
			notInit = false;
		SetResizingMode(controlled->ResizingMode());
		Invalidate(Bounds());
		BMessage active;
		active.AddPointer("view", (const void*)cur);

		if (cur)
		{
			BMessage ctrl;
			cur->Archive(&ctrl);
			ctrl.AddMessenger("Resourcer:messenger", cur);
			active.AddMessage("ctrl", &ctrl);
		}
		editpanel->SetTo(&active);
	}
	}
}
//------------------------------------------------------------------------------
void EditingControl::MouseDown(BPoint point) {
	if (controlled != NULL) {
		BPoint tempit = ConvertToScreen(point);
		tempit = controlled->ConvertFromScreen(tempit);
		if (controlled->Bounds().Contains(tempit)) {
			controlled->MouseDown(tempit);
			return;
		}
	}
	if (is_kind_of(controlled,BMenuBar))
		return;
	uint32 buttons;
	BPoint temp;
	uint32 tmpbtn;
	Window()->Lock();
	GetMouse(&point,&buttons,true);
	int32 xdiff = -32767;
	int32 ydiff = -32767;
	signed char resizing = -2;
	for(;true;snooze(20000)) {
		GetMouse(&temp,&tmpbtn,true);
		if (tmpbtn != B_PRIMARY_MOUSE_BUTTON)
			break;
		if (temp == point)
			continue;
		if (resizing == -2) {
			if ((point.x > Bounds().right - 5) && (point.y > Bounds().bottom - 5))
				resizing = true;
			else
				resizing = false;
		} else {
			if (resizing) {
				ResizeTo(point.x + 5,point.y + 5);
				controlled->ResizeTo(Bounds().right - 15,Bounds().bottom - 15);
			} else {
				if (xdiff == -32767) {
					xdiff = int32(temp.x - Bounds().left);
					ydiff = int32(temp.y - Bounds().top);
				} else {
					MoveBy(temp.x - xdiff,temp.y - ydiff);
					controlled->MoveTo(Frame().left + 5,Frame().top + 5);
				}
			}
		}
		point = temp;
	}
	Window()->Unlock();
	Invalidate(Bounds());
}
//------------------------------------------------------------------------------

/*
 * $Log $
 *
 * $Id  $
 *
 */

