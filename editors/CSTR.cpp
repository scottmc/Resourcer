//------------Includes----------------------
#include <interface/GroupLayout.h>
#include <interface/ScrollView.h>
#include <interface/TextView.h>
#include <interface/Window.h>
#include <interface/Size.h>
#include <image.h>
#include <interface/Window.h>
#include <stdio.h>
#include <interface/TextView.h>
#include <interface/ScrollView.h>
//-------------------------------------------

extern "C" _EXPORT void loaddata(unsigned char *,size_t,BView *);
extern "C" _EXPORT unsigned char* savedata(size_t *,BView *);
extern "C" _EXPORT void messaging(BMessage *,BView *);
//	Tells Resourcer that this editor wants a resizable window
extern "C" _EXPORT const bool resizable = true;
extern "C" _EXPORT const char description[] = "Text String";


void loaddata(unsigned char *data,size_t length,BView *bkgview) {
	//	The text view and its scroll bar are layout managed, so they follow the
	//	window when it is resized.
	BTextView *view = new BTextView("String",B_WILL_DRAW | B_FRAME_EVENTS);
	if (length == 0 || data == NULL)
		view->SetText("");
	else
		view->SetText((char*)(data));
	BScrollView *scroll = new BScrollView("StringScroll",view,B_WILL_DRAW | B_FRAME_EVENTS,false,true,B_FANCY_BORDER);
	BGroupLayout *group = new BGroupLayout(B_VERTICAL);
	bkgview->SetLayout(group);
	group->SetInsets(10,10,10,10);
	group->AddView(scroll);

	//	A comfortable starting size, based on the current font
	font_height height;
	be_plain_font->GetHeight(&height);
	float line = height.ascent + height.descent + height.leading;
	scroll->SetExplicitPreferredSize(BSize(be_plain_font->StringWidth("M") * 40,line * 12));
	//	Ask for the minimum size first: the preferred size can come back as NaN
	//	until the layout is validated, so the tests replace a NaN.
	BSize min = bkgview->MinSize();
	BSize size = bkgview->PreferredSize();
	if (!(size.width >= min.width))
		size.width = min.width;
	if (!(size.height >= min.height))
		size.height = min.height;
	bkgview->Window()->SetSizeLimits(min.width,B_SIZE_UNLIMITED,min.height,B_SIZE_UNLIMITED);
	bkgview->Window()->ResizeTo(size.width,size.height);
}

unsigned char* savedata(size_t *length,BView *bkgview) {
	BTextView *view = (BTextView *)(bkgview->FindView("String"));
	char *hjk = new char[strlen(view->Text()) + 1];
	strcpy(hjk,view->Text());
	*length = strlen(hjk) + 1;
	return (unsigned char*)(hjk);
}

void messaging(BMessage *message,BView *bkgview) {
}