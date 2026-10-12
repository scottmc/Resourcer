// Resource list items and the resource list view.
// Declarations are in res.h.

#include "includes.h"
#include "preamble.h"
#include "res.h"
#include "reswindow.h"
#include "pluginwindow.h"
#include "optwindow.h"
#include <storage/FindDirectory.h>
#include <strings.h>

TypeItem::TypeItem(type_code code) : BStringItem("type") {
	type = code;
	descbuf = NULL;
	app_info info;
	be_app->GetAppInfo(&info);
	BEntry entry(&(info.ref));
	BDirectory dir;
	entry.GetParent(&dir);
	entry.SetTo(&dir,"editors");
	BPath path;
	entry.GetPath(&path);
	path.Append(EditorName());
	//	entry still names the editors directory, so look at the editor itself
	entry.SetTo(path.Path());
	if (entry.Exists() == false) {
		description = "Unknown";
	} else {
		image_id editor = load_add_on(path.Path());
		char *temp;
		if (get_image_symbol(editor,"description",B_SYMBOL_TYPE_DATA,(void **)(&temp)) != B_NO_ERROR)
			description = "Unknown";
		else {
			descbuf = new char[strlen(temp) + 1];
			strcpy(descbuf,temp);
			description = descbuf;
		}
		if (editor >= 0)
			unload_add_on(editor);
	}
	char *tempy = new char[10 + strlen(description)];
	sprintf(tempy,"%s (%s)",TypeCode(),description);
	SetText(tempy);
	delete [] tempy;
}

TypeItem::~TypeItem(void) {
	delete [] descbuf;
}

char *
TypeItem::TypeCode(void) {
	type_code code = flipcode(type);
	strncpy(codebuf,(char *)(&code),4);
	codebuf[4] = 0;
	return codebuf;
}

//	The name of the editor for this type: the type code without any spaces at
//	its end, so 'PNG ' is edited by PNG.
char *
TypeItem::EditorName(void) {
	strcpy(editorbuf,TypeCode());
	for (int i = 3;i > 0 && editorbuf[i] == ' ';i--)
		editorbuf[i] = 0;
	return editorbuf;
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

//	The row height follows the font, as it does for the type rows
void
DoubleItem::Update(BView *owner, const BFont *font) {
	BListItem::Update(owner,font);
	font_height height;
	font->GetHeight(&height);
	SetHeight(ceilf(height.ascent) + ceilf(height.descent) + ceilf(height.leading) + 2);
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

	//	Centre the text vertically in the row and scale the id column with the
	//	font instead of using fixed pixel offsets.
	font_height height;
	owner->GetFontHeight(&height);
	float text_height = ceilf(height.ascent) + ceilf(height.descent);
	float baseline = frame.top + (frame.Height() - text_height) / 2 + ceilf(height.ascent);
	if (idstring != NULL) {
		float name_left = frame.left + 5 + owner->StringWidth("00000") + 10;
		owner->DrawString(idstring,BPoint(frame.left + 5,baseline));
		owner->DrawString(name,BPoint(name_left,baseline));
	} else {
		owner->DrawString(name,BPoint(frame.left + 5,baseline));
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

restypeview::restypeview(BResources *openres) : BOutlineListView("restype",B_SINGLE_SELECTION_LIST) {
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
			//	Redraw rather than stroke the edge in the background colour: that
			//	line is drawn into the scrolling content, so it was carried into
			//	the middle of the list as soon as the list was scrolled.
			Invalidate();
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
	if (mime == BMimeType("image/x-hvif"))		//---a vector icon is not a picture
		type = B_VECTOR_ICON_TYPE;
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
		case B_VECTOR_ICON_TYPE:
			MIME = "image/x-hvif";
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

//	Adds the vector icon resources ('VICN') of a dropped file, usually an
//	application, to the open resources. A resource whose id is already taken gets
//	the next free one. Returns how many were added, zero for a file that is not an
//	application or has no vector icons.
static int32 add_vector_icons(restypeview *list,reswindow *window,BFile *file) {
	BResources source;
	if (source.SetTo(file) != B_OK)
		return 0;
	int32 added = 0;
	int32 sourceid;
	const char *sourcename;
	size_t length;
	for (int32 i = 0;source.GetResourceInfo(B_VECTOR_ICON_TYPE,i,&sourceid,&sourcename,&length);i++) {
		BString name(sourcename != NULL ? sourcename : "");
		const void *loaded = source.LoadResource(B_VECTOR_ICON_TYPE,sourceid,&length);
		if (loaded == NULL || length == 0)
			continue;
		unsigned char *copy = new unsigned char[length];
		memcpy(copy,loaded,length);
		int32 id = sourceid;
		while (window->openres->HasResource(B_VECTOR_ICON_TYPE,id))
			id++;
		list->AddResource(B_VECTOR_ICON_TYPE,id,name.String(),length,copy,false,false);
		delete [] copy;
		added++;
	}
	return added;
}

//	The image formats that have an editor of their own: the same list as
//	IMAGE_EDITORS in the top level Makefile.
static bool is_image_type(type_code type) {
	static const type_code types[] = {'GIF ','JPEG','PNG ','PPM ','TGA ','BMP ','TIFF','WebP','PSD ','AVIF','ICNS','ICO ','PCX ','JP2 ','SGI ','WBI ','RAWI'};
	for (size_t i = 0;i < sizeof(types) / sizeof(types[0]);i++) {
		if (types[i] == type)
			return true;
	}
	return false;
}

//	If a drop landed on the heading of an image type, or on one of its resources,
//	says which type. Other types, and attributes, are left to the usual drop.
static bool image_type_dropped_on(restypeview *list,BMessage *msg,type_code *type) {
	BPoint where = msg->DropPoint();
	list->ConvertFromScreen(&where);
	int32 index = list->IndexOf(where);
	if (index < 0)
		return false;
	BListItem *item = list->ItemAt(index);
	if (is_instance_of(item,DoubleItem))
		item = list->Superitem(item);
	if (item == NULL || !is_instance_of(item,TypeItem) || list->Superitem(item) != list->resources)
		return false;
	*type = ((TypeItem *)(item))->type;
	return is_image_type(*type);
}

//	Reads an image file as the given image format. A file that already is in that
//	format is kept as it is, anything else is converted by Haiku's translators.
//	On success the data is new[]'d and belongs to the caller.
static bool image_for_type(BFile *file,type_code type,unsigned char **data,size_t *length) {
	BTranslatorRoster *roster = BTranslatorRoster::Default();
	translator_info info;
	if (roster->Identify(file,NULL,&info,0,NULL,B_TRANSLATOR_BITMAP) != B_OK) {
		(new BAlert("alert","That file is not an image Haiku can read.","OK",NULL,NULL,B_WIDTH_AS_USUAL,B_STOP_ALERT))->Go();
		return false;
	}
	if (info.type == type) {
		off_t size = 0;
		file->GetSize(&size);
		*data = new unsigned char[size > 0 ? size : 1];
		*length = size > 0 ? file->ReadAt(0,*data,size) : 0;
		return true;
	}
	BMallocIO io;
	file->Seek(0,SEEK_SET);
	if (roster->Translate(file,&info,NULL,&io,type) != B_OK || io.BufferLength() == 0) {
		(new BAlert("alert","That image could not be converted to this format.","OK",NULL,NULL,B_WIDTH_AS_USUAL,B_STOP_ALERT))->Go();
		return false;
	}
	*length = io.BufferLength();
	*data = new unsigned char[*length];
	memcpy(*data,io.Buffer(),*length);
	return true;
}

//	True for a file named like an rdef source, such as icons.rdef.
static bool is_rdef_name(const char *name) {
	size_t length = strlen(name);
	return length > 5 && strcasecmp(name + length - 5,".rdef") == 0;
}

//	Adds an argument to a shell command line, quoted so that spaces and quotes
//	in a file name reach the command unchanged.
static void add_quoted(BString *command,const char *argument) {
	*command << "'";
	for (const char *c = argument;*c != 0;c++) {
		if (*c == '\'')
			*command << "'\\''";
		else
			command->Append(c,1);
	}
	*command << "'";
}

//	Compiles an rdef file into the resource file at output with Haiku's own rc,
//	which also finds the rdef file's #include files next to it. The output name
//	has to end in ".rsrc", as rc adds that itself if it doesn't. rc exits with
//	success even when it fails, so what counts is whether it made the file. When it
//	didn't, the reason rc gave is left in errors.
static bool compile_rdef(const entry_ref *ref,const char *output,BString *errors) {
	BPath path(ref);
	BPath folder;
	path.GetParent(&folder);
	BString command("rc -I ");
	add_quoted(&command,folder.Path());
	command << " -o ";
	add_quoted(&command,output);
	command << " ";
	add_quoted(&command,path.Path());
	command << " 2>&1";
	FILE *pipe = popen(command.String(),"r");
	if (pipe == NULL) {
		*errors = "rc could not be started.";
		return false;
	}
	char buffer[256];
	size_t got;
	while ((got = fread(buffer,1,sizeof(buffer) - 1,pipe)) > 0) {
		buffer[got] = 0;
		if (errors->Length() < 1000)
			*errors << buffer;
	}
	pclose(pipe);
	if (!BEntry(output).Exists()) {
		if (errors->Length() == 0)
			*errors = "rc did not make a resource file.";
		return false;
	}
	return true;
}

//	Picks a name for a new temporary resource file, the way File > New does.
static void temporary_resource_path(BPath *path) {
	BPath folder;
	find_directory(B_SYSTEM_TEMP_DIRECTORY,&folder);
	BDirectory tmp(folder.Path());
	BEntry entry;
	char text[B_FILE_NAME_LENGTH];
	sprintf(text,"tmpresfile0.rsrc");
	for (int i = 1;tmp.FindEntry(text,&entry) != B_ENTRY_NOT_FOUND;i++)
		sprintf(text,"tmpresfile%d.rsrc",i);
	path->SetTo(folder.Path(),text);
}

//	An rdef file was dropped on the list: compile it, then either open the result
//	as a new resource file or merge its resources into this window's file.
static void rdef_dropped(restypeview *list,reswindow *window,const entry_ref *ref) {
	char name[B_FILE_NAME_LENGTH];
	BEntry(ref,true).GetName(name);
	BString text("\"");
	text << name << "\" is an rdef file.\n\nCreate a new resource file from it, or merge its resources into \"" << window->Title() << "\"?";
	BAlert *alert = new BAlert("rdef",text.String(),"Cancel","Merge","New File",B_WIDTH_AS_USUAL,B_OFFSET_SPACING,B_INFO_ALERT);
	alert->SetShortcut(0,B_ESCAPE);
	int32 choice = alert->Go();
	if (choice != 1 && choice != 2)
		return;
	BPath temp;
	temporary_resource_path(&temp);
	BString errors;
	if (!compile_rdef(ref,temp.Path(),&errors)) {
		BEntry(temp.Path()).Remove();
		BString message("rc could not compile \"");
		message << name << "\":\n\n" << errors;
		(new BAlert("alert",message.String(),"OK",NULL,NULL,B_WIDTH_AS_USUAL,B_STOP_ALERT))->Go();
		return;
	}
	if (choice == 1) {
		//	Merge. A resource with the same type and ID as one already here is
		//	replaced by the one from the rdef file.
		BFile compiled(temp.Path(),B_READ_ONLY);
		window->CloseWindows();
		list->MakeEmpty();
		status_t result = window->openres->MergeFrom(&compiled);
		list->Init(window->openres);
		BEntry(temp.Path()).Remove();
		if (result != B_OK) {
			(new BAlert("alert","The resources could not be merged.","OK",NULL,NULL,B_WIDTH_AS_USUAL,B_STOP_ALERT))->Go();
			return;
		}
		window->changes = true;
		return;
	}
	//	New file: the compiled file becomes an untitled window, like File > New.
	//	Closing it asks to save it, and Save As moves it out of the temporary folder.
	BFile compiled(temp.Path(),B_READ_WRITE);
	BNodeInfo(&compiled).SetType("application/x-be-resource");
	entry_ref created;
	get_ref_for_path(temp.Path(),&created);
	BString title(name);
	title.Truncate(title.Length() - 5);
	title << ".rsrc";
	reswindow *fresh = new reswindow(title.String(),created);
	fresh->istempfile = true;
	fresh->changes = true;
}

void restypeview::MessageReceived(BMessage *msg) {
	//	A drop is handled below. Passing it to the list view as well only made
	//	Haiku print "couldn't understand the message" for every file dropped.
	if (msg->WasDropped())
		Invalidate();		//---removes the drop highlight; see MouseMoved
	else
		BOutlineListView::MessageReceived(msg);
	reswindow *x = (reswindow *)(Window());
	char *name;
	char *ownedname = NULL;		//---allocated here, freed once the resource is added
	type_code type;
	void *dat;
	ssize_t size;
	int32 id = 0;
	BMessage *message = msg;
	BMessage replied;			//---reply to a drag, owned here instead of leaked
	BMessage clipped;			//---a message archived inside a clipping
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
				ssize_t typesize;
				prev->FindData("type",(uint32)(B_UINT32_TYPE),(const void **)&temp,&typesize);
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
				ssize_t typesize;
				prev->FindData("type",(uint32)(B_UINT32_TYPE),(const void **)&temp,&typesize);
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
				delete msg1;
				delete [] (unsigned char *)data;
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
		message->SendReply('REQU',&replied);
		message->FindString("name",(const char **)(&name));
		if (message->HasInt32("id"))
			message->FindInt32("id",&id);
		else
			id = 0;
		message = &replied;
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
				BMessage reply(B_COPY_TARGET);
				reply.AddString("be:types","image/x-bmp");
				msg->SendReply(&reply,this);
				return;
			}
			name = ownedname = new char[B_FILE_NAME_LENGTH];
			BEntry(&ref).GetName(name);
			if (is_rdef_name(name)) {
				rdef_dropped(this,x,&ref);
				delete [] ownedname;
				return;
			}
			BFile file(&ref, B_READ_ONLY);
			BNodeInfo(&file).GetType(adname); 
			if ((strcmp(adname,"application/x-be-resource") == 0)) {
				((reswindow *)(Window()))->CloseWindows();
				Window()->Lock();
				MakeEmpty();
				if (((reswindow *)(Window()))->openres->MergeFrom(&file) != B_OK) {
					beep();
					Window()->Unlock();
					delete [] ownedname;
					return;
				}
				FillRect(Bounds(),B_SOLID_LOW);
				Window()->Unlock();
				Init(((reswindow *)(Window()))->openres);
				((reswindow *)(Window()))->changes = true;
				delete [] ownedname;
				return;
			} else {
				type_code target;
				if (image_type_dropped_on(this,msg,&target)) {
					unsigned char *image;
					size_t imagelength;
					if (image_for_type(&file,target,&image,&imagelength)) {
						for (id = 0;x->openres->HasResource(target,id);id++) {}
						AddResource(target,id,name,imagelength,image,false,false);
						delete [] image;
					}
					delete [] ownedname;
					return;
				}
				{
					BMessage *q = &clipped;
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
					if (add_vector_icons(this,x,&file) > 0) {
						delete [] ownedname;
						return;
					}
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
				name = ownedname = new char[strlen(name2) + 1];
				strcpy(name,name2);
			} else {
				name = ownedname = new char[1];
				name[0] = 0;
			}
		}
	}
	for(;x->openres->HasResource(type,id);id++) {}
	AddResource(type,id,name,size_t(size),data,false,false);
	delete [] data;
	delete [] ownedname;
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

void DoubleItem::Invoke(bool generic) {
	reswindow *x = (reswindow *)parent->Window(); //----Context is important
	bool genoverride = generic;					//------whether to use hex editor or not
	if (win != NULL) {							//------If we are open, we only need to activate our window---
		win->Hide();
		win->Show();
		win->Activate(true);
		return;
	}
	app_info info;                        //-Find the editors directory
	be_app->GetAppInfo(&info);            //-|
	BEntry entry(&(info.ref));            //-|
	BDirectory dir;						  //-|
	entry.GetParent(&dir);				  //-|
	entry.SetTo(&dir,"editors");          //-|
	dir.SetTo(&entry);                    //-|
	TypeItem *super = (TypeItem *)parent->Superitem(this);  //-Get our type code
	type_code type = super->type;                           //-|
	BPath path(&dir,super->EditorName());					//--editors/typecode, the path of our editor
	plug_in plugin;										//--handy little struct, holds useful information, the def is in preamble.h
	plugin.win = x;                                     //-Set it up
	entry.SetTo(path.Path());
	BEntry entry2(&dir,"unknown");                      //-The hex editor
	if ((entry.Exists() == false) || (genoverride == true)) { //-Do we not have an editor or do we want to use the hex editor?
		if (entry2.Exists() == false) {                       //-Does the hex editor exist?
			BAlert *alert = new BAlert("noedit","No editor found.","OK",NULL,NULL,B_WIDTH_AS_USUAL,B_STOP_ALERT); //-Yikes!
			alert->Go();
			return;
		} else {
			entry2.GetPath(&path); //--Let's use the hex editor
		}
	}
	plugin.plugin = load_add_on(path.Path()); //--Load the add-on
	BResources **tfile;
	if (get_image_symbol(plugin.plugin,"file",B_SYMBOL_TYPE_DATA,(void **)(&tfile)) == B_NO_ERROR)
		*tfile = x->openres;  //--------WIND uses this to get info on bits and CSTR resources
	image_id *tplugin;
	if (get_image_symbol(plugin.plugin,"me",B_SYMBOL_TYPE_DATA,(void **)(&tplugin)) == B_NO_ERROR)
		*tplugin = plugin.plugin; //---No one uses, but you could to get your plug-in id
	if (get_image_symbol(plugin.plugin,"loaddata",B_SYMBOL_TYPE_TEXT,(void **)(&(plugin.loaddata))) != B_NO_ERROR) {
			BAlert *alert = new BAlert("noedit","The editor could not be launched.","OK",NULL,NULL,B_WIDTH_AS_USUAL,B_STOP_ALERT);
			alert->Go();
			return;         //---Load Data loads the data. If it doesn't exist, there's a problem
	}
	get_image_symbol(plugin.plugin,"savedata",B_SYMBOL_TYPE_TEXT,(void **)(&(plugin.savedata)));
	get_image_symbol(plugin.plugin,"messaging",B_SYMBOL_TYPE_TEXT,(void **)(&(plugin.messaging)));
	unsigned char *data; //---Send in our data, we'll delete it later
	const void *dat;
	size_t length;
	if (idstring != NULL) {
		dat = x->openres->LoadResource(type,id,&length);
		if (dat == NULL) {
			data = NULL;
			length = 0;
		} else {
			data = new unsigned char[length];
			memcpy(data,dat,length);
		}
	} else {
		attr_info inf;
		if (x->file->GetAttrInfo(name,&inf) == B_OK) {
			length = inf.size;
			data = new unsigned char[length];
			x->file->ReadAttr(name,type,0,data,length);
		} else {
			data = NULL;
			length = 0;
		}
	}
	plugin.olddata = data;        //---To check if it's changed, we need the original data
	plugin.oldlength = length;    //-|
	char winname[255];            //--Set the window name
	if (idstring == NULL)
		sprintf(winname,"%s Attribute Name: %s",super->TypeCode(),name);
	else {
		if (name[0] == 0)
			sprintf(winname,"%s Resource Id: %ld",super->TypeCode(),(long)id);
		else
			sprintf(winname,"%s Resource Id: %ld Name: %s",super->TypeCode(),(long)id,name);
	}
	//--An editor can export "const bool resizable = true" to get a resizable window
	bool *resizable = NULL;
	bool canresize = false;
	if (get_image_symbol(plugin.plugin,"resizable",B_SYMBOL_TYPE_DATA,(void **)(&resizable)) == B_NO_ERROR && resizable != NULL)
		canresize = *resizable;
	pluginwindow *win = new pluginwindow(&(this->win),plugin,winname,id,type,name,(idstring == NULL),canresize);
	win->gray = new BView(BRect(0,0,300,300),"graybkgrd",B_FOLLOW_ALL_SIDES,B_WILL_DRAW | B_FRAME_EVENTS);
	win->gray->SetViewColor(216,216,216);
	win->AddChild(win->gray);
	if (length != 0) {	
		(*(plugin.loaddata))(data,length,win->gray);  //--Send in data
	} else {
		(*(plugin.loaddata))(NULL,0,win->gray);       //--It's a new resource
	}
	//--Whatever size the editor asked for, keep the whole window on the screen
	BRect screen = BScreen(win).Frame();
	BRect frame = win->Frame();
	if (frame.Width() > screen.Width() - 20 || frame.Height() > screen.Height() - 60)
	{
		float width = frame.Width();
		float height = frame.Height();
		if (width > screen.Width() - 20)
			width = screen.Width() - 20;
		if (height > screen.Height() - 60)
			height = screen.Height() - 60;
		win->ResizeTo(width,height);
	}
	frame = win->Frame();
	float shiftx = 0, shifty = 0;
	if (frame.right > screen.right - 10)
		shiftx = screen.right - 10 - frame.right;
	if (frame.left + shiftx < screen.left + 10)
		shiftx = screen.left + 10 - frame.left;
	if (frame.bottom > screen.bottom - 10)
		shifty = screen.bottom - 10 - frame.bottom;
	if (frame.top + shifty < screen.top + 30)
		shifty = screen.top + 30 - frame.top;
	if (shiftx != 0 || shifty != 0)
		win->MoveBy(shiftx,shifty);
	win->Show();
}
