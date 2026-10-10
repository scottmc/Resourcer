//------------------------------------------------------------------------------
//	ALGN.cpp
//
//	Editor for BAlignment resources (type 'ALGN'): a horizontal and a vertical
//	alignment, each stored as a 32 bit number.
//------------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <image.h>
#include <interface/Window.h>
#include <interface/MenuField.h>
#include <interface/MenuItem.h>
#include <interface/PopUpMenu.h>

extern "C" _EXPORT void loaddata(unsigned char*, size_t, BView*);
extern "C" _EXPORT unsigned char* savedata(size_t*, BView*);
extern "C" _EXPORT void messaging(BMessage*, BView*);
extern "C" _EXPORT const char description[] = "BAlignment";

struct alignment_name {
	const char*	label;
	int32		value;
};

//	Values of enum alignment and enum vertical_alignment from InterfaceDefs.h
static const alignment_name kHorizontal[] = {
	{ "Left", 0 },
	{ "Right", 1 },
	{ "Center", 2 },
	{ "Unset", -1 },
	{ "Use full width", -2 }
};
static const alignment_name kVertical[] = {
	{ "Top", 0x10 },
	{ "Middle", 0x20 },
	{ "Bottom", 0x30 },
	{ "Unset", -1 },
	{ "Use full height", -2 }
};
static const int kNameCount = 5;

//	A value that is not in the list is kept as an extra item, so it is not lost
static BMenuField*
make_field(BRect frame, const char* name, const char* label,
	const alignment_name* names, int32 value)
{
	BPopUpMenu* menu = new BPopUpMenu(label);
	bool found = false;
	for (int i = 0; i < kNameCount; i++) {
		BMenuItem* item = new BMenuItem(names[i].label, new BMessage('algn'));
		if (names[i].value == value) {
			item->SetMarked(true);
			found = true;
		}
		menu->AddItem(item);
	}
	if (!found) {
		char text[40];
		sprintf(text, "Other (%ld)", (long)value);
		BMenuItem* item = new BMenuItem(text, new BMessage('algn'));
		item->SetMarked(true);
		menu->AddItem(item);
	}
	BMenuField* field = new BMenuField(frame, name, label, menu);
	field->SetDivider(be_plain_font->StringWidth("Horizontal: "));
	return field;
}

static int32
selected_value(BView* view, const alignment_name* names)
{
	BMenuField* field = (BMenuField*)view;
	BMenuItem* marked = field->Menu()->FindMarked();
	if (marked == NULL)
		return -1;
	for (int i = 0; i < kNameCount; i++) {
		if (strcmp(marked->Label(), names[i].label) == 0)
			return names[i].value;
	}
	//"Other (n)"
	const char* open = strchr(marked->Label(), '(');
	if (open != NULL)
		return (int32)strtol(open + 1, NULL, 10);
	return -1;
}

//	Runs when the add-on starts. The window is initialized, you are expected
//	to write to it: the view is 300 x 300
void
loaddata(unsigned char* data, size_t length, BView* bkgview)
{
	int32 values[2] = { -1, -1 };		//both unset
	if (length == sizeof (values) && data != NULL)
		memcpy(values, data, sizeof (values));

	bkgview->AddChild(make_field(BRect(10, 5, 280, 25), "BAlignment::horizontal",
		"Horizontal: ", kHorizontal, values[0]));
	bkgview->AddChild(make_field(BRect(10, 35, 280, 55), "BAlignment::vertical",
		"Vertical: ", kVertical, values[1]));
	bkgview->Window()->ResizeTo(300, 65);
}

//	Return data, clean up, and set length to the size of data
unsigned char*
savedata(size_t* length, BView* bkgview)
{
	int32 values[2];
	values[0] = selected_value(bkgview->ChildAt(0), kHorizontal);
	values[1] = selected_value(bkgview->ChildAt(1), kVertical);
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
