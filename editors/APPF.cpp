//------------Includes----------------------
#include <image.h>
#include <app/Roster.h>
#include <interface/RadioButton.h>
#include <interface/CheckBox.h> 
#include <interface/GroupLayout.h>
#include <interface/Size.h>
#include <interface/SpaceLayoutItem.h>
#include <interface/Window.h>
#include <stdio.h>
//------------------------------------------

extern "C" _EXPORT void loaddata(unsigned char *,size_t,BView *);
extern "C" _EXPORT unsigned char* savedata(size_t *,BView *);
extern "C" _EXPORT void messaging(BMessage *,BView *);
extern "C" _EXPORT const char description[] = "Application Flags";


void loaddata(unsigned char *data,size_t length,BView *bkgview) {//runs when add-on starts, window is initialized, you are expected to write to it: rect is 300 x 300
	uint32 value;
	uint32 app_flags = 0;
	if (length == 0) {
		value = 0;
	} else {
		value = (*((uint32 *)(data))) & B_LAUNCH_MASK;
		switch (value) {
			case B_EXCLUSIVE_LAUNCH:
				value = 2;
				break;
			case B_SINGLE_LAUNCH:
				value = 0;
				break;
			case B_MULTIPLE_LAUNCH:
				value = 1;
				break;
			default:
				value = 0;
		}
	}
	if (length != 0)
		app_flags = *((uint32 *)(data));
	//	The launch modes are radio buttons in one column and the flags are check
	//	boxes in another; the layout sizes both to the font.
	BRadioButton *buts[3];
	buts[0] = new BRadioButton("1","Single Launch",new BMessage(-100));
	buts[1] = new BRadioButton("2","Multiple Launch",new BMessage(-200));
	buts[2] = new BRadioButton("3","Exclusive Launch",new BMessage(-200));
	buts[value]->SetValue(true);
	BCheckBox *boxes[2];
	boxes[0] = new BCheckBox("4","Background App",new BMessage(-200));
	boxes[1] = new BCheckBox("5","ARGV Only",new BMessage(-200));
	if (app_flags & B_BACKGROUND_APP)
		boxes[0]->SetValue(true);
	if (app_flags & B_ARGV_ONLY)
		boxes[1]->SetValue(true);

	BGroupLayout *top = new BGroupLayout(B_HORIZONTAL,20);
	bkgview->SetLayout(top);
	top->SetInsets(10,10,10,10);
	BGroupLayout *modes = new BGroupLayout(B_VERTICAL,5);
	BGroupLayout *flags = new BGroupLayout(B_VERTICAL,5);
	top->AddItem(modes);
	top->AddItem(flags);
	for (int i = 0; i < 3; i++)
		modes->AddView(buts[i]);
	flags->AddView(boxes[0]);
	flags->AddView(boxes[1]);
	flags->AddItem(BSpaceLayoutItem::CreateGlue());

	//	Ask for the minimum size first: the preferred size can come back as NaN
	//	until the layout is validated, so the tests replace a NaN.
	BSize min = bkgview->MinSize();
	BSize size = bkgview->PreferredSize();
	if (!(size.width >= min.width))
		size.width = min.width;
	if (!(size.height >= min.height))
		size.height = min.height;
	bkgview->Window()->ResizeTo(size.width,size.height);
}

unsigned char* savedata(size_t *length,BView *bkgview) { //return data, clean up, and set sizeto the size of data
	uint32 one = 0;
	for(int i = 0; i <= 2; i++) {
		char name[2] = {char('1' + i),0};
		BRadioButton *but = (BRadioButton *)(bkgview->FindView(name));
		if (but->Value() == true) {
			one = i;
			break;
		}
	}
	uint32 value = one;
	switch (value) {
		case 2:
			value = B_EXCLUSIVE_LAUNCH;
			break;
		case 0:
			value = B_SINGLE_LAUNCH;
			break;
		case 1:
			value = B_MULTIPLE_LAUNCH;
			break;
		default:
			value = B_SINGLE_LAUNCH;
	}
	one = value;
	BCheckBox *boxes[2];
	boxes[0] = (BCheckBox *)(bkgview->FindView("4"));
	boxes[1] = (BCheckBox *)(bkgview->FindView("5"));
	uint32 a = boxes[0]->Value();
	if (a) {
		a = B_BACKGROUND_APP;
	}
	uint32 b = boxes[1]->Value();
	if (b) {
		b = B_ARGV_ONLY;
	}
	value = one | a | b;
	*length = 4;
	return (unsigned char *)(new uint32(value));
}

void messaging(BMessage *message,BView *bkgview) { //receives messages from window with negative 'what' members or B_REFS_RECEIVED messages
}