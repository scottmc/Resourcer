//	The viewer for a resource that holds a pointer, 'PNTR' (B_POINTER_TYPE) or
//	'OPTR' (B_OBJECT_TYPE). A pointer is an address in the memory of the program
//	that stored it, so the value means nothing once it has been saved in a file.
//	It is shown, but not edited, and the resource is saved back as it was.
//
//	This is not a complete editor, it is included by a one-line editor for each
//	type, which defines:
//		POINTER_DESCRIPTION		its name in the type list, such as "Pointer"

//------------Includes----------------------
#include <image.h>
#include <stdio.h>
#include <string.h>
#include <app/Message.h>
#include <interface/GroupLayout.h>
#include <interface/Size.h>
#include <interface/StringView.h>
#include <interface/View.h>
#include <interface/Window.h>
//-------------------------------------------

extern "C" _EXPORT void loaddata(unsigned char *,size_t,BView *);
extern "C" _EXPORT unsigned char* savedata(size_t *,BView *);
extern "C" _EXPORT void messaging(BMessage *,BView *);
extern "C" _EXPORT const char description[] = POINTER_DESCRIPTION;

//	The line that shows the value also keeps the bytes of the resource, so that
//	each window has its own copy to save back unchanged.
class ValueLine : public BStringView {
	public:
		ValueLine(const char *text,const unsigned char *data,size_t length)
			: BStringView("value",text) {
			kept = NULL;
			keptsize = 0;
			if (data != NULL && length > 0) {
				kept = new unsigned char[length];
				memcpy(kept,data,length);
				keptsize = length;
			}
		}
		~ValueLine() {
			delete [] kept;
		}
		unsigned char *kept;
		size_t keptsize;
};

static BStringView *make_line(const char *name,const char *text) {
	return new BStringView(name,text);
}

void loaddata(unsigned char *data,size_t length,BView *bkgview) {
	char value[100];
	char decimal[100];
	char note[200];
	value[0] = '\0';
	decimal[0] = '\0';
	if (length == 0 || data == NULL) {
		strcpy(note,"No value");
	} else if (length == 4 || length == 8) {
		//	a pointer is as wide as the program that made it, in its byte order
		uint64 number = 0;
		for (size_t i = 0; i < length; i++)
			number |= ((uint64)(data[i])) << (8 * i);
		if (length == 4)
			sprintf(value,"0x%08lx",(unsigned long)(number));
		else
			sprintf(value,"0x%08lx%08lx",(unsigned long)(number >> 32),(unsigned long)(number & 0xffffffff));
		sprintf(decimal,"%lu",(unsigned long)(number));
		sprintf(note,"%lu byte pointer",(unsigned long)(length));
	} else {
		//	not the size of a pointer, so show the bytes
		size_t shown = length < 16 ? length : 16;
		for (size_t i = 0; i < shown; i++)
			sprintf(value + strlen(value),"%02x ",data[i]);
		if (shown < length)
			strcat(value,"...");
		sprintf(note,"%lu bytes, not the size of a pointer",(unsigned long)(length));
	}

	BGroupLayout *top = new BGroupLayout(B_VERTICAL,8);
	bkgview->SetLayout(top);
	top->SetInsets(10,10,10,10);
	top->AddView(new ValueLine(value,data,length));
	if (decimal[0] != '\0')
		top->AddView(make_line("decimal",decimal));
	top->AddView(make_line("note",note));
	top->AddView(make_line("hint","A pointer only has a meaning inside the program that made it"));
	for (int32 i = 0; i < top->CountItems(); i++)
		top->ItemAt(i)->SetExplicitAlignment(BAlignment(B_ALIGN_HORIZONTAL_CENTER,B_ALIGN_TOP));

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

//	Nothing can be changed here, so the resource is saved as it was.
unsigned char* savedata(size_t *length,BView *bkgview) {
	ValueLine *line = (ValueLine *)(bkgview->FindView("value"));
	if (line == NULL || line->kept == NULL || line->keptsize == 0) {
		*length = 0;
		return new unsigned char[1];
	}
	unsigned char *data = new unsigned char[line->keptsize];
	memcpy(data,line->kept,line->keptsize);
	*length = line->keptsize;
	return data;
}

void messaging(BMessage *,BView *) {
}
