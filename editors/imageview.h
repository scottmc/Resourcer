//	The editor for a resource that holds an image in one of the formats Haiku has a
//	translator for (PNG, JPEG, GIF and so on). It shows the image and takes
//	a new one dropped on the window, or chosen with Replace...
//
//	This is not a complete editor, it is included by a one-line editor for each
//	format, which defines:
//		IMAGE_FORMAT		the type code, such as 'PNG '
//		IMAGE_DESCRIPTION	its name in the type list, such as "PNG Image"
//	Dropping an image already in that format keeps its bytes as they are; an
//	image in another format is converted with the translators.

//------------Includes----------------------
#include <image.h>
#include <stdio.h>
#include <string.h>
#include <app/Message.h>
#include <app/Messenger.h>
#include <interface/Alert.h>
#include <interface/Bitmap.h>
#include <interface/Button.h>
#include <interface/GroupLayout.h>
#include <interface/Size.h>
#include <interface/SpaceLayoutItem.h>
#include <interface/StringView.h>
#include <interface/View.h>
#include <interface/Window.h>
#include <storage/File.h>
#include <storage/FilePanel.h>
#include <support/DataIO.h>
#include <support/String.h>
#include <translation/TranslationUtils.h>
#include <translation/TranslatorFormats.h>
#include <translation/TranslatorRoster.h>
//-------------------------------------------

extern "C" _EXPORT void loaddata(unsigned char *,size_t,BView *);
extern "C" _EXPORT unsigned char* savedata(size_t *,BView *);
extern "C" _EXPORT void messaging(BMessage *,BView *);
extern "C" _EXPORT const char description[] = IMAGE_DESCRIPTION;

//	Tells Resourcer that this editor wants a resizable window
extern "C" _EXPORT const bool resizable = true;

static const uint32 kReplaceImage = (uint32)-100;

static BFilePanel *panel = NULL;

//	Holds the image's bytes, and the decoded picture of them to draw.
class ImagePreview : public BView {
	public:
		ImagePreview(void);
		~ImagePreview(void);
		void AttachedToWindow(void);
		void Draw(BRect update);
		bool SetData(const unsigned char *data,size_t length);
		const unsigned char *Data(void) const { return bytes; }
		size_t Size(void) const { return size; }
		bool IsValid(void) const { return picture != NULL; }
		BRect PictureBounds(void) const { return picture != NULL ? picture->Bounds() : BRect(0,0,-1,-1); }
	private:
		unsigned char *bytes;
		size_t size;
		BBitmap *picture;
};

ImagePreview::ImagePreview(void) : BView("preview",B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE) {
	bytes = NULL;
	size = 0;
	picture = NULL;
	SetExplicitMinSize(BSize(120,90));
	SetExplicitPreferredSize(BSize(320,240));
}

ImagePreview::~ImagePreview(void) {
	delete [] bytes;
	delete picture;
}

void ImagePreview::AttachedToWindow(void) {
	if (Parent() != NULL)
		SetViewColor(Parent()->ViewColor());
	SetLowColor(ViewColor());
}

//	Takes a copy of the data and tries to decode it. The data is kept even if it
//	can't be decoded, so that it is handed back unchanged. Returns whether it was
//	decoded.
bool ImagePreview::SetData(const unsigned char *data,size_t length) {
	delete [] bytes;
	delete picture;
	bytes = new unsigned char[length > 0 ? length : 1];
	if (length > 0)
		memcpy(bytes,data,length);
	size = length;
	picture = NULL;
	if (length > 0) {
		BMemoryIO input(bytes,size);
		picture = BTranslationUtils::GetBitmap(&input);
	}
	Invalidate();
	return picture != NULL;
}

void ImagePreview::Draw(BRect update) {
	BRect bounds = Bounds();
	SetHighColor(tint_color(ViewColor(),B_DARKEN_2_TINT));
	StrokeRect(bounds);
	if (picture == NULL)
		return;
	//	Small pictures are drawn as they are, large ones are scaled down to fit
	BRect area = bounds;
	area.InsetBy(4,4);
	BRect source = picture->Bounds();
	float scale = 1;
	if (source.Width() > area.Width())
		scale = area.Width() / source.Width();
	if (source.Height() * scale > area.Height())
		scale = area.Height() / source.Height();
	float width = source.Width() * scale;
	float height = source.Height() * scale;
	BRect target(0,0,width,height);
	target.OffsetTo(area.left + (area.Width() - width) / 2,area.top + (area.Height() - height) / 2);
	if (picture->ColorSpace() == B_RGBA32 || picture->ColorSpace() == B_RGBA32_BIG) {
		SetDrawingMode(B_OP_ALPHA);
		SetBlendingMode(B_PIXEL_ALPHA,B_ALPHA_OVERLAY);
	}
	DrawBitmap(picture,source,target,scale < 1 ? B_FILTER_BITMAP_BILINEAR : 0);
	SetDrawingMode(B_OP_COPY);
}

//	Says what is in the picture view.
static void update_status(BView *bkgview) {
	ImagePreview *preview = (ImagePreview *)(bkgview->FindView("preview"));
	BStringView *status = (BStringView *)(bkgview->FindView("status"));
	if (preview == NULL || status == NULL)
		return;
	BString text;
	if (preview->Size() == 0) {
		text = "No image yet";
	} else if (preview->IsValid()) {
		BRect bounds = preview->PictureBounds();
		text << (long)(bounds.Width() + 1) << " x " << (long)(bounds.Height() + 1) << ", " << (unsigned long)(preview->Size()) << " bytes";
	} else {
		text << (unsigned long)(preview->Size()) << " bytes, can't be read as an image";
	}
	status->SetText(text.String());
}

static void tell(const char *text) {
	BAlert *alert = new BAlert("alert",text,"OK",NULL,NULL,B_WIDTH_AS_USUAL,B_WARNING_ALERT);
	alert->Go(NULL);		//	asynchronous: this runs in the window's thread
}

//	Reads an image file for the resource: its own bytes if it is already in this
//	format, otherwise converted. Returns false (after saying why) if it can't.
static bool load_ref(const entry_ref *ref,BView *bkgview) {
	BFile file(ref,B_READ_ONLY);
	if (file.InitCheck() != B_OK)
		return false;
	BTranslatorRoster *roster = BTranslatorRoster::Default();
	translator_info info;
	if (roster->Identify(&file,NULL,&info,0,NULL,B_TRANSLATOR_BITMAP) != B_OK) {
		tell("That file is not an image Haiku can read.");
		return false;
	}
	BMallocIO converted;
	const unsigned char *data = NULL;
	size_t length = 0;
	unsigned char *raw = NULL;
	if (info.type == IMAGE_FORMAT) {
		off_t fileSize;
		if (file.GetSize(&fileSize) != B_OK || fileSize <= 0)
			return false;
		raw = new unsigned char[fileSize];
		file.Seek(0,SEEK_SET);
		if (file.Read(raw,fileSize) != (ssize_t)fileSize) {
			delete [] raw;
			return false;
		}
		data = raw;
		length = fileSize;
	} else {
		file.Seek(0,SEEK_SET);
		if (roster->Translate(&file,&info,NULL,&converted,IMAGE_FORMAT) != B_OK || converted.BufferLength() == 0) {
			tell("That image could not be converted to this format.");
			return false;
		}
		data = (const unsigned char *)(converted.Buffer());
		length = converted.BufferLength();
	}
	ImagePreview *preview = (ImagePreview *)(bkgview->FindView("preview"));
	if (preview != NULL) {
		preview->SetData(data,length);
		update_status(bkgview);
	}
	delete [] raw;
	return true;
}

void loaddata(unsigned char *data,size_t length,BView *bkgview) {
	ImagePreview *preview = new ImagePreview;
	BStringView *status = new BStringView("status","");
	status->SetAlignment(B_ALIGN_CENTER);
	BStringView *hint = new BStringView("hint","Drop an image file here");
	hint->SetAlignment(B_ALIGN_CENTER);
	BButton *replace = new BButton("replace","Replace...",new BMessage(kReplaceImage));
	BGroupLayout *group = new BGroupLayout(B_VERTICAL,6);
	bkgview->SetLayout(group);
	group->SetInsets(10,10,10,10);
	group->AddView(preview);
	group->AddView(status);
	group->AddView(hint);
	group->AddView(replace);
	group->ItemAt(3)->SetExplicitAlignment(BAlignment(B_ALIGN_HORIZONTAL_CENTER,B_ALIGN_BOTTOM));
	if (length > 0 && data != NULL)
		preview->SetData(data,length);
	update_status(bkgview);
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
	delete panel;
	panel = NULL;
	ImagePreview *preview = (ImagePreview *)(bkgview->FindView("preview"));
	if (preview == NULL || preview->Size() == 0) {
		*length = 0;
		return new unsigned char[1];
	}
	unsigned char *data = new unsigned char[preview->Size()];
	memcpy(data,preview->Data(),preview->Size());
	*length = preview->Size();
	return data;
}

void messaging(BMessage *message,BView *bkgview) {
	switch (message->what) {
		case kReplaceImage:
			if (panel == NULL)
				panel = new BFilePanel(B_OPEN_PANEL,new BMessenger(bkgview->Window()),NULL,0,false);
			panel->Show();
			break;
		case B_SIMPLE_DATA:
		case B_REFS_RECEIVED:
			{
				//	files dropped on the window, or chosen: the first image that works wins
				entry_ref ref;
				for (int32 i = 0; message->FindRef("refs",i,&ref) == B_OK; i++) {
					if (load_ref(&ref,bkgview))
						break;
				}
			}
			break;
	}
}
