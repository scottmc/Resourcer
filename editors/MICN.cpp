//------------Includes----------------------
#include <image.h>
#include <interface/Window.h>
#include <stdio.h>
#include <interface/CheckBox.h>
#include <interface/ColorControl.h>
#include <interface/GroupLayout.h>
#include <interface/Size.h>
#include <Screen.h>
//-------------------------------------------

extern "C" _EXPORT void loaddata(unsigned char *,size_t,BView *);
extern "C" _EXPORT unsigned char* savedata(size_t *,BView *);
extern "C" _EXPORT void messaging(BMessage *,BView *);
extern "C" _EXPORT const char description[] = "16x16 Icon";

class pixel : public BView {
	public:
	pixel(BRect rect, char *name, unsigned char color) : BView(rect,name,B_FOLLOW_NONE,B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE | B_FRAME_EVENTS) {
		state = color;
		if (state == B_TRANSPARENT_8_BIT) {
			SetViewColor(255,255,255);
			SetHighColor(0,127,255);
			FillRect(BRect(0,0,16,16),B_MIXED_COLORS);
		} else {
			SetViewColor(BScreen().ColorForIndex(state));
		}
		Invalidate();
	}
	void MouseDown(BPoint point);
	void Draw(BRect updateRect);
	unsigned char state;
};

class icon : public BView {
	public:
	icon(unsigned char *data) : BView(BRect(0,0,256,256),"icon",B_FOLLOW_NONE,B_WILL_DRAW | B_FRAME_EVENTS | B_FULL_UPDATE_ON_RESIZE) {
		int row = 0;
		pixel *pix;
		for (int i = 0;!((i == 16) && (row == 15)); i++) {
			if (i == 16) {
				row++;
				i = 0;
			}
			pix = new pixel(BRect(i*16,row*16,(i*16)+16,(row*16)+16),NULL,data[i + (row *16)]);
			AddChild(pix);
			pix->Invalidate();
		}
		//	The grid is a fixed 257 pixel square: the pixels are drawn in
		//	device pixels, so the layout must not stretch or shrink it.
		SetExplicitMinSize(BSize(256,256));
		SetExplicitPreferredSize(BSize(256,256));
		SetExplicitMaxSize(BSize(256,256));
	}
};

void pixel::MouseDown(BPoint point) {
		rgb_color col = ((BColorControl *)(Window()->ChildAt(0)->FindView("iconcolors")))->ValueAsColor();
		BScreen screen;
		uint8 x = screen.IndexForColor(col);
		col = screen.ColorForIndex(x);
		if (((BCheckBox *)(Window()->ChildAt(0)->FindView("Tp")))->Value() == 1) {
			x = B_TRANSPARENT_8_BIT;
			SetViewColor(255,255,255);
			SetHighColor(0,127,255);
			FillRect(BRect(0,0,16,16),B_MIXED_COLORS);
		} else {
			SetViewColor(col);
		}
		state = x;
		Invalidate();
	}

void pixel::Draw(BRect updateRect) {
		if (state == B_TRANSPARENT_8_BIT) {
			SetViewColor(255,255,255);
			SetHighColor(0,127,255);
			FillRect(BRect(0,0,16,16),B_MIXED_COLORS);
		} else {
			SetViewColor(BScreen().ColorForIndex(state));
		}
	}

/*class BIView : public BView {
	public:
		BIView(BBitmap *map) : BView(BRect(0,0,31,31),"icon",B_FOLLOW_NONE,B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE) {bits = map;}
		void Draw(BRect updateRect) {
			if (Window() != NULL) {
				Window()->Lock();
				DrawBitmap(bits);
				Window()->Unlock();
			}
		}
	private:
		BBitmap *bits;
	};*/

void loaddata(unsigned char *data,size_t length,BView *bkgview) {//runs when add-on starts, window is initialized, you are expected to write to it: rect is 300 x 300
	//BBitmap *bits = new BBitmap(BRect(0,0,31,31),B_CMAP8,false,true);
	//memcpy(bits->Bits(),data + sizeof(TranslatorBitmap),length);
	//BIView *v = new BIView(bits);
	if (length == 0) {
		data = new unsigned char[16*16];
		for (int i = 0; i < (16*16); i++) {
			data[i] = B_TRANSPARENT_8_BIT;
		}
	}
	icon *v = new icon(data);
	BCheckBox *transparent = new BCheckBox("Tp","Transparent",new BMessage(-100));
	BColorControl *colors = new BColorControl(BPoint(0,0),B_CELLS_32x8,8,"iconcolors");
	//	The grid keeps its pixel size; the check box and the colour control follow
	//	the font.
	BGroupLayout *top = new BGroupLayout(B_VERTICAL,5);
	bkgview->SetLayout(top);
	top->SetInsets(4,4,4,4);
	top->AddView(v);
	top->AddView(transparent);
	top->AddView(colors);
	top->ItemAt(0)->SetExplicitAlignment(BAlignment(B_ALIGN_LEFT,B_ALIGN_TOP));
	top->ItemAt(1)->SetExplicitAlignment(BAlignment(B_ALIGN_LEFT,B_ALIGN_TOP));

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

unsigned char* savedata(size_t *length,BView *bkgview) { //return data, clean up, and set sizeto the size of data
	BView *icon = bkgview->FindView("icon");
	unsigned char *data = new unsigned char[16*16];
	for (int i = 0; i < (16*16); i++) {
		data[i] = ((pixel *)(icon->ChildAt(i)))->state;
	}
	*length = 16*16;
	return data;
}

void messaging(BMessage *message,BView *bkgview) { //receives messages from window with negative 'what' members or B_REFS_RECEIVED messages
}