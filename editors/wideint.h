//------------------------------------------------------------------------------
//	wideint.h
//
//	Shared by the SIZT, SSZT and OFFT editors: whole numbers whose width
//	depends on who wrote them (size_t is 4 bytes on 32 bit Haiku and BeOS and
//	8 bytes on 64 bit Haiku). The width is taken from the length of the data:
//	4 or 8 bytes is edited and saved at that width, so nothing is cut short or
//	widened, and a new, empty resource gets the width of the type on this
//	machine. Any other length is not edited, it is handed back as it was.
//------------------------------------------------------------------------------

#ifndef WIDEINT_H
#define WIDEINT_H

#include <image.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <interface/Alert.h>
#include <interface/GridLayout.h>
#include <interface/LayoutItem.h>
#include <interface/SpaceLayoutItem.h>
#include <interface/StringView.h>
#include <interface/TextControl.h>
#include <interface/Window.h>

//	What the editor was given, and the width it is working at (0: not editable)
static unsigned char *wide_original = NULL;
static size_t wide_original_length = 0;
static size_t wide_width = 0;
static bool wide_signed = false;

static void wide_load(unsigned char *data,size_t length,BView *bkgview,
					  size_t native,bool isSigned) {
	wide_signed = isSigned;
	delete [] wide_original;
	wide_original = NULL;
	wide_original_length = length;
	if (length > 0) {
		wide_original = new unsigned char[length];
		memcpy(wide_original,data,length);
	}

	if (length == 0)
		wide_width = native;
	else if (length == 4 || length == 8)
		wide_width = length;
	else
		wide_width = 0;

	char text[64];
	strcpy(text,"0");
	if (wide_width == 8) {
		if (length == 0) {
			strcpy(text,"0");
		} else if (isSigned) {
			int64 value;
			memcpy(&value,data,8);
			sprintf(text,"%lld",(long long)value);
		} else {
			uint64 value;
			memcpy(&value,data,8);
			sprintf(text,"%llu",(unsigned long long)value);
		}
	} else if (wide_width == 4 && length == 4) {
		if (isSigned) {
			int32 value;
			memcpy(&value,data,4);
			sprintf(text,"%ld",(long)value);
		} else {
			uint32 value;
			memcpy(&value,data,4);
			sprintf(text,"%lu",(unsigned long)value);
		}
	}

	//	The layout manager sizes the label and field from the current font.
	BTextControl *v = new BTextControl("Integer","Data:",text,NULL);
	v->SetEnabled(wide_width != 0);
	BGridLayout *grid = new BGridLayout(8,4);
	bkgview->SetLayout(grid);
	grid->SetInsets(10,10,10,10);
	grid->AddItem(v->CreateLabelLayoutItem(),0,0);
	//	Room for the longest value this type can have, so it is never clipped
	BLayoutItem *field = v->CreateTextViewLayoutItem();
	float fieldwidth = be_plain_font->StringWidth("-9223372036854775808") + 24;
	field->SetExplicitMinSize(BSize(fieldwidth,B_SIZE_UNSET));
	field->SetExplicitPreferredSize(BSize(fieldwidth,B_SIZE_UNSET));
	grid->AddItem(field,1,0);
	if (wide_width == 0) {
		char note[128];
		sprintf(note,"This is %lu bytes long, not 4 or 8. Use the generic editor.",
				(unsigned long)length);
		BStringView *message = new BStringView("note",note);
		grid->AddView(message,0,1,2,1);
		grid->AddItem(BSpaceLayoutItem::CreateGlue(),0,2,2,1);
	} else {
		grid->AddItem(BSpaceLayoutItem::CreateGlue(),0,1,2,1);
	}
	//	The host window is not layout managed, so size it to the content.
	//	Ask for the minimum size first: the preferred size can come back as NaN
	//	until the layout is validated, so the tests are written to replace a NaN.
	BSize min = bkgview->MinSize();
	BSize size = bkgview->PreferredSize();
	if (!(size.width >= min.width))
		size.width = min.width;
	if (!(size.height >= min.height))
		size.height = min.height;
	bkgview->Window()->ResizeTo(size.width > 220 ? size.width : 220,size.height);
}

//	The bytes to give back when there is nothing new to write
static unsigned char *wide_unchanged(size_t *length) {
	size_t count = wide_original_length;
	unsigned char *copy = new unsigned char[count > 0 ? count : 1];
	if (wide_original != NULL)
		memcpy(copy,wide_original,count);
	*length = count;
	return copy;
}

static unsigned char *wide_save(size_t *length,BView *bkgview) {
	if (wide_width == 0)
		return wide_unchanged(length);

	BTextControl *text = (BTextControl *)(bkgview->FindView("Integer"));
	const char *string = text->Text();
	while (*string == ' ' || *string == '\t')
		string++;

	bool valid = (*string != 0);
	int64 svalue = 0;
	uint64 uvalue = 0;
	if (valid) {
		char *end = NULL;
		errno = 0;
		if (wide_signed) {
			svalue = strtoll(string,&end,10);
			if (errno == ERANGE)
				valid = false;
			else if (wide_width == 4 && (svalue < -2147483647LL - 1 || svalue > 2147483647LL))
				valid = false;
		} else {
			if (*string == '-')
				valid = false;
			else
				uvalue = strtoull(string,&end,10);
			if (valid && errno == ERANGE)
				valid = false;
			else if (valid && wide_width == 4 && uvalue > 0xFFFFFFFFULL)
				valid = false;
		}
		while (valid && end != NULL && (*end == ' ' || *end == '\t'))
			end++;
		if (valid && (end == NULL || *end != 0))
			valid = false;
	}

	if (!valid) {
		char message[256];
		sprintf(message,"\"%s\" is not a valid %s %lu byte number, so the value was left as it was.",
				text->Text(),wide_signed ? "signed" : "unsigned",
				(unsigned long)wide_width);
		(new BAlert("invalid",message,"OK",NULL,NULL,B_WIDTH_AS_USUAL,B_STOP_ALERT))->Go(NULL);
		if (wide_original_length == wide_width)
			return wide_unchanged(length);
		//	A new resource has no earlier value, write zero
		svalue = 0;
		uvalue = 0;
	}

	unsigned char *result = new unsigned char[wide_width];
	if (wide_width == 8) {
		if (wide_signed)
			memcpy(result,&svalue,8);
		else
			memcpy(result,&uvalue,8);
	} else {
		if (wide_signed) {
			int32 value = (int32)svalue;
			memcpy(result,&value,4);
		} else {
			uint32 value = (uint32)uvalue;
			memcpy(result,&value,4);
		}
	}
	*length = wide_width;
	return result;
}

#endif	// WIDEINT_H
