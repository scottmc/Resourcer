//------------------------------------------------------------------------------
//	SIZE.cpp
//
//	Editor for BSize resources (type 'SIZE'): a size with float width and height.
//------------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <image.h>
#include <interface/Window.h>
#include <interface/TextControl.h>

extern "C" _EXPORT void loaddata(unsigned char*, size_t, BView*);
extern "C" _EXPORT unsigned char* savedata(size_t*, BView*);
extern "C" _EXPORT void messaging(BMessage*, BView*);
extern "C" _EXPORT const char description[] = "BSize";

//	Writes the shortest text that reads back as exactly the same float, so that
//	saving without editing does not change the data.
static void
format_value(float value, char* buffer, size_t size)
{
	for (int digits = 6; digits <= 9; digits++) {
		snprintf(buffer, size, "%.*g", digits, (double)value);
		if ((float)strtod(buffer, NULL) == value)
			return;
	}
}

static const int kFieldCount = 2;

//	Runs when the add-on starts. The window is initialized, you are expected
//	to write to it: the view is 300 x 300
void
loaddata(unsigned char* data, size_t length, BView* bkgview)
{
	float values[kFieldCount];
	for (int i = 0; i < kFieldCount; i++)
		values[i] = 0;
	if (length == sizeof (values) && data != NULL)
		memcpy(values, data, sizeof (values));

	static const char* const labels[kFieldCount] = { "Width: ", "Height: " };
	static const char* const names[kFieldCount] = { "BSize::width", "BSize::height" };
	BRect frame(10, 5, 280, 25);
	for (int i = 0; i < kFieldCount; i++) {
		char text[40];
		format_value(values[i], text, sizeof (text));
		BTextControl* v = new BTextControl(frame, names[i], labels[i], text, NULL);
		v->SetDivider(be_plain_font->StringWidth("Height: "));
		bkgview->AddChild(v);
		frame.OffsetBy(0, 25);
	}
	bkgview->Window()->ResizeTo(300, frame.top + 5);
}

//	Return data, clean up, and set length to the size of data
unsigned char*
savedata(size_t* length, BView* bkgview)
{
	float values[kFieldCount];
	for (int i = 0; i < kFieldCount; i++) {
		BTextControl* text = (BTextControl*)(bkgview->ChildAt(i));
		values[i] = (float)strtod(text->Text(), NULL);
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
