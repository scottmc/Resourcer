#ifndef RESOURCER_MAIN_WINDOW_H
#define RESOURCER_MAIN_WINDOW_H

#include "reswindow.h"
#include "pluginwindow.h"
#include "optwindow.h"

int32 copytoattr(void *y);
int32 copy(void *y);
int32 getdata(void *y);
int32 copytoattr(void *y);
	
bool restypeview::InitiateDrag(BPoint point,int32 index, bool selected) {
	if (!selected)
		return false;
	if (ItemAt(index)->OutlineLevel() < 2)
		return false;
	DoubleItem *y = (DoubleItem *)(ItemAt(index));
	BMessage *todrag = new BMessage(B_SIMPLE_DATA);
	size_t size;
	type_code code = ((TypeItem *)(Superitem(y)))->type;
	unsigned char *data;
	if (y->idstring != NULL) {
		const void *dat = ((reswindow *)(Window()))->openres->LoadResource(code,y->id,&size);
		data = new unsigned char[size];
		memcpy(data,dat,size);
	} else {
		attr_info inf;
		((reswindow *)(Window()))->file->GetAttrInfo(y->name,&inf);
		size = inf.size;
		data = new unsigned char[size];
		((reswindow *)(Window()))->file->ReadAttr(y->name,code,0,data,size);
	}
	char m_type[B_MIME_TYPE_LENGTH];
	strcpy(m_type,MIME_from_code(code));
	todrag->AddString("be:types", B_FILE_MIME_TYPE);
	todrag->AddString("be:types", m_type);
	todrag->AddString("be:filetypes", m_type);
	todrag->AddInt32("be:actions", B_COPY_TARGET);
	todrag->AddInt32("be:actions", B_TRASH_TARGET);
	char filename[B_FILE_NAME_LENGTH];
	if (y->name[0] == 0)
		sprintf(filename,"Resource ID: %ld",(long)y->id);
	else
		strcpy(filename,y->name);
	todrag->AddString("be:clip_name", filename);
	if (y->idstring != NULL)
		todrag->AddBool("isattr",false);
	else
		todrag->AddBool("isattr",true);
	todrag->AddData("type",B_UINT32_TYPE,&code,4);
	if (code == 'CSTR')
		todrag->AddData(m_type,'MIME',data,ssize_t(size));
	delete  [] data;
	todrag->AddString("name",y->name);
	todrag->AddPointer("this",this);
	if (y->idstring == NULL)
		todrag->AddInt32("id",0);
	else
		todrag->AddInt32("id",y->id);
	DragMessage(todrag,ItemFrame(IndexOf(y)));
	return true;
}

void restypeview::MessageReceived(BMessage *msg) {
	if (msg->WasDropped())
		StrokeRect(Bounds(),B_SOLID_LOW);
	BOutlineListView::MessageReceived(msg);
	reswindow *x = (reswindow *)(Window());
	char *name;
	type_code type;
	void *dat;
	ssize_t size;
	int32 id = 0;
	BMessage *message = msg;
	BOutlineListView *old;
	unsigned char *data;
	if (msg->WasDropped() == false) {
		switch (msg->what) {
			case B_COPY_TARGET:
				{
				const BMessage *prev = msg->Previous();
				if (prev->HasInt32("id"))
					prev->FindInt32("id",&id);
				else
					id = 0;
				size_t size;
				type_code *temp;
				prev->FindData("type",(uint32)(B_UINT32_TYPE),(const void **)&temp,new ssize_t);
				type = *temp;
				entry_ref ref;
				msg->FindRef("directory",&ref);
				msg->FindString("name",(const char **)(&name));
				BDirectory dir(&ref);
				BFile file(&dir,name,B_READ_WRITE | B_CREATE_FILE);
				unsigned char *data;
				bool isattr;
				prev->FindBool("isattr",&isattr);
				if (!isattr) {
					const void *dat = ((reswindow *)(Window()))->openres->LoadResource(type,id,&size);
					data = new unsigned char[size];
					memcpy(data,dat,size);
				} else {
					char *name;
					prev->FindString("be:clip_name",(const char **)(&name));
					attr_info inf;
					((reswindow *)(Window()))->file->GetAttrInfo(name,&inf);
					size = inf.size;
					data = new unsigned char[size];
					((reswindow *)(Window()))->file->ReadAttr(name,type,0,data,size);
				}
				file.Write(data,size);
				delete [] data;
				BNodeInfo ni(&file);
				/*if (type == 'bits')
					ni.SetType("image/x-portable-pixmap");
				else */{
					char *mime;
					msg->FindString("be:filetypes",(const char **)(&mime));
					ni.SetType(mime);
				}
				return;
				}
				break;
			case B_TRASH_TARGET:
				DeleteSelection();
				break;
			case 'REQU':
				{
				const BMessage *prev = msg->Previous();
				if (prev->HasInt32("id"))
					prev->FindInt32("id",&id);
				else
					id = 0;
				size_t size;
				type_code *temp;
				prev->FindData("type",(uint32)(B_UINT32_TYPE),(const void **)&temp,new ssize_t);
				type = *temp;
				void *data;
				bool isattr;
				prev->FindBool("isattr",&isattr);
				if (!isattr) {
					const void *dat = ((reswindow *)(Window()))->openres->LoadResource(type,id,&size);
					data = new unsigned char[size];
					memcpy(data,dat,size);
				} else {
					char *name;
					prev->FindString("be:clip_name",(const char **)(&name));
					attr_info inf;
					((reswindow *)(Window()))->file->GetAttrInfo(name,&inf);
					size = inf.size;
					data = new unsigned char[size];
					((reswindow *)(Window()))->file->ReadAttr(name,type,0,data,size);
				}
				BMessage *msg1 = new BMessage('REQ1');
				msg1->AddData("data",type,data,size);
				msg->SendReply(msg1);
				}
				break;
			case B_MIME_DATA:
				goto app_reply;
				break;
		}
		return;
	}
	if (msg->HasPointer("this")) {
		message->FindPointer("this",(void **)(&old));
		if (old == this)
			return;
		BMessage *reply = new BMessage;
		message->SendReply('REQU',reply);
		message->FindString("name",(const char **)(&name));
		if (message->HasInt32("id"))
			message->FindInt32("id",&id);
		else
			id = 0;
		message = reply;
		message->GetInfo("data",&type);
		message->FindData("data",type,(const void **)&dat,&size);
		data = new unsigned char[size];
		memcpy(data,dat,size);
		if (data == NULL) {
			return;
		}
	} else {
		if (msg->GetInfo('MIME',0,&name,&type) != B_OK) {
			char adname[B_MIME_TYPE_LENGTH];
			entry_ref ref;
			if (msg->FindRef("refs",&ref) != B_OK) {
				BMessage *reply = new BMessage(B_COPY_TARGET);
				reply->AddString("be:types","image/x-bmp");
				msg->SendReply(reply,this);
				return;
			}
			name = new char[B_FILE_NAME_LENGTH];
			BEntry(&ref).GetName(name);
			BFile file(&ref, B_READ_ONLY);
			BNodeInfo(&file).GetType(adname); 
			if ((strcmp(adname,"application/x-be-resource") == 0)) {
				((reswindow *)(Window()))->CloseWindows();
				Window()->Lock();
				MakeEmpty();
				if (((reswindow *)(Window()))->openres->MergeFrom(&file) != B_OK) {
					beep();
					return;
				}
				FillRect(Bounds(),B_SOLID_LOW);
				Window()->Unlock();
				Init(((reswindow *)(Window()))->openres);
				((reswindow *)(Window()))->changes = true;
				return;
			} else {
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
					type = 'bits';
					static char clipping[] = "Clipping";
					name = clipping;
				} else {
					type = code_from_MIME(adname);
					if (type == 'bits') {
						BMallocIO io;
						BTranslatorRoster::Default()->Translate(&file,NULL,NULL,&io,'bits');
						size = io.BufferLength();
						data = new unsigned char[size];
						memcpy(data,io.Buffer(),size);
					} else {
						off_t temporar;
						file.GetSize(&temporar);
						size = temporar;
						data = new unsigned char[size];
						file.Read(data,size);
					}
				}
				id = 0;
			}
		} else {
	app_reply:
			unsigned char *data2;
			char *name2;
			msg->GetInfo(B_MIME_TYPE,0,&name2,&type);
			if (msg->FindData(name2,B_MIME_TYPE,0,(const void **)&data2,&size) != B_OK)
				return;
			type = code_from_MIME(name2);
			if (type == 'CSTR')
				data = new unsigned char[size + 1];
			else
				data = new unsigned char[size];
			memcpy(data,data2,size);
			if (type == 'CSTR') {
				data[size] = 0;
				size++;
			}
			if (msg->FindString("be:clip_name",(const char **)(&name2)) == B_OK) {
				name = new char[strlen(name2) + 1];
				strcpy(name,name2);
			} else
				name = new char(0);
		}
	}
	for(;x->openres->HasResource(type,id);id++) {}
	AddResource(type,id,name,size_t(size),data,false,false);
	delete [] data;
	return;
}

DoubleItem *restypeview::AddResource(type_code type,int32 id,const char *name,size_t length,void *data,bool isattr,bool invoke) {
	if (isattr)
		((reswindow *)(Window()))->file->WriteAttr(name,type,0,data,length);
	else
		((reswindow *)(Window()))->openres->AddResource(type,id,data,length,name);
	DoubleItem *item = new DoubleItem(id,name,this,isattr);
	Window()->Lock();
	AddUnder(item,FindType(type,isattr));
	Sort();
	if (invoke)
		item->Invoke();
	((reswindow *)(Window()))->changes = true;
	Invalidate(Bounds());
	if (Window()->IsLocked())
		Window()->Unlock();
	return item;
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
	
void restypeview::KeyDown(const char *bytes, int32 numBytes) {
	if (numBytes == 1) {
		if ((*bytes == B_BACKSPACE) || (*bytes == B_DELETE)) {
			if (CurrentSelection() < 0) {
				BOutlineListView::KeyDown(bytes,numBytes);
				return;
			}
			if (ItemAt(CurrentSelection())->OutlineLevel() < 2) {
				BOutlineListView::KeyDown(bytes,numBytes);
				return;
			}
			DeleteSelection();
		}
	}
	BOutlineListView::KeyDown(bytes,numBytes);
}

void restypeview::DeleteSelection(DoubleItem *item) {
	if (item == NULL)
		item = (DoubleItem *)(ItemAt(CurrentSelection()));
	TypeItem *type = (TypeItem *)(Superitem(item));
	if (item->idstring != NULL)
		((reswindow *)(Window()))->openres->RemoveResource(type->type,item->id);
	else
		((reswindow *)(Window()))->file->RemoveAttr(item->name);
	if (item->win != NULL) {
		item->win->PostMessage(B_QUIT_REQUESTED);
		item->win = NULL;
	}
	if (CountItemsUnder(type,true) == 1)
		RemoveItem(type);
	else
		RemoveItem(item);
	if (CountItemsUnder(resources,true) == 0) {
		resources->SetText("No Resources");
		resources->SetEnabled(false);
	}
	if (CountItemsUnder(attributes,true) == 0) {
		attributes->SetText("No Attributes");
		attributes->SetEnabled(false);
	}
	((reswindow *)(Window()))->changes = true;
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

#endif /* RESOURCER_MAIN_WINDOW_H */
