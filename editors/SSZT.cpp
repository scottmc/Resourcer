//------------Includes----------------------
#include "wideint.h"
//-------------------------------------------

extern "C" _EXPORT void loaddata(unsigned char *,size_t,BView *);
extern "C" _EXPORT unsigned char* savedata(size_t *,BView *);
extern "C" _EXPORT void messaging(BMessage *,BView *);
extern "C" _EXPORT const char description[] = "ssize_t Value";

//	The width comes from the data, a new resource gets the width of the type here
void loaddata(unsigned char *data,size_t length,BView *bkgview) {
	wide_load(data,length,bkgview,sizeof(ssize_t),true);
}

unsigned char* savedata(size_t *length,BView *bkgview) {
	return wide_save(length,bkgview);
}

void messaging(BMessage *message,BView *bkgview) {
}
