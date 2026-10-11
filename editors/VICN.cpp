//------------Includes----------------------
#include <image.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <fs_attr.h>
#include <app/Message.h>
#include <app/Messenger.h>
#include <app/Roster.h>
#include <interface/Alert.h>
#include <interface/Bitmap.h>
#include <interface/Button.h>
#include <interface/GroupLayout.h>
#include <interface/IconUtils.h>
#include <interface/Size.h>
#include <interface/StringView.h>
#include <interface/View.h>
#include <interface/Window.h>
#include <storage/File.h>
#include <storage/NodeInfo.h>
#include <storage/Resources.h>
#include <support/TypeConstants.h>
//-------------------------------------------

//	A vector icon is stored as its flattened HVIF data, the same bytes an
//	application keeps in a 'VICN' resource (named "BEOS:ICON", id 101) or a
//	file keeps in its "BEOS:ICON" attribute, and what a .hvif file contains.
//	They start with the magic "ncif".

extern "C" _EXPORT void loaddata(unsigned char *,size_t,BView *);
extern "C" _EXPORT unsigned char* savedata(size_t *,BView *);
extern "C" _EXPORT void messaging(BMessage *,BView *);
extern "C" _EXPORT const char description[] = "Vector Icon (HVIF)";

//	Icon-O-Matic has a small protocol for editing icon data owned by another
//	application (headers/libs/icon/IconEditorProtocol.h in the Haiku sources).
//	The values are repeated here as that is not a public header.
//	B_EDIT_ICON_DATA: "reply to" (B_MESSENGER_TYPE) is told about every save with
//	B_ICON_DATA_EDITED, which carries the new "icon data" (B_VECTOR_ICON_TYPE).
//	"icon data" in the request is optional, without it a new icon is started.
static const uint32 kEditIconData = 'EICN';
static const uint32 kIconDataEdited = 'ICNE';
static const char *kIconOMaticSignature = "application/x-vnd.haiku-icon_o_matic";

static const uint32 kEditMessage = (uint32)-100;
static const char *kIconAttribute = "BEOS:ICON";

//	The preview shows the icon at the three sizes Tracker uses.
static const int kSizes[3] = {64,32,16};
static const float kGap = 12;
static const float kPreviewWidth = 64 + 32 + 16 + 2 * 12;
static const float kPreviewHeight = 64;

class IconPreview : public BView {
	public:
		IconPreview() : BView("preview",B_WILL_DRAW) {
			data = NULL;
			size = 0;
			valid = false;
			for (int i = 0; i < 3; i++)
				bitmaps[i] = NULL;
			//	Icons are drawn in device pixels, so the preview keeps its size
			//	whatever the font is.
			SetExplicitMinSize(BSize(kPreviewWidth - 1,kPreviewHeight - 1));
			SetExplicitPreferredSize(BSize(kPreviewWidth - 1,kPreviewHeight - 1));
			SetExplicitMaxSize(BSize(kPreviewWidth - 1,kPreviewHeight - 1));
		}
		~IconPreview() {
			delete [] data;
			for (int i = 0; i < 3; i++)
				delete bitmaps[i];
		}
		void AttachedToWindow() {
			if (Parent() != NULL)
				SetViewColor(Parent()->ViewColor());
		}
		//	Takes a copy of the data and renders it. Returns false, keeping the
		//	data, when it can not be read as a vector icon.
		bool SetData(const uint8 *newdata,size_t newsize) {
			delete [] data;
			data = NULL;
			size = 0;
			for (int i = 0; i < 3; i++) {
				delete bitmaps[i];
				bitmaps[i] = NULL;
			}
			valid = false;
			if (newdata != NULL && newsize > 0) {
				data = new uint8[newsize];
				memcpy(data,newdata,newsize);
				size = newsize;
				valid = true;
				for (int i = 0; i < 3 && valid; i++) {
					bitmaps[i] = new BBitmap(BRect(0,0,kSizes[i] - 1,kSizes[i] - 1),B_RGBA32);
					if (bitmaps[i]->InitCheck() != B_OK
						|| BIconUtils::GetVectorIcon(data,size,bitmaps[i]) != B_OK)
						valid = false;
				}
				if (!valid) {
					for (int i = 0; i < 3; i++) {
						delete bitmaps[i];
						bitmaps[i] = NULL;
					}
				}
			}
			if (Window() != NULL)
				Invalidate();
			return valid;
		}
		void Draw(BRect updateRect) {
			if (!valid) {
				//	nothing to show: an empty frame the size of the biggest icon
				SetHighColor(ui_color(B_CONTROL_BORDER_COLOR));
				StrokeRect(BRect(0,0,kSizes[0] - 1,kSizes[0] - 1));
				return;
			}
			SetDrawingMode(B_OP_ALPHA);
			SetBlendingMode(B_PIXEL_ALPHA,B_ALPHA_OVERLAY);
			float left = 0;
			for (int i = 0; i < 3; i++) {
				DrawBitmap(bitmaps[i],BPoint(left,kSizes[0] - kSizes[i]));
				left += kSizes[i] + kGap;
			}
			SetDrawingMode(B_OP_COPY);
		}
		const uint8 *Data() const {return data;}
		size_t Size() const {return size;}
		bool IsValid() const {return valid;}
	private:
		uint8 *data;
		size_t size;
		bool valid;
		BBitmap *bitmaps[3];
};

static void alert(const char *text) {
	BAlert *box = new BAlert("vicn",text,"OK",NULL,NULL,B_WIDTH_AS_USUAL,B_STOP_ALERT);
	box->Go(NULL);		//	asynchronous: this runs in the window's thread
}

//	Describes the state of the icon under the preview.
static void update_status(BView *bkgview) {
	IconPreview *preview = (IconPreview *)(bkgview->FindView("preview"));
	BStringView *status = (BStringView *)(bkgview->FindView("status"));
	if (preview == NULL || status == NULL)
		return;
	char text[100];
	if (preview->Size() == 0)
		strcpy(text,"No icon yet");
	else if (!preview->IsValid())
		sprintf(text,"%lu bytes, not valid icon data",(unsigned long)(preview->Size()));
	else
		sprintf(text,"%lu bytes",(unsigned long)(preview->Size()));
	status->SetText(text);
}

//	Looks for icon data in a dropped file: the file itself when it is an HVIF
//	file, otherwise the vector icon resource of an application, otherwise the
//	icon attribute of the file. Returns the data in a new[] buffer.
static unsigned char *icon_from_file(const entry_ref *ref,size_t *length) {
	BFile file(ref,B_READ_ONLY);
	if (file.InitCheck() != B_OK)
		return NULL;
	off_t filesize = 0;
	file.GetSize(&filesize);

	//	an HVIF file
	char magic[4];
	if (filesize >= 4 && file.ReadAt(0,magic,4) == 4 && memcmp(magic,"ncif",4) == 0) {
		unsigned char *data = new unsigned char[filesize];
		if (file.ReadAt(0,data,filesize) != filesize) {
			delete [] data;
			return NULL;
		}
		*length = filesize;
		return data;
	}

	//	an application, or any file that has resources, with a vector icon
	BResources resources;
	if (resources.SetTo(&file) == B_OK) {
		int32 id = 0;
		size_t size = 0;
		//	the application icon first, then whatever vector icon there is
		bool found = resources.GetResourceInfo(B_VECTOR_ICON_TYPE,kIconAttribute,&id,&size);
		if (!found) {
			const char *resourcename = NULL;
			found = resources.GetResourceInfo(B_VECTOR_ICON_TYPE,(int32)0,&id,&resourcename,&size);
		}
		if (found) {
			const void *loaded = resources.LoadResource(B_VECTOR_ICON_TYPE,id,&size);
			if (loaded != NULL && size > 0) {
				unsigned char *data = new unsigned char[size];
				memcpy(data,loaded,size);
				*length = size;
				return data;
			}
		}
	}

	//	a file with an icon attribute
	attr_info info;
	if (file.GetAttrInfo(kIconAttribute,&info) == B_OK && info.type == B_VECTOR_ICON_TYPE && info.size > 0) {
		unsigned char *data = new unsigned char[info.size];
		if (file.ReadAttr(kIconAttribute,B_VECTOR_ICON_TYPE,0,data,info.size) == info.size) {
			*length = info.size;
			return data;
		}
		delete [] data;
	}
	return NULL;
}

//	Takes the icon of a dropped file. Returns true when there was one.
static bool load_ref(const entry_ref *ref,BView *bkgview) {
	IconPreview *preview = (IconPreview *)(bkgview->FindView("preview"));
	if (preview == NULL)
		return false;
	size_t length = 0;
	unsigned char *data = icon_from_file(ref,&length);
	if (data == NULL) {
		//	Icon-O-Matic's own documents are not HVIF and can not be read here
		size_t name = strlen(ref->name);
		if (name > 4 && strcasecmp(ref->name + name - 4,".iom") == 0)
			alert("That is an Icon-O-Matic document, which can not be read here. Open it in Icon-O-Matic, use Export As to save it as HVIF, and drop that file instead.");
		else
			alert("No vector icon was found in that file. Drop an HVIF file, an application, or a file that has an icon.");
		return false;
	}
	bool good = preview->SetData(data,length);
	delete [] data;
	if (!good)
		alert("The file does not contain valid vector icon data.");
	update_status(bkgview);
	return good;
}

//	Asks Icon-O-Matic to edit the icon. Every save there is sent back to the
//	window as B_ICON_DATA_EDITED.
static void edit_icon(BView *bkgview) {
	IconPreview *preview = (IconPreview *)(bkgview->FindView("preview"));
	BMessage message(kEditIconData);
	message.AddMessenger("reply to",BMessenger(bkgview->Window()));
	if (preview != NULL && preview->IsValid())
		message.AddData("icon data",B_VECTOR_ICON_TYPE,preview->Data(),preview->Size());
	status_t result = be_roster->Launch(kIconOMaticSignature,&message);
	if (result != B_OK && result != B_ALREADY_RUNNING)
		alert("Icon-O-Matic could not be started.");
}

void loaddata(unsigned char *data,size_t length,BView *bkgview) {
	IconPreview *preview = new IconPreview();
	if (length != 0 && data != NULL)
		preview->SetData(data,length);

	BStringView *status = new BStringView("status","");
	BStringView *hint = new BStringView("hint","Drop an HVIF file or an application here");
	BButton *edit = new BButton("edit","Edit in Icon-O-Matic",new BMessage(kEditMessage));

	//	Show Icon-O-Matic's own icon on the button, as Tracker does
	entry_ref app;
	if (be_roster->FindApp(kIconOMaticSignature,&app) == B_OK) {
		BBitmap *icon = new BBitmap(BRect(0,0,15,15),B_RGBA32);
		if (icon->InitCheck() == B_OK && BNodeInfo::GetTrackerIcon(&app,icon,B_MINI_ICON) == B_OK)
			edit->SetIcon(icon);
		delete icon;
	}

	BGroupLayout *top = new BGroupLayout(B_VERTICAL,8);
	bkgview->SetLayout(top);
	top->SetInsets(10,10,10,10);
	top->AddView(preview);
	top->AddView(status);
	top->AddView(hint);
	top->AddView(edit);
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

unsigned char* savedata(size_t *length,BView *bkgview) {
	IconPreview *preview = (IconPreview *)(bkgview->FindView("preview"));
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
		case kEditMessage:
			edit_icon(bkgview);
			break;
		case kIconDataEdited:
			{
				//	a save in Icon-O-Matic
				const void *data;
				ssize_t size;
				IconPreview *preview = (IconPreview *)(bkgview->FindView("preview"));
				if (preview != NULL && message->FindData("icon data",B_VECTOR_ICON_TYPE,&data,&size) == B_OK) {
					preview->SetData((const uint8 *)(data),size);
					update_status(bkgview);
				}
			}
			break;
		case B_SIMPLE_DATA:
		case B_REFS_RECEIVED:
			{
				//	files dropped on the window; the first one with an icon wins
				entry_ref ref;
				for (int32 i = 0; message->FindRef("refs",i,&ref) == B_OK; i++) {
					if (load_ref(&ref,bkgview))
						break;
				}
			}
			break;
	}
}
