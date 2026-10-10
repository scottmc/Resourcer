//------------------------------------------------------------------------------
//	RECT.cpp
//
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------
#include <stdlib.h>

// System Includes -------------------------------------------------------------
#include <support/String.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

//------------Includes----------------------
#include <image.h>
#include <interface/Window.h>
#include <stdio.h>
#include <interface/TextControl.h>
#include <interface/GridLayout.h>
#include <interface/SpaceLayoutItem.h>
//-------------------------------------------

extern "C" _EXPORT void loaddata(unsigned char*, size_t, BView*);
extern "C" _EXPORT unsigned char* savedata(size_t*, BView*);
extern "C" _EXPORT void messaging(BMessage*, BView*);
extern "C" _EXPORT const char description[] = "BRect";

static const char* const kNames[4] = { "BRect::left", "BRect::top",
	"BRect::right", "BRect::bottom" };


//------------------------------------------------------------------------------
//	Runs when add-on starts, window is initialized, you are expected
//	to write to it: rect is 300 x 300
void loaddata(unsigned char* data, size_t length, BView* bkgview)
{
	BRect rect;
	if (length == sizeof (rect.left) * 4)
	{
		float* rdata = (float*)data;
		rect.left	= rdata[0];
		rect.top	= rdata[1];
		rect.right	= rdata[2];
		rect.bottom	= rdata[3];
	}

	static const char* const labels[4] = { "Left:", "Top:", "Right:", "Bottom:" };
	float values[4] = { rect.left, rect.top, rect.right, rect.bottom };

	//	The layout manager sizes the labels and fields from the current font.
	BGridLayout* grid = new BGridLayout(8, 4);
	bkgview->SetLayout(grid);
	grid->SetInsets(10, 10, 10, 10);
	for (int i = 0; i < 4; i++) {
		BString sdata;
		sdata << values[i];
		BTextControl* v = new BTextControl(kNames[i], labels[i], sdata.String(), NULL);
		grid->AddItem(v->CreateLabelLayoutItem(), 0, i);
		grid->AddItem(v->CreateTextViewLayoutItem(), 1, i);
	}
	grid->AddItem(BSpaceLayoutItem::CreateGlue(), 0, 4, 2, 1);

	//	The host window is not layout managed, so size it to the content.
	BSize size = bkgview->PreferredSize();
	bkgview->Window()->ResizeTo(size.width > 220 ? size.width : 220, size.height);
}
//------------------------------------------------------------------------------
//	Return data, clean up, and set length to the size of data
unsigned char* savedata(size_t* length, BView* bkgview)
{
	BRect rect;
	float* fields[4] = { &rect.left, &rect.top, &rect.right, &rect.bottom };
	for (int i = 0; i < 4; i++) {
		BTextControl* text = (BTextControl*)(bkgview->FindView(kNames[i]));
		if (text != NULL)
			*fields[i] = atof(text->Text());
	}

	*length = sizeof (rect.left) * 4;
	float* data = new float[4];
	data[0] = rect.left;
	data[1] = rect.top;
	data[2] = rect.right;
	data[3] = rect.bottom;

	return (unsigned char*)data;
}
//------------------------------------------------------------------------------
//	Receives messages from window with negative 'what' members or
//	B_REFS_RECEIVED messages
void messaging(BMessage* message, BView* bkgview)
{
}

/*
 * $Log $
 *
 * $Id  $
 *
 */

