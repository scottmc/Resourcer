//	The viewer for a resource that holds raw pixels with no size stored with
//	them: 'CLRB' (B_COLOR_8_BIT_TYPE), 'GRYB' (B_GRAYSCALE_8_BIT_TYPE), 'MNOB'
//	(B_MONOCHROME_1_BIT_TYPE) and 'RGBB' (B_RGB_32_BIT_TYPE). The width has to be
//	told, so there is a Width field that starts with a guess, and the height is
//	what the bytes make. Nothing is changed: the resource is saved as it was.
//
//	This is not a complete editor, it is included by a one-line editor for each
//	type, which defines:
//		RAW_DESCRIPTION		its name in the type list
//		RAW_FORMAT			RAW_CMAP8, RAW_GRAY8, RAW_GRAY1 or RAW_RGB32
//
//		RAW_CMAP8	a byte per pixel, an index into the system palette
//		RAW_GRAY8	a byte per pixel, 0 is black and 255 is white
//		RAW_GRAY1	a bit per pixel, the first pixel in the high bit, 1 is
//					black, and every row filled out to a whole byte
//		RAW_RGB32	four bytes per pixel, blue then green then red then alpha

#define RAW_CMAP8 1
#define RAW_GRAY8 2
#define RAW_GRAY1 3
#define RAW_RGB32 4

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
#include <interface/TextControl.h>
#include <interface/View.h>
#include <interface/Window.h>
//-------------------------------------------

extern "C" _EXPORT void loaddata(unsigned char *,size_t,BView *);
extern "C" _EXPORT unsigned char* savedata(size_t *,BView *);
extern "C" _EXPORT void messaging(BMessage *,BView *);
extern "C" _EXPORT const char description[] = RAW_DESCRIPTION;

static const uint32 kWidthChanged = (uint32)-100;
static const int32 kMostWidth = 4096;

//	Small pictures are drawn enlarged by a whole number of pixels, up to about
//	this size, and very large ones are shrunk to fit.
static const float kEnlargeTo = 128;
static const float kShrinkFrom = 400;

//	Holds the resource's bytes and the picture made from them at the width given.
class RawPreview : public BView {
	public:
		RawPreview(const unsigned char *newdata,size_t newsize) : BView("preview",B_WILL_DRAW) {
			data = NULL;
			size = 0;
			bitmap = NULL;
			scale = 1;
			width = 0;
			height = 0;
			if (newdata != NULL && newsize > 0) {
				data = new unsigned char[newsize];
				memcpy(data,newdata,newsize);
				size = newsize;
			}
			SetSizes();
		}
		~RawPreview() {
			delete [] data;
			delete bitmap;
		}
		void AttachedToWindow() {
			if (Parent() != NULL)
				SetViewColor(Parent()->ViewColor());
		}

		//	How many bytes make a row of this width.
		static size_t RowBytes(int32 across) {
#if RAW_FORMAT == RAW_GRAY1
			return (across + 7) / 8;
#elif RAW_FORMAT == RAW_RGB32
			return (size_t)(across) * 4;
#else
			return (size_t)(across);
#endif
		}

		//	A width to start with: the side of a square when the pixels make
		//	one, else the first usual width that makes whole rows.
		int32 GuessWidth() const {
#if RAW_FORMAT == RAW_GRAY1
			size_t pixels = size * 8;
#elif RAW_FORMAT == RAW_RGB32
			size_t pixels = size / 4;
#else
			size_t pixels = size;
#endif
			if (pixels == 0)
				return 16;
			int32 side = (int32)(floor(sqrt((double)(pixels)) + 0.5));
			if (side > 0 && (size_t)(side) * side == pixels)
				return side;
			static const int32 usual[] = {32,16,64,48,24,128,256,12,8};
			for (size_t i = 0; i < sizeof(usual) / sizeof(usual[0]); i++) {
				if (RowBytes(usual[i]) <= size && size % RowBytes(usual[i]) == 0)
					return usual[i];
			}
			return side > 0 ? side : 16;
		}

		//	Makes the picture at this width. Returns false when there is nothing
		//	to show: no bytes, or fewer than a row of them.
		bool SetWidth(int32 across) {
			delete bitmap;
			bitmap = NULL;
			scale = 1;
			width = across;
			height = 0;
			if (data != NULL && across > 0 && across <= kMostWidth) {
				size_t rowbytes = RowBytes(across);
				height = (int32)(size / rowbytes);
				if (height > 0 && height <= kMostWidth) {
					bitmap = new BBitmap(BRect(0,0,across - 1,height - 1),B_RGBA32);
					if (bitmap->InitCheck() == B_OK) {
						Convert(rowbytes);
					} else {
						delete bitmap;
						bitmap = NULL;
					}
				}
			}
			if (bitmap != NULL) {
				float biggest = across > height ? across : height;
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
			SetDrawingMode(B_OP_ALPHA);
			SetBlendingMode(B_PIXEL_ALPHA,B_ALPHA_OVERLAY);
			DrawBitmap(bitmap,bitmap->Bounds(),shown);
			SetDrawingMode(B_OP_COPY);
			SetHighColor(ui_color(B_CONTROL_BORDER_COLOR));
			StrokeRect(shown.InsetByCopy(-1,-1));
		}
		const unsigned char *Data() const {return data;}
		size_t Size() const {return size;}
		bool HasPicture() const {return bitmap != NULL;}
		int32 Width() const {return width;}
		int32 Height() const {return height;}
		float Scale() const {return scale;}
	private:
		//	Turns the raw pixels into 32 bit ones with an alpha channel, so that
		//	they can be drawn whatever the screen is. B_RGBA32 is blue, green,
		//	red, alpha in memory.
		void Convert(size_t rowbytes) {
#if RAW_FORMAT == RAW_CMAP8
			const color_map *map = system_colors();
#endif
			unsigned char *out = (unsigned char *)(bitmap->Bits());
			int32 outrow = bitmap->BytesPerRow();
			for (int32 y = 0; y < height; y++) {
				const unsigned char *in = data + (size_t)(y) * rowbytes;
				unsigned char *to = out + (size_t)(y) * outrow;
				for (int32 x = 0; x < width; x++) {
					unsigned char red = 0,green = 0,blue = 0,alpha = 255;
#if RAW_FORMAT == RAW_CMAP8
					if (in[x] == B_TRANSPARENT_8_BIT) {
						alpha = 0;
					} else {
						red = map->color_list[in[x]].red;
						green = map->color_list[in[x]].green;
						blue = map->color_list[in[x]].blue;
					}
#elif RAW_FORMAT == RAW_GRAY8
					red = green = blue = in[x];
#elif RAW_FORMAT == RAW_GRAY1
					red = green = blue = (in[x / 8] & (0x80 >> (x % 8))) ? 0 : 255;
#else
					blue = in[x * 4];
					green = in[x * 4 + 1];
					red = in[x * 4 + 2];
					alpha = in[x * 4 + 3];
#endif
					to[x * 4] = blue;
					to[x * 4 + 1] = green;
					to[x * 4 + 2] = red;
					to[x * 4 + 3] = alpha;
				}
			}
		}
		//	The size the picture is drawn at, in device pixels.
		BRect ShownRect() const {
			if (bitmap == NULL)
				return BRect(1,1,64,64);
			return BRect(1,1,1 + width * scale - 1,1 + height * scale - 1);
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
		int32 width;
		int32 height;
};

//	Says what the picture is made of, and how it is shown.
static void update_status(BView *bkgview) {
	RawPreview *preview = (RawPreview *)(bkgview->FindView("preview"));
	BStringView *status = (BStringView *)(bkgview->FindView("status"));
	BStringView *detail = (BStringView *)(bkgview->FindView("detail"));
	if (preview == NULL || status == NULL || detail == NULL)
		return;
	char text[200];
	char more[200];
	more[0] = '\0';
	if (preview->Size() == 0) {
		strcpy(text,"No pixels");
	} else if (!preview->HasPicture()) {
		sprintf(text,"%lu bytes, no picture at this width",(unsigned long)(preview->Size()));
	} else {
		size_t used = (size_t)(preview->Height()) * RawPreview::RowBytes(preview->Width());
		sprintf(text,"%ld x %ld pixels, %lu bytes",(long)(preview->Width()),
			(long)(preview->Height()),(unsigned long)(preview->Size()));
		if (used < preview->Size())
			sprintf(more,"%lu bytes at the end do not fill a row. ",
				(unsigned long)(preview->Size() - used));
		if (preview->Scale() > 1)
			sprintf(more + strlen(more),"Shown at %ldx",(long)(preview->Scale()));
		else if (preview->Scale() < 1)
			strcat(more,"Shown reduced to fit");
	}
	status->SetText(text);
	detail->SetText(more);
}

void loaddata(unsigned char *data,size_t length,BView *bkgview) {
	RawPreview *preview = new RawPreview(data,length);
	int32 guess = preview->GuessWidth();
	preview->SetWidth(guess);

	char text[20];
	sprintf(text,"%ld",(long)(guess));
	BTextControl *across = new BTextControl("across","Width:",text,new BMessage(kWidthChanged));
	across->SetModificationMessage(new BMessage(kWidthChanged));
	across->SetTarget(bkgview->Window());
	BStringView *status = new BStringView("status","");
	BStringView *detail = new BStringView("detail","");
	BStringView *hint = new BStringView("hint","The width is not stored: set it so the picture lines up");

	BGroupLayout *top = new BGroupLayout(B_VERTICAL,8);
	bkgview->SetLayout(top);
	top->SetInsets(10,10,10,10);
	top->AddView(preview);
	top->AddView(across);
	top->AddView(status);
	top->AddView(detail);
	top->AddView(hint);
	for (int32 i = 0; i < 5; i++) {
		if (i != 1)
			top->ItemAt(i)->SetExplicitAlignment(BAlignment(B_ALIGN_HORIZONTAL_CENTER,B_ALIGN_TOP));
	}
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
	RawPreview *preview = (RawPreview *)(bkgview->FindView("preview"));
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
	if (message->what == kWidthChanged) {
		RawPreview *preview = (RawPreview *)(bkgview->FindView("preview"));
		BTextControl *across = (BTextControl *)(bkgview->FindView("across"));
		if (preview == NULL || across == NULL)
			return;
		preview->SetWidth((int32)(atol(across->Text())));
		update_status(bkgview);
		//	The picture changes size, so the window follows it
		BSize min = bkgview->MinSize();
		BSize size = bkgview->PreferredSize();
		if (!(size.width >= min.width))
			size.width = min.width;
		if (!(size.height >= min.height))
			size.height = min.height;
		bkgview->Window()->ResizeTo(size.width,size.height);
	}
}
