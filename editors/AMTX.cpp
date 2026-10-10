//------------------------------------------------------------------------------
//	AMTX.cpp
//
//	Editor for BAffineTransform resources (type 'AMTX'): six doubles: sx, shy, shx, sy, tx, ty.
//------------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <image.h>
#include <interface/Window.h>
#include <interface/TextControl.h>
#include <interface/GridLayout.h>
#include <interface/SpaceLayoutItem.h>

extern "C" _EXPORT void loaddata(unsigned char*, size_t, BView*);
extern "C" _EXPORT unsigned char* savedata(size_t*, BView*);
extern "C" _EXPORT void messaging(BMessage*, BView*);
extern "C" _EXPORT const char description[] = "BAffineTransform";

//	Writes the shortest text that reads back as exactly the same double, so that
//	saving without editing does not change the data.
static void
format_value(double value, char* buffer, size_t size)
{
	for (int digits = 15; digits <= 17; digits++) {
		snprintf(buffer, size, "%.*g", digits, (double)value);
		if ((double)strtod(buffer, NULL) == value)
			return;
	}
}

static const int kFieldCount = 6;
static const char* const kNames[kFieldCount] = { "BAffineTransform::sx",
	"BAffineTransform::shy", "BAffineTransform::shx", "BAffineTransform::sy",
	"BAffineTransform::tx", "BAffineTransform::ty" };

//	Runs when the add-on starts. The window is initialized, you are expected
//	to write to it: the view is 300 x 300
void
loaddata(unsigned char* data, size_t length, BView* bkgview)
{
	double values[kFieldCount];
	for (int i = 0; i < kFieldCount; i++)
		values[i] = 0;
	if (length == sizeof (values) && data != NULL)
		memcpy(values, data, sizeof (values));

	static const char* const labels[kFieldCount] = { "sx:", "shy:", "shx:", "sy:", "tx:", "ty:" };

	//	The layout manager sizes the labels and fields from the current font.
	BGridLayout* grid = new BGridLayout(8, 4);
	bkgview->SetLayout(grid);
	grid->SetInsets(10, 10, 10, 10);
	for (int i = 0; i < kFieldCount; i++) {
		char text[40];
		format_value(values[i], text, sizeof (text));
		BTextControl* v = new BTextControl(kNames[i], labels[i], text, NULL);
		grid->AddItem(v->CreateLabelLayoutItem(), 0, i);
		grid->AddItem(v->CreateTextViewLayoutItem(), 1, i);
	}
	grid->AddItem(BSpaceLayoutItem::CreateGlue(), 0, kFieldCount, 2, 1);

	//	The host window is not layout managed, so size it to the content.
	BSize size = bkgview->PreferredSize();
	bkgview->Window()->ResizeTo(size.width > 220 ? size.width : 220, size.height);
}

//	Return data, clean up, and set length to the size of data
unsigned char*
savedata(size_t* length, BView* bkgview)
{
	double values[kFieldCount];
	for (int i = 0; i < kFieldCount; i++) {
		BTextControl* text = (BTextControl*)(bkgview->FindView(kNames[i]));
		if (text == NULL)
			continue;
		values[i] = (double)strtod(text->Text(), NULL);
	}
	*length = sizeof (values);
	unsigned char* data = new unsigned char[sizeof (values)];
	memcpy(data, values, sizeof (values));
	return data;
}

//	Receives messages from window with negative 'what' members or
//	B_REFS_RECEIVED messages
void
messaging(BMessage* message, BView* bkgview)
{
}
