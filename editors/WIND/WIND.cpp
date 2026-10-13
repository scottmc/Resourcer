//------------------------------------------------------------------------------
//	WIND.cpp
//
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------
#include <stdio.h>
#include <string.h>

// System Includes -------------------------------------------------------------
#include <app/Application.h>
#include <app/Message.h>
#include <app/MessageFilter.h>

#include <interface/Alert.h>
#include <interface/Bitmap.h>
#include <interface/Box.h>
#include <interface/Button.h>
#include <interface/ControlLook.h>
#include <interface/CheckBox.h>
#include <interface/ColorControl.h>
#include <interface/ListItem.h>
#include <interface/MenuBar.h>
#include <interface/MenuField.h>
#include <interface/MenuItem.h>
#include <interface/OutlineListView.h>
#include <interface/PopUpMenu.h>
#include <interface/RadioButton.h>
#include <interface/TabView.h>
#include <interface/TextControl.h>
#include <interface/Screen.h>
#include <interface/ScrollView.h>
#include <interface/Slider.h>
#include <interface/StringView.h>
#include <interface/View.h>
#include <interface/Window.h>

#include <kernel/image.h>

#include <storage/Resources.h>

#include <support/Beep.h>
#include <support/ClassInfo.h>

#include <translation/TranslationUtils.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "DragOutlineListView.h"
#include "EditingControl.h"
#include "EditingPanel.h"
#include "EditingWindow.h"
#include "Filters.h"
#include "WIND.h"
#include "WindowEditor.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------
extern "C" _EXPORT void loaddata(unsigned char *,size_t,BView *);
extern "C" _EXPORT unsigned char* savedata(size_t *,BView *);
extern "C" _EXPORT void messaging(BMessage *,BView *);
extern "C" _EXPORT const char description[] = "Window";
extern "C" _EXPORT BResources *file;
extern "C" _EXPORT image_id me;

#if __POWERPC__
#else
int32 flipcode (int32 original)
{
	int32 type = original;
	char *one = (char *)(&original);
	char *two = one + 1;
	char *three = two + 1;
	char *four = three + 1;
	*((char *)(&type)) = *four;
	*((char *)(&type) + 1) = *three;
	*((char *)(&type) + 2) = *two;
	*((char *)(&type) + 3) = *one;
	return type;
}
#endif
BResources *file;
image_id me;
BResources *resources;
BMessage *widemsg;

class EditingWindow;

BMessage *current;
EditingWindow *editingwin;
WindowEditor *wineditor;
BWindow *mainwin;
window_feel danger_feel = B_NORMAL_WINDOW_FEEL;
int32 track_over(void *view);

const pattern STIPPLED = {{240,240,240,240,15,15,15,15}};


class InterfaceItem : public BStringItem
{
	public:
		// If preview is given, it is archived instead and only used to draw
		// the drag image (see InterfaceListView::InitiateDrag).
		InterfaceItem(const char *label,BView *tocreate,BView *preview = NULL) : BStringItem(label)
		{
			what_a_drag = new BMessage;
			what_a_preview = NULL;
			tocreate->ResizeToPreferred();
			tocreate->Archive(what_a_drag,true);
			drag_size = tocreate->Bounds();
			delete tocreate;	
			if (preview != NULL)
			{
				what_a_preview = new BMessage;
				preview->ResizeToPreferred();
				preview->Archive(what_a_preview,true);
				delete preview;
			}
		}

		~InterfaceItem()
		{
			delete what_a_drag;
			delete what_a_preview;
		}

		BMessage *what_a_drag;
		BMessage *what_a_preview;
		BRect drag_size;
};

class InterfaceListView : public BOutlineListView
{
	public:
		InterfaceListView(float width) : BOutlineListView(BRect(20,20,width - 20 - B_V_SCROLL_BAR_WIDTH,200),"elements") {}
		bool InitiateDrag(BPoint /*point*/, int32 index, bool /*wasSelected*/)
		{
			if (ItemAt(index)->OutlineLevel() != 1)
				return false;
			BRect temp = ((InterfaceItem *)(ItemAt(index)))->drag_size;
			BMessage *msg = ((InterfaceItem *)(ItemAt(index)))->what_a_drag;
			BBitmap *bits = new BBitmap(temp,BScreen(Window()).ColorSpace(),true);
			InterfaceItem *item = (InterfaceItem *)(ItemAt(index));
			BMessage *pmsg = item->what_a_preview != NULL ? item->what_a_preview : msg;
			BView *view = (BView *)(instantiate_object(pmsg));
			bits->AddChild(view);
			view->Window()->Lock();
			view->Draw(view->Bounds());
			view->Sync();
			if (view->Window()->IsLocked())
				view->Window()->Unlock();
			DragMessage(msg,bits,BPoint(0,0));
			return true;
		}
};

int32 track_over(void *view) {
	DragOutlineListView *lview = (DragOutlineListView *)(view);
	lview->WatchMouse();
	return B_OK;
}

BCheckBox *menubox;
EditingPanel *editpanel;

uint32 make_bitmask(void);
bool startingup;

//------------------------------------------------------------------------------
void loaddata(unsigned char* data,size_t length, BView* bkgview)
{
	resources = file;
	startingup = false;
	mainwin = bkgview->Window();

	//-------------Init Focus View & Window Options------
	bkgview->Window()->SetFlags(B_NOT_H_RESIZABLE | B_NOT_ZOOMABLE |
								B_NOT_MINIMIZABLE | B_WILL_ACCEPT_FIRST_CLICK);
	bkgview->Window()->SetLook(B_FLOATING_WINDOW_LOOK);

	//-------------Size the window for the widest text-----
	// Works for any plain font size: take the widest of the Window tab, the
	// Elements tab instruction and the row of tab labels.
	font_height fh;
	be_plain_font->GetHeight(&fh);
	float fontheight = ceilf(fh.ascent + fh.descent);
	const char *instruction = "Drag a List Item to the Window to Add It";
	float instructionwidth = be_plain_font->StringWidth(instruction);
	// BTabView does not resize the tab views, so measure what it leaves for
	// them: a throwaway tab view shows the border around its container.
	BTabView *probe = new BTabView(BRect(0,0,300,300),"probe",B_WIDTH_FROM_WIDEST);
	float borderwidth = 300 - probe->ContainerView()->Bounds().Width();
	delete probe;

	float need = WindowEditor::PreferredWidth();
	if (instructionwidth + 40 > need)
		need = instructionwidth + 40;
	float width = need + borderwidth;
	// BTabView with B_WIDTH_FROM_WIDEST makes every tab as wide as the widest
	// label plus padding (see BTabView::TabFrame).
	float tabpad = ceilf(be_control_look->DefaultLabelSpacing() * 3.3f);
	float tabwidest = be_plain_font->StringWidth("Focus View");
	if (be_plain_font->StringWidth("Window") > tabwidest)
		tabwidest = be_plain_font->StringWidth("Window");
	if (be_plain_font->StringWidth("Elements") > tabwidest)
		tabwidest = be_plain_font->StringWidth("Elements");
	float tabstrip = 2 * be_control_look->ComposeSpacing(B_USE_WINDOW_SPACING)
		+ 3 * (tabwidest + tabpad);
	if (tabstrip > width)
		width = tabstrip;
	width = ceilf(width);
	if (width < 250)
		width = 250;
	float height = 350;
	if (fontheight * 17 > height)
		height = ceilf(fontheight * 17);

	bkgview->Window()->ResizeTo(width,height);
	BTabView *tabs = new BTabView(BRect(0,0,width,height),"tabs",B_WIDTH_FROM_WIDEST);
	// The tab views are made as big as the container the tab view gives them
	BRect panel = tabs->ContainerView()->Bounds();
	float panelwidth = panel.Width();
	float panelheight = panel.Height();
	editpanel = new EditingPanel;
	tabs->AddTab(editpanel);
	tabs->TabAt(0)->SetLabel("Focus View");

	//-------------Init Editable Window------------------
	if (length == 0)
	{
		wineditor = new WindowEditor(BRect(0, 0, panelwidth, panelheight), "wineditpanel",
									 mainwin, editpanel);
	}
	else
	{
		wineditor = new WindowEditor(BRect(0, 0, panelwidth, panelheight), "wineditpanel",
									 mainwin, editpanel, data);
	}
	tabs->AddTab(wineditor);
	tabs->TabAt(1)->SetLabel("Window");

	//-------------Init Toolbar---------------------------
	BView *y = new BView(BRect(0,0,panelwidth,panelheight),"viewtoolbar",B_FOLLOW_ALL_SIDES,0);
	y->SetViewColor(216,216,216);
	y->AddChild(new BStringView(BRect(20,210,20+instructionwidth,210+fontheight+2),"instructions",instruction,B_FOLLOW_TOP | B_FOLLOW_H_CENTER));
	BOutlineListView *list = new InterfaceListView(panelwidth);
	BStringItem *controls = new BStringItem("Controls");
	BStringItem *visuals = new BStringItem("Visual Organizers");
	BStringItem *editable = new BStringItem("Editable Items");
		list->AddItem(controls);
			list->AddUnder(new InterfaceItem("Button",new BButton(BRect(0,0,100,50),"newbutton","New Button",new BMessage('none'))),controls);
			list->AddUnder(new InterfaceItem("Checkbox",new BCheckBox(BRect(0,0,100,50),"newcheckbox","New Checkbox",new BMessage('none'))),controls);
			list->AddUnder(new InterfaceItem("Radio Button",new BRadioButton(BRect(0,0,100,50),"newrbutton","New Radio Button",new BMessage('none'))),controls);
			list->AddUnder(new InterfaceItem("Slider",new BSlider(BRect(0,0,100,50),"newslider","New Slider",new BMessage('none'),0,100)),controls);
			// An empty menu makes the dragged preview crash (double free of a
			// menu item label) when the preview bitmap is deleted, so give it
			// an item.
			BPopUpMenu *tempmenu = new BPopUpMenu("newpopupmenu");
			tempmenu->AddItem(new BMenuItem("Item",new BMessage('none')));
			BMenuField *tempx = new BMenuField(BRect(0,0,100,50),"newpopupmenu","New Pop-Up Menu: ",tempmenu);
			tempx->SetDivider(be_plain_font->StringWidth(tempx->Label()));
			// Deleting an unarchived BMenuField (the drag preview) crashes with
			// a double free on Haiku, so the drag image uses a plain text
			// view instead of a live pop-up menu.
			list->AddUnder(new InterfaceItem("Pop-Up Menu",tempx,new BStringView(BRect(0,0,100,50),"newpopupmenupreview","New Pop-Up Menu")),controls);
			list->AddUnder(new InterfaceItem("Color Picker",new BColorControl(BPoint(0,0),B_CELLS_32x8,8,"newcolpicker",new BMessage('none'))),controls);
		list->AddItem(visuals);
			list->AddUnder(new InterfaceItem("Static Text",new BStringView(BRect(0,0,100,50),"newstatictext","Static Text")),visuals);
//			list->AddUnder(new InterfaceItem("Resource Text",new ResourceStringView()),visuals);
			list->AddUnder(new InterfaceItem("Box",new BBox(BRect(0,0,100,50),"newbox")),visuals);
//			list->AddUnder(new InterfaceItem("Bitmap",new ResImageView(resources)),visuals);
		list->AddItem(editable);
			list->AddUnder(new InterfaceItem("Text Box",new BTextView(BRect(0,0,100,50),"newtextbox",BRect(5,5,95,45),B_FOLLOW_LEFT | B_FOLLOW_TOP,B_WILL_DRAW | B_NAVIGABLE | B_PULSE_NEEDED)),editable);
			BTextControl *tempy = new BTextControl(BRect(0,0,100,50),"newtextfield","Text Field ",NULL,new BMessage('none'));
			tempy->SetDivider(be_plain_font->StringWidth(tempy->Label()));
			list->AddUnder(new InterfaceItem("Text Field",tempy),editable);
	// Same padding on both sides: 20 from the left and right edge.
	BScrollView *scroller = new BScrollView("scroller",list,B_FOLLOW_ALL_SIDES,0,false,true);
	scroller->MoveTo(20,20);
	scroller->ResizeTo(panelwidth - 40,182);
	y->AddChild(scroller);
	tabs->AddTab(y);
	tabs->TabAt(2)->SetLabel("Elements");
	bkgview->AddChild(tabs);
}
	
	
unsigned char* savedata(size_t* length, BView* bkgview)
{
	BMessage *archive = new BMessage;
	BTabView* Tabs = dynamic_cast<BTabView*>(bkgview->FindView("tabs"));
	if (!Tabs)
	{
		// Without the tabs, we're basically up a creek
		debugger("No tabview found");
	}
	// Because of the massive lameness of BTabView, we have to do
	// a bait and switch if the "Window" is not the currently
	// selected tab.  There is the remote possibility that the user
	// will select another tab while we're saving, but we'll hope
	// that's negligible
	WindowEditor* WinEditor = dynamic_cast<WindowEditor*>(Tabs->TabAt(1)->View());
	if (!WinEditor)
	{
		// Here again, without the window editor, we're in a bad way
		debugger("No window editor found");
	}
	bool detach = false;
	if (!WinEditor->Parent())
	{
		detach = true;
		WinEditor->Hide();
		mainwin->AddChild(WinEditor);
	}
	if (WinEditor->ArchiveWindow(archive) != B_OK)
	{
		(new BAlert("yikes", "Archive is failing" B_UTF8_ELLIPSIS, "OK"))->Go();
	}
	if (detach)
	{
		WinEditor->RemoveSelf();
	}
	BMallocIO buffer;
	ssize_t xyz;
	archive->Flatten(&buffer,&xyz);
	*length = xyz;
	unsigned char *data = new unsigned char[*length];
	memcpy(data,buffer.Buffer(),*length);
	delete archive;
//	editingwin->Quit();
	//(new BAlert("helo","savedata executing properly.","OK"))->Go();
	return data;
}

/*
 * $Log $
 *
 * $Id  $
 *
 */

