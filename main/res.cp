// Resource list items and the resource list view.
// Declarations are in res.h.

#include "includes.h"
#include "preamble.h"
#include "res.h"
#include "reswindow.h"
#include "pluginwindow.h"
#include "optwindow.h"

TypeItem::TypeItem(type_code code) : BStringItem("type") {
	type = code;
	app_info info;
	be_app->GetAppInfo(&info);
	BEntry entry(&(info.ref));
	BDirectory dir;
	entry.GetParent(&dir);
	entry.SetTo(&dir,"editors");
	BPath path;
	entry.GetPath(&path);
	path.Append(TypeCode());
	if (entry.Exists() == false) {
		description = "Unknown";
	} else {
		image_id editor = load_add_on(path.Path());
		char *temp;
		if (get_image_symbol(editor,"description",B_SYMBOL_TYPE_DATA,(void **)(&temp)) != B_NO_ERROR)
			description = "Unknown";
		else {
			char *copy = new char[strlen(temp) + 1];
			strcpy(copy,temp);
			description = copy;
		}
	}
	char *tempy = new char[10 + strlen(description)];
	sprintf(tempy,"%s (%s)",TypeCode(),description);
	SetText(tempy);
}

char *
TypeItem::TypeCode(void) {
	type_code code = flipcode(type);
	char *typea = new char[5];
	strncpy(typea,(char *)(&code),4);
	typea[4] = 0;
	return typea;
}

void
TypeItem::SetTypeCode(char *code) {
	 strncpy((char *)(&type),code,4);
	 type = flipcode(type);
}

DoubleItem::DoubleItem(int32 resid,const char *resname,BOutlineListView *owner,bool isattr) : BListItem(0,true) {
	if (isattr) {
		idstring = NULL;
	} else {
		idstring = new char[20];
		sprintf(idstring,"%ld",(long)resid);
	}
	name = new char[strlen(resname) + 1];
	strcpy(name,resname);
	id = resid;
	win = NULL;
	parent = owner;
}

void
DoubleItem::DrawItem(BView *owner, BRect frame, bool complete) {
	if (IsSelected()) {
		owner->SetHighColor(150,150,150);
		owner->SetLowColor(150,150,150);
	} else
		owner->SetHighColor(255,255,255);
	owner->FillRect(frame);
	owner->SetHighColor(0,0,0);
	if (idstring != NULL) {
		owner->DrawString(idstring,BPoint(frame.left + 5,frame.top + 10));
		owner->DrawString(name,BPoint(frame.left + 50,frame.top + 10));
	} else {
		owner->DrawString(name,BPoint(frame.left + 5,frame.top + 10));
	}
	owner->SetLowColor(255,255,255);
}

DoubleItem::~DoubleItem(void) {
	delete [] name;
	if (idstring != NULL)
		delete [] idstring;
}

int sortList(const BListItem *one,const BListItem *two) {
	if (is_instance_of((BListItem *)one,BStringItem)) {
		return 0;
	}
	/*if (one->OutlineLevel() != two->OutlineLevel())
		(new BAlert("ck","Yikes!","OK"))->Go();*/
	if (is_kind_of((BListItem *)one,TypeItem)) {
		if (alphabet(((TypeItem *)(one))->TypeCode(),((TypeItem *)(two))->TypeCode()))
			return 1;
		else
			return -1;
	}
	if (is_kind_of((BListItem *)one,DoubleItem)) {
		if (((DoubleItem *)(one))->idstring != NULL) {
			if (((DoubleItem *)(one))->id > ((DoubleItem *)(two))->id)
				return 1;
			else
				return -1;
		} else {	
			if (alphabet(((DoubleItem *)(one))->name,((DoubleItem *)(two))->name))
				return 1;
			else
				return -1;
		}
	}
	return 0;
}

restypeview::restypeview(BResources *openres) : BOutlineListView(BRect(0,1 + mbheight,350 - B_V_SCROLL_BAR_WIDTH,400 - B_H_SCROLL_BAR_HEIGHT),"restype",B_SINGLE_SELECTION_LIST,B_FOLLOW_ALL_SIDES) {
	Init(openres);
	SetInvocationMessage(new BMessage('rsiv'));
}

void
restypeview::Init(BResources *openres) {
	attr_info inf;
	list_state rescol;
	list_state attrcol;
	BNode node("/boot");
	if (node.GetAttrInfo("rescol",&inf) == B_OK) {
		node.ReadAttr("rescol",'STAT',0,&rescol,sizeof(list_state));
		node.ReadAttr("attrcol",'STAT',0,&attrcol,sizeof(list_state));
	} else {
		rescol = expanded;
		attrcol = expanded;
	}
	resources = new BStringItem("Resources");
	AddItem(resources);
	if (rescol == collapsed)
		Collapse(resources);
	attributes = new BStringItem("Attributes");
	AddItem(attributes);
	if (attrcol == collapsed)
		Collapse(attributes);
	BPoint point = find_center(Bounds(),be_plain_font->StringWidth("Loading Resources..."),20).LeftTop();
	DrawString("Loading Resources...",point);
	AddResources(openres);
	Invalidate(Bounds());
	if (CountItemsUnder(resources,true) == 0) {
		resources->SetText("No Resources");
		resources->SetEnabled(false);
	}
	if (CountItemsUnder(attributes,true) == 0) {
		attributes->SetText("No Attributes");
		attributes->SetEnabled(false);
	}
}

void
restypeview::AddResources(BResources *res) {
	int32 index;
	type_code type_found;
	int32 id;
	char *name;
	size_t length;
	for (index = 0;res->GetResourceInfo(index, &type_found, &id,(const char **)(&name), &length);index++) {
		AddUnder(new DoubleItem(id,name,this,false),FindType(type_found,false));
	}
	BFile file(res->File());
	file.RewindAttrs();
	attr_info attrinfo;
	name = new char[B_ATTR_NAME_LENGTH];
	for (index = 0;file.GetNextAttrName(name) == B_OK;index++) {
		file.GetAttrInfo(name,&attrinfo);
		type_found = attrinfo.type;
		AddUnder(new DoubleItem(id,name,this,true),FindType(type_found,true));
	}
	Sort();
}

void
restypeview::Sort(void) {
	SortItemsUnder(resources,false,&sortList);
	SortItemsUnder(attributes,false,&sortList);
	BListItem *cur;
	for (int32 i = CountItemsUnder(resources,true); i >= 0; i--) {
		cur = ItemUnderAt(resources,true,i);
		SortItemsUnder(cur,false,&sortList);
	}
	for (int32 i = CountItemsUnder(attributes,true); i >= 0; i--) {
		cur = ItemUnderAt(attributes,true,i);
		SortItemsUnder(cur,false,&sortList);
	}
}

TypeItem *
restypeview::FindType(type_code type,bool attr) {
	TypeItem *cur;
	if (attr) {
		attributes->SetText("Attributes");
		attributes->SetEnabled(true);
		for (int32 i = 0;true;i++) {
			cur = (TypeItem *)(ItemUnderAt(attributes,true,i));
			if (cur == NULL) {
				cur = new TypeItem(type);
				AddUnder(cur,attributes);
				Collapse(cur);
				break;
			}
			if (cur->type == type) {
				break;
			}
		}
		return cur;
	} else {
		resources->SetText("Resources");
		resources->SetEnabled(true);
		for (int32 i = 0;true;i++) {
			cur = (TypeItem *)(ItemUnderAt(resources,true,i));
			if (cur == NULL) {
				cur = new TypeItem(type);
				AddUnder(cur,resources);
				Collapse(cur);
				break;
			}
			if (cur->type == type) {
				break;
			}
		}
		return cur;
	}
}

void
restypeview::MouseMoved(BPoint point,uint32 transit,const BMessage *msg) {
	rgb_color oldcolor;
	switch (transit) {
		case B_ENTERED_VIEW:
			if (msg != NULL) {
				if (msg->HasPointer("this")) {
					restypeview *t;
					msg->FindPointer("this",(void **)&t);
					if (t == this)
						break;
				}
				oldcolor = HighColor();
				SetHighColor(0,152,255);
				StrokeRect(Bounds());
				SetHighColor(oldcolor);
			}
			break;
		case B_INSIDE_VIEW:
			break;
		case B_EXITED_VIEW:
			StrokeRect(Bounds(),B_SOLID_LOW);
			break;
	}
}

void
restypeview::SelectionChanged(void) {
	if (CurrentSelection() >= 0) {
		if (ItemAt(CurrentSelection())->OutlineLevel() != 2)
			goto DISABLEITEMS;
		BMenuBar *mbar = Window()->KeyMenuBar();
		mbar->FindItem(-200)->SetEnabled(true);
		mbar->FindItem(-300)->SetEnabled(true);
		mbar->FindItem(-400)->SetEnabled(true);
		mbar->FindItem(-500)->SetEnabled(true);
		mbar->FindItem(-800)->SetEnabled(true);
		mbar->FindItem(-900)->SetEnabled(true);
	} else {
		DISABLEITEMS:
		BMenuBar *mbar = Window()->KeyMenuBar();
		mbar->FindItem(-200)->SetEnabled(false);
		mbar->FindItem(-300)->SetEnabled(false);
		mbar->FindItem(-400)->SetEnabled(false);
		mbar->FindItem(-500)->SetEnabled(false);
		mbar->FindItem(-800)->SetEnabled(false);
		mbar->FindItem(-900)->SetEnabled(false);
	}
}

type_code code_from_MIME(char *MIME) {
	type_code type = 'RAWT';
	BMimeType mime(MIME);
	if (mime == BMimeType("text/plain"))
		type = 'CSTR';
	if (BMimeType("image").Contains(&mime))
		type = 'bits';
	if (BMimeType("video").Contains(&mime))
		type = 'MOOV';
	if (BMimeType("resource").Contains(&mime)) {
		TypeItem xyz('none');
		xyz.SetTypeCode(MIME + 9);
		type = xyz.type;
	}
	return type;
}

const char *MIME_from_code(type_code code) {
	const char *MIME;
	switch (code) {
		case 'CSTR':
			MIME = "text/plain";
			break;
		case 'bits':
			MIME = "image/x-portable-pixmap";
			break;
		case 'MOOV':
			MIME = "video";
			break;
		default:
			{
				char *resmime = new char[B_MIME_TYPE_LENGTH];
				sprintf(resmime,"resource/%s",TypeItem(code).TypeCode());
				MIME = resmime;
			}
			break;
	}
	return MIME;
}

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
