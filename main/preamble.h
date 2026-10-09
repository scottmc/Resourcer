#ifndef RESOURCER_MAIN_PREAMBLE_H
#define RESOURCER_MAIN_PREAMBLE_H

class reswindow;
BRect find_center(uint32 width, uint32 height);
BRect find_center(BRect rect,uint32 width, uint32 height);
bool alphabet(const char *string1, const char *string2);
int sortmenu(const void *first,const void *second);
#if __INTEL__ || defined(__x86_64__)
	type_code flipcode (type_code original);
#endif
#if __POWERPC__
	#define flipcode(o) o
#endif
typedef struct{
	image_id plugin;
	void (*loaddata)(unsigned char *,size_t,BView *);
	unsigned char* (*savedata)(size_t *,BView *);
	void (*messaging)(BMessage *,BView *);
	reswindow *win;
	unsigned char *olddata;
	size_t oldlength;
} plug_in;



class appFilter : public BRefFilter {
	public:
		bool Filter(const entry_ref *ref, BNode *node, struct stat_beos *st,
					   const char *mimetype) {
			if (BEntry(ref,true).IsDirectory())
				return true;
			BFile type(ref,B_READ_WRITE);
			BResources super;
			if (super.SetTo(&type) == B_OK)
				return true;
			return false;
		}
//		virtual	bool Filter(const entry_ref* ref, BNode* node,
//						struct stat_beos* stat, const char* mimeType) = 0;
};

extern BFilePanel *openpanel;
extern BFilePanel *newpanel;
extern BAlert *alert;
extern int32 mbheight;
extern int32 openmwindows;
extern BMessage *itemLista;
class pluginwindow;

#endif /* RESOURCER_MAIN_PREAMBLE_H */
