// The dialog for adding a resource or changing its properties.
// Declaration is in optwindow.h.

#include "includes.h"
#include "preamble.h"
#include "res.h"
#include "reswindow.h"
#include "optwindow.h"

optwindow::optwindow(bool add,DoubleItem* to,type_code typex,reswindow *x) : BWindow(BRect(find_center(400,140).left,find_center(200,140).top - 150,find_center(400,140).right,find_center(200,140).bottom - 150),"Change Resource Properties",B_FLOATING_WINDOW_LOOK,B_MODAL_APP_WINDOW_FEEL,B_NOT_RESIZABLE | B_NOT_ZOOMABLE/* | B_ASYNCHRONOUS_CONTROLS*/) {
	AddShortcut('W',0,new BMessage(B_QUIT_REQUESTED),this);
	AddShortcut('.',0,new BMessage(B_QUIT_REQUESTED),this);
	win = x;
	gray = new BView(Bounds(),"graybkgrd",B_FOLLOW_ALL_SIDES,B_WILL_DRAW | B_FRAME_EVENTS);
	gray->SetViewColor(216,216,216);
	if (add) {
		SetTitle("New Resource Properties");
		BPopUpMenu *attrmenu = new BPopUpMenu("Resource");
		attrmenu->AddItem(new BMenuItem("Resource",new BMessage('resr')));
		attrmenu->AddItem(new BMenuItem("Attribute",new BMessage('attr')));
		BMenuField *attrchoice = new BMenuField(BRect(10,9,390,29),"attrchoice","Data Format: ",attrmenu);
		attrchoice->SetDivider(be_plain_font->StringWidth("Data Format: "));
		gray->AddChild(attrchoice);
		type = new BTextControl(BRect(10,34,be_plain_font->StringWidth("AAAA")+be_plain_font->StringWidth("Data Type: ")+20,54),"Type","Data Type: ","",new BMessage('tyc2'));
		type->SetDivider(be_plain_font->StringWidth("Data Type: "));
		type->TextView()->SetMaxBytes(4);
		BPopUpMenu *typemenu = new BPopUpMenu("Available Data Types",false,false);
		init_typemenu(typemenu);
		BMenuField *typechoice = new BMenuField(BRect(be_plain_font->StringWidth("AAAA")+be_plain_font->StringWidth("Data Type: ")+40,34,390,54),"typechoice","",typemenu);
		typechoice->SetDivider(0);
		gray->AddChild(typechoice);
		gray->AddChild(type);
		id = new BTextControl(BRect(10,56,390,76),"ID","ID: ","",NULL);
		id->SetDivider(be_plain_font->StringWidth("ID: "));
		gray->AddChild(id);
		name = new BTextControl(BRect(10,78,390,98),"Name","Name: ",NULL,NULL);
		name->SetDivider(be_plain_font->StringWidth("Name: "));
		gray->AddChild(name);
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
		BMenuField *attrchoice = new BMenuField(BRect(10,9,190,29),"attrchoice","Data Format: ",attrmenu);
		attrchoice->SetDivider(be_plain_font->StringWidth("Data Format: "));
		gray->AddChild(attrchoice);
		size_t len;
		char *bogus;
		attr_info inf;
		if (to->idstring != NULL)
			x->openres->GetResourceInfo(typex,to->id,(const char **)(&bogus),&len);
		else {
			x->file->GetAttrInfo(to->name,&inf);
			len = inf.size;
		}
		gray->AddChild(new BStringView(BRect(210,9,390,29),"Size",parse_size(len),B_FOLLOW_RIGHT | B_FOLLOW_TOP));
		id = new BTextControl(BRect(10,43,390,63),"ID","ID: ",ida,NULL);
		id->SetDivider(be_plain_font->StringWidth("ID: "));
		if (to->idstring == NULL) {
			id->SetEnabled(false);
		}
		gray->AddChild(id);
		name = new BTextControl(BRect(10,77,390,97),"Name","Name: ",to->name,NULL);
		name->SetDivider(be_plain_font->StringWidth("Name: "));
		gray->AddChild(name);
		if (to->idstring == NULL)
			willberes = false;
		else
			willberes = true;
	}
	whattodo = add;
	toop = to;
	typeb = typex;
	gray->AddChild(new BButton(BRect(240,110,300,130),"Cancel","Cancel",new BMessage(B_QUIT_REQUESTED)));
	BButton *OK = new BButton(BRect(320,110,380,130),"OK","OK",new BMessage('bxok'));
	gray->AddChild(OK);
	OK->MakeDefault(true);
	AddChild(gray);
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
	char *namea = new char[strlen(name->Text()) + 1];
	strcpy(namea,name->Text());
	if (*namea == 0)
		namea = NULL;
	if (willberes) {
		if ((win->openres->HasResource(typeb,test) == true) && (test != toop->id)) {
			BAlert *alert = new BAlert("alert","This resource already exists. Would you like to overwrite the existing one?","Cancel","OK",NULL,B_WIDTH_AS_USUAL,B_STOP_ALERT);
			if (alert->Go() == 0) {
				return;
			}
		}
	} else {
		if (namea == NULL) {
			(new BAlert("alert","Attributes must have a name.","OK",NULL,NULL,B_WIDTH_AS_USUAL,B_STOP_ALERT))->Go();
			return;
		} 
		attr_info inf;
		if ((win->file->GetAttrInfo(namea,&inf) != B_ENTRY_NOT_FOUND) && (strcmp(namea,toop->name))) {
			BAlert *alert = new BAlert("alert","An attribute with this name already exists. Would you like to overwrite the existing one?","Cancel","OK",NULL,B_WIDTH_AS_USUAL,B_STOP_ALERT);
			if (alert->Go() == 0) {
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
	toop->id = test;
	if (namea == NULL)
		toop->name = new char(0);
	else {
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
