#include "includes.h"
#include <stdio.h>
#include <time.h>
#include <math.h>
#include "preamble.h"
#include "class.h"

int main(void) {
	openmwindows = 0;       //---This tracks open documents, so we can quit when it's zero--
	openpanel = NULL;
	itemLista = NULL;
	new BRApplication;
	newpanel = new BFilePanel(B_SAVE_PANEL,NULL,NULL,B_FILE_NODE,false,NULL,NULL,false,true);
	openpanel = new BFilePanel(B_OPEN_PANEL,NULL,NULL,B_FILE_NODE,false,NULL,new appFilter,false,true);
	be_app->Run();
	delete be_app;
	return 0;
}
