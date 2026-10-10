//------------Includes----------------------
#include <image.h>
#include <interface/Window.h>
#include <stdio.h>
#include <interface/ColorControl.h>
#include <interface/Box.h>
#include <interface/GroupLayout.h>
#include <interface/Size.h>
#include <app/MessageFilter.h>
#include <support/ClassInfo.h>
//-------------------------------------------

extern "C" _EXPORT void loaddata(unsigned char *,size_t,BView *);
extern "C" _EXPORT unsigned char* savedata(size_t *,BView *);
extern "C" _EXPORT void messaging(BMessage *,BView *);
extern "C" _EXPORT const char description[] = "RGB Color";

filter_result dummy_patch(BMessage *message,BHandler **target,BMessageFilter *messageFilter) {
	messaging(message,((BWindow *)(*target))->ChildAt(0));
	return B_DISPATCH_MESSAGE;
}

void loaddata(unsigned char *data,size_t length,BView *bkgview) {//runs when add-on starts, window is initialized, you are expected to write to it: rect is 300 x 300
	//	The colour control and the swatch are layout managed, so the swatch's
	//	label is not clipped whatever font is in use.
	BGroupLayout *group = new BGroupLayout(B_VERTICAL,10);
	bkgview->SetLayout(group);
	group->SetInsets(10,10,10,10);

	BColorControl *colorPicker = new BColorControl(BPoint(0,0),B_CELLS_32x8,8,"colorPicker",new BMessage('COLR'));
	BMessageFilter *filter = new BMessageFilter(B_ANY_DELIVERY,B_ANY_SOURCE,&dummy_patch);
	colorPicker->SetTarget(bkgview->Window());
	bkgview->Window()->AddFilter(filter);
	if (length == 0) {
		rgb_color white;
		white.red = 0xff; white.blue = 0xff; white.green = 0xff; white.alpha = 0xff;
		colorPicker->SetValue(white);
	} else {
		colorPicker->SetValue(*((rgb_color *)(data)));
	}
	group->AddView(colorPicker);

	BBox *color2 = new BBox("color");
	color2->SetLabel("Color");
	BGroupLayout *boxlayout = new BGroupLayout(B_VERTICAL);
	color2->SetLayout(boxlayout);
	//	Leave room for the box's label above the swatch
	font_height labelheight;
	be_bold_font->GetHeight(&labelheight);
	boxlayout->SetInsets(5,labelheight.ascent + labelheight.descent + labelheight.leading + 5,5,5);
	BView *color3 = new BView("color2",B_WILL_DRAW);
	color3->SetViewColor(colorPicker->ValueAsColor());
	font_height height;
	be_plain_font->GetHeight(&height);
	color3->SetExplicitMinSize(BSize(B_SIZE_UNSET,(height.ascent + height.descent + height.leading) * 1.5));
	boxlayout->AddView(color3);
	group->AddView(color2);

	//	The host window is not layout managed, so size it to the content.
	//	Ask for the minimum size first: the preferred size can come back as NaN
	//	until the layout is validated, so the tests are written to replace a NaN.
	BSize min = bkgview->MinSize();
	BSize size = bkgview->PreferredSize();
	if (!(size.width >= min.width))
		size.width = min.width;
	if (!(size.height >= min.height))
		size.height = min.height;
	bkgview->Window()->ResizeTo(size.width,size.height);
}

unsigned char* savedata(size_t *length,BView *bkgview) { //return data, clean up, and set length to the size of data
	BColorControl *color = (BColorControl *)(bkgview->FindView("colorPicker"));
	rgb_color *toreturn = new rgb_color;
	*toreturn = color->ValueAsColor();
	*length = sizeof(rgb_color);
	return (unsigned char*)(toreturn);
}

void messaging(BMessage *message,BView *bkgview) { //receives messages from window with negative 'what' members or B_REFS_RECEIVED messages
//	if (message->what == 'COLR') {
		BColorControl *color = (BColorControl *)(bkgview->FindView("colorPicker"));
		rgb_color new_color = color->ValueAsColor();
		BView *view = bkgview->FindView("color")->FindView("color2");
		if ((new_color.green == view->ViewColor().green) && (new_color.blue == view->ViewColor().blue) && (new_color.red == view->ViewColor().red))
			return;
		view->SetViewColor(new_color);
		view->Invalidate(view->Bounds());
		
//	}
}