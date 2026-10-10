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
extern "C" _EXPORT const char description[] = "MIME Type";


void loaddata(unsigned char *data,size_t length,BView *bkgview) {//runs when add-on starts, window is initialized, you are expected to write to it: rect is 300 x 300
	char mime[256];
	if (length == 0) {
		mime[0] = 0;
	} else {
		char *dataa;
		dataa = (char *)(data);
		strcpy(mime,dataa);
	}
	//	The layout manager sizes the label and field from the current font.
	BTextControl *v = new BTextControl("Data","MIME Type:",mime,NULL);
	BGridLayout *grid = new BGridLayout(8,4);
	bkgview->SetLayout(grid);
	grid->SetInsets(10,10,10,10);
	grid->AddItem(v->CreateLabelLayoutItem(),0,0);
	grid->AddItem(v->CreateTextViewLayoutItem(),1,0);
	grid->AddItem(BSpaceLayoutItem::CreateGlue(),0,1,2,1);
	//	The host window is not layout managed, so size it to the content.
	BSize size = bkgview->PreferredSize();
	bkgview->Window()->ResizeTo(size.width > 280 ? size.width : 280,size.height);
}

unsigned char* savedata(size_t *length,BView *bkgview) { //return data, clean up, and set length to the size of data
	BTextControl *text = (BTextControl *)(bkgview->FindView("Data"));
	char *data = new char[strlen(text->Text()) + 1];
	strcpy(data,text->Text());
	*length = strlen(data) + 1;
	return (unsigned char*)(data);
}

void messaging(BMessage *message,BView *bkgview) { //receives messages from window with negative 'what' members or B_REFS_RECEIVED messages
}