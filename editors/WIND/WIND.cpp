//------------------------------------------------------------------------------
//	WIND.cpp
//
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------
#include <math.h>
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
#include <interface/CardLayout.h>
#include <interface/CheckBox.h>
#include <interface/ColorControl.h>
#include <interface/ListItem.h>
#include <interface/MenuBar.h>
#include <interface/MenuField.h>
#include <interface/MenuItem.h>
#include <interface/OutlineListView.h>
#include <interface/PopUpMenu.h>
#include <interface/RadioButton.h>
#include <interface/TextControl.h>
#include <interface/Screen.h>
#include <interface/ScrollBar.h>
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
		InterfaceListView(float width) : BOutlineListView(BRect(20,20,width - 20 - B_V_SCROLL_BAR_WIDTH,200),"elements",B_SINGLE_SELECTION_LIST,B_FOLLOW_ALL_SIDES) {}
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

// The row of three buttons above the cards, one for each card, each a third of
// the window width.  They switch the visible card, and the button of the card
// showing is the window's default button, which draws it with a heavier border.
class CardButtons : public BView
{
	public:
		CardButtons(BRect frame, BCardLayout *layout, const char **labels, int32 count)
			:	BView(frame, "cardbuttons", B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP, B_FRAME_EVENTS),
				fLayout(layout),
				fCount(count),
				fCurrent(0)
		{
			SetViewColor(216,216,216);
			for (int32 i = 0; i < count && i < kMaxCards; i++)
			{
				BMessage *message = new BMessage(kShowCard);
				message->AddInt32("card", i);
				fButtons[i] = new BButton(BRect(0,0,10,10), labels[i], labels[i], message);
				AddChild(fButtons[i]);
			}
		}

		// The height the buttons ask for with the current font
		static float PreferredHeight()
		{
			BButton probe(BRect(0,0,10,10), "probe", "Elements", NULL);
			float width, height;
			probe.GetPreferredSize(&width, &height);
			return ceilf(height);
		}

		void AttachedToWindow()
		{
			BView::AttachedToWindow();
			for (int32 i = 0; i < fCount && i < kMaxCards; i++)
				fButtons[i]->SetTarget(this);
			PlaceButtons();
			ShowCard(fCurrent);
		}

		void FrameResized(float width, float height)
		{
			BView::FrameResized(width, height);
			PlaceButtons();
		}

		void MessageReceived(BMessage *message)
		{
			int32 card;
			if (message->what == kShowCard && message->FindInt32("card", &card) == B_OK)
				ShowCard(card);
			else
				BView::MessageReceived(message);
		}

		void ShowCard(int32 index)
		{
			if (index < 0 || index >= fCount)
				return;
			fCurrent = index;
			fLayout->SetVisibleItem(index);
			if (Window() != NULL)
			{
				for (int32 i = 0; i < fCount && i < kMaxCards; i++)
				{
					if (i != index)
						fButtons[i]->MakeDefault(false);
				}
				fButtons[index]->MakeDefault(true);
			}
		}

	private:
		static const int32 kMaxCards = 3;
		static const uint32 kShowCard = 'card';

		// Thirds of the width, the last one takes what is left over
		void PlaceButtons()
		{
			float width = Bounds().Width() + 1;
			float height = Bounds().Height();
			for (int32 i = 0; i < fCount && i < kMaxCards; i++)
			{
				float left = floorf(i * width / fCount);
				float right = (i == fCount - 1) ? width : floorf((i + 1) * width / fCount);
				fButtons[i]->MoveTo(left, 0);
				fButtons[i]->ResizeTo(right - left - 1, height);
			}
		}

		BCardLayout	*fLayout;
		int32		fCount;
		int32		fCurrent;
		BButton		*fButtons[kMaxCards];
};

// The Elements card.  The list and the instruction text are placed from the
// panel's own size whenever it changes, instead of relying on follow modes:
// the card layout and the window resize the panel after it is built, and
// follow modes then stretch the list over the instruction text.
class ElementsPanel : public BView
{
	public:
		ElementsPanel(BRect frame)
			:	BView(frame, "viewtoolbar", B_FOLLOW_ALL_SIDES, B_FRAME_EVENTS),
				fList(NULL),
				fInstruction(NULL)
		{
			SetViewColor(216,216,216);
		}

		void SetContent(BView *list, BStringView *instruction)
		{
			fList = list;
			fInstruction = instruction;
			AddChild(fList);
			AddChild(fInstruction);
			// No PlaceViews() yet: resizing a scroll view that is not on screen
			// leaves its list and scroll bar behind at the old size (their
			// follow modes only work once attached), so wait for AllAttached().
		}

		void AllAttached()
		{
			BView::AllAttached();
			PlaceViews();
		}

		void FrameResized(float width, float height)
		{
			BView::FrameResized(width, height);
			PlaceViews();
		}

	private:
		void PlaceViews()
		{
			if (fList == NULL || fInstruction == NULL)
				return;

			const float padding = 20;
			BRect bounds = Bounds();
			float textheight = fInstruction->Frame().Height();
			float textwidth = fInstruction->Frame().Width();

			// instruction text along the bottom, centred
			float texttop = bounds.bottom - padding / 2 - textheight;
			fInstruction->MoveTo(floorf((bounds.Width() - textwidth) / 2), texttop);

			// list fills the rest with the same padding on both sides
			float listheight = texttop - padding / 2 - padding;
			if (listheight < 40)
				listheight = 40;
			fList->MoveTo(padding, padding);
			fList->ResizeTo(bounds.Width() - 2 * padding, listheight);
		}

		BView		*fList;
		BStringView	*fInstruction;
};

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
	// (Not B_WILL_ACCEPT_FIRST_CLICK: the window server then never activates
	// this window when it is clicked, so its text fields could not be typed
	// into and the window being edited kept the focus.)
	bkgview->Window()->SetFlags(B_NOT_H_RESIZABLE | B_NOT_ZOOMABLE |
								B_NOT_MINIMIZABLE);
	bkgview->Window()->SetLook(B_FLOATING_WINDOW_LOOK);

	//-------------Size the window for the widest text-----
	// Works for any plain font size: take the widest of the Window tab, the
	// Elements tab instruction and the row of tab labels.
	font_height fh;
	be_plain_font->GetHeight(&fh);
	float fontheight = ceilf(fh.ascent + fh.descent);
	const char *instruction = "Drag a List Item to the Window to Add It";
	float instructionwidth = be_plain_font->StringWidth(instruction);
	float need = WindowEditor::PreferredWidth();
	if (instructionwidth + 40 > need)
		need = instructionwidth + 40;
	float width = ceilf(need);
	if (width < 250)
		width = 250;

	// A row of buttons along the top flips between the cards, which fill the
	// rest of the window.
	float buttonheight = CardButtons::PreferredHeight();
	float gap = 4;

	// Smallest height that still shows the title, look and behavior fields,
	// the flags box heading and six flag rows.  Everything scales with the
	// plain font size, so it also fits at small and large fonts.
	float fieldrow = fontheight + 14 + 8;
	float flagrow = fontheight + 8;
	float panelheight = ceilf(10 + 3 * fieldrow + (fontheight + 6)
		+ 6 * flagrow + 12 + 10);
	float height = buttonheight + gap + panelheight;
	float panelwidth = width;
	bkgview->Window()->SetSizeLimits(width,width,height,32767);
	bkgview->Window()->ResizeTo(width,height);

	static const char *cardlabels[3] = { "Focus", "Window", "Elements" };
	BView *cards = new BView(BRect(0,buttonheight + gap,width,buttonheight + gap + panelheight),"cards",B_FOLLOW_ALL_SIDES,B_WILL_DRAW);
	cards->SetViewColor(216,216,216);
	BCardLayout *cardlayout = new BCardLayout();
	cards->SetLayout(cardlayout);

	editpanel = new EditingPanel;
	editpanel->ResizeTo(panelwidth,panelheight);
	cardlayout->AddView(editpanel);

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
	cardlayout->AddView(wineditor);

	//-------------Init Toolbar---------------------------
	ElementsPanel *y = new ElementsPanel(BRect(0,0,panelwidth,panelheight));
	BStringView *instructionview = new BStringView(BRect(0,0,instructionwidth,fontheight+2),"instructions",instruction);
	instructionview->SetViewColor(216,216,216);
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
	BScrollView *scroller = new BScrollView("scroller",list,B_FOLLOW_LEFT | B_FOLLOW_TOP,0,false,true);
	y->SetContent(scroller,instructionview);
	cardlayout->AddView(y);

	// Three buttons, one per card, across the top
	CardButtons *buttons = new CardButtons(BRect(0,0,width,buttonheight - 1),cardlayout,cardlabels,3);
	bkgview->AddChild(buttons);
	bkgview->AddChild(cards);
}
	
	
// The window editor's own controls send their messages to the Focus View panel
// and the windows it edits, so none reach the editor window.  The function is
// exported and has to exist, though: without it Resourcer is left with no
// pointer to call, and crashes on the first message that reaches it.
void messaging(BMessage* message, BView* bkgview)
{
}


unsigned char* savedata(size_t* length, BView* bkgview)
{
	BMessage *archive = new BMessage;
	// All three cards stay attached to the window (the card layout only hides
	// the ones not showing), so the editor can be asked to archive directly.
	WindowEditor* WinEditor = dynamic_cast<WindowEditor*>(bkgview->FindView("wineditpanel"));
	if (!WinEditor)
	{
		// Without the window editor, we're basically up a creek
		debugger("No window editor found");
	}
	if (WinEditor->ArchiveWindow(archive) != B_OK)
	{
		(new BAlert("yikes", "Archive is failing" B_UTF8_ELLIPSIS, "OK"))->Go();
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

