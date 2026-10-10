// The dialog for adding a resource or changing its properties.
// Declaration is in optwindow.h.

#include "includes.h"
#include "preamble.h"
#include "res.h"
#include "reswindow.h"
#include "optwindow.h"

optwindow::optwindow(bool add,DoubleItem* to,type_code typex,reswindow *x) : BWindow(BRect(100,100,500,240),"Change Resource Properties",B_FLOATING_WINDOW_LOOK,B_MODAL_APP_WINDOW_FEEL,B_NOT_ZOOMABLE | B_AUTO_UPDATE_SIZE_LIMITS) {
	AddShortcut('W',0,new BMessage(B_QUIT_REQUESTED),this);
	AddShortcut('.',0,new BMessage(B_QUIT_REQUESTED),this);
	win = x;
	//	The window is layout managed, so labels and fields are sized from the
	//	current font and nothing overlaps or is clipped whatever size it is.
	BGroupLayout *top = new BGroupLayout(B_VERTICAL,10);
	SetLayout(top);
	top->SetInsets(10,10,10,10);
	top->Owner()->SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
	BGridLayout *grid = new BGridLayout(8,6);
	top->AddItem(grid);
	float fieldwidth = be_plain_font->StringWidth("M") * 24;
	if (add) {
		SetTitle("New Resource Properties");
		BPopUpMenu *attrmenu = new BPopUpMenu("Resource");
		attrmenu->AddItem(new BMenuItem("Resource",new BMessage('resr')));
		attrmenu->AddItem(new BMenuItem("Attribute",new BMessage('attr')));
		BMenuField *attrchoice = new BMenuField("attrchoice","Data Format:",attrmenu);
		grid->AddItem(attrchoice->CreateLabelLayoutItem(),0,0);
		grid->AddItem(attrchoice->CreateMenuBarLayoutItem(),1,0,2,1);
		type = new BTextControl("Type","Data Type:","",new BMessage('tyc2'));
		type->TextView()->SetMaxBytes(4);
		BPopUpMenu *typemenu = new BPopUpMenu("Available Data Types",false,false);
		init_typemenu(typemenu);
		BMenuField *typechoice = new BMenuField("typechoice","",typemenu);
		grid->AddItem(type->CreateLabelLayoutItem(),0,1);
		BLayoutItem *typefield = type->CreateTextViewLayoutItem();
		float typewidth = be_plain_font->StringWidth("WWWW") + 24;
		typefield->SetExplicitMinSize(BSize(typewidth,B_SIZE_UNSET));
		typefield->SetExplicitPreferredSize(BSize(typewidth,B_SIZE_UNSET));
		typefield->SetExplicitMaxSize(BSize(typewidth,B_SIZE_UNSET));
		grid->AddItem(typefield,1,1);
		//	A menu field only lays itself out when it is given both of its layout
		//	items, so this one gets an (empty) label item next to its menu.
		BGroupLayout *typepart = new BGroupLayout(B_HORIZONTAL,0);
		grid->AddItem(typepart,2,1);
		typepart->AddItem(typechoice->CreateLabelLayoutItem());
		typepart->AddItem(typechoice->CreateMenuBarLayoutItem());
		id = new BTextControl("ID","ID:","",NULL);
		grid->AddItem(id->CreateLabelLayoutItem(),0,2);
		grid->AddItem(id->CreateTextViewLayoutItem(),1,2,2,1);
		name = new BTextControl("Name","Name:",NULL,NULL);
		BLayoutItem *namefield = name->CreateTextViewLayoutItem();
		namefield->SetExplicitPreferredSize(BSize(fieldwidth,B_SIZE_UNSET));
		grid->AddItem(name->CreateLabelLayoutItem(),0,3);
		grid->AddItem(namefield,1,3,2,1);
		willberes = true;
	} else {
		char ida[255];
		sela = x->res->CurrentSelection();
		if (to->idstring != NULL) {
			sprintf(ida,"%ld",(long)to->id);
			sel = sela; //check
		} else {
			ida[0] = 0;
			sel = sela; //check
		}
		BPopUpMenu *attrmenu = new BPopUpMenu("Resource");
		attrmenu->AddItem(new BMenuItem("Resource",new BMessage('resr')));
		BMenuItem *attritem = new BMenuItem("Attribute",new BMessage('attr'));
		attrmenu->AddItem(attritem);
		if (to->idstring == NULL) {
			attritem->SetMarked(true);
		}
		BMenuField *attrchoice = new BMenuField("attrchoice","Data Format:",attrmenu);
		grid->AddItem(attrchoice->CreateLabelLayoutItem(),0,0);
		grid->AddItem(attrchoice->CreateMenuBarLayoutItem(),1,0);
		size_t len;
		char *bogus;
		attr_info inf;
		if (to->idstring != NULL)
			x->openres->GetResourceInfo(typex,to->id,(const char **)(&bogus),&len);
		else {
			x->file->GetAttrInfo(to->name,&inf);
			len = inf.size;
		}
		BStringView *sizeview = new BStringView("Size",parse_size(len));
		sizeview->SetAlignment(B_ALIGN_RIGHT);
		grid->AddView(sizeview,2,0);
		id = new BTextControl("ID","ID:",ida,NULL);
		if (to->idstring == NULL) {
			id->SetEnabled(false);
		}
		grid->AddItem(id->CreateLabelLayoutItem(),0,1);
		grid->AddItem(id->CreateTextViewLayoutItem(),1,1,2,1);
		name = new BTextControl("Name","Name:",to->name,NULL);
		BLayoutItem *namefield = name->CreateTextViewLayoutItem();
		namefield->SetExplicitPreferredSize(BSize(fieldwidth,B_SIZE_UNSET));
		grid->AddItem(name->CreateLabelLayoutItem(),0,2);
		grid->AddItem(namefield,1,2,2,1);
		if (to->idstring == NULL)
			willberes = false;
		else
			willberes = true;
	}
	whattodo = add;
	toop = to;
	typeb = typex;
	BGroupLayout *buttons = new BGroupLayout(B_HORIZONTAL,8);
	top->AddItem(buttons);
	buttons->AddItem(BSpaceLayoutItem::CreateGlue());
	buttons->AddView(new BButton("Cancel","Cancel",new BMessage(B_QUIT_REQUESTED)));
	BButton *OK = new BButton("OK","OK",new BMessage('bxok'));
	buttons->AddView(OK);
	OK->MakeDefault(true);
	//	Open at the size the contents need (the preferred size can be NaN before
	//	the layout is validated, so ask for the minimum first), within the
	//	screen, and a little above the middle as before.
	BSize min = top->MinSize();
	BSize size = top->PreferredSize();
	if (!(size.width >= min.width))
		size.width = min.width;
	if (!(size.height >= min.height))
		size.height = min.height;
	BRect screen = BScreen(this).Frame();
	if (size.width > screen.Width() - 20)
		size.width = screen.Width() - 20;
	BRect where = find_center((uint32)size.width,(uint32)size.height);
	ResizeTo(size.width,size.height);
	MoveTo(where.left,where.top - 150);
	Show();
}

void
optwindow::init_typemenu(BPopUpMenu *menu) {
	if (itemLista == NULL) {
		itemLista = new BMessage;
		app_info info;
		be_app->GetAppInfo(&info);
		BEntry entry(&(info.ref));
		BDirectory dir;
		entry.GetParent(&dir);
		entry.SetTo(&dir,"editors");
		dir.SetTo(&entry);
		BPath path;
		char name[B_FILE_NAME_LENGTH];
		dir.Rewind();
		while (dir.GetNextEntry(&entry,true) == B_OK) {
			if (entry.Exists() == false)
				break;
			entry.GetPath(&path);
			image_id editor = load_add_on(path.Path());
			if (editor < 0)
				continue;
			const char *temp;
			if (get_image_symbol(editor,"description",B_SYMBOL_TYPE_DATA,(void **)(&temp)) == B_NO_ERROR) {
				entry.GetName(name);
				itemLista->AddString("typecode",name);
				itemLista->AddString("description",temp);
			}
			unload_add_on(editor);
		}
	}
	BList itemList;
	const char *typecode;
	const char *description;
	for (int32 i = 0;itemLista->FindString("typecode",i,&typecode) == B_OK && itemLista->FindString("description",i,&description) == B_OK;i++) {
		BMessage *message = new BMessage('tycd');
		message->AddString("typecode",typecode);
		itemList.AddItem(new BMenuItem(description,message));
	}
	if (itemList.CountItems() == 0) {
		menu->SetEnabled(false);
		return;
	}
	itemList.SortItems(&sortmenu);
	for (int32 i = itemList.CountItems() - 1;i >= 0;i--) {
		menu->AddItem((BMenuItem *)(itemList.ItemAt(i)));
	}
}

char *
optwindow::parse_size(size_t sizer) {
	float size = sizer;
	static char textsize[255];
	char temp[20];
	const char *suffix;
	if (size >= 1024*1024) {
		size /= 1024*1024;
		suffix = "MB";
	} else {
		if (size >= 1024) {
			size /= 1024;
			suffix = "KB";
		} else {
			suffix = "B";
		}
	}
	sprintf(temp,"%.2f",size);
	int i;
	for (i = 0;(temp[i] != '.') && (temp[i] != 0);i++) {}
	for (;(temp[i] != '0') && (temp[i] != 0);i++) {}
	temp[i] = 0;
	if (temp[i-1] == '.')
		temp[i-1] = 0;
	sprintf(textsize,"Resource Size: %s %s",temp,suffix);
	return textsize;
}

void
optwindow::add(void) {
	bool notype = true;			//---a type of nothing but spaces is as good as none
	for (const char *c = type->Text(); *c != 0; c++) {
		if (*c != ' ') {
			notype = false;
			break;
		}
	}
	if (notype) {
		(new BAlert("alert","Please enter a data type of up to four characters, for example TEXT or LONG.","OK",NULL,NULL,B_WIDTH_AS_USUAL,B_STOP_ALERT))->Go();
		type->MakeFocus(true);
		return;
	}
	Lock();
	int32 test = 1;
	long value;
	char temp[15];
	for (int i = 0; i < 4;i++) {
		temp[i] = ' ';
	}
	if (sscanf(id->Text(),"%ld",&value) == 1)
		test = value;
	strcpy(temp,type->Text());
	for (int i = strlen(temp); i < 4;i++) {
		temp[i] = ' ';
	}
	temp[4] = 0;
	type_code typea;
	strncpy((char *)(&typea),temp,4);
	typea = flipcode(typea);
	char *namea = new char[strlen(name->Text()) + 1];
	strcpy(namea,name->Text());
	if ((namea[0] == 0) && (!willberes)) {
		delete [] namea;
		namea = new char[8];
		strcpy(namea,"Unnamed");
	}
	if (willberes) {
		if (win->openres->HasResource(typea,test) == true) {
			BAlert *alert = new BAlert("alert","This resource already exists. Would you like to overwrite the existing one?","Cancel","OK",NULL,B_WIDTH_AS_USUAL,B_STOP_ALERT);
			if (alert->Go() == 0) {
				Unlock();
				delete [] namea;
				return;
			}
			TypeItem *super = win->res->FindType(typea,false);
			DoubleItem *to_remove;
			for (int32 i = 0; (to_remove = (DoubleItem *)win->res->ItemUnderAt(super,true,i)) != NULL; i++) {
				if (to_remove->id == test) {
					win->Lock();
					win->res->DeleteSelection(to_remove);
					win->Unlock();
					break;
				}
			}
		}
	} else {
		attr_info inf;
		if (win->file->GetAttrInfo(namea,&inf) != B_ENTRY_NOT_FOUND) {
			BAlert *alert = new BAlert("alert","An attribute with this name already exists. Would you like to overwrite the existing one?","Cancel","OK",NULL,B_WIDTH_AS_USUAL,B_STOP_ALERT);
			if (alert->Go() == 0) {
				Unlock();
				delete [] namea;
				return;
			}
			DoubleItem *to_remove;
			for (int32 i = 0; win->res->ItemUnderAt(win->res->attributes,false,i) != NULL; i++) {
				if (!is_instance_of(win->res->ItemUnderAt(win->res->attributes,false,i),DoubleItem))
					continue;
				else
					to_remove = (DoubleItem *)win->res->ItemUnderAt(win->res->attributes,false,i);
				if (strcmp(to_remove->name,namea) == 0) {
					win->Lock();
					win->res->DeleteSelection(to_remove);
					win->Unlock();
					break;
				}
			}
		}
	}
	DoubleItem *item = win->res->AddResource(typea,test,namea,0,NULL,!(willberes),true);
	win->Lock();
	win->res->Expand(win->res->Superitem(item));
	win->res->Expand(win->res->Superitem(win->res->Superitem(item)));
	win->res->Select(win->res->IndexOf(item));
	win->Unlock();
	Unlock();
	delete [] namea;
	PostMessage(B_QUIT_REQUESTED);
	//name->SetText(NULL);
}

void
optwindow::changeinfo(void) {
	int32 test = 1;
	long value;
	if (sscanf(id->Text(),"%ld",&value) == 1)
		test = value;
	char *namebuf = new char[strlen(name->Text()) + 1];	//---owns the allocation
	strcpy(namebuf,name->Text());
	char *namea = namebuf;
	if (*namea == 0)
		namea = NULL;
	if (willberes) {
		if ((win->openres->HasResource(typeb,test) == true) && (test != toop->id)) {
			BAlert *alert = new BAlert("alert","This resource already exists. Would you like to overwrite the existing one?","Cancel","OK",NULL,B_WIDTH_AS_USUAL,B_STOP_ALERT);
			if (alert->Go() == 0) {
				delete [] namebuf;
				return;
			}
		}
	} else {
		if (namea == NULL) {
			(new BAlert("alert","Attributes must have a name.","OK",NULL,NULL,B_WIDTH_AS_USUAL,B_STOP_ALERT))->Go();
			delete [] namebuf;
			return;
		}
		attr_info inf;
		if ((win->file->GetAttrInfo(namea,&inf) != B_ENTRY_NOT_FOUND) && (strcmp(namea,toop->name))) {
			BAlert *alert = new BAlert("alert","An attribute with this name already exists. Would you like to overwrite the existing one?","Cancel","OK",NULL,B_WIDTH_AS_USUAL,B_STOP_ALERT);
			if (alert->Go() == 0) {
				delete [] namebuf;
				return;
			}
		}
	}
	size_t length;
	unsigned char *data;
	bool isattr;
	if (toop->idstring == NULL)
		isattr = true;
	else
		isattr = false;
	if (isattr) {
		attr_info info;
		win->file->GetAttrInfo(toop->name,&info);
		length = info.size;
		data = new unsigned char[length];
		win->file->ReadAttr(toop->name,typeb,0,data,length);
	} else {
		const void *dat = win->openres->LoadResource(typeb,toop->id,&length);
		data = new unsigned char[length];
		memcpy(data,dat,length);
	}
	if (isattr)
		win->file->RemoveAttr(toop->name);
	else
		win->openres->RemoveResource(typeb,toop->id);
	if (willberes)
		win->openres->AddResource(typeb,test,data,length,namea);
	else
		win->file->WriteAttr(namea,typeb,0,data,length);
	delete [] data;
	toop->id = test;
	delete [] toop->name;
	if (namea == NULL) {
		toop->name = new char[1];
		toop->name[0] = 0;
	} else {
		toop->name = new char[strlen(namea) + 1];
		strcpy(toop->name,namea);
	}
	win->Lock();
	if ((willberes) && (toop->idstring == NULL)) {
		if (win->res->CountItemsUnder(win->res->Superitem(toop),true) == 1) {
			BListItem *super = win->res->Superitem(toop);
			if (win->res->CountItemsUnder(win->res->attributes,true) == 1) {
				win->res->attributes->SetText("No Attributes");
				win->res->attributes->SetEnabled(false);
			}
			win->res->RemoveItem(toop);
			win->res->RemoveItem(super);
		} else
			win->res->RemoveItem(toop);
		win->res->AddUnder(toop,win->res->FindType(typeb,false));
	}
	if ((!willberes) && (toop->idstring != NULL)) {
		if (win->res->CountItemsUnder(win->res->Superitem(toop),true) == 1) {
			BListItem *super = win->res->Superitem(toop);
			if (win->res->CountItemsUnder(win->res->resources,true) == 1) {
				win->res->resources->SetText("No Resources");
				win->res->resources->SetEnabled(false);
			}
			win->res->RemoveItem(toop);
			win->res->RemoveItem(super);
		} else
			win->res->RemoveItem(toop);
		win->res->AddUnder(toop,win->res->FindType(typeb,true));
	}
	delete [] toop->idstring;
	if (willberes) {
		toop->idstring = new char[20];
		sprintf(toop->idstring,"%ld",(long)toop->id);
	} else
		toop->idstring = NULL;
	win->res->Invalidate(win->res->ItemFrame(win->res->IndexOf(toop)));
	win->res->Sort();
	if (win->IsLocked())
		win->Unlock();
	win->changes = true;
	delete [] namebuf;
	PostMessage(B_QUIT_REQUESTED);
}

void
optwindow::MessageReceived(BMessage *message) {
	char *string;
	switch (message->what) {
		case 'bxok':
			if (whattodo == true) {
				add();
			} else {
				changeinfo();
			}
			win->changes = true;
			break;
		case 'attr':
			willberes = false;
			//Lock();
			id->SetEnabled(false);
			//Unlock();
			break;
		case 'tycd':
			message->FindString("typecode",(const char **)(&string));
			type->SetText(string);
			break;
		case 'resr':
			willberes = true;
			//Lock();
			id->SetEnabled(true);
			//Unlock();
			break;
		default:
			BWindow::MessageReceived(message);
		}
}
