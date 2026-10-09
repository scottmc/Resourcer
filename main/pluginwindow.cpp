// The window that hosts a resource editor add-on.
// Declaration is in pluginwindow.h.

#include "includes.h"
#include "preamble.h"
#include "res.h"
#include "reswindow.h"
#include "pluginwindow.h"

pluginwindow::pluginwindow(pluginwindow **reg,plug_in plugin1,char *name,int32 resid,type_code type,char *xname,bool isattr) : BWindow(BRect(400,300,700,600),name,B_TITLED_WINDOW,B_NOT_RESIZABLE | B_NOT_ZOOMABLE/* | B_ASYNCHRONOUS_CONTROLS*/) {
	registration = reg;
	*registration = this;
	plugin = plugin1;
	id = resid;
	typea = type;
	yname = new char[strlen(xname) + 1];
	strcpy(yname,xname);
	attr = isattr;
}

void
pluginwindow::RemoveChildren(void) {
	BView *view;
	for (int32 x = 0;;) {
		view = gray->ChildAt(x);
		if (view == NULL) {
			break;
		}
		view->RemoveSelf();
		delete view;
	}
	gray->RemoveSelf();
	delete gray;
}

bool
pluginwindow::QuitRequested(void) {
	be_app->SetCursor(B_HAND_CURSOR);
	size_t give;
	unsigned char *data;
	Hide();
	*registration = NULL;
	bool changes = true;
	data = (*(plugin.savedata))(&give,gray);
	if (data == NULL) {
		BAlert *alert = new BAlert("whoops","The resource could not be written because the editor passed a NULL pointer (i.e. it didn't write any data).","OK",NULL,NULL,B_WIDTH_AS_USUAL,B_STOP_ALERT);
		alert->Go();
		RemoveChildren();
		unload_add_on(plugin.plugin);
		delete [] plugin.olddata;
		return true;
	}
	if (give == plugin.oldlength) {
		//(new BAlert("h","Que es?","OK"))->Go();
		//(new BAlert("h","Qu'est-ce que c'est?","OK"))->Go();
		//(new BAlert("h","What is it?","OK"))->Go();
		if (memcmp(data,plugin.olddata,give) == 0) {
			changes = false;
		}
	}
	if (attr == false) {
		plugin.win->openres->RemoveResource(typea,id);
		if (plugin.win->openres->AddResource(typea,id,data,give,yname) != B_NO_ERROR) {
			BAlert *alert = new BAlert("whoops","The resource could not be written.","OK",NULL,NULL,B_WIDTH_AS_USUAL,B_STOP_ALERT);
			alert->Go();
			RemoveChildren();
			unload_add_on(plugin.plugin);
			delete [] data;
			delete [] plugin.olddata;
			return true;
		}
	} else {
		if (plugin.win->file->WriteAttr(yname,typea,0,data,give) <= 0) {
			BAlert *alert = new BAlert("whoops","The attribute could not be written.","OK",NULL,NULL,B_WIDTH_AS_USUAL,B_STOP_ALERT);
			alert->Go();
			RemoveChildren();
			unload_add_on(plugin.plugin);
			delete [] data;
			delete [] plugin.olddata;
			return true;
		}
	}
	if ((attr == false) && changes)
		plugin.win->changes = true;
	RemoveChildren();
	unload_add_on(plugin.plugin);
	delete [] data;
	delete [] plugin.olddata;
	delete [] yname;
	return true;
}

void
pluginwindow::MessageReceived(BMessage *message) {
	int32 y = message->what;
	if ((y < 0) || (y == B_REFS_RECEIVED)) {
		(*(plugin.messaging))(message,gray);
	} else {
		BWindow::MessageReceived(message);
	}
}
