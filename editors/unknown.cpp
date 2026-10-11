//------------Includes----------------------
#include <image.h>
#include <interface/Alert.h>
#include <stdio.h>
#include <string.h>
#include <interface/GroupLayout.h>
#include <interface/Size.h>
#include <interface/SpaceLayoutItem.h>
#include <interface/Window.h>
#include <interface/TextView.h>
#include <interface/ScrollView.h>
#include <interface/Button.h>
#include <interface/Font.h>
#include <interface/StatusBar.h>
#include <interface/StringView.h>
#include <interface/TextControl.h>
#include <storage/FilePanel.h>
#include <storage/File.h>
#include <support/String.h>
#include <support/Beep.h>
//-------------------------------------------

#define PAGE 256

char hexd[16];

BFilePanel *panel;
typedef struct {
	BView *bkgview;
	entry_ref *ref;
} winmessage;

char* hexbyte(unsigned char byte);
void dehex(const char *data,size_t length,unsigned char *toreturn);
void seek0s(char *seekin,size_t length);
long openfile(void *m);
class HEXView;
class ASCIIView;
class AddressView;

extern "C" _EXPORT void loaddata(unsigned char *,size_t,BView *);
extern "C" _EXPORT unsigned char* savedata(size_t *,BView *);
extern "C" _EXPORT void messaging(BMessage *,BView *);
//	Tells Resourcer that this editor wants a resizable window
extern "C" _EXPORT const bool resizable = true;

class ContainerView;
ContainerView *ownerof(BView *view);

class ContainerView : public BView {
	public:
		ContainerView(unsigned char *data, size_t length);
		void GoToPage(int32 page,bool refresh = true);
		void UseFixedFont(void);
		size_t CopyTo(void **into);
		HEXView *HEX;
		ASCIIView *ASCII;
		AddressView *Address;
		unsigned char *res;
		size_t size;
		int32 curPage;
};

//	A text view that can be told to wrap after a fixed number of characters, so
//	that with a fixed-width font each line holds one row of 16 bytes: 32 hex
//	characters or 16 text characters. Zero columns means wrap at the view's width.
class FixedWrapView : public BTextView {
	public:
		FixedWrapView(const char *name,uint32 flags) : BTextView(name,flags) {cols = 0;}
		void SetColumns(int columns) {
			cols = columns;
			FixWrap();
		}
		void FrameResized(float width,float height) {
			BTextView::FrameResized(width,height);
			FixWrap();
		}
	private:
		void FixWrap(void) {
			if (cols <= 0)
				return;
			BFont font;
			GetFont(&font);
			float cw = font.StringWidth("M");
			BRect rect = TextRect();
			rect.right = rect.left + cols * cw + cw / 2;
			SetTextRect(rect);
		}
		int cols;
};

class HEXView : public FixedWrapView {
	public:
		HEXView(const char *name,uint32 flags) : FixedWrapView(name,flags) {prcsed =false;wasdown = false;}
		void InsertText(const char *text, int32 length, int32 offset, const text_run_array *runs);
		void DeleteText(int32 start,int32 finish);
		bool prcsed;
		bool wasdown;
		int32 tempos;
};

class ASCIIView : public FixedWrapView {
	public:
		ASCIIView(const char *name,uint32 flags) : FixedWrapView(name,flags) {prcsed =false;}
		void InsertText(const char *text, int32 length, int32 offset, const text_run_array *runs) {
			for (int i = 0;i <length;i++) {
				if ((text[i] == 0) || (text[i] == '\n'))
					BTextView::InsertText(new char(1),1,offset+i,runs);
				else
					BTextView::InsertText(text+i,1,offset+i,runs);
			}
			if (prcsed == false) {
			if (length > 1) {
				char *hex;
				char *hexed = new char[length*2+1];
				for (int i = 0; i < length; i++) {
					hex = hexbyte(text[i]);
					hexed[i*2] = hex[0];
					hexed[(i*2)+1] = hex[1];
				}
				hexed[length*2] = 0;
				ownerof(this)->HEX->prcsed = true;
				ownerof(this)->HEX->Insert(offset*2,hexed,length*2,runs);
				ownerof(this)->HEX->prcsed = false;
				delete hexed;
			} else {
				ownerof(this)->HEX->prcsed = true;
				ownerof(this)->HEX->Insert(offset*2,hexbyte(uint8(*text)),length*2,runs);
				ownerof(this)->HEX->prcsed = false;
			}
			}
		}
		void DeleteText(int32 start,int32 finish) {
			BTextView::DeleteText(start,finish);
			if (prcsed == false) {
			ownerof(this)->HEX->prcsed = true;
			ownerof(this)->HEX->Delete(start*2,finish*2);
			ownerof(this)->HEX->prcsed = false;
			}
		}
		bool prcsed;
};



/*inline void HEXView::Highlight(int32 start,int32 finish) {
	if (prcsed == false) {
		ownerof(this)->ASCII->prcsed = true;
		int32 a,b;
		//ownerof(this)->ASCII->SelectAll();
		ownerof(this)->ASCII->Select(start/2,finish/2);
		//ownerof(this)->ASCII->Highlight(start/2,finish/2);
		ownerof(this)->ASCII->prcsed = false;
	} else {
		BTextView::Highlight(start,finish);
	}
}*/

void HEXView::InsertText(const char *text, int32 length, int32 offset, const text_run_array *runs) {
	BString str(text);
	str.ToUpper();
	strcpy((char *)(text),str.String());
	bool isvalid = true;
	char *x = new char[length];
	char *y = new char[length+1];
	for (int i = 0; i < length; i++) {
		switch (text[i]) {
			case '0':
				break;
			case '1':
				break;
			case '2':
				break;
			case '3':
				break;
			case '4':
				break;
			case '5':
				break;
			case '6':
				break;
			case '7':
				break;
			case '8':
				break;
			case '9':
				break;
			case 'A':
				break;
			case 'B':
				break;
			case 'C':
				break;
			case 'D':
				break;
			case 'E':
				break;
			case 'F':
				break;
			default:
				isvalid = false;
				break;
		}
	}
	if (isvalid == true) {
		//if ((length % 2) != 0)
		//	BTextView::InsertText("0",1,length + offset,runs);
		/*if ((offset % 2) == 0) 
			isend = true;*/
		BTextView::InsertText(text,length,offset,runs);
		/*if (isend) {
			BTextView::InsertText("0",1,length + offset,runs);
			strncpy(y,text,length);
			dehex(y,length,(unsigned char *)(x));
		} else {
			dehex(text,length,(unsigned char *)(x));
		}*/
		if (prcsed == false) {
			if (ownerof(this)->ASCII != NULL) {
				ownerof(this)->ASCII->prcsed = true;
				char *dehexed = new char[strlen(Text()) + 1];
				dehex(Text(),strlen(Text()),(unsigned char *)(dehexed));
				seek0s(dehexed,strlen(Text())/2);
				ownerof(this)->ASCII->SetText(dehexed);
				ownerof(this)->ASCII->prcsed = false;
			}
		}
	} else {
		Select(offset-1,offset-1);
		beep();
	}
	delete x;
	delete y;
}
void HEXView::DeleteText(int32 start,int32 finish) {
	//if ((((finish-start) % 2) != 0) && ((finish % 2) == 0))
	//	Insert(finish,"0",1);
	//if ((finish % 2) == 0)
	//	finish++;
	BTextView::DeleteText(start,finish);
	if (prcsed == false) {
			if (ownerof(this)->ASCII != NULL) {
				ownerof(this)->ASCII->prcsed = true;
				char *dehexed = new char[strlen(Text()) + 1];
				dehex(Text(),strlen(Text()),(unsigned char *)(dehexed));
				seek0s(dehexed,strlen(Text()) /2);
				ownerof(this)->ASCII->SetText(dehexed);
				ownerof(this)->ASCII->prcsed = false;
			}
		}
}

void seek0s(char *seekin,size_t length) {
	for (size_t i = 0;i < length;i++) {
		if (seekin[i] == 0)
			seekin[i] = 1;
	}
	seekin[length] = 0;
}

class AddressView : public BTextControl {
  public:
	AddressView(ContainerView *parent) : BTextControl("address","Current Page: ","0",new BMessage(-200)) {
		//	Room for the label and a few digits, no more
		SetExplicitMaxSize(BSize(be_plain_font->StringWidth(Label()) + be_plain_font->StringWidth("00000") + 30,B_SIZE_UNSET));
	}
};

//	The text views sit inside scroll views, so the container is found by name.
ContainerView *ownerof(BView *view) {
	BView *parent = view->Parent();
	while (parent != NULL && strcmp(parent->Name(),"hex") != 0)
		parent = parent->Parent();
	return (ContainerView *)(parent);
}

ContainerView::ContainerView(unsigned char *data, size_t length) : BView("hex",B_WILL_DRAW) {
		SetViewColor(216,216,216);
		HEX = new HEXView("HEXView",B_FRAME_EVENTS | B_WILL_DRAW | B_PULSE_NEEDED | B_NAVIGABLE);
		ASCII = new ASCIIView("ASCIIView",B_FRAME_EVENTS | B_WILL_DRAW | B_PULSE_NEEDED | B_NAVIGABLE);
		BScrollView *hexscroll = new BScrollView("HEXScroll",HEX,B_WILL_DRAW | B_FRAME_EVENTS,false,true,B_FANCY_BORDER);
		BScrollView *asciiscroll = new BScrollView("ASCIIScroll",ASCII,B_WILL_DRAW | B_FRAME_EVENTS,false,true,B_FANCY_BORDER);
		res = data;
		size = length;
		curPage = 0;
		Address = new AddressView(this);
		char string[20];
		sprintf(string," / %lu",(unsigned long)(length/PAGE));
		//	The page controls run along the top, the two views share the rest of
		//	the room, the hex view getting twice the width of the text view as it
		//	shows two characters per byte.
		BGroupLayout *top = new BGroupLayout(B_VERTICAL,5);
		SetLayout(top);
		BGroupLayout *controls = new BGroupLayout(B_HORIZONTAL,8);
		BGroupLayout *views = new BGroupLayout(B_HORIZONTAL,8);
		top->AddItem(controls);
		top->AddItem(views);
		controls->AddView(new BButton("back","Previous",new BMessage(-400)));
		controls->AddView(Address);
		controls->AddView(new BStringView("total_blocks",string));
		controls->AddItem(BSpaceLayoutItem::CreateGlue());
		controls->AddView(new BButton("next","Next",new BMessage(-300)));
		views->AddView(hexscroll,2);
		views->AddView(asciiscroll,1);
		//	A page is 256 bytes: 16 per line, so 32 hex characters per line
		UseFixedFont();
		//	The container is complete, so the two views can fill themselves in
		if (length < PAGE) {
			ASCII->SetText((char *)(data),length);
			ASCII->SetMaxBytes(PAGE);
		} else {
			ASCII->SetMaxBytes(PAGE);
			ASCII->SetText((char *)(data),PAGE);
		}
		HEX->SetMaxBytes(PAGE * 2);
	}
	
//	Both views use the fixed font, wrapping after one row of 16 bytes (32 hex
//	characters, 16 text characters) so the two line up row for row. That row is
//	also the narrowest the views can be without cutting the rows off.
void ContainerView::UseFixedFont(void) {
	HEX->SetFontAndColor(be_fixed_font);
	ASCII->SetFontAndColor(be_fixed_font);
	HEX->SetColumns(32);
	ASCII->SetColumns(16);
	font_height height;
	be_fixed_font->GetHeight(&height);
	float line = height.ascent + height.descent + height.leading;
	float cw = be_fixed_font->StringWidth("M");
	BView *hexscroll = FindView("HEXScroll");
	BView *asciiscroll = FindView("ASCIIScroll");
	if (hexscroll != NULL) {
		hexscroll->SetExplicitMinSize(BSize(cw * 32.5 + 30,B_SIZE_UNSET));
		hexscroll->SetExplicitPreferredSize(BSize(cw * 32.5 + 30,line * 16));
	}
	if (asciiscroll != NULL) {
		asciiscroll->SetExplicitMinSize(BSize(cw * 16.5 + 30,B_SIZE_UNSET));
		asciiscroll->SetExplicitPreferredSize(BSize(cw * 16.5 + 30,line * 16));
	}
	InvalidateLayout();
}

void ContainerView::GoToPage(int32 page,bool refresh) {
	if (page * PAGE > size) {
		beep();
		return;
	}
	if (refresh) {
		unsigned char *yikes = new unsigned char[strlen(HEX->Text()) /2];
		dehex(HEX->Text(),strlen(HEX->Text()),yikes);
		memcpy(res + (curPage * PAGE),yikes,strlen(HEX->Text()) /2);
		if ((curPage + 1) * PAGE > size)
			size = (curPage * PAGE) + (strlen(HEX->Text())/2);
	}
	if ((page + 1) * PAGE > size)
		ASCII->SetText(((char *)(res) + (page * PAGE)),size - (page * PAGE));
	else
		ASCII->SetText(((char *)(res) + (page * PAGE)),PAGE);
	curPage = page;
	//	New text may not carry the fixed font
	HEX->SetFontAndColor(be_fixed_font);
	ASCII->SetFontAndColor(be_fixed_font);
}

size_t ContainerView::CopyTo(void **into) {
	GoToPage(0,true);
	*into = new unsigned char[size];
	memcpy(*into,res,size);
	return size;
}

char* hexbyte(unsigned char byte) {
	unsigned char high;
	static char string[3];
	high = byte/16;
	string[0] = hexd[high];
	string[1] = hexd[byte-high*16];
	return string;
}

void dehex(const char *data,size_t length,unsigned char *toreturn) {
	int x;
	char hexd[20];
	int high;
	char left;       // left part of the hex-value
	int low;
	char right;
	char* count;
	int i;
	int j;
	j = 0;
	hexd[1] = '0'; hexd[2] = '1'; hexd[3] = '2'; hexd[4] = '3'; hexd[5] = '4'; hexd[6] = '5'; hexd[7] = '6'; hexd[8] = '7'; hexd[9] = '8'; hexd[10] = '9';hexd[11]='A'; hexd[12]='B'; hexd[13]='C'; hexd[14]='D'; hexd[15]='E'; hexd[16]='F';
	for (count = (char *)(data);count < (data + length);count+=2) {
		left = *count;
		right = *(count + 1);
		for (i = 1;i <= 16;i++) {
			if (hexd[i] == left) {
				left = i-1;
				break;
			}
		}
		high=left*16;              //1
		for (i = 1;i <= 16;i++) {
			if (hexd[i] == right) {
				right = i-1;
				break;
			}
		}
		low = right;
		x = high + low;
		*(toreturn + j) = x;
		j++;
	}
} 
long openfile(void *m) {
	winmessage *ma = (winmessage *)(m);
	BView *bkgview = ma->bkgview;
	BFile file(ma->ref,B_READ_ONLY);
	unsigned char *data;
	if (file.InitCheck() != B_OK) {
		const char *text;
		switch (file.InitCheck()) {
			case B_BAD_VALUE:
				text = "The file panel was sent an incorrect value and the file could not be opened.";
				break;
			case B_ENTRY_NOT_FOUND:
				text = "The file does not exist and so the file could not be opened.";
				break;
			case B_PERMISSION_DENIED:
				text = "You do not have permission to open this file.";
				break;
			case B_NO_MEMORY:
				text = "There was not enough memory to open the file.";
				break;
			default:
				text = "The file could not be opened because of an unknown error.";
				break;
			}
		if ((new BAlert("ahh!",text,"Try Again","Cancel",NULL,B_WIDTH_FROM_WIDEST,B_STOP_ALERT))->Go() == 0) {
			bkgview->Window()->PostMessage(-100);
		}
		return 0;
		}
	size_t size;
	off_t size2;
	file.GetSize(&size2);
	size = size2;
	data = new unsigned char[size];
	if (data == NULL) {
		(new BAlert("ahh!","There was not enough memory to open the file.","OK",NULL,NULL,B_WIDTH_FROM_WIDEST,B_STOP_ALERT))->Go();
		return 0;
	}
	file.Read(data,size);
	bkgview->Window()->Lock();
	((ContainerView *)(bkgview->FindView("hex")))->res = data;
	((ContainerView *)(bkgview->FindView("hex")))->size = size;
	((ContainerView *)(bkgview->FindView("hex")))->curPage = 0;
	((ContainerView *)(bkgview->FindView("hex")))->GoToPage(0,false);
	char *string = new char[20];
	sprintf(string," / %lu",(unsigned long)(size / PAGE));
	((BStringView *)(bkgview->FindView("hex")->FindView("total_blocks")))->SetText(string);
	delete string;
	bkgview->Window()->Unlock();
	return(0);
	}
	


void loaddata(unsigned char *data,size_t length,BView *bkgview) {//runs when add-on starts, window is initialized, you are expected to write to it: rect is 300 x 300
	hexd[0] = '0'; hexd[1] = '1'; hexd[2] = '2'; hexd[3] = '3'; hexd[4] = '4'; hexd[5] = '5'; hexd[6] = '6'; hexd[7] = '7'; hexd[8] = '8'; hexd[9] = '9';hexd[10]='A'; hexd[11]='B'; hexd[12]='C'; hexd[13]='D'; hexd[14]='E'; hexd[15]='F';
	panel = new BFilePanel(B_OPEN_PANEL,new BMessenger(bkgview->Window()),NULL,0,false);
	ContainerView *container = new ContainerView(data,length);
	BButton *insert = new BButton("file","Insert File...",new BMessage(-100));
	BGroupLayout *top = new BGroupLayout(B_VERTICAL,10);
	bkgview->SetLayout(top);
	top->SetInsets(10,10,10,10);
	top->AddView(container);
	top->AddView(insert);
	top->ItemAt(1)->SetExplicitAlignment(BAlignment(B_ALIGN_HORIZONTAL_CENTER,B_ALIGN_BOTTOM));
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

unsigned char* savedata(size_t *length,BView *bkgview) { //return data, clean up, and set sizeto the size of data
	unsigned char *toreturn;
	/*if (((ContainerView *)(bkgview->FindView("hex")))->size <= 256) {
		*length = strlen(((ContainerView *)(bkgview->FindView("hex")))->HEX->Text()) / 2;
		toreturn = new unsigned char[*length];
		dehex(((ContainerView *)(bkgview->FindView("hex")))->HEX->Text(),strlen(((ContainerView *)(bkgview->FindView("hex")))->HEX->Text()),toreturn);
	} else {*/
		*length = ((ContainerView *)(bkgview->FindView("hex")))->CopyTo((void **)(&toreturn));
	//}
	return toreturn;
}

void messaging(BMessage *message,BView *bkgview) { //receives messages from window with negative 'what' members or B_REFS_RECEIVED messages
	int32 newPage;
	char hoober[35];
	switch(message->what) {
		case (uint32)-100:
			panel->Show();
			break;
		case (uint32)-200:
			{
				long page;
				if (sscanf(((ContainerView *)(bkgview->FindView("hex")))->Address->Text(),"%ld",&page) != 1 || page < 0)
					break;
				newPage = page;
			}
			((ContainerView *)(bkgview->FindView("hex")))->GoToPage(newPage);
			break;
		case (uint32)-300:
			((ContainerView *)(bkgview->FindView("hex")))->GoToPage(((ContainerView *)(bkgview->FindView("hex")))->curPage + 1);
			sprintf(hoober,"%ld",(long)((ContainerView *)(bkgview->FindView("hex")))->curPage);
			((ContainerView *)(bkgview->FindView("hex")))->Address->SetText(hoober);
			break;
		case (uint32)-400:
			((ContainerView *)(bkgview->FindView("hex")))->GoToPage(((ContainerView *)(bkgview->FindView("hex")))->curPage - 1);
			sprintf(hoober,"%ld",(long)((ContainerView *)(bkgview->FindView("hex")))->curPage);
			((ContainerView *)(bkgview->FindView("hex")))->Address->SetText(hoober);
			break;
		case B_REFS_RECEIVED:
			winmessage *m = new winmessage;
			m->ref = new entry_ref;
			message->FindRef("refs",m->ref);
			m->bkgview = bkgview;
			openfile(m);
			break;
	}
}
