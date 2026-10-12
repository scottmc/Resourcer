#include "fields.h"

//	An 'NREF' resource is a node_ref, as BMessage::AddNodeRef stores it: the
//	volume's device number (4 bytes) and the node number (8 bytes).

extern "C" _EXPORT void loaddata(unsigned char *,size_t,BView *);
extern "C" _EXPORT unsigned char* savedata(size_t *,BView *);
extern "C" _EXPORT void messaging(BMessage *,BView *);
extern "C" _EXPORT const char description[] = "Node Reference";

static const size_t kSize = sizeof(int32) + sizeof(int64);

void loaddata(unsigned char *data,size_t length,BView *bkgview) {
	int32 device = 0;
	int64 node = 0;
	if (data != NULL && length >= kSize) {
		memcpy(&device,data,sizeof(device));
		memcpy(&node,data + sizeof(device),sizeof(node));
	}
	char text[32];
	BGridLayout *grid = start_form(bkgview);
	sprintf(text,"%ld",(long)device);
	add_field(grid,0,"Device","Device:",text,"-2147483648");
	sprintf(text,"%lld",(long long)node);
	add_field(grid,1,"Node","Node:",text,"-9223372036854775808");
	finish_form(grid,bkgview,2);
}

unsigned char* savedata(size_t *length,BView *bkgview) {
	int32 device = (int32)strtol(field_text(bkgview,"Device"),NULL,10);
	int64 node = strtoll(field_text(bkgview,"Node"),NULL,10);
	unsigned char *data = new unsigned char[kSize];
	memcpy(data,&device,sizeof(device));
	memcpy(data + sizeof(device),&node,sizeof(node));
	*length = kSize;
	return data;
}

void messaging(BMessage *message,BView *bkgview) {
}
