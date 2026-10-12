#include "fields.h"

//	An 'MSNG' resource is a BMessenger as BMessage::AddMessenger stores it: the
//	port, the handler's token and the team, 4 bytes each, then 12 reserved bytes.
//	A messenger only reaches anything while that team and port are alive, so
//	one kept in a file is mostly of interest for what it was.

extern "C" _EXPORT void loaddata(unsigned char *,size_t,BView *);
extern "C" _EXPORT unsigned char* savedata(size_t *,BView *);
extern "C" _EXPORT void messaging(BMessage *,BView *);
extern "C" _EXPORT const char description[] = "Messenger";

static const size_t kSize = 6 * sizeof(int32);

void loaddata(unsigned char *data,size_t length,BView *bkgview) {
	int32 values[3] = { -1,-1,-1 };
	if (data != NULL && length >= sizeof(values))
		memcpy(values,data,sizeof(values));
	char text[16];
	BGridLayout *grid = start_form(bkgview);
	sprintf(text,"%ld",(long)values[0]);
	add_field(grid,0,"Port","Port:",text,"-2147483648");
	sprintf(text,"%ld",(long)values[1]);
	add_field(grid,1,"Token","Token:",text,"-2147483648");
	sprintf(text,"%ld",(long)values[2]);
	add_field(grid,2,"Team","Team:",text,"-2147483648");
	finish_form(grid,bkgview,3);
}

unsigned char* savedata(size_t *length,BView *bkgview) {
	int32 values[3];
	values[0] = (int32)strtol(field_text(bkgview,"Port"),NULL,10);
	values[1] = (int32)strtol(field_text(bkgview,"Token"),NULL,10);
	values[2] = (int32)strtol(field_text(bkgview,"Team"),NULL,10);
	unsigned char *data = new unsigned char[kSize];
	memset(data,0,kSize);
	memcpy(data,values,sizeof(values));
	*length = kSize;
	return data;
}

void messaging(BMessage *message,BView *bkgview) {
}
