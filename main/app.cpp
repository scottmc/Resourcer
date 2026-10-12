// The BRApplication class: startup, file panels and save handling.
// Declaration is in class.h.

#include <AboutWindow.h>
#include <time.h>
#include <storage/AppFileInfo.h>
#include <storage/File.h>
#include "includes.h"
#include "preamble.h"
#include "class.h"
#include "res.h"
#include "reswindow.h"

BRApplication::BRApplication(void) : BApplication(RESOURCER_SIGNATURE) {
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

//	The About window takes its icon and version from this running copy of
//	Resourcer: the HVIF icon and the app version (APPV) resource in its file.
void
BRApplication::AboutRequested(void) {
	BAboutWindow *about = new BAboutWindow("Resourcer",RESOURCER_SIGNATURE);
	app_info info;
	if (GetAppInfo(&info) == B_OK) {
		BFile file(&info.ref,B_READ_ONLY);
		BAppFileInfo appinfo(&file);
		if (appinfo.InitCheck() == B_OK) {
			version_info version;
			if (appinfo.GetVersionInfo(&version,B_APP_VERSION_KIND) == B_OK) {
				BString text("Version ");
				text << (int)(version.major) << "." << (int)(version.middle);
				if (version.minor > 0)
					text << "." << (int)(version.minor);
				static const char *varieties[] = {"development","alpha","beta","gamma","gold master"};
				if (version.variety <= B_GOLDEN_MASTER_VERSION)
					text << "-" << varieties[version.variety];
				about->SetVersion(text.String());
			}
			BBitmap *icon = new BBitmap(BRect(0,0,63,63),B_RGBA32);
			if (appinfo.GetIcon(icon,(icon_size)64) == B_OK)
				about->SetIcon(icon);
			else
				delete icon;
		}
	}
	about->AddDescription("A resource editor for Haiku.");
	//	AddCopyright() puts its own holder first and runs its years up to the
	//	current one, so the two lines are written out to keep the original
	//	author first and his years as they were.
	time_t now = time(NULL);
	char year[8];
	strftime(year,sizeof(year),"%Y",localtime(&now));
	BString current(B_UTF8_COPYRIGHT " 2012-");
	current << year << " HaikuArchives Team";
	const char *copyrights[] = {
		B_UTF8_COPYRIGHT " 1999-2000 Nathan Whitehorn",
		current.String(),
		"All Rights Reserved.",
		NULL};
	about->AddText(NULL,copyrights);
	static const char *credits[] = {
		"The RDEF export uses Haiku's rc decompiler,",
		B_UTF8_COPYRIGHT " 2003 Matthijs Hollemans (MIT license).",
		NULL};
	about->AddText("Includes code from:",credits);
	about->Show();
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
	if (is_rdef_name(title)) {
		//	An rdef file is compiled and opened as a new file
		open_rdef_as_new(&ref);
		openpan = false;
		saveas = false;
		return;
	}
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
