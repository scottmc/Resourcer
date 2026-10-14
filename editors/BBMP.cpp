//------------Includes----------------------
#include <image.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <app/Message.h>
#include <interface/Bitmap.h>
#include <interface/GraphicsDefs.h>
#include <interface/GroupLayout.h>
#include <interface/Size.h>
#include <interface/StringView.h>
#include <interface/View.h>
#include <interface/Window.h>
#include <support/TypeConstants.h>
//-------------------------------------------

//	A 'BBMP' resource holds a BBitmap that was archived into a BMessage, which
//	is then flattened. The message has the fields "class" ("BBitmap"),
//	"_frame", "_cspace", "_bmflags", "_rowbytes" and "_data" (the pixels).
//	Applications such as Deskbar keep their small images this way.
//
//	This is a viewer only: it draws the bitmap and says what it is, and
//	hands back the resource's own bytes when saving, so nothing changes.

extern "C" _EXPORT void loaddata(unsigned char *,size_t,BView *);
extern "C" _EXPORT unsigned char* savedata(size_t *,BView *);
extern "C" _EXPORT void messaging(BMessage *,BView *);
extern "C" _EXPORT const char description[] = "Bitmap (archived BBitmap)";

//	Small bitmaps are drawn enlarged by a whole number of pixels, up to about
//	this size, and very large ones are shrunk to fit.
static const float kEnlargeTo = 128;
static const float kShrinkFrom = 400;

//	Holds the resource's bytes and the bitmap made from them.
class BitmapPreview : public BView {
	public:
		BitmapPreview() : BView("preview",B_WILL_DRAW) {
			data = NULL;
			size = 0;
			bitmap = NULL;
			scale = 1;
			SetSizes();
		}
		~BitmapPreview() {
			delete [] data;
			delete bitmap;
		}
		void AttachedToWindow() {
			if (Parent() != NULL)
				SetViewColor(Parent()->ViewColor());
		}
		//	Takes a copy of the resource, and makes the bitmap from it when it
		//	can. Returns false when the bytes are not an archived bitmap.
		bool SetData(const unsigned char *newdata,size_t newsize) {
			delete [] data;
			data = NULL;
			size = 0;
			delete bitmap;
			bitmap = NULL;
			scale = 1;
			if (newdata != NULL && newsize > 0) {
				data = new unsigned char[newsize];
				memcpy(data,newdata,newsize);
				size = newsize;

				//	Unflatten reads both the BeOS R5 and the Haiku format
				BMessage archive;
				if (archive.Unflatten((const char *)(data)) == B_OK) {
					BArchivable *object = BBitmap::Instantiate(&archive);
					bitmap = dynamic_cast<BBitmap *>(object);
					if (bitmap == NULL)
						delete object;
				}
				if (bitmap != NULL && bitmap->InitCheck() != B_OK) {
					delete bitmap;
					bitmap = NULL;
				}
			}
			if (bitmap != NULL) {
				float biggest = bitmap->Bounds().Width() + 1;
				if (bitmap->Bounds().Height() + 1 > biggest)
					biggest = bitmap->Bounds().Height() + 1;
				if (biggest < kEnlargeTo)
					scale = floor(kEnlargeTo / biggest);
				else if (biggest > kShrinkFrom)
					scale = kShrinkFrom / biggest;
			}
			SetSizes();
			InvalidateLayout();
			if (Window() != NULL)
				Invalidate();
			return bitmap != NULL;
		}
		void Draw(BRect updateRect) {
			BRect shown = ShownRect();
			if (bitmap == NULL) {
				//	nothing to show: an empty frame
				SetHighColor(ui_color(B_CONTROL_BORDER_COLOR));
				StrokeRect(shown);
				return;
			}
			//	pictures with an alpha channel are blended, the others show their
			//	transparent pixels (as B_CMAP8 has them) as holes
			color_space space = bitmap->ColorSpace();
			if (space == B_RGBA32 || space == B_RGBA32_BIG
				|| space == B_RGBA15 || space == B_RGBA15_BIG) {
				SetDrawingMode(B_OP_ALPHA);
				SetBlendingMode(B_PIXEL_ALPHA,B_ALPHA_OVERLAY);
			} else {
				SetDrawingMode(B_OP_OVER);
			}
			DrawBitmap(bitmap,bitmap->Bounds(),shown);
			SetDrawingMode(B_OP_COPY);
			SetHighColor(ui_color(B_CONTROL_BORDER_COLOR));
			StrokeRect(shown.InsetByCopy(-1,-1));
		}
		const unsigned char *Data() const {return data;}
		size_t Size() const {return size;}
		const BBitmap *Bitmap() const {return bitmap;}
		float Scale() const {return scale;}
	private:
		//	The size the bitmap is drawn at, in device pixels.
		BRect ShownRect() const {
			if (bitmap == NULL)
				return BRect(1,1,64,64);
			return BRect(1,1,1 + (bitmap->Bounds().Width() + 1) * scale - 1,
				1 + (bitmap->Bounds().Height() + 1) * scale - 1);
		}
		//	Pixels are drawn in device pixels, so the view keeps its size
		//	whatever the font is. There is room for a one pixel frame around.
		void SetSizes() {
			BRect shown = ShownRect();
			BSize all(shown.Width() + 2,shown.Height() + 2);
			SetExplicitMinSize(all);
			SetExplicitPreferredSize(all);
			SetExplicitMaxSize(all);
		}
		unsigned char *data;
		size_t size;
		BBitmap *bitmap;
		float scale;
};

//	The name of a color space for the status line.
static const char *space_name(color_space space) {
	switch (space) {
		case B_RGB32: return "32-bit color (B_RGB32)";
		case B_RGBA32: return "32-bit color with alpha (B_RGBA32)";
		case B_RGB32_BIG: return "32-bit color, big-endian (B_RGB32_BIG)";
		case B_RGBA32_BIG: return "32-bit color with alpha, big-endian (B_RGBA32_BIG)";
		case B_RGB24: return "24-bit color (B_RGB24)";
		case B_RGB24_BIG: return "24-bit color, big-endian (B_RGB24_BIG)";
		case B_RGB16: return "16-bit color (B_RGB16)";
		case B_RGB16_BIG: return "16-bit color, big-endian (B_RGB16_BIG)";
		case B_RGB15: return "15-bit color (B_RGB15)";
		case B_RGBA15: return "15-bit color with alpha (B_RGBA15)";
		case B_RGB15_BIG: return "15-bit color, big-endian (B_RGB15_BIG)";
		case B_RGBA15_BIG: return "15-bit color with alpha, big-endian (B_RGBA15_BIG)";
		case B_CMAP8: return "8-bit palette (B_CMAP8)";
		case B_GRAY8: return "8-bit gray (B_GRAY8)";
		case B_GRAY1: return "1-bit black and white (B_GRAY1)";
		default: break;
	}
	return NULL;
}

//	Says what the resource holds, and how it is shown.
static void update_status(BView *bkgview) {
	BitmapPreview *preview = (BitmapPreview *)(bkgview->FindView("preview"));
	BStringView *status = (BStringView *)(bkgview->FindView("status"));
	BStringView *detail = (BStringView *)(bkgview->FindView("detail"));
	if (preview == NULL || status == NULL || detail == NULL)
		return;
	char text[200];
	char more[200];
	more[0] = '\0';
	if (preview->Size() == 0) {
		strcpy(text,"No bitmap");
	} else if (preview->Bitmap() == NULL) {
		sprintf(text,"Not a bitmap archive (%lu bytes)",(unsigned long)(preview->Size()));
	} else {
		const BBitmap *bitmap = preview->Bitmap();
		int32 width = (int32)(bitmap->Bounds().Width()) + 1;
		int32 height = (int32)(bitmap->Bounds().Height()) + 1;
		sprintf(text,"%ld x %ld pixels, %lu bytes",(long)(width),(long)(height),
			(unsigned long)(preview->Size()));
		const char *name = space_name(bitmap->ColorSpace());
		if (name != NULL)
			strcpy(more,name);
		else
			sprintf(more,"color space 0x%04lx",(unsigned long)(bitmap->ColorSpace()));
		if (preview->Scale() > 1)
			sprintf(more + strlen(more),", shown at %ldx",(long)(preview->Scale()));
		else if (preview->Scale() < 1)
			strcat(more,", shown reduced to fit");
	}
	status->SetText(text);
	detail->SetText(more);
}

void loaddata(unsigned char *data,size_t length,BView *bkgview) {
	BitmapPreview *preview = new BitmapPreview();
	if (length != 0 && data != NULL)
		preview->SetData(data,length);

	BStringView *status = new BStringView("status","");
	BStringView *detail = new BStringView("detail","");
	BStringView *hint = new BStringView("hint","This bitmap can be viewed but not edited");

	BGroupLayout *top = new BGroupLayout(B_VERTICAL,8);
	bkgview->SetLayout(top);
	top->SetInsets(10,10,10,10);
	top->AddView(preview);
	top->AddView(status);
	top->AddView(detail);
	top->AddView(hint);
	for (int32 i = 0; i < 4; i++)
		top->ItemAt(i)->SetExplicitAlignment(BAlignment(B_ALIGN_HORIZONTAL_CENTER,B_ALIGN_TOP));
	update_status(bkgview);

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
	BitmapPreview *preview = (BitmapPreview *)(bkgview->FindView("preview"));
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
}
