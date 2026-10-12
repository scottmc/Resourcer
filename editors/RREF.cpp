#include "fields.h"

//	A 'RREF' resource is an entry_ref, as BMessage::AddRef stores it: the
//	volume's device number (4 bytes) and the directory's node number (8 bytes),
//	followed by the file's name and its ending zero. The numbers only mean
//	something on the volume the reference was made on.

extern "C" _EXPORT void loaddata(unsigned char *,size_t,BView *);
extern "C" _EXPORT unsigned char* savedata(size_t *,BView *);
extern "C" _EXPORT void messaging(BMessage *,BView *);
extern "C" _EXPORT const char description[] = "Entry Reference";

static const size_t kNumbers = sizeof(int32) + sizeof(int64);

void loaddata(unsigned char *data,size_t length,BView *bkgview) {
	int32 device = 0;
	int64 directory = 0;
	char name[B_FILE_NAME_LENGTH];
	name[0] = 0;
	if (data != NULL && length >= kNumbers) {
		memcpy(&device,data,sizeof(device));
		memcpy(&directory,data + sizeof(device),sizeof(directory));
		size_t room = length - kNumbers;
		if (room > sizeof(name) - 1)
			room = sizeof(name) - 1;
		memcpy(name,data + kNumbers,room);
		name[room] = 0;
	}
	char text[32];
	BGridLayout *grid = start_form(bkgview);
	sprintf(text,"%ld",(long)device);
	add_field(grid,0,"Device","Device:",text,"-2147483648");
	sprintf(text,"%lld",(long long)directory);
	add_field(grid,1,"Directory","Directory:",text,"-9223372036854775808");
	add_field(grid,2,"Name","Name:",name,"MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM");
	finish_form(grid,bkgview,3);
}

unsigned char* savedata(size_t *length,BView *bkgview) {
	int32 device = (int32)strtol(field_text(bkgview,"Device"),NULL,10);
	int64 directory = strtoll(field_text(bkgview,"Directory"),NULL,10);
	const char *name = field_text(bkgview,"Name");
	//	No name leaves just the two numbers, as BMessage does
	size_t size = kNumbers + (name[0] != 0 ? strlen(name) + 1 : 0);
	unsigned char *data = new unsigned char[size];
	memcpy(data,&device,sizeof(device));
	memcpy(data + sizeof(device),&directory,sizeof(directory));
	if (name[0] != 0)
		memcpy(data + kNumbers,name,strlen(name) + 1);
	*length = size;
	return data;
}

void messaging(BMessage *message,BView *bkgview) {
}
