#ifndef RESOURCER_MAIN_RES_H
#define RESOURCER_MAIN_RES_H

enum list_state{
	collapsed,
	expanded
};

class DoubleItem;
class pluginwindow;

int sortList(const BListItem *one,const BListItem *two);
type_code code_from_MIME(char *MIME);
const char *MIME_from_code(type_code code);

class TypeItem : public BStringItem {
	public:
		TypeItem(type_code code);
		char *TypeCode(void);
		void SetTypeCode(char *code);
		type_code type;
		const char *description;
};

class DoubleItem : public BListItem {
	public:
		DoubleItem(int32 resid,const char *resname,BOutlineListView *owner,bool isattr);
		void DrawItem(BView *owner, BRect frame, bool complete = false);
		void Invoke(bool generic = false);
		~DoubleItem(void);
		BOutlineListView *parent;
		int32 id;
		char *idstring;
		char *name;
		pluginwindow *win;
};

class restypeview : public BOutlineListView {
	public:
		restypeview(BResources *openres);
		void Init(BResources *openres);
		void AddResources(BResources *res);
		void Sort(void);
		DoubleItem *AddResource(type_code type,int32 id,const char *name,size_t length,void *data,bool isattr,bool invoke = true);
		TypeItem *FindType(type_code type,bool attr);
		void MouseMoved(BPoint point,uint32 transit,const BMessage *msg);
		void SelectionChanged(void);
		void MessageReceived(BMessage *msg);
		bool InitiateDrag(BPoint point,int32 index, bool selected);
		void KeyDown(const char *bytes, int32 numBytes);
		void DeleteSelection(DoubleItem *item = NULL);
		BStringItem *resources;
		BStringItem *attributes;
};

#endif /* RESOURCER_MAIN_RES_H */
