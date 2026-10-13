//------------------------------------------------------------------------------
//	WindowEditor.cpp
//
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------
#include <interface/Box.h>
#include <interface/CheckBox.h>
#include <interface/MenuField.h>
#include <interface/MenuItem.h>
#include <interface/PopUpMenu.h>
#include <interface/ScrollBar.h>
#include <interface/ScrollView.h>
#include <interface/TextControl.h>
#include <math.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "EditingWindow.h"
#include "WindowEditor.h"
#include "WIND.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

struct LabelMessagePair
{
	int32		message;
	const char*	label;
};

LabelMessagePair WindowLooks[] =
{
	{ B_DOCUMENT_WINDOW_LOOK,	"Document Window"	},
	{ B_TITLED_WINDOW_LOOK,		"Titled Window"		},
	{ B_FLOATING_WINDOW_LOOK,	"Floating Window"	},
	{ B_MODAL_WINDOW_LOOK,		"Modal Window"		},
	{ B_BORDERED_WINDOW_LOOK,	"Bordered Window"	},
	{ B_NO_BORDER_WINDOW_LOOK,	"Borderless Window"	},
	{ 0, 0 }
};

LabelMessagePair WindowFeels[] =
{
	{ B_NORMAL_WINDOW_FEEL,			"Normal"						},
	{ B_MODAL_APP_WINDOW_FEEL,		"Modal Across Application"		},
	{ B_MODAL_ALL_WINDOW_FEEL,		"Modal Across System"			},
	{ B_FLOATING_APP_WINDOW_FEEL,	"Floating Across Application"	},
	{ B_FLOATING_ALL_WINDOW_FEEL,	"Floating Across System"		},
	{ 0, 0 }
};

struct FlagFlipNameLabelQuad
{
	int32		flag;
	bool		flip;
	const char*	name;
	const char* label;
};

FlagFlipNameLabelQuad WindowFlags[] =
{
	{ B_NOT_MOVABLE,				false,	"nmov",		"Movable"								},
	{ B_NOT_CLOSABLE,				false,	"clos",		"Closable"								},
	{ B_NOT_ZOOMABLE,				false,	"zoom",		"Zoomable"								},
	{ B_NOT_MINIMIZABLE,			false,	"mini",		"Minimizable"							},
	{ B_NOT_H_RESIZABLE,			false,	"hres",		"Horizontally Resizable"				},
	{ B_NOT_V_RESIZABLE,			false,	"vres",		"Vertically Resizable"					},
	{ (int32)MSG_WINDOW_ADD_MENU,			false,	"menus",	"Menu Bar"								},
	{ B_OUTLINE_RESIZE,				true,	"roiw",		"Resize with Outline Instead of Window"	},
	{ B_WILL_ACCEPT_FIRST_CLICK,	true,	"wafc",		"Will Accept First Click"				},
	{ B_AVOID_FRONT,				true,	"avfr",		"Avoid Front"							},
	{ B_AVOID_FOCUS,				true,	"avfo",		"Avoid Focus"							},
	{ B_NO_WORKSPACE_ACTIVATION,	true,	"nwoa",		"Do Not Activate Workspace"				},
	{ B_NOT_ANCHORED_ON_ACTIVATE,	true,	"brcu",		"Bring Window To Current Workspace"		},
	{ B_ASYNCHRONOUS_CONTROLS,		true,	"async",	"Asynchronous Controls (Should Be On)"	},
	{ 0, 0, 0, 0 }
};

//------------------------------------------------------------------------------
// The view that holds the window flag checkboxes in a single column.  It sits
// inside a BScrollView and keeps the vertical scroll bar in step with its size.
class FlagsContent : public BView
{
	public:
		FlagsContent(BRect frame)
			:	BView(frame, "flagscontent", B_FOLLOW_ALL_SIDES,
					  B_FRAME_EVENTS),
				fContentHeight(0),
				fStep(1)
		{
			SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
		}

		void SetContentSize(float height, float step)
		{
			fContentHeight = height;
			fStep = step;
		}

		void AttachedToWindow()
		{
			BView::AttachedToWindow();
			UpdateScrollBar();
		}

		void FrameResized(float width, float height)
		{
			BView::FrameResized(width, height);
			UpdateScrollBar();
		}

	private:
		void UpdateScrollBar()
		{
			BScrollBar* bar = ScrollBar(B_VERTICAL);
			if (bar == NULL)
				return;

			float visible = Bounds().Height();
			float range = fContentHeight - visible;
			if (range < 0)
				range = 0;
			bar->SetRange(0, range);
			if (fContentHeight > 0)
				bar->SetProportion(visible / fContentHeight);
			bar->SetSteps(fStep, visible > fStep ? visible - fStep : fStep);
		}

		float	fContentHeight;
		float	fStep;
};
//------------------------------------------------------------------------------
WindowEditor::WindowEditor(BRect frame, const char* name,
						   const BMessenger& MainWindow,
						   const BMessenger& EditPanel)
	:	BView(frame, name, B_FOLLOW_ALL_SIDES, 0),
		fInit(false)
{
	EditingWindow* ew = new EditingWindow;
	ew->SetMainWindow(MainWindow);
	ew->SetEditPanel(EditPanel);
	ew->Show();

	fEditWindow = ew;
	fWindowInfo.SetTo(ew);
}
//------------------------------------------------------------------------------
WindowEditor::WindowEditor(BRect frame, const char* name,
						   const BMessenger& MainWindow,
						   const BMessenger& EditPanel,
						   unsigned char* data)
	:	BView(frame, name, B_FOLLOW_ALL_SIDES, 0),
		fInit(false)
{
	BMessage current;
	current.Unflatten((char*)data);
	EditingWindow* ew = new EditingWindow(&current);
	ew->SetMainWindow(MainWindow);
	ew->SetEditPanel(EditPanel);
	ew->Show();

	fEditWindow = ew;
	fWindowInfo.SetTo(ew);
}
//------------------------------------------------------------------------------
WindowEditor::~WindowEditor()
{
	// The editing window lives in its own thread and its views' classes live
	// in this add-on.  Ask it to quit and wait until its thread has finished,
	// otherwise the add-on can be unloaded while the window is still being
	// destroyed (crash in BView::_Detach).
	BLooper *looper = NULL;
	thread_id thread = -1;
	if (fEditWindow.Target(&looper) != NULL && looper != NULL)
		thread = looper->Thread();
	fEditWindow.SendMessage(MSG_WINDOW_QUIT);
	if (thread >= 0)
	{
		status_t result;
		wait_for_thread(thread, &result);
	}
}
//------------------------------------------------------------------------------
void WindowEditor::AttachedToWindow()
{
	if (!fInit)
	{
		// Init() adds all the controls, so it must only run once; this is
		// called every time the tab is selected again.
		fInit = true;
		Init();
	}

	fLookField->Menu()->SetTargetForItems(this);
	fFeelField->Menu()->SetTargetForItems(this);
	fTitleText->SetTarget(this);
	for (uint32 i = 0; i < fFlagBoxes.size(); ++i)
	{
		fFlagBoxes[i]->SetTarget(this);
	}
}
//------------------------------------------------------------------------------
void WindowEditor::MessageReceived(BMessage *msg)
{
	BMessage fwd;
	switch (msg->what)
	{
		case MSG_WINDOW_SET_TITLE:
			fwd.what = msg->what;
			fwd.AddString("title", fTitleText->Text());
			fEditWindow.SendMessage(&fwd);
			break;

		case MSG_WINDOW_SET_LOOK:
			fEditWindow.SendMessage(msg);
			break;

		case MSG_WINDOW_SET_FLAG:
			fwd.what = msg->what;
			fwd.AddInt32("flags", MakeBitmask());
			fEditWindow.SendMessage(&fwd);
			break;

		case MSG_WINDOW_ADD_MENU:
		{
			BCheckBox* cbox;
			msg->FindPointer("source",(void **)(&cbox));
			fwd.what = msg->what;
			fwd.AddBool("addmenu", cbox->Value() == B_CONTROL_ON);
			fEditWindow.SendMessage(&fwd);
			break;
		}

		case MSG_WINDOW_SET_FEEL:
			fEditWindow.SendMessage(msg);
			break;

		default:
			BView::MessageReceived(msg);
			break;
	}
}
//------------------------------------------------------------------------------
status_t WindowEditor::ArchiveWindow(BMessage* archive)
{
	status_t err = B_OK;
	BMessage msg(MSG_WINDOW_ARCHIVE);
	err = msg.AddPointer("archive", (const void*)archive);
	if (!err)
	{
		err = fEditWindow.SendMessage(&msg, &msg);
		if (!err)
		{
			status_t tempErr;
			err = msg.FindInt32("err", &tempErr);
			if (!err)
			{
				err = tempErr;
			}
		}
	}

	return err;
}
//------------------------------------------------------------------------------
void WindowEditor::Init()
{
	SetViewColor(216, 216, 216);
	float width = Bounds().Width() - 20;
	float w, h;
	fTitleText = new BTextControl(BRect(10,10,10 + width,30),"title","Title: ",fWindowInfo.title.c_str(),new BMessage(MSG_WINDOW_SET_TITLE));
	fTitleText->SetDivider(be_plain_font->StringWidth("Title: "));
	fTitleText->GetPreferredSize(&w, &h);
	fTitleText->ResizeTo(width, h);
	AddChild(fTitleText);

	InitLookMenu();
	InitFeelMenu();
	InitFlagsBoxes();
}
//------------------------------------------------------------------------------
float WindowEditor::PreferredWidth()
{
	// Width needed by the widest member, for the current plain font size.
	font_height fh;
	be_plain_font->GetHeight(&fh);
	float em = ceilf(fh.ascent + fh.descent);
	float menuPad = 2 * em + 12;
	float widest = 0;
	float w;

	w = be_plain_font->StringWidth("Title: ") +
		be_plain_font->StringWidth("Untitled Window");
	if (w > widest)
		widest = w;

	float label = be_plain_font->StringWidth("Window Look: ");
	for (int i = 0; WindowLooks[i].label; ++i)
	{
		w = label + be_plain_font->StringWidth(WindowLooks[i].label) + menuPad;
		if (w > widest)
			widest = w;
	}

	label = be_plain_font->StringWidth("Window Behavior: ");
	for (int i = 0; WindowFeels[i].label; ++i)
	{
		w = label + be_plain_font->StringWidth(WindowFeels[i].label) + menuPad;
		if (w > widest)
			widest = w;
	}
	// margins either side of the fields
	widest += 20;

	// The check boxes sit in a scrolling view inside a labelled box, which
	// adds margins, the scroll bar and borders around the widest box.
	float widestBox = 0;
	for (int i = 0; WindowFlags[i].name; ++i)
	{
		BCheckBox temp(BRect(0, 0, 100, 10), "temp", WindowFlags[i].label, NULL);
		float h;
		temp.GetPreferredSize(&w, &h);
		if (w > widestBox)
			widestBox = w;
	}
	w = widestBox + 8 + B_V_SCROLL_BAR_WIDTH + 3 + 12 + 4 + 28;
	if (w > widest)
		widest = w;

	return ceilf(widest);
}
//------------------------------------------------------------------------------
void WindowEditor::InitLookMenu()
{
	BPopUpMenu *menu = new BPopUpMenu("Document Window");
	BMessage *msg;
	for (int i = 0; WindowLooks[i].label; ++i)
	{
		msg = new BMessage(MSG_WINDOW_SET_LOOK);
		msg->AddInt32("newlook", WindowLooks[i].message);
		menu->AddItem(new BMenuItem(WindowLooks[i].label, msg));
	}
#if 0
	msg = new BMessage(MSG_WINDOW_SET_LOOK);
	msg->AddInt32("newlook",B_TITLED_WINDOW_LOOK);
	menu->AddItem(new BMenuItem("Titled Window",msg));
	msg = new BMessage(MSG_WINDOW_SET_LOOK);
	msg->AddInt32("newlook",B_FLOATING_WINDOW_LOOK);
	menu->AddItem(new BMenuItem("Floating Window",msg));
	msg = new BMessage(MSG_WINDOW_SET_LOOK);
	msg->AddInt32("newlook",B_MODAL_WINDOW_LOOK);
	menu->AddItem(new BMenuItem("Modal Window",msg));
	msg = new BMessage(MSG_WINDOW_SET_LOOK);
	msg->AddInt32("newlook",B_BORDERED_WINDOW_LOOK);
	menu->AddItem(new BMenuItem("Bordered Window",msg));
	msg = new BMessage(MSG_WINDOW_SET_LOOK);
	msg->AddInt32("newlook",B_NO_BORDER_WINDOW_LOOK);
	menu->AddItem(new BMenuItem("Borderless Window",msg));
#endif
	switch (fWindowInfo.look)
	{
		case B_DOCUMENT_WINDOW_LOOK:
			menu->ItemAt(0)->SetMarked(true);
			break;
		case B_TITLED_WINDOW_LOOK:
			menu->ItemAt(1)->SetMarked(true);
			break;
		case B_FLOATING_WINDOW_LOOK:
			menu->ItemAt(2)->SetMarked(true);
			break;
		case B_MODAL_WINDOW_LOOK:
			menu->ItemAt(3)->SetMarked(true);
			break;
		case B_BORDERED_WINDOW_LOOK:
			menu->ItemAt(4)->SetMarked(true);
			break;
		case B_NO_BORDER_WINDOW_LOOK:
			menu->ItemAt(5)->SetMarked(true);
			break;
	}

	float h = fTitleText->Frame().Height();
	float top = fTitleText->Frame().bottom + 8;
	fLookField = new BMenuField(BRect(10,top,Bounds().Width() - 10,top + h),"look","Window Look: ",menu);
	fLookField->SetDivider(be_plain_font->StringWidth("Window Look: "));
	AddChild(fLookField);
}
//------------------------------------------------------------------------------
void WindowEditor::InitFeelMenu()
{
	BPopUpMenu* menu = new BPopUpMenu("Normal");
	BMessage* msg;
	for (int i = 0; WindowFeels[i].label; ++i)
	{
		msg = new BMessage(MSG_WINDOW_SET_FEEL);
		msg->AddInt32("newfeel", WindowFeels[i].message);
		menu->AddItem(new BMenuItem(WindowFeels[i].label, msg));
	}
#if 0
	msg->AddInt32("newfeel",B_NORMAL_WINDOW_FEEL);
	menu->AddItem(new BMenuItem("Normal",msg));
	msg = new BMessage(MSG_WINDOW_SET_FEEL);
	msg->AddInt32("newfeel",B_MODAL_APP_WINDOW_FEEL);
	menu->AddItem(new BMenuItem("Modal Across Application",msg));
	msg = new BMessage(MSG_WINDOW_SET_FEEL);
	msg->AddInt32("newfeel",B_MODAL_ALL_WINDOW_FEEL);
	menu->AddItem(new BMenuItem("Modal Across System",msg));
	msg = new BMessage(MSG_WINDOW_SET_FEEL);
	msg->AddInt32("newfeel",B_FLOATING_APP_WINDOW_FEEL);
	menu->AddItem(new BMenuItem("Floating Across Application",msg));
	msg = new BMessage(MSG_WINDOW_SET_FEEL);
	msg->AddInt32("newfeel",B_FLOATING_ALL_WINDOW_FEEL);
	menu->AddItem(new BMenuItem("Floating Across System",msg));
#endif
	switch (fWindowInfo.feel)
	{
		case B_NORMAL_WINDOW_FEEL:
			menu->ItemAt(0)->SetMarked(true);
			break;
		case B_MODAL_APP_WINDOW_FEEL:
			menu->ItemAt(1)->SetMarked(true);
			break;
		case B_MODAL_SUBSET_WINDOW_FEEL:
			menu->ItemAt(1)->SetMarked(true);
			break;
		case B_MODAL_ALL_WINDOW_FEEL:
			menu->ItemAt(2)->SetMarked(true);
			break;
		case B_FLOATING_APP_WINDOW_FEEL:
			menu->ItemAt(3)->SetMarked(true);
			break;
		case B_FLOATING_SUBSET_WINDOW_FEEL:
			menu->ItemAt(3)->SetMarked(true);
			break;
		case B_FLOATING_ALL_WINDOW_FEEL:
			menu->ItemAt(4)->SetMarked(true);
			break;
	}

	float h = fLookField->Frame().Height();
	float top = fLookField->Frame().bottom + 8;
	fFeelField = new BMenuField(BRect(10,top,Bounds().Width() - 10,top + h),"look","Window Behavior: ",menu);
	fFeelField->SetDivider(be_plain_font->StringWidth("Window Behavior: "));
	AddChild(fFeelField);
}
//------------------------------------------------------------------------------
void WindowEditor::InitFlagsBoxes()
{
	BRect work = Bounds();
	// Same side padding as the list on the Elements tab: the scroller inside
	// the box ends up 20 from each edge (14 here plus 6 inside the box).
	work.left = 14;
	work.top = fFeelField->Frame().bottom + 10;
	work.bottom -= 5;
	work.right -= 14;
	BBox* box = new BBox(work, "flags");
	BCheckBox* cbox;
	BMessage* msg;
	box->SetLabel("Window Flags");
	uint32 flags = fWindowInfo.flags;

	// All the checkboxes go in one column on the left, inside a scrolling
	// view, so none of them are squeezed or clipped.
	font_height fh;
	be_bold_font->GetHeight(&fh);
	float top = ceilf(fh.ascent + fh.descent) + 6;
	BRect inner(6, top, work.Width() - 6, work.Height() - 6);
	FlagsContent* content = new FlagsContent(BRect(0, 0,
		inner.Width() - B_V_SCROLL_BAR_WIDTH - 3, inner.Height() - 2));

	float y = 4;
	float step = 20;
	for (int index = 0; WindowFlags[index].name; ++index)
	{
		msg = new BMessage(MSG_WINDOW_SET_FLAG);
		msg->AddInt32("flags", WindowFlags[index].flag);
		cbox = new BCheckBox(BRect(4, y, 100, y + 10),
							 WindowFlags[index].name, WindowFlags[index].label, msg);
		cbox->ResizeToPreferred();
		cbox->MoveTo(4, y);
		fFlagBoxes.push_back(cbox);
		content->AddChild(cbox);
		if (WindowFlags[index].flip == (flags & WindowFlags[index].flag))
		{
			cbox->SetValue(B_CONTROL_ON);
		}

		step = cbox->Bounds().Height() + 4;
		y += step;
	}
	content->SetContentSize(y + 2, step);

	BScrollView* scroller = new BScrollView("flagsscroller", content,
		B_FOLLOW_ALL_SIDES, 0, false, true);
	scroller->MoveTo(inner.LeftTop());
	scroller->ResizeTo(inner.Width(), inner.Height());
	box->AddChild(scroller);
#if 0
	int i = 20;
	int inc = 10;
	msg = new BMessage(MSG_WINDOW_SET_FLAG);
	msg->AddInt32("flags",B_NOT_MOVABLE);
	box->AddChild(new BCheckBox(BRect(10,i,120,i+inc),"nmov","Movable",msg));
	msg = new BMessage(MSG_WINDOW_SET_FLAG);
	msg->AddInt32("flags",B_NOT_CLOSABLE);
	box->AddChild(new BCheckBox(BRect(130,i,210,i+inc),"clos","Closable",msg));
	msg = new BMessage(MSG_WINDOW_SET_FLAG);
	i+= inc*2;
	msg->AddInt32("flags",B_NOT_ZOOMABLE);
	box->AddChild(new BCheckBox(BRect(10,i,120,i+inc),"zoom","Zoomable",msg));
	msg = new BMessage(MSG_WINDOW_SET_FLAG);
	msg->AddInt32("flags",B_NOT_MINIMIZABLE);
	box->AddChild(new BCheckBox(BRect(130,i,210,i+inc),"mini","Minimizable",msg));
	msg = new BMessage(MSG_WINDOW_SET_FLAG);
	i+= inc*2;
	msg->AddInt32("flags",B_NOT_H_RESIZABLE);
	box->AddChild(new BCheckBox(BRect(10,i,210,i+inc),"hres","Horizontally Resizable",msg));
	msg = new BMessage(MSG_WINDOW_SET_FLAG);
	i+= inc*2;
	msg->AddInt32("flags",B_NOT_V_RESIZABLE);
	box->AddChild(new BCheckBox(BRect(10,i,125,i+inc),"vres","Vertically Resizable",msg));
	menubox = new BCheckBox(BRect(130,i,210,i+inc),"menus","Menu Bar",new BMessage(MSG_WINDOW_ADD_MENU));
	box->AddChild(menubox);
	if (fWindowInfo.has_menu)
		menubox->SetValue(B_CONTROL_ON);
	msg = new BMessage(MSG_WINDOW_SET_FLAG);
	i+= inc*2;
	msg->AddInt32("flags",B_OUTLINE_RESIZE);
	box->AddChild(new BCheckBox(BRect(10,i,210,i+inc),"roiw","Resize with Outline Instead of Window",msg));
	msg = new BMessage(MSG_WINDOW_SET_FLAG);
	i+= inc*2;
	msg->AddInt32("flags",B_WILL_ACCEPT_FIRST_CLICK);
	box->AddChild(new BCheckBox(BRect(10,i,210,i+inc),"wafc","Will Accept First Click",msg));
	msg = new BMessage(MSG_WINDOW_SET_FLAG);
	i+= inc*2;
	msg->AddInt32("flags",B_AVOID_FRONT);
	box->AddChild(new BCheckBox(BRect(10,i,120,i+inc),"avfr","Avoid Front",msg));
	msg = new BMessage(MSG_WINDOW_SET_FLAG);
	msg->AddInt32("flags",B_AVOID_FOCUS);
	box->AddChild(new BCheckBox(BRect(130,i,210,i+inc),"avfo","Avoid Focus",msg));
	msg = new BMessage(MSG_WINDOW_SET_FLAG);
	i+= inc*2;
	msg->AddInt32("flags",B_NO_WORKSPACE_ACTIVATION);
	box->AddChild(new BCheckBox(BRect(10,i,210,i+inc),"nwoa","Do Not Activate Workspace",msg));
	msg = new BMessage(MSG_WINDOW_SET_FLAG);
	i+= inc*2;
	msg->AddInt32("flags",B_NOT_ANCHORED_ON_ACTIVATE);
	box->AddChild(new BCheckBox(BRect(10,i,210,i+inc),"brcu","Bring Window To Current Workspace",msg));
	msg = new BMessage(MSG_WINDOW_SET_FLAG);
	i+= inc*2;
	msg->AddInt32("flags",B_ASYNCHRONOUS_CONTROLS);
	box->AddChild(new BCheckBox(BRect(10,i,210,+inc),"async","Asynchronous Controls (Should Be On)",msg));
#endif
#if 0
	if (!(flags & B_NOT_MOVABLE))
		((BCheckBox *)(box->ChildAt(0)))->SetValue(B_CONTROL_ON);
	if (!(flags & B_NOT_CLOSABLE))
		((BCheckBox *)(box->ChildAt(1)))->SetValue(B_CONTROL_ON);
	if (!(flags & B_NOT_ZOOMABLE))
		((BCheckBox *)(box->ChildAt(2)))->SetValue(B_CONTROL_ON);
	if (!(flags & B_NOT_MINIMIZABLE))
		((BCheckBox *)(box->ChildAt(3)))->SetValue(B_CONTROL_ON);
	if (!(flags & B_NOT_H_RESIZABLE))
		((BCheckBox *)(box->ChildAt(4)))->SetValue(B_CONTROL_ON);
	if (!(flags & B_NOT_V_RESIZABLE))
		((BCheckBox *)(box->ChildAt(5)))->SetValue(B_CONTROL_ON);
	if (flags & B_OUTLINE_RESIZE)
		((BCheckBox *)(box->ChildAt(7)))->SetValue(B_CONTROL_ON);
	if (flags & B_WILL_ACCEPT_FIRST_CLICK)
		((BCheckBox *)(box->ChildAt(8)))->SetValue(B_CONTROL_ON);
	if (flags & B_AVOID_FRONT)
		((BCheckBox *)(box->ChildAt(9)))->SetValue(B_CONTROL_ON);
	if (flags & B_AVOID_FOCUS)
		((BCheckBox *)(box->ChildAt(10)))->SetValue(B_CONTROL_ON);
	if (flags & B_NO_WORKSPACE_ACTIVATION)
		((BCheckBox *)(box->ChildAt(11)))->SetValue(B_CONTROL_ON);
	if (flags & B_NOT_ANCHORED_ON_ACTIVATE)
		((BCheckBox *)(box->ChildAt(12)))->SetValue(B_CONTROL_ON);
	if (flags & B_ASYNCHRONOUS_CONTROLS)
		((BCheckBox *)(box->ChildAt(13)))->SetValue(B_CONTROL_ON);
#endif

	AddChild(box);

	cbox = (BCheckBox*)FindView("menus");
	if (cbox)
	{
		cbox->SetValue(fWindowInfo.has_menu);
		cbox->SetMessage(new BMessage(MSG_WINDOW_ADD_MENU));
	}
}
//------------------------------------------------------------------------------
uint32 WindowEditor::MakeBitmask(void)
{
	uint32 bitmask = 0;

	for (uint32 i = 0; i < fFlagBoxes.size() && WindowFlags[i].name; ++i)
	{
		if (WindowFlags[i].flag != (int32)MSG_WINDOW_ADD_MENU &&
			WindowFlags[i].flip == fFlagBoxes[i]->Value())
		{
			bitmask |= WindowFlags[i].flag;
		}
	}

	return bitmask;
#if 0
	BView *box = ((BTabView *)(main->FindView("tabs")))->TabAt(1)->View()->FindView("flags");
	uint32 bitmask = 0;
	if (((BCheckBox *)(box->ChildAt(0)))->Value() == B_CONTROL_OFF)
		bitmask = bitmask | B_NOT_MOVABLE;
	if (((BCheckBox *)(box->ChildAt(1)))->Value() == B_CONTROL_OFF)
		bitmask = bitmask | B_NOT_CLOSABLE;
	if (((BCheckBox *)(box->ChildAt(2)))->Value() == B_CONTROL_OFF)
		bitmask = bitmask | B_NOT_ZOOMABLE;
	if (((BCheckBox *)(box->ChildAt(3)))->Value() == B_CONTROL_OFF)
		bitmask = bitmask | B_NOT_MINIMIZABLE;
	if (((BCheckBox *)(box->ChildAt(4)))->Value() == B_CONTROL_OFF)
		bitmask = bitmask | B_NOT_H_RESIZABLE;
	if (((BCheckBox *)(box->ChildAt(5)))->Value() == B_CONTROL_OFF)
		bitmask = bitmask | B_NOT_V_RESIZABLE;
	if (((BCheckBox *)(box->ChildAt(7)))->Value() == B_CONTROL_ON)
		bitmask = bitmask | B_OUTLINE_RESIZE;
	if (((BCheckBox *)(box->ChildAt(8)))->Value() == B_CONTROL_ON)
		bitmask = bitmask | B_WILL_ACCEPT_FIRST_CLICK;
	if (((BCheckBox *)(box->ChildAt(9)))->Value() == B_CONTROL_ON)
		bitmask = bitmask | B_AVOID_FRONT;
	if (((BCheckBox *)(box->ChildAt(10)))->Value() == B_CONTROL_ON)
		bitmask = bitmask | B_AVOID_FOCUS;
	if (((BCheckBox *)(box->ChildAt(11)))->Value() == B_CONTROL_ON)
		bitmask = bitmask | B_NO_WORKSPACE_ACTIVATION;
	if (((BCheckBox *)(box->ChildAt(12)))->Value() == B_CONTROL_ON)
		bitmask = bitmask | B_NOT_ANCHORED_ON_ACTIVATE;
	if (((BCheckBox *)(box->ChildAt(13)))->Value() == B_CONTROL_ON)
		bitmask = bitmask | B_ASYNCHRONOUS_CONTROLS;
	return bitmask;
#endif
}
//------------------------------------------------------------------------------

// #pragma mark -
//------------------------------------------------------------------------------
void WindowEditor::WindowInfo::SetTo(BWindow* Window)
{
	title = Window->Title();
	look = Window->Look();
	feel = Window->Feel();
	flags = Window->Flags();
	has_menu = Window->KeyMenuBar();
}
//------------------------------------------------------------------------------


/*
 * $Log $
 *
 * $Id  $
 *
 */

