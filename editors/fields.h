//	Shared by the editors that show a few labelled fields: RREF, NREF and MSNG.
//	Each editor includes it once, so the functions are static.
#ifndef RESOURCER_EDITORS_FIELDS_H
#define RESOURCER_EDITORS_FIELDS_H

#include <image.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <interface/Window.h>
#include <interface/TextControl.h>
#include <interface/GridLayout.h>
#include <interface/LayoutItem.h>
#include <interface/SpaceLayoutItem.h>

//	Gives the window a grid to put the fields in.
static BGridLayout *start_form(BView *bkgview) {
	BGridLayout *grid = new BGridLayout(8,4);
	bkgview->SetLayout(grid);
	grid->SetInsets(10,10,10,10);
	return grid;
}

//	Adds a labelled text field in the given row. The field is wide enough for
//	the widest text, so a long value is never clipped.
static void add_field(BGridLayout *grid,int32 row,const char *name,const char *label,const char *text,const char *widest) {
	BTextControl *control = new BTextControl(name,label,text,NULL);
	grid->AddItem(control->CreateLabelLayoutItem(),0,row);
	BLayoutItem *field = control->CreateTextViewLayoutItem();
	float width = be_plain_font->StringWidth(widest) + 24;
	field->SetExplicitMinSize(BSize(width,B_SIZE_UNSET));
	field->SetExplicitPreferredSize(BSize(width,B_SIZE_UNSET));
	grid->AddItem(field,1,row);
}

//	Sizes the window to its fields once all of them are added. The host window
//	is not layout managed. Ask for the minimum size first: the preferred size can
//	come back as NaN until the layout is validated, so the tests replace a NaN.
static void finish_form(BGridLayout *grid,BView *bkgview,int32 rows) {
	grid->AddItem(BSpaceLayoutItem::CreateGlue(),0,rows,2,1);
	BSize min = bkgview->MinSize();
	BSize size = bkgview->PreferredSize();
	if (!(size.width >= min.width))
		size.width = min.width;
	if (!(size.height >= min.height))
		size.height = min.height;
	bkgview->Window()->ResizeTo(size.width > 220 ? size.width : 220,size.height);
}

//	What was typed in a field.
static const char *field_text(BView *bkgview,const char *name) {
	BTextControl *control = (BTextControl *)(bkgview->FindView(name));
	if (control == NULL)
		return "";
	return control->Text();
}

#endif
