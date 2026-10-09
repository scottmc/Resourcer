// The main document window and its helper threads.
// Declarations are in reswindow.h.

#include "includes.h"
#include "preamble.h"
#include "class.h"
#include "res.h"
#include "reswindow.h"
#include "pluginwindow.h"
#include "optwindow.h"

int32 copy(void *y);
int32 getdata(void *y);
int32 copytoattr(void *y);

FillerView::FillerView(void) : BView(BRect(0,400-B_H_SCROLL_BAR_HEIGHT,350-B_V_SCROLL_BAR_WIDTH,400),"BottomFiller",B_FOLLOW_LEFT_RIGHT | B_FOLLOW_BOTTOM,B_WILL_DRAW | B_FRAME_EVENTS) {
	Draw(Bounds());
}

void
FillerView::Draw(BRect toUpdate) {
	BRect temp = toUpdate;
	temp.top = 2;
	SetHighColor(216,216,216);
	FillRect(temp);
	SetHighColor(255,255,255);
	temp.top = 2;
	temp.bottom = 2;
	FillRect(temp);
	SetHighColor(152,152,152);
	temp.top = 1;
	temp.bottom = 1;
	FillRect(temp);
}

BRect
reswindow::gettiledrect(void) {
	BRect work = find_center(350,400);
	work.top += openmwindows * 30;
	work.left += openmwindows * 30;
	work.right += openmwindows * 30;
	work.bottom += openmwindows * 30;
	return work;
}

reswindow::reswindow(const char *title,entry_ref ref) : BWindow(gettiledrect(),title,B_DOCUMENT_WINDOW,0/*B_ASYNCHRONOUS_CONTROLS*/) {
	istempfile = false;
	isquitting = false;
	changes = false;
	fileref = ref;
	file = new BFile(&ref,B_READ_WRITE);
	openres = new BResources;
	if (openres->SetTo(file) != B_NO_ERROR) {
		return;
	}
	openmwindows++;
	Show(); 
	menus = new BMenuBar(BRect(0,0,600,5),"Menus");
	AddChild(menus);
	Lock();
	mbheight = (int32)(menus->Bounds().bottom);
	Unlock();
	AddChild(new FillerView);
	BMenu *file = new BMenu("File");
	menus->AddItem(file);
	file->AddItem(new BMenuItem("About Resourcer...",new BMessage(B_ABOUT_REQUESTED)));
	file->AddItem(new BSeparatorItem);
	file->AddItem(new BMenuItem("New",new BMessage('newf'),'N'));
	file->AddItem(new BMenuItem("Open...",new BMessage('open'),'O'));
	file->AddItem(new BSeparatorItem);
	file->AddItem(new BMenuItem("Save",new BMessage('save'),'S'));
	file->AddItem(new BMenuItem("Save As...",new BMessage('svas'),'S',B_SHIFT_KEY));
	file->AddItem(new BSeparatorItem);
	file->AddItem(new BMenuItem("Revert Resources",new BMessage('rvrt'),'R'));
	file->AddItem(new BMenuItem("Edit File Type...",new BMessage('mime'),'M'));
	file->AddItem(new BSeparatorItem);
	BMenuItem *quit = new BMenuItem("Quit",new BMessage(B_QUIT_REQUESTED),'Q');
	quit->SetTarget(be_app);
	file->AddItem(quit);
	BMenu *edit = new BMenu("Edit");
	menus->AddItem(edit);
	edit->AddItem(new BMenuItem("Cut",new BMessage(-400),'X'));
	edit->AddItem(new BMenuItem("Copy",new BMessage(-500),'C'));
	edit->AddItem(new BMenuItem("Paste",new BMessage(B_PASTE),'V'));
	BMenu *res = new BMenu("Resource");
	menus->AddItem(res);
	res->AddItem(new BMenuItem("Add Resource...",new BMessage(-100),'A'));
	res->AddItem(new BMenuItem("Delete Resource",new BMessage(-200),'D'));
	res->AddItem(new BMenuItem("Resource Info...",new BMessage(-300),'I'));
	res->AddItem(new BSeparatorItem);
	res->AddItem(new BMenuItem("Copy to Attribute/Resource",new BMessage(-900)));
	res->AddItem(new BSeparatorItem);
	res->AddItem(new BMenuItem("Open with Generic Editor",new BMessage(-800)));
	this->res = new restypeview(openres);
	BScrollView *resa = new BScrollView("resa",this->res,B_FOLLOW_ALL_SIDES,0,false,true,B_NO_BORDER);
	AddChild(resa);
	this->res->SelectionChanged();
	Lock();
	this->res->MakeFocus(true);
	if (IsLocked())
		Unlock();
}

void
reswindow::WindowActivated(bool active) {
	if (active == true)
		be_app->SetCursor(B_HAND_CURSOR);
}

bool
reswindow::QuitRequested(void) {
	char text[800];
	sprintf(text,"Save changes to \"%s\"?\n\nChanges to attributes are saved automatically.",Title());
	CloseWindows();
	isquitting = false;
	if (changes == true) {
		BAlert *alert = new BAlert("save",text,"Cancel","Don't Save","Save",B_WIDTH_AS_USUAL,B_OFFSET_SPACING,B_WARNING_ALERT);
		alert->SetShortcut(0,B_ESCAPE);
		switch(alert->Go()) {
			case 0:
				return false;
				break;
			case 1:
				if (istempfile)
					BEntry(&fileref).Remove();
				break;
			case 2:
				openres->Sync();
				file->Sync();
				changes = false;
				if (istempfile) {
					MessageReceived(new BMessage('svas'));
					isquitting = true;
					return false;
				}
				break;
		}
	} else {
		delete openres;
		delete file;
	}
	openmwindows--;
	if (openmwindows <= 0) {
		BNode node("/boot");
		list_state rescol = collapsed;
		if (res->resources->IsExpanded())
			rescol = expanded;
		node.WriteAttr("rescol",'STAT',0,&rescol,sizeof(list_state));
		rescol = collapsed;
		if (res->attributes->IsExpanded())
			rescol = expanded;
		node.WriteAttr("attrcol",'STAT',0,&rescol,sizeof(list_state));
		be_app->PostMessage(B_QUIT_REQUESTED);
	}
	return true;
}


void reswindow::CloseWindows(void) {
		TypeItem *a;
		DoubleItem *b;
		int32 total = res->CountItemsUnder(res->resources,true);
		for (int32 i = 0;i < total;i++) {
			a = (TypeItem *)res->ItemUnderAt(res->resources,true,i);
			if (a == NULL)
				break;
			for (int32 y = 0;true;y++) {
				b = (DoubleItem *)res->ItemUnderAt(a,true,y);
				if (b == NULL)
					break;
				if (b->win != NULL) {
					b->win->PostMessage(B_QUIT_REQUESTED);
				}
			}
		}
		total = res->CountItemsUnder(res->attributes,true);
		for (int32 i = 0;i < total;i++) {
			a = (TypeItem *)res->ItemUnderAt(res->attributes,true,i);
			if (a == NULL)
				break;
			for (int32 y = 0;true;y++) {
				b = (DoubleItem *)res->ItemUnderAt(a,true,y);
				if (b == NULL)
					break;
				if (b->win != NULL) {
					b->win->PostMessage(B_QUIT_REQUESTED);
				}
			}
		}
	}

void reswindow::MessageReceived(BMessage *message) {
	thread_id thread;
	BMessage *msg;
	switch (message->what) {
		case B_ABOUT_REQUESTED:
			#if __POWERPC__
				(new BAlert("About...","Resourcer 3.0 for PowerPC\n" B_UTF8_COPYRIGHT "1999-2000 Nathan Whitehorn","OK"))->Go();
			#endif
			#if !__POWERPC__
				(new BAlert("About...","Resourcer 3.0 for Intel\n" B_UTF8_COPYRIGHT "1999-2000 Nathan Whitehorn","OK"))->Go();
			#endif
			break;
		case 'newf':
			((BRApplication *)(be_app))->saveas = false;
			alredopen = false;
			((BRApplication *)(be_app))->ReadyToRun();
			break;
		case 'open':
			openpanel->Show();
			break;
		case 'save':
			openres->Sync();
			file->Sync();
			changes = false;
			if (!istempfile)
				break;
		case 'rsiv':
			if (res->CurrentSelection() < 0)
				break;
			if (res->ItemAt(res->CurrentSelection())->OutlineLevel() < 2) {
				if (res->ItemAt(res->CurrentSelection())->IsExpanded())
					res->Collapse(res->ItemAt(res->CurrentSelection()));
				else
					res->Expand(res->ItemAt(res->CurrentSelection()));
				break;
			}
			((DoubleItem *)(res->ItemAt(res->CurrentSelection())))->Invoke(false); 
			break;
		case 'svas':
			((BRApplication *)(be_app))->saveas = true;
			if (newpanel == NULL)
				newpanel = new BFilePanel(B_SAVE_PANEL,NULL,NULL,B_FILE_NODE,false,NULL,NULL,false,true);
			msg = new BMessage(B_SAVE_REQUESTED);
			msg->AddPointer("reswindow",this);
			newpanel->SetMessage(msg);
			newpanel->Show();
			break;
		case 'mime':
			{
				TypeItem *high = res->FindType('MIMS',true);
				DoubleItem *ite = NULL;
				DoubleItem *temp;
				for (int32 i = res->CountItemsUnder(high,true) - 1;i >= 0;i--) {
					temp = (DoubleItem *)res->ItemUnderAt(high,true,i);
					if (strcmp(temp->name,"BEOS:TYPE") == 0) {
						ite = temp;
						temp->Invoke();
					}
				}
				if (ite == NULL)
					ite = res->AddResource('MIMS',0,"BEOS:TYPE",0,NULL,true);
			}
			break;			
		case (uint32)-100:
			new optwindow(true,NULL,'blah',this);
			break;
		case (uint32)-200:
			res->DeleteSelection();
			break;
		case (uint32)-300:
			{
				DoubleItem *cura;
				TypeItem *cur;
				if (res->CurrentSelection() < 0)
					return;
				if (res->ItemAt(res->CurrentSelection())->OutlineLevel() < 2)
					return;
				cura = (DoubleItem *)(res->ItemAt(res->CurrentSelection()));
				cur = (TypeItem *)(res->Superitem(cura));
				if (cura->win != NULL) {
					alert = new BAlert("alert","This resource is open, and so its properties cannot be changed.","OK",NULL,NULL,B_WIDTH_AS_USUAL,B_WARNING_ALERT);
					alert->Go();
					return;
				}
				new optwindow(false,cura,cur->type,this);
			}
			break;
		case (uint32)-400:
			copy(this);
			Lock();
			res->DeleteSelection();
			if (IsLocked())
				Unlock();
			break;
		case (uint32)-500:
			thread = spawn_thread(&copy,"copying",10,this);
			if (thread < B_OK)
				copy(this);
			else {
				if (resume_thread(thread) != B_OK) {
					copy(this);
				}
			}
			break;
		case B_PASTE:
			thread = spawn_thread(&getdata,"pasting",10,this);
			if (thread < B_OK)
				getdata(this);
			else {
				if (resume_thread(thread) != B_OK) {
					getdata(this);
				}
			}
			break;
		case (uint32)-800:
			if (res->ItemAt(res->CurrentSelection())->OutlineLevel() == 2) 
				((DoubleItem *)(res->ItemAt(res->CurrentSelection())))->Invoke(true);
			break;
		case (uint32)-900:
			thread = spawn_thread(&copytoattr,"copying",10,this);
			if (thread < B_OK)
				copytoattr(this);
			else {
				if (resume_thread(thread) != B_OK) {
					copytoattr(this);
				}
			}
			break;
		case 'rvrt':
			CloseWindows();
			res->MakeEmpty();
			openres = new BResources(file);
			res->FillRect(res->Bounds(),B_SOLID_LOW);
			res->Init(openres);
			break;
		default:
			BWindow::MessageReceived(message);
		}
	}

		
int32 copy(void *y) {
	reswindow *x = (reswindow *)(y);
	char *name;
	type_code type;
	unsigned char *data;
	size_t size;
	int32 id;
	TypeItem *cur;
	DoubleItem *cura;
	if (x->res->CurrentSelection() < 0)
		return 0;
	if (x->res->ItemAt(x->res->CurrentSelection())->OutlineLevel() < 2)
		return 0;
	cura = ((DoubleItem *)(x->res->ItemAt(x->res->CurrentSelection())));
	cur = (TypeItem *)x->res->Superitem(cura);
	type = cur->type;
	id = cura->id;
	if (cura->idstring != NULL)
		data = (unsigned char *)x->openres->LoadResource(type,id,&size);
	else {
		attr_info info;
		x->file->GetAttrInfo(cura->name,&info);
		data = new unsigned char[info.size];
		size = info.size;
		x->file->ReadAttr(cura->name,type,0,data,info.size);
	}	
	be_clipboard->Lock();
	be_clipboard->Clear();
	name = cura->name;
	if (name == NULL) {
		name = new char;
		*name = 0;
	}
	if (type == 'CSTR') {
		type = 'MIME';
		static char textplain[] = "text/plain";
		name = textplain;
	}
	bool isattr;
	if (cura->idstring == NULL)
		isattr = true;
	else
		isattr = false;
	be_clipboard->Data()->AddData(name,type,data,size,isattr);
	be_clipboard->Commit();
	be_clipboard->Unlock();
	delete [] data;
	return 0;
}

	
int32 getdata(void *y) {
		reswindow *x = (reswindow *)(y);
		char *name;
		type_code type;
		void *data;
		ssize_t size;
		int32 id;
		bool construct = false;
		be_clipboard->Lock();
		BMessage *message = new BMessage(*(be_clipboard->Data()));
		message->PrintToStream();
		be_clipboard->Unlock();
		if (message == NULL)
			 return 0;
		{
			BMessage *q = new BMessage;
			char *temp;
			type_code fool;
			if (message->GetInfo('MSGG',0,&temp,&fool) == B_OK) {
				message->FindMessage(temp,q);
				if (validate_instantiation(q,"BBitmap"))
					message = q;
			}
		}
		if (message->GetInfo(B_ANY_TYPE,0,&name,&type) == B_BAD_TYPE)
			 return 0;
		message->FindData(name,type,(const void **)&data,&size);
		if (data == NULL)
			 return 0;
		if (message->HasString("class")) {
			if (validate_instantiation(message,"BBitmap")) {
				BBitmap *temp = new BBitmap(message);
				BBitmapStream map(temp);
				size = map.Size();
				data = new unsigned char[size];
				map.Read(data,size);
				size_t tempSize = temp->BitsLength();
				memcpy((void *)(addr_t(data) + (size - tempSize)),temp->Bits(),tempSize);
				map.DetachBitmap(&temp);
				delete temp;
				construct = true;
				type = 'bits';
				static char clipping[] = "Clipping";
				name = clipping;
			} else
				goto NORMAL;
		} else {
			NORMAL:
			if (type == 'MIME') {
				type = code_from_MIME(name);
			}
			if (type == 'bits') {
				BMallocIO out;
				BMemoryIO in(data,size);
				BTranslatorRoster::Default()->Translate(&in,NULL,NULL,&out,'bits');
				size = out.BufferLength();
				data = new unsigned char[size];
				memcpy(data,out.Buffer(),size);
				construct = true;
			}
		}
		for(id = 0;x->openres->HasResource(type,id);id++) {}
		x->res->AddResource(type,id,name,size_t(size),data,false,false);
		if (construct)
			delete [] (unsigned char *)data;
		return 0;
	}

	
int32 copytoattr(void *y) {
		reswindow *x = (reswindow *)(y);
		type_code type;
		int32 id;
		TypeItem *cur;
		DoubleItem *cura;
		if (x->res->CurrentSelection() < 0)
			return 0;
		if (x->res->ItemAt(x->res->CurrentSelection())->OutlineLevel() < 2)
			return 0;
		cura = ((DoubleItem *)(x->res->ItemAt(x->res->CurrentSelection())));
		cur = (TypeItem *)x->res->Superitem(cura);
		type = cur->type;
		id = cura->id;
		if (cura->idstring != NULL) {
				unsigned char *data;
				size_t size;
				char *name;
				data = (unsigned char *)x->openres->LoadResource(type,id,&size);
				attr_info inf;
				if (cura->name == NULL) {
					name = new char[255];
					sprintf(name,"Resource ID: %ld",(long)id);
				} else {
					if (cura->name[0] == 0) {
						name = new char[255];
						sprintf(name,"Resource ID: %ld",(long)id);
					} else {
						name = new char[strlen(cura->name) + 1];
						strcpy(name,cura->name);
					}
				}
				if (x->file->GetAttrInfo(name,&inf) != B_ENTRY_NOT_FOUND) {
					BAlert *a = new BAlert("attrexists","An attribute of the same name or ID already exists. Would you like to overwrite it?","Cancel","OK",NULL,B_WIDTH_AS_USUAL,B_STOP_ALERT);
					if (a->Go() == 0) {
						return 0;
					} else {
						TypeItem *high = x->res->FindType(type,true);
						DoubleItem *ite = NULL;
						DoubleItem *temp;
						for (int32 i = x->res->CountItemsUnder(high,true) - 1;i >= 0;i--) {
							temp = (DoubleItem *)x->res->ItemUnderAt(high,true,i);
							if (strcmp(temp->name,name) == 0) {
								ite = temp;
								x->Lock();
								x->res->RemoveItem(x->res->IndexOf(temp));
								x->Unlock();
							}
					}
				}
			}
			x->res->AddResource(type,0,name,size,data,true,false);
			delete [] name;
			delete [] data;
		} else {
			unsigned char *data;
			char *name = new char[strlen(cura->name) + 1];
			if (cura->name[0] == 0) {
				delete [] name;
				name = NULL;
			} else {
				strcpy(name,cura->name);
			}
			size_t length;
			attr_info inf;
			x->file->GetAttrInfo(cura->name,&inf);
			data = new unsigned char[inf.size];
			length = inf.size;
			x->file->ReadAttr(cura->name,type,0,data,inf.size);
			int32 id;
			if (x->openres->HasResource(type,name)) {
				BAlert *a = new BAlert("resexists","A resource of the same name already exists. Would you like to assign the new data a unique ID or overwrite the existing data?","Overwrite Existing","Unique ID",NULL,B_WIDTH_FROM_LABEL,B_STOP_ALERT);
				if (a->Go() == 0) {
					x->openres->GetResourceInfo(type,name,&id,&length); 
					{
						TypeItem *high = x->res->FindType(type,false);
						DoubleItem *ite = NULL;
						DoubleItem *temp;
						for (int32 i = x->res->CountItemsUnder(high,true) - 1;i >= 0;i--) {
							temp = (DoubleItem *)x->res->ItemUnderAt(high,true,i);
							if (id == temp->id) {
								ite = temp;
								x->Lock();
								x->res->RemoveItem(x->res->IndexOf(temp));
								x->Unlock();
							}
						}
					}
					x->res->AddResource(type,id,name,length,data,false,false);
					return 0;
				}
			}
			for(id = 0;x->openres->HasResource(type,id);id++) {}
			x->res->AddResource(type,id,name,length,data,false,false);
		}
		return 0;
}
