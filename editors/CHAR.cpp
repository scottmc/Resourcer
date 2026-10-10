//------------Includes----------------------
#include <image.h>
#include <interface/Window.h>
#include <stdio.h>
#include <interface/TextControl.h>
#include <interface/GridLayout.h>
#include <interface/SpaceLayoutItem.h>
//-------------------------------------------

extern "C" _EXPORT void loaddata(unsigned char *,size_t,BView *);
extern "C" _EXPORT unsigned char* savedata(size_t *,BView *);
extern "C" _EXPORT void messaging(BMessage *,BView *);
extern "C" _EXPORT const char description[] = "Single Character";


void loaddata(unsigned char *data,size_t length,BView *bkgview) {//runs when add-on starts, window is initialized, you are expected to write to it: rect is 300 x 300
	char mime[256];
	if (length == 0) {
		mime[0] = 0;
	} else {
		mime[0] = *((char*)(data));
		mime[1] = 0;
	}
	//	The layout manager sizes the label and field from the current font.
	BTextControl *v = new BTextControl("Integer","Character:",mime,NULL);
	v->TextView()->SetMaxBytes(1);
	BGridLayout *grid = new BGridLayout(8,4);
	bkgview->SetLayout(grid);
	grid->SetInsets(10,10,10,10);
	grid->AddItem(v->CreateLabelLayoutItem(),0,0);
	grid->AddItem(v->CreateTextViewLayoutItem(),1,0);
	grid->AddItem(BSpaceLayoutItem::CreateGlue(),0,1,2,1);
	//	The host window is not layout managed, so size it to the content.
	BSize size = bkgview->PreferredSize();
	bkgview->Window()->ResizeTo(size.width > 220 ? size.width : 220,size.height);
}

unsigned char* savedata(size_t *length,BView *bkgview) { //return data, clean up, and set length to the size of data
	BTextControl *text = (BTextControl *)(bkgview->FindView("Integer"));
	char* toreturn = new char;
	*toreturn = *(text->Text());
	*length = 1;
	return (unsigned char*)(toreturn);
}

void messaging(BMessage *message,BView *bkgview) { //receives messages from window with negative 'what' members or B_REFS_RECEIVED messages
}