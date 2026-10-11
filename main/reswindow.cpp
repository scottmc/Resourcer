// The main document window and its helper threads.
// Declarations are in reswindow.h.

#include <ctype.h>
#include <errno.h>
#include "includes.h"
#include "preamble.h"
#include "class.h"
#include "res.h"
#include "reswindow.h"
#include "rdefexport.h"
#include "pluginwindow.h"
#include "optwindow.h"

int32 copy(void *y);
int32 getdata(void *y);
int32 copytoattr(void *y);

FillerView::FillerView(float height) : BView("BottomFiller",B_WILL_DRAW) {
	SetViewColor(B_TRANSPARENT_COLOR);
	SetExplicitMinSize(BSize(0,height));
	SetExplicitPreferredSize(BSize(0,height));
	SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED,height));
}

void
FillerView::Draw(BRect toUpdate) {
	rgb_color panel = ui_color(B_PANEL_BACKGROUND_COLOR);
	SetHighColor(panel);
	FillRect(toUpdate);
}

ListScrollView::ListScrollView(const char *name,BView *target,uint32 flags,bool horizontal,bool vertical,border_style border) : BScrollView(name,target,flags,horizontal,vertical,border) {
}

//	The scroll bar's top edge shares a line with the menu bar's bottom edge, as
//	it does in Haiku's own apps (this is how Koder does it): after every layout
//	pass move the bar up one pixel and make it one pixel taller, which leaves
//	its bottom where it was.
void
ListScrollView::DoLayout(void) {
	BScrollView::DoLayout();
	BScrollBar *bar = ScrollBar(B_VERTICAL);
	if (bar == NULL)
		return;
	bar->ResizeBy(0,1);
	bar->MoveBy(0,-1);
}

//	Types that only make sense once in a file: the application's flags and
//	version information and its signature. Everything else may have several.
static bool
can_duplicate(type_code type) {
	return (type != 'APPF') && (type != 'APPV') && (type != 'MIMS');
}

//	The resource that is highlighted, or NULL if the highlight is on a type, a
//	heading, an attribute or nothing.
static DoubleItem *
selected_resource(restypeview *list) {
	int32 selection = list->CurrentSelection();
	if (selection < 0)
		return NULL;
	BListItem *item = list->ItemAt(selection);
	if (item->OutlineLevel() < 2)
		return NULL;
	DoubleItem *resource = (DoubleItem *)(item);
	if (resource->idstring == NULL)
		return NULL;
	return resource;
}

void
reswindow::MenusBeginning(void) {
	DoubleItem *resource = selected_resource(res);
	BMenuItem *item = menus->FindItem('dupl');
	if (item != NULL)
		item->SetEnabled((resource != NULL) && can_duplicate(((TypeItem *)(res->Superitem(resource)))->type));
	item = menus->FindItem('rdfr');
	if (item != NULL)
		item->SetEnabled(resource != NULL);
	item = menus->FindItem('rdfc');
	if (item != NULL)
		item->SetEnabled(resource != NULL);
}

//	A name for the type, such as 'ICON', for use in file names.
static void
type_name(type_code type,char *into) {
	into[0] = (type >> 24) & 0xFF;
	into[1] = (type >> 16) & 0xFF;
	into[2] = (type >> 8) & 0xFF;
	into[3] = type & 0xFF;
	into[4] = 0;
	for (int i = 0;i < 4;i++) {
		if (!isalnum(into[i]))
			into[i] = '_';
	}
}

//	Asks where to put the rdef text, for the whole file or the highlighted
//	resource. The answer comes back as 'rdfx' to SaveRDEF.
void
reswindow::ExportRDEF(bool whole) {
	type_code type = 0;
	int32 id = 0;
	char suggestion[B_FILE_NAME_LENGTH];
	if (whole) {
		sprintf(suggestion,"%.*s.rdef",(int)(B_FILE_NAME_LENGTH - 6),fileref.name);
	} else {
		DoubleItem *current = selected_resource(res);
		if (current == NULL)
			return;
		type = ((TypeItem *)(res->Superitem(current)))->type;
		id = current->id;
		if ((current->name != NULL) && (current->name[0] != 0)) {
			sprintf(suggestion,"%.*s.rdef",(int)(B_FILE_NAME_LENGTH - 6),current->name);
			//	Names such as BEOS:ICON are awkward in a file name
			for (char *c = suggestion;*c != 0;c++) {
				if ((*c == ':') || (*c == '/'))
					*c = '_';
			}
		} else {
			char code[5];
			type_name(type,code);
			sprintf(suggestion,"%s_%ld.rdef",code,(long)(id));
		}
	}
	if (rdefpanel == NULL)
		rdefpanel = new BFilePanel(B_SAVE_PANEL,new BMessenger(this),NULL,B_FILE_NODE,false);
	BMessage *msg = new BMessage('rdfx');
	msg->AddBool("whole",whole);
	msg->AddUInt32("type",type);
	msg->AddInt32("id",id);
	rdefpanel->SetMessage(msg);
	rdefpanel->SetSaveText(suggestion);
	rdefpanel->Show();
}

//	Writes the rdef text to the place chosen in the panel.
void
reswindow::SaveRDEF(BMessage *message) {
	entry_ref dir;
	const char *name;
	if ((message->FindRef("directory",&dir) != B_OK) || (message->FindString("name",&name) != B_OK))
		return;
	bool whole = true;
	uint32 type = 0;
	int32 id = 0;
	message->FindBool("whole",&whole);
	message->FindUInt32("type",&type);
	message->FindInt32("id",&id);
	BPath path(&dir);
	path.Append(name);
	FILE *output = fopen(path.Path(),"w");
	if (output == NULL) {
		char text[B_PATH_NAME_LENGTH + 100];
		sprintf(text,"The RDEF file could not be written: %s",strerror(errno));
		(new BAlert("alert",text,"OK",NULL,NULL,B_WIDTH_AS_USUAL,B_STOP_ALERT))->Go();
		return;
	}
	status_t error;
	int32 count = 1;
	if (whole)
		error = rdef_export_all(openres,output,&count);
	else
		error = rdef_export_resource(openres,type,id,output);
	fclose(output);
	if (error != B_OK) {
		(new BAlert("alert","That resource could not be found, so nothing was exported.","OK",NULL,NULL,B_WIDTH_AS_USUAL,B_STOP_ALERT))->Go();
		return;
	}
	if (!whole)
		return;
	//	An rdef script only holds resources, so say if attributes were left out
	int32 attributes = 0;
	char attrname[B_ATTR_NAME_LENGTH];
	file->RewindAttrs();
	while (file->GetNextAttrName(attrname) == B_OK)
		attributes++;
	if ((attributes > 0) || (count == 0)) {
		char text[300];
		if (count == 0)
			sprintf(text,"This file has no resources, so the RDEF file is empty.");
		else
			sprintf(text,"The RDEF file has %ld resources. The file's %ld attributes can't be written in RDEF, so they were left out.",(long)(count),(long)(attributes));
		(new BAlert("alert",text,"OK",NULL,NULL,B_WIDTH_AS_USUAL,B_INFO_ALERT))->Go();
	}
}

//	Puts the highlighted resource on the clipboard as rdef text.
void
reswindow::CopyRDEF(void) {
	DoubleItem *current = selected_resource(res);
	if (current == NULL)
		return;
	type_code type = ((TypeItem *)(res->Superitem(current)))->type;
	FILE *temporary = tmpfile();
	if (temporary == NULL)
		return;
	if (rdef_export_resource(openres,type,current->id,temporary) == B_OK) {
		fseek(temporary,0,SEEK_END);
		long length = ftell(temporary);
		if (length > 0) {
			char *text = new char[length];
			rewind(temporary);
			size_t got = fread(text,1,length,temporary);
			if (be_clipboard->Lock()) {
				be_clipboard->Clear();
				be_clipboard->Data()->AddData("text/plain",B_MIME_TYPE,text,got);
				be_clipboard->Commit();
				be_clipboard->Unlock();
			}
			delete [] text;
		}
	}
	fclose(temporary);
}

//	Copies the highlighted resource to the next unused ID above its own, keeping
//	its name, and highlights the copy.
void
reswindow::DuplicateSelection(void) {
	DoubleItem *current = selected_resource(res);
	if (current == NULL)
		return;
	type_code type = ((TypeItem *)(res->Superitem(current)))->type;
	if (!can_duplicate(type)) {
		(new BAlert("alert","A file only needs one resource of this type, so it can't be duplicated.","OK",NULL,NULL,B_WIDTH_AS_USUAL,B_WARNING_ALERT))->Go();
		return;
	}
	size_t length;
	const void *loaded = openres->LoadResource(type,current->id,&length);
	if (loaded == NULL)
		return;
	if (length >= 1024) {
		char text[200];
		sprintf(text,"This resource is %lu bytes (%.1f KB). Are you sure you want to duplicate it?",(unsigned long)(length),length / 1024.0);
		BAlert *alert = new BAlert("alert",text,"Cancel","Duplicate",NULL,B_WIDTH_AS_USUAL,B_WARNING_ALERT);
		if (alert->Go() == 0)
			return;
		//	The alert let other messages through, so look the resource up again
		loaded = openres->LoadResource(type,current->id,&length);
		if (loaded == NULL)
			return;
	}
	int32 id = current->id + 1;
	while (openres->HasResource(type,id))
		id++;
	unsigned char *copy = new unsigned char[length > 0 ? length : 1];
	memcpy(copy,loaded,length);
	char *name = new char[(current->name != NULL ? strlen(current->name) : 0) + 1];
	strcpy(name,current->name != NULL ? current->name : "");
	DoubleItem *item = res->AddResource(type,id,name,length,copy,false,false);
	delete [] copy;
	delete [] name;
	Lock();
	res->Select(res->IndexOf(item));
	res->ScrollToSelection();
	Unlock();
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

reswindow::reswindow(const char *title,entry_ref ref) : BWindow(gettiledrect(),title,B_DOCUMENT_WINDOW,B_AUTO_UPDATE_SIZE_LIMITS) {
	istempfile = false;
	isquitting = false;
	rdefpanel = NULL;
	changes = false;
	fileref = ref;
	file = new BFile(&ref,B_READ_WRITE);
	openres = new BResources;
	if (openres->SetTo(file) != B_NO_ERROR) {
		return;
	}
	openmwindows++;
	//	The window is layout managed: menu bar on top, the list filling the rest.
	//	It is shown at the end, once everything is in place.
	BGroupLayout *layout = new BGroupLayout(B_VERTICAL,0);
	SetLayout(layout);
	//	A right inset of -1 lets the content run one pixel under the window frame,
	//	so the scroll bar's right edge and the frame's edge are drawn once.
	layout->SetInsets(0,0,-1,0);
	menus = new BMenuBar("Menus");
	layout->AddView(menus);
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
	file->AddItem(new BMenuItem("Export as RDEF...",new BMessage('rdfa')));
	file->AddItem(new BSeparatorItem);
	BMenuItem *quit = new BMenuItem("Quit",new BMessage(B_QUIT_REQUESTED),'Q');
	quit->SetTarget(be_app);
	file->AddItem(quit);
	BMenu *edit = new BMenu("Edit");
	menus->AddItem(edit);
	edit->AddItem(new BMenuItem("Cut",new BMessage(-400),'X'));
	edit->AddItem(new BMenuItem("Copy",new BMessage(-500),'C'));
	edit->AddItem(new BMenuItem("Copy as RDEF",new BMessage('rdfc'),'C',B_SHIFT_KEY));
	edit->AddItem(new BMenuItem("Paste",new BMessage(B_PASTE),'V'));
	edit->AddItem(new BSeparatorItem);
	edit->AddItem(new BMenuItem("Duplicate",new BMessage('dupl'),'U'));
	BMenu *res = new BMenu("Resource");
	menus->AddItem(res);
	res->AddItem(new BMenuItem("Add Resource...",new BMessage(-100),'A'));
	res->AddItem(new BMenuItem("Delete Resource",new BMessage(-200),'D'));
	res->AddItem(new BMenuItem("Resource Info...",new BMessage(-300),'I'));
	res->AddItem(new BSeparatorItem);
	res->AddItem(new BMenuItem("Copy to Attribute/Resource",new BMessage(-900)));
	res->AddItem(new BMenuItem("Export Resource as RDEF...",new BMessage('rdfr')));
	res->AddItem(new BSeparatorItem);
	res->AddItem(new BMenuItem("Open with Generic Editor",new BMessage(-800)));
	this->res = new restypeview(openres);
	BScrollView *resa = new ListScrollView("resa",this->res,B_WILL_DRAW | B_FRAME_EVENTS,false,true,B_NO_BORDER);
	//	The strip leaves room for the window's resize knob. Measured against
	//	Terminal's scroll bar at font sizes 8, 18 and 24, the knob overlaps the
	//	bottom edge of the bar by a pixel, which makes the strip two pixels less
	//	than the bar is thick (14 at size 8, 21 at 18, 27 at 24). The preferred
	//	size can be NaN before the layout is validated, hence the minimum size
	//	first and the NaN safe test; B_H_SCROLL_BAR_HEIGHT is only the fallback.
	float strip = B_H_SCROLL_BAR_HEIGHT;
	BScrollBar *bar = resa->ScrollBar(B_VERTICAL);
	if (bar != NULL) {
		bar->MinSize();
		float thickness = bar->PreferredSize().width - 2;
		if (thickness >= 1)
			strip = thickness;
	}
	Lock();
	layout->AddView(resa);
	layout->AddView(new FillerView(strip));
	this->res->SelectionChanged();
	this->res->MakeFocus(true);
	Unlock();
	//	Start wide enough for the widest row, but never smaller than the menu
	//	bar needs, never smaller than the 350 x 400 default and never wider than
	//	the screen. Rows are measured with the plain font; an outline row is
	//	indented by about three characters per level.
	float widest = 0;
	float indent = be_plain_font->Size() * 3;
	for (int32 i = 0;i < this->res->CountItems();i++) {
		BListItem *item = this->res->ItemAt(i);
		const char *text = NULL;
		float extra = 0;
		if (is_kind_of(item,BStringItem)) {
			text = ((BStringItem *)item)->Text();
		} else if (is_kind_of(item,DoubleItem)) {
			DoubleItem *row = (DoubleItem *)item;
			text = row->name;
			if (row->idstring != NULL)
				extra = be_plain_font->StringWidth("00000") + 10;
		}
		if (text == NULL)
			continue;
		float rowwidth = (item->OutlineLevel() + 1) * indent + extra + be_plain_font->StringWidth(text);
		if (rowwidth > widest)
			widest = rowwidth;
	}
	BScrollBar *vbar = resa->ScrollBar(B_VERTICAL);
	if (vbar != NULL) {
		float barwidth = vbar->MinSize().width;
		if (barwidth >= 0)
			widest += barwidth;
	}
	BRect screen = BScreen(this).Frame();
	BSize min = layout->MinSize();
	float width = min.width > 350 ? min.width : 350;
	if (widest > width)
		width = widest;
	if (width > screen.Width() - 20)
		width = screen.Width() - 20;
	float height = min.height > 400 ? min.height : 400;
	ResizeTo(width,height);
	Show();
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
		case 'rdfa':
			ExportRDEF(true);
			break;
		case 'rdfr':
			ExportRDEF(false);
			break;
		case 'rdfc':
			CopyRDEF();
			break;
		case 'rdfx':
			SaveRDEF(message);
			break;
		case 'dupl':
			DuplicateSelection();
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
		{
		//	LoadResource hands back memory owned by the BResources, so work on a copy
		const void *loaded = x->openres->LoadResource(type,id,&size);
		if (loaded == NULL)
			return 0;
		data = new unsigned char[size];
		memcpy(data,loaded,size);
		}
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
				//	LoadResource hands back memory owned by the BResources, so work on a copy
				const void *loaded = x->openres->LoadResource(type,id,&size);
				if (loaded == NULL)
					return 0;
				data = new unsigned char[size];
				memcpy(data,loaded,size);
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
