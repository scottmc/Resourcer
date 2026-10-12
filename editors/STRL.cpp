#include <image.h>
#include <string.h>
#include <interface/GroupLayout.h>
#include <interface/ScrollView.h>
#include <interface/Size.h>
#include <interface/StringView.h>
#include <interface/TextView.h>
#include <interface/Window.h>
#include <support/String.h>

//	An 'STRL' resource is a BStringList: the strings one after another, each
//	with its ending zero. The editor shows one string per line.

extern "C" _EXPORT void loaddata(unsigned char *,size_t,BView *);
extern "C" _EXPORT unsigned char* savedata(size_t *,BView *);
extern "C" _EXPORT void messaging(BMessage *,BView *);
//	Tells Resourcer that this editor wants a resizable window
extern "C" _EXPORT const bool resizable = true;
extern "C" _EXPORT const char description[] = "String List";

void loaddata(unsigned char *data,size_t length,BView *bkgview) {
	//	The strings become lines, so a string with a new line in it would split
	//	in two when saved
	BString lines;
	size_t position = 0;
	bool first = true;
	while (data != NULL && position < length) {
		const char *string = (const char *)(data + position);
		size_t size = 0;
		while (position + size < length && string[size] != 0)
			size++;
		if (!first)
			lines << "\n";
		lines.Append(string,size);
		first = false;
		position += size + 1;
	}
	BStringView *hint = new BStringView("hint","One string per line");
	BTextView *view = new BTextView("Strings",B_WILL_DRAW | B_FRAME_EVENTS);
	view->SetText(lines.String());
	BScrollView *scroll = new BScrollView("StringsScroll",view,B_WILL_DRAW | B_FRAME_EVENTS,false,true,B_FANCY_BORDER);
	BGroupLayout *group = new BGroupLayout(B_VERTICAL,4);
	bkgview->SetLayout(group);
	group->SetInsets(10,10,10,10);
	group->AddView(hint);
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
	BTextView *view = (BTextView *)(bkgview->FindView("Strings"));
	const char *text = view->Text();
	size_t size = strlen(text);
	//	No text is no strings. Otherwise every line ends with a zero, which is
	//	where the new lines were, plus one for the last line.
	if (size == 0) {
		*length = 0;
		return new unsigned char[1];
	}
	unsigned char *data = new unsigned char[size + 1];
	for (size_t i = 0;i < size;i++)
		data[i] = (text[i] == '\n') ? 0 : (unsigned char)(text[i]);
	data[size] = 0;
	*length = size + 1;
	return data;
}

void messaging(BMessage *message,BView *bkgview) {
}
