//------------Includes----------------------
#include <image.h>
#include <interface/Window.h>
#include <stdio.h>
#include <string.h>
#include <interface/Bitmap.h>
#include <interface/View.h>
#include <interface/Button.h>
#include <interface/GroupLayout.h>
#include <interface/Size.h>
#include <storage/FilePanel.h>
#include <storage/Entry.h>
#include <storage/File.h>
#include <support/DataIO.h>
#include <translation/TranslatorRoster.h>
#include <translation/TranslationUtils.h>
#include <translation/BitmapStream.h>
//-------------------------------------------

extern "C" _EXPORT void loaddata(unsigned char *,size_t,BView *);
extern "C" _EXPORT unsigned char* savedata(size_t *,BView *);
extern "C" _EXPORT void messaging(BMessage *,BView *);
extern "C" _EXPORT const char description[] = "Translator Bitmap";

class BPView;
void processTIFF(BMessage *TIFF, BPView *view);
void loadopenbox(BPView *view);

class TIFFilter : public BRefFilter {
	virtual bool Filter(const entry_ref *ref, BNode *node, struct stat_beos *st, const char *filetype) {
		if (BEntry(ref,true).IsDirectory())
			return true;
		if (BEntry(ref,true).IsFile()) {
			BFile file(ref,B_READ_ONLY);
			BTranslatorRoster *roster = BTranslatorRoster::Default();
			translator_info check;
			if ((((((filetype[0] == 'i') && (filetype[1] == 'm')) && (filetype[2] == 'a')) && (filetype[3] == 'g')) && (filetype[4] == 'e'))) {
			if (roster->Identify(&file,NULL,&check) == B_OK) {
				if (check.capability != 0) {
					return true;
				}
			}
			}
		}
		return false;
	}
};

class BPView : public BView {
	public:
	BPView (const char *name, uint32 flags,BView *parenta,BBitmap *imagea) : BView(name,flags) {
		image = imagea;
		parent = parenta;
		opbut = new BButton("opbut","Open...",new BMessage(-200));
		//	The image keeps its pixel size and the button sits centred below it.
		//	The layout follows the font for the button, and fit() follows the
		//	size of the picture.
		BGroupLayout *top = new BGroupLayout(B_VERTICAL,10);
		parent->SetLayout(top);
		top->SetInsets(10,10,10,10);
		top->AddView(this);
		top->AddView(opbut);
		top->ItemAt(0)->SetExplicitAlignment(BAlignment(B_ALIGN_HORIZONTAL_CENTER,B_ALIGN_TOP));
		top->ItemAt(1)->SetExplicitAlignment(BAlignment(B_ALIGN_HORIZONTAL_CENTER,B_ALIGN_TOP));
		openpanel = new BFilePanel(B_OPEN_PANEL,new BMessenger(parent),NULL,B_FILE_NODE,false,NULL,new TIFFilter,false,true);
		fit();
	}
	
	void Draw(BRect updateRect) {
		if (image != NULL) {
			DrawBitmap(image);
		}
	}
	
	//	Sizes the view to its picture (or to a blank area without one) and the
	//	window to what the layout then asks for.
	void fit() {
		BSize size(300,250);
		if (image != NULL)
			size.Set(image->Bounds().Width(),image->Bounds().Height());
		SetExplicitMinSize(size);
		SetExplicitPreferredSize(size);
		SetExplicitMaxSize(size);
		parent->InvalidateLayout();
		//	Ask for the minimum size first: the preferred size can come back as
		//	NaN until the layout is validated, so the tests replace a NaN.
		BSize min = parent->MinSize();
		BSize pref = parent->PreferredSize();
		if (!(pref.width >= min.width))
			pref.width = min.width;
		if (!(pref.height >= min.height))
			pref.height = min.height;
		if (parent->Window() != NULL)
			parent->Window()->ResizeTo(pref.width,pref.height);
		Invalidate();
	}
	
	void change_image(BBitmap *bitmap) {
		image = bitmap;
		fit();
	}
	//---------------------Data------------------------
	BBitmap *image;
	BView *parent;
	BButton *opbut;
	BFilePanel *openpanel;
	};

void processTIFF(BMessage *TIFF, BPView *view) {
	entry_ref ref;
	TIFF->FindRef("refs",&ref);
	BFile file(&ref,B_READ_ONLY);
	BBitmap *bitmap = BTranslationUtils::GetBitmap(&file);
	view->change_image(bitmap);
}

void loadopenbox(BPView *view) {
	view->openpanel->Show();
}

void loaddata(unsigned char *data,size_t length,BView *bkgview) { //runs when editor launched, window rect is 300 x 300 pixels
	BMemoryIO *io = new BMemoryIO(data,length);
	BBitmap *bitmap = BTranslationUtils::GetBitmap(io);
	delete io;
	new BPView("view",B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE | B_FRAME_EVENTS,bkgview,bitmap);
}

unsigned char* savedata(size_t *length,BView *bkgview) { //runs when window closes, passes back window address given in loaddata()
	BPView *x = (BPView *)(bkgview->FindView("view"));
	unsigned char *data;
	if (x->image == NULL) {
		*length = 0;
		data = new unsigned char(0);
		return data;
	}
	BBitmapStream *stream = new BBitmapStream(x->image);
	stream->Seek(0,SEEK_END);
	*length = stream->Position();
	data = new unsigned char[*length];
	stream->Seek(0,SEEK_SET);
	stream->Read(data,*length);
	stream->DetachBitmap(&(x->image));
	size_t head_length = *length - x->image->BitsLength();
	memcpy(data + head_length,x->image->Bits(),*length - head_length);
	((BPView *)(bkgview->FindView("view")))->openpanel->Window()->PostMessage(B_QUIT_REQUESTED);
	delete ((BPView *)(bkgview->FindView("view")))->openpanel;
	/*BBitmapStream *s2 = new BBitmapStream;
	s2->Write(data,*length);
	s2->DetachBitmap(&x->image);
	bkgview->Window()->Show();
	x->Invalidate(x->Bounds());
	(new BAlert("ck","Check...","OK"))->Go();*/
	delete stream;
	return data;
}

void messaging(BMessage *message,BView *bkgview) { //BMessages from window, to be passed here 'what' field must be a negative number
	switch (message->what) {
		case (uint32)-200:
			loadopenbox((BPView *)(bkgview->FindView("view")));
			break;
		case B_REFS_RECEIVED:
			processTIFF(message,(BPView *)(bkgview->FindView("view")));
			break;
	}
}
