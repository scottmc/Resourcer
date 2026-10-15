//------------------------------------------------------------------------------
//	MenuEditor.cpp
//
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------
#include <math.h>
#include <string.h>

// System Includes -------------------------------------------------------------
#include <interface/Button.h>
#include <interface/CheckBox.h>
#include <interface/GroupLayout.h>
#include <interface/Menu.h>
#include <interface/MenuField.h>
#include <interface/Font.h>
#include <interface/MenuItem.h>
#include <interface/ScrollView.h>
#include <interface/TextControl.h>

#include <support/ClassInfo.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "DragOutlineListView.h"
#include "MenuEditor.h"
#include "MenuListItem.h"
#include "WIND.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

//------------------------------------------------------------------------------
static float LabelWidth(const char* label)
{
	return ceilf(be_plain_font->StringWidth(label)) + 10;
}
//------------------------------------------------------------------------------
MenuEditor::MenuEditor(BRect rect, BMenu* men)
	:	BBox(rect, "menueditor"),
		fListView(NULL),
		fMenu(men),
		fLabelText(NULL),
		fMsgText(NULL),
		fShortcutText(NULL),
		fModifierField(NULL),
		fEnabledBox(NULL),
		fAddItemBtn(NULL),
		fAddSeparatorBtn(NULL),
		fInit(false)
{
	SetViewColor(216,216,216);

	BGroupLayout* layout = new BGroupLayout(B_VERTICAL, 4);
	layout->SetInsets(10, 10, 10, 10);
	SetLayout(layout);

	// the item list, tall enough for a handful of rows at any font size
	font_height fh;
	be_plain_font->GetHeight(&fh);
	float rowHeight = ceilf(fh.ascent + fh.descent + fh.leading) + 4;

	fListView = new DragOutlineListView(BRect(0, 0, 150, 100), Menu());
	fListView->SetSelectionMessage(new BMessage(MSG_MENU_SELECTION_CHANGED));
	BScrollView* scroller = new BScrollView("itemscroll", fListView,
											B_FOLLOW_LEFT | B_FOLLOW_TOP, 0,
											false, true);
	scroller->SetExplicitMinSize(BSize(100, rowHeight * 6));
	scroller->SetExplicitPreferredSize(BSize(200, rowHeight * 6));
	layout->AddView(scroller);

	fLabelText = new BTextControl("label", "Label: ", "",
								  new BMessage(MSG_MENU_SET_ITEM_LABEL));
	fLabelText->SetDivider(LabelWidth("Label: "));
	fLabelText->SetEnabled(false);
	layout->AddView(fLabelText);

	fMsgText = new BTextControl("msg", "Message: ", "none",
								new BMessage(MSG_MENU_SET_ITEM_MSG));
	fMsgText->TextView()->SetMaxBytes(4);
	fMsgText->SetDivider(LabelWidth("Message: "));
	fMsgText->SetEnabled(false);
	layout->AddView(fMsgText);

	// shortcut key and its modifiers share a row
	BGroupLayout* shortcutRow = new BGroupLayout(B_HORIZONTAL, 4);
	layout->AddItem(shortcutRow);

	float shortcutDivider = LabelWidth("Shortcut: ");
	fShortcutText = new BTextControl("shrtct", "Shortcut: ", "",
									 new BMessage(MSG_MENU_SET_ITEM_SHORTCUT));
	fShortcutText->SetDivider(shortcutDivider);
	fShortcutText->SetEnabled(false);
	fShortcutText->TextView()->SetMaxBytes(1);
	fShortcutText->SetExplicitMaxSize(BSize(shortcutDivider
		+ ceilf(be_plain_font->StringWidth("MMM")) + 10, B_SIZE_UNSET));
	shortcutRow->AddView(fShortcutText);

	BMenu *meniu = new BMenu("Modifier Keys");
	meniu->AddItem(new BMenuItem("Option", new BMessage(MSG_MENU_SET_ITEM_MARKED)));
	meniu->AddItem(new BMenuItem("Shift", new BMessage(MSG_MENU_SET_ITEM_MARKED)));
	meniu->AddItem(new BMenuItem("Control", new BMessage(MSG_MENU_SET_ITEM_MARKED)));
	fModifierField = new BMenuField("modchoice", NULL, meniu);
	fModifierField->SetEnabled(false);
	meniu->SetRadioMode(false);
	meniu->SetLabelFromMarked(false);
	shortcutRow->AddView(fModifierField);

	fEnabledBox = new BCheckBox("enabled", "Enabled",
								new BMessage(MSG_MENU_SET_ITEM_ENABLED));
	fEnabledBox->SetEnabled(false);
	layout->AddView(fEnabledBox);

	BGroupLayout* buttonRow = new BGroupLayout(B_HORIZONTAL, 8);
	layout->AddItem(buttonRow);
	fAddItemBtn = new BButton("addiantem", "Add Item",
							  new BMessage(MSG_MENU_NEW_ITEM));
	buttonRow->AddView(fAddItemBtn);
	fAddSeparatorBtn = new BButton("addsepitem", "Add Separator",
								   new BMessage(MSG_MENU_ADD_SEPARATOR));
	buttonRow->AddView(fAddSeparatorBtn);

	// start out at the size the layout wants
	ResizeTo(PreferredSize().width, PreferredSize().height);
}
//------------------------------------------------------------------------------
BSize MenuEditor::MaxSize()
{
	// grow with the panel's width, but never taller than needed
	return BSize(B_SIZE_UNLIMITED, PreferredSize().height);
}
//------------------------------------------------------------------------------
void MenuEditor::AttachedToWindow()
{
	fListView->SetTarget(this);
	fLabelText->SetTarget(this);
	fMsgText->SetTarget(this);
	fShortcutText->SetTarget(this);
	fModifierField->Menu()->SetTargetForItems(this);
	fEnabledBox->SetTarget(this);
	fAddItemBtn->SetTarget(this);
	fAddSeparatorBtn->SetTarget(this);
}
//------------------------------------------------------------------------------
void MenuEditor::MessageReceived(BMessage* msg)
{
	BMenuItem* item;
	switch (msg->what)
	{
		case MSG_MENU_NEW_ITEM:
		{
			if (fMenu)
			{
				fMenu->AddItem(new BMenuItem("New Item",
											 new BMessage('none')));
				fListView->Refresh(false);
			}
			break;
		}

		case MSG_MENU_SET_ITEM_LABEL:
			CurrentItem()->SetLabel(fLabelText->Text());
			fListView->Refresh(true);
			break;

		case MSG_MENU_SET_ITEM_MSG:
		{
			int32 what = flipcode(*((int32 *)(fMsgText->Text())));
			CurrentItem()->SetMessage(new BMessage(what));
			break;
		}

		case MSG_MENU_SET_ITEM_MARKED:
			msg->FindPointer("source", (void **)(&item));
			item->SetMarked(!item->IsMarked());

		case MSG_MENU_SET_ITEM_SHORTCUT:
		{
			if (fShortcutText->TextView()->TextLength() == 0)
			{
				fModifierField->Menu()->ItemAt(0)->SetMarked(false);
				fModifierField->Menu()->ItemAt(1)->SetMarked(false);
				fModifierField->Menu()->ItemAt(2)->SetMarked(false);
				fModifierField->SetEnabled(false);
			}
			else if (!fModifierField->IsEnabled())
			{
				fModifierField->SetEnabled(true);
			}
			CurrentItem()->SetShortcut(*(fShortcutText->Text()), MakeModMask());

			break;
		}

		case MSG_MENU_SET_ITEM_ENABLED:
		{
			CurrentItem()->SetEnabled(fEnabledBox->Value() == B_CONTROL_ON);
			break;
		}

		case MSG_MENU_ADD_SEPARATOR:
		{
			if (fMenu)
			{
				item = new BSeparatorItem;
				fMenu->AddItem(item);
				fListView->Refresh(false);
			}
			break;
		}

		case MSG_MENU_SELECTION_CHANGED:
		{
			OnSelectionChanged(msg);
			break;
		}

		default:
			BBox::MessageReceived(msg);
			break;
	}
}
//------------------------------------------------------------------------------
void MenuEditor::SetMenu(BMenu* menu)
{
	fMenu = menu;
	fListView->SetMenu(menu);
}
//------------------------------------------------------------------------------
BMenu* MenuEditor::Menu()
{
	return fMenu;
}
//------------------------------------------------------------------------------
uint32 MenuEditor::MakeModMask(void)
{
	uint32 modifiers = 0;
	if (fModifierField->Menu()->ItemAt(0)->IsMarked())
	{
		modifiers |= B_OPTION_KEY;
	}
	if (fModifierField->Menu()->ItemAt(1)->IsMarked())
	{
		modifiers |= B_SHIFT_KEY;
	}
	if (fModifierField->Menu()->ItemAt(2)->IsMarked())
	{
		modifiers |= B_CONTROL_KEY;
	}
	return modifiers;
}	
//------------------------------------------------------------------------------
BMenuItem* MenuEditor::CurrentItem()
{
	int32 index = fListView->FullListCurrentSelection();
	MenuListItem* listItem = (MenuListItem *)(fListView->FullListItemAt(index));
	return listItem ? listItem->item : NULL;
}
//------------------------------------------------------------------------------
void MenuEditor::OnSelectionChanged(BMessage* /*msg*/)
{
	BMenuItem* current = (fListView->CurrentSelection() < 0) ? NULL : CurrentItem();
	if (current == NULL || is_kind_of(current, BSeparatorItem))
	{
		fLabelText->SetText("");
		fLabelText->SetEnabled(false);
		fMsgText->SetText("none");
		fMsgText->SetEnabled(false);
		fShortcutText->SetText("");
		fShortcutText->SetEnabled(false);
		fModifierField->Menu()->ItemAt(0)->SetMarked(false);
		fModifierField->Menu()->ItemAt(1)->SetMarked(false);
		fModifierField->Menu()->ItemAt(2)->SetMarked(false);
		fModifierField->SetEnabled(false);
		fEnabledBox->SetEnabled(false);
		fEnabledBox->SetValue(B_CONTROL_OFF);
		return;
	}
	else
	{
		fLabelText->SetEnabled(true);
		fMsgText->SetEnabled(true);

		fModifierField->Menu()->ItemAt(0)->SetMarked(false);
		fModifierField->Menu()->ItemAt(1)->SetMarked(false);
		fModifierField->Menu()->ItemAt(2)->SetMarked(false);

		fEnabledBox->SetEnabled(true);
		fEnabledBox->SetValue(CurrentItem()->IsEnabled());

		fShortcutText->SetEnabled(CurrentItem()->Submenu() == NULL);
	}
	fLabelText->SetText(CurrentItem()->Label());
	char temp[5];
	BMenuItem *item = CurrentItem();
	if (item->Message() != NULL)
		strncpy(temp, (char *)(new int32(flipcode(item->Message()->what))), 4);
	else
		strcpy(temp, "none");
	temp[4] = 0;
	fMsgText->SetText(temp);

	uint32 mod;
	temp[1] = 0;
	temp[0] = item->Shortcut(&mod);
	fShortcutText->SetText(temp);

	fModifierField->SetEnabled(*temp);
	if (*temp)
	{
		fModifierField->Menu()->ItemAt(0)->SetMarked(mod & B_OPTION_KEY);
		fModifierField->Menu()->ItemAt(1)->SetMarked(mod & B_SHIFT_KEY);
		fModifierField->Menu()->ItemAt(2)->SetMarked(mod & B_CONTROL_KEY);
	}
}
//------------------------------------------------------------------------------

/*
 * $Log $
 *
 * $Id  $
 *
 */

