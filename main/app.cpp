// The BRApplication class: startup, file panels and save handling.
// Declaration is in class.h.

#include "includes.h"
#include "preamble.h"
#include "class.h"
#include "res.h"
#include "reswindow.h"

BRApplication::BRApplication(void) : BApplication("application/x-vnd.NW-resourcer") {
	openpan = false;
	retrieve = new char[2];
	retrieve[0] = 0;
}

void
BRApplication::MessageReceived(BMessage *message) {
	switch (message->what) {
		case B_SAVE_REQUESTED:
			getready(message);
			break;
		case B_CANCEL:
			if (openmwindows == 0) {
				be_app->Quit();
			}
			if ((panelopen == true) && (openpan == false)) {
				savepan = true;
			}
			dissectwindow(message);
			break;
		case 'chge':
			delete [] retrieve;
			BTextControl *text;
			message->FindPointer("source",(void **)(&text));
			retrieve = new char[strlen(text->Text()) + 1];
			strcpy(retrieve,text->Text());
			break;
		default:
			BApplication::MessageReceived(message);
		}
}

void
BRApplication::ReadyToRun(void) {
	launched = true;
	saveas = false;
	if (alredopen)
		return;
	BMessage *msg = new BMessage(B_SAVE_REQUESTED);
	BPath path;
	find_directory(B_SYSTEM_TEMP_DIRECTORY, &path);
	entry_ref ref;
	get_ref_for_path(path.Path(), &ref);
	//BEntry entry;
	msg->AddRef("directory",&ref);
	char text[B_FILE_NAME_LENGTH];
	sprintf(text,"tmpresfile0");
	BEntry entry;
	BDirectory tmp(path.Path());
	for (int i = 1;tmp.FindEntry(text,&entry) != B_ENTRY_NOT_FOUND; i++) {
		sprintf(text,"tmpresfile%d",i);
	}
	msg->AddString("name",text);
	getready(msg,true);
}

void BRApplication::RefsReceived(BMessage *message) {
	entry_ref ref;
	message->FindRef("refs",&ref);
	char title[B_FILE_NAME_LENGTH];
	BEntry entry(&ref,true);
	entry.GetName(title);
	new reswindow(title,ref); //---reswindow does all the dirty work
	openpan = false;          //---All these booleans could probably be consolidated
	saveas = false;
}

void BRApplication::dissectwindow(BMessage *msg) { //---Do panel close
	if (!saveas)
		return;
	reswindow *x;
	msg->FindPointer("reswindow",(void **)(&x));
	if (x->istempfile) {
		BEntry(&(x->fileref)).Remove();  //---------Remove the temp file
		x->changes = false;
	}
	if (x->isquitting)					//----------Was the window quitting and we hit save as?
		x->PostMessage(B_QUIT_REQUESTED);//---------Close the window
}

void BRApplication::getready(BMessage *message,bool istempfile) { //---Do save as dirty work
	entry_ref dira;
	char *name;
	message->FindRef("directory",&dira);
	message->FindString("name",(const char **)(&name));
	BDirectory dir(&dira);
	BEntry entrya(&dir,name);
	entrya.GetRef(&dira);
	BFile entry(&entrya,B_READ_WRITE | B_CREATE_FILE);
	BNodeInfo info(&entry);
	info.SetType("application/x-be-resource");
	if (saveas) {
		saveas = false;
		char title[B_FILE_NAME_LENGTH];
		char name[B_ATTR_NAME_LENGTH];
		reswindow *x;
		message->FindPointer("reswindow",(void **)(&x));
		unsigned char *buffer;
		entrya.GetName(title);
		x->SetTitle(title);
		x->openres->WriteTo(&entry);
		x->file->RewindAttrs();
		attr_info info;
		for (;x->file->GetNextAttrName(name) == B_OK;) {
			x->file->GetAttrInfo(name,&info);
			buffer = new unsigned char[info.size];
			x->file->ReadAttr(name,info.type,0,buffer,info.size);
			entry.WriteAttr(name,info.type,0,buffer,info.size);
			delete [] buffer;
		}
		if (x->istempfile) {
			BEntry entry(&(x->fileref));
			entry.Remove();
		}
		if (x->isquitting)
			x->PostMessage(B_QUIT_REQUESTED);
		delete x->file;
		x->file = new BFile(entry);
		x->fileref = dira;
		x->istempfile = false;
	} else {
		BResources res(&entry,true);
		char title[B_FILE_NAME_LENGTH];
		entrya.GetName(title);
		if (istempfile)
			strcpy(title,"Untitled");
		reswindow *tempder = new reswindow(title,dira);
		tempder->istempfile = istempfile;
	}
	saveas = false;
}
