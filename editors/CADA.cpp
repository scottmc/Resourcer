//------------Includes----------------------
#include <image.h>
#include <stdio.h>
#include <string.h>
#include <app/Message.h>
#include <interface/GroupLayout.h>
#include <interface/ScrollView.h>
#include <interface/Size.h>
#include <interface/StringView.h>
#include <interface/TextView.h>
#include <interface/View.h>
#include <interface/Window.h>
#include <support/ByteOrder.h>
#include <support/DataIO.h>
#include <support/String.h>
//-------------------------------------------

//	A 'CADA' resource is a locale catalog, the translations of an application
//	into one language (Haiku's DefaultCatalog, src/kits/locale/). The resource's
//	name is the language, its ID a hash of that name. The data is a stream of
//	flattened BMessages: a header message ("class", "c:sz" the number of
//	strings, "c:ver", "c:lang", "c:sig" the application's signature and "c:fpr",
//	a fingerprint) followed by one message per string ("c:ostr" the original,
//	"c:ctxt" its context, "c:comt" a comment, "c:hash" and "c:tstr" the
//	translation). The fingerprint is the sum of the strings' hashes, which Haiku
//	checks when it loads the catalog.
//
//	This editor only shows the catalog. Editing one by hand would mean keeping
//	the hashes and fingerprint right, which Haiku's own catalog tools do.

extern "C" _EXPORT void loaddata(unsigned char *,size_t,BView *);
extern "C" _EXPORT unsigned char* savedata(size_t *,BView *);
extern "C" _EXPORT void messaging(BMessage *,BView *);
extern "C" _EXPORT const char description[] = "Locale Catalog";

//	Tells Resourcer that this editor wants a resizable window
extern "C" _EXPORT const bool resizable = true;

//	Holds the catalog's bytes, so that they are handed back unchanged, and the
//	views that show them.
class CatalogView : public BView {
	public:
		CatalogView(const unsigned char *data,size_t length);
		~CatalogView(void);
		const unsigned char *Data(void) const { return bytes; }
		size_t Size(void) const { return size; }
	private:
		void Build(const char *line1,const char *line2,const char *line3,const char *strings);
		void Read(void);
		unsigned char *bytes;
		size_t size;
};

CatalogView::CatalogView(const unsigned char *data,size_t length) : BView("catalog",B_WILL_DRAW) {
	size = length;
	bytes = new unsigned char[length > 0 ? length : 1];
	if (length > 0)
		memcpy(bytes,data,length);
	Read();
}

CatalogView::~CatalogView(void) {
	delete [] bytes;
}

//	Fills in the three summary lines and the list of strings.
void CatalogView::Build(const char *line1,const char *line2,const char *line3,const char *strings) {
	BGroupLayout *group = new BGroupLayout(B_VERTICAL,4);
	SetLayout(group);
	group->AddView(new BStringView("language",line1));
	group->AddView(new BStringView("signature",line2));
	group->AddView(new BStringView("summary",line3));
	BTextView *list = new BTextView("strings",B_WILL_DRAW | B_FRAME_EVENTS);
	list->MakeEditable(false);
	list->SetWordWrap(true);
	list->SetText(strings);
	BScrollView *scroll = new BScrollView("stringscroll",list,B_WILL_DRAW | B_FRAME_EVENTS,false,true,B_FANCY_BORDER);
	//	A comfortable starting size, based on the current font
	font_height height;
	be_plain_font->GetHeight(&height);
	float line = height.ascent + height.descent + height.leading;
	scroll->SetExplicitPreferredSize(BSize(be_plain_font->StringWidth("M") * 55,line * 16));
	group->AddView(scroll);
}

void CatalogView::Read(void) {
	if (size == 0) {
		Build("No catalog data","","","");
		return;
	}
	BMemoryIO input(bytes,size);
	BMessage header;
	int32 count = 0;
	int16 version = 0;
	if (header.Unflatten(&input) != B_OK || header.FindInt32("c:sz",&count) != B_OK || header.FindInt16("c:ver",&version) != B_OK) {
		Build("This is not locale catalog data","","","");
		return;
	}
	BString language("Language: ");
	language << header.GetString("c:lang","");
	BString signature("Application: ");
	signature << header.GetString("c:sig","");
	//	Read each string, adding up the hashes for the fingerprint as Haiku does
	BString strings;
	uint32 checksum = 0;
	int32 found = 0;
	BMessage entry;
	for (int32 i = 0;i < count;i++) {
		if (entry.Unflatten(&input) != B_OK)
			break;
		const char *original = entry.GetString("c:ostr","");
		const char *context = entry.GetString("c:ctxt","");
		const char *comment = entry.GetString("c:comt","");
		const char *translated = entry.GetString("c:tstr","");
		int32 hash = 0;
		entry.FindInt32("c:hash",&hash);
		checksum += (uint32)(B_HOST_TO_LENDIAN_INT32(hash));
		if (found > 0)
			strings << "\n";
		if (context[0] != 0)
			strings << "[" << context << "] ";
		strings << original << "\n    " << translated << "\n";
		if (comment[0] != 0)
			strings << "    (" << comment << ")\n";
		found++;
	}
	BString summary;
	summary << (long)(count) << (count == 1 ? " string" : " strings");
	if (found < count) {
		summary << ", but the data ends after " << (long)(found);
	} else {
		uint32 stored = (uint32)(header.GetInt32("c:fpr",0));
		if ((count > 0) && (stored != checksum))
			summary << ", fingerprint doesn't match, Haiku would refuse this catalog";
		else
			summary << ", fingerprint is correct";
	}
	Build(language.String(),signature.String(),summary.String(),strings.String());
}

void loaddata(unsigned char *data,size_t length,BView *bkgview) {
	CatalogView *catalog = new CatalogView(data,length);
	BGroupLayout *group = new BGroupLayout(B_VERTICAL);
	bkgview->SetLayout(group);
	group->SetInsets(10,10,10,10);
	group->AddView(catalog);
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
	CatalogView *catalog = (CatalogView *)(bkgview->FindView("catalog"));
	if (catalog == NULL || catalog->Size() == 0) {
		*length = 0;
		return new unsigned char[1];
	}
	unsigned char *data = new unsigned char[catalog->Size()];
	memcpy(data,catalog->Data(),catalog->Size());
	*length = catalog->Size();
	return data;
}

void messaging(BMessage *message,BView *bkgview) {
}
