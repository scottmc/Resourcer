//------------Includes----------------------
#include <image.h>
#include <interface/Window.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <interface/TextControl.h>
#include <interface/MenuField.h>
#include <interface/MenuItem.h>
#include <interface/PopUpMenu.h>
#include <interface/Screen.h>
#include <interface/GridLayout.h>
#include <interface/LayoutItem.h>
#include <interface/GroupLayout.h>
#include <interface/SpaceLayoutItem.h>
#include <interface/StringView.h>
#include <storage/AppFileInfo.h>
//------------------------------------------

extern "C" _EXPORT void loaddata(unsigned char *,size_t,BView *);
extern "C" _EXPORT unsigned char* savedata(size_t *,BView *);
extern "C" _EXPORT void messaging(BMessage *,BView *);
extern "C" _EXPORT const char description[] = "Application Version";

version_info *info2;

void loaddata(unsigned char *data,size_t length,BView *bkgview) {
	version_info *info = (version_info *)(data);
	if (length == 680)
		info2 = new version_info(*((version_info *)(data + 340)));
	else
		info2 = new version_info;
	if (length == 0) {
		info = new version_info;
		info->major = 0;
		info->minor = 0;
		info->middle = 0;
		info->variety = 0;
		info->internal = 0;
		info->short_info[0] = 0;
		info->long_info[0] = 0;
	}
	char buffer[80];
	static const char *const states[6] = { "Development", "Alpha", "Beta",
		"Gamma", "Golden master", "Final" };

	//	The layout manager sizes labels and fields from the current font. The
	//	version numbers, release menu and internal number share one row.
	BGridLayout *grid = new BGridLayout(8,4);
	bkgview->SetLayout(grid);
	grid->SetInsets(10,10,10,10);
	BGroupLayout *row = new BGroupLayout(B_HORIZONTAL,4);
	grid->AddItem(row,1,0);

	float number_width = be_plain_font->StringWidth("00000") + 16;
	float field_width = be_plain_font->StringWidth("M") * 40;
	const char *const names[3] = { "vers","middle","minor" };
	const uint32 numbers[3] = { info->major,info->middle,info->minor };
	for (int i = 0; i < 3; i++) {
		sprintf(buffer,"%lu",(unsigned long)numbers[i]);
		BTextControl *version = new BTextControl(names[i],i == 0 ? "Version:" : NULL,buffer,NULL);
		if (i == 0)
			grid->AddItem(version->CreateLabelLayoutItem(),0,0);
		BLayoutItem *item = version->CreateTextViewLayoutItem();
		item->SetExplicitMaxSize(BSize(number_width,B_SIZE_UNSET));
		row->AddItem(item);
		if (i < 2)
			row->AddView(new BStringView("dot","."));
	}

	BPopUpMenu *menu = new BPopUpMenu("states");
	for (int i = 0; i < 6; i++)
		menu->AddItem(new BMenuItem(states[i],new BMessage(-200)));
	if (info->variety < 6)
		menu->ItemAt(info->variety)->SetMarked(true);
	BMenuField *variety = new BMenuField("Variety","Release:",menu);
	row->AddItem(variety->CreateLabelLayoutItem());
	//	Size the menu for the longest release name so that changing the selection
	//	does not resize it and clip the fields after it.
	float menu_width = 0;
	for (int i = 0; i < 6; i++) {
		float width = be_plain_font->StringWidth(states[i]);
		if (width > menu_width)
			menu_width = width;
	}
	menu_width += 40;		//item padding and the pop-up arrow
	BLayoutItem *menu_item = variety->CreateMenuBarLayoutItem();
	menu_item->SetExplicitMinSize(BSize(menu_width,B_SIZE_UNSET));
	menu_item->SetExplicitPreferredSize(BSize(menu_width,B_SIZE_UNSET));
	row->AddItem(menu_item);

	sprintf(buffer,"%lu",(unsigned long)info->internal);
	BTextControl *internal = new BTextControl("internal",NULL,buffer,NULL);
	BLayoutItem *internal_item = internal->CreateTextViewLayoutItem();
	internal_item->SetExplicitMaxSize(BSize(number_width,B_SIZE_UNSET));
	row->AddItem(internal_item);

	BTextControl *short_info = new BTextControl("short","Short Info:",info->short_info,NULL);
	grid->AddItem(short_info->CreateLabelLayoutItem(),0,1);
	BLayoutItem *short_item = short_info->CreateTextViewLayoutItem();
	short_item->SetExplicitPreferredSize(BSize(field_width,B_SIZE_UNSET));
	grid->AddItem(short_item,1,1);
	BTextControl *long_info = new BTextControl("long","Long Info:",info->long_info,NULL);
	grid->AddItem(long_info->CreateLabelLayoutItem(),0,2);
	BLayoutItem *long_item = long_info->CreateTextViewLayoutItem();
	long_item->SetExplicitPreferredSize(BSize(field_width,B_SIZE_UNSET));
	grid->AddItem(long_item,1,2);
	grid->AddItem(BSpaceLayoutItem::CreateGlue(),0,3,2,1);

	//	The host window is not layout managed, so size it to the content.
	//	Use the larger of the minimum and preferred sizes so that nothing in the
	//	version row is squeezed and the right inset is kept.
	BSize size = bkgview->PreferredSize();
	BSize min = bkgview->MinSize();
	if (size.width < min.width)
		size.width = min.width;
	if (size.height < min.height)
		size.height = min.height;
	//	Never let the window grow past the screen, whatever the layout asks for
	//	(an empty resource used to make it far too wide).
	BRect screen = BScreen(bkgview->Window()).Frame();
	float max_width = screen.Width() - 40;
	float max_height = screen.Height() - 80;
	if (size.width > max_width)
		size.width = max_width;
	if (size.height > max_height)
		size.height = max_height;
	bkgview->Window()->ResizeTo(size.width > 300 ? size.width : 300,size.height);
}

//	Text of the named BTextControl, or an empty string if it is missing
static const char *text_of(BView *parent,const char *name) {
	BTextControl *text = (BTextControl *)(parent->FindView(name));
	return text != NULL ? text->Text() : "";
}

unsigned char* savedata(size_t *length,BView *bkgview) {
	version_info *data = new version_info;
	data->major = strtoul(text_of(bkgview,"vers"),NULL,10);
	data->middle = strtoul(text_of(bkgview,"middle"),NULL,10);
	data->minor = strtoul(text_of(bkgview,"minor"),NULL,10);
	data->internal = strtoul(text_of(bkgview,"internal"),NULL,10);
	strncpy(data->short_info,text_of(bkgview,"short"),sizeof(data->short_info) - 1);
	data->short_info[sizeof(data->short_info) - 1] = 0;
	strncpy(data->long_info,text_of(bkgview,"long"),sizeof(data->long_info) - 1);
	data->long_info[sizeof(data->long_info) - 1] = 0;
	BMenuField *variety = (BMenuField *)(bkgview->FindView("Variety"));
	BMenu *menu = variety->Menu();
	data->variety = menu->IndexOf(menu->FindMarked());
	*length = 680;
	unsigned char *info = new unsigned char[680];
	memcpy(info,data,340);
	delete data;
	memcpy(info + 340,info2,340);
	delete info2;
	return info;
}

void messaging(BMessage *message,BView *bkgview) {
}