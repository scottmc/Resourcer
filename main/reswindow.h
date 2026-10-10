#ifndef RESOURCER_MAIN_RESWINDOW_H
#define RESOURCER_MAIN_RESWINDOW_H

//	A strip under the list, as high as Haiku's own scroll view leaves for it. It leaves the
//	window's resize corner to itself, so the list's scroll bar ends above the
//	corner and both of its bottom arrows stay visible.
class FillerView : public BView {
	public:
		FillerView(float height);
		void Draw(BRect toUpdate);
};

class ListScrollView : public BScrollView {
	public:
		ListScrollView(const char *name,BView *target,uint32 flags,bool horizontal,bool vertical,border_style border);
		void DoLayout(void);
};

class reswindow : public BWindow {
	public:
		BRect gettiledrect(void);
		reswindow(const char *title,entry_ref ref);
		void CloseWindows(void);
		void WindowActivated(bool active);
		bool QuitRequested(void);
		void MessageReceived(BMessage *message);
		restypeview *res;
		bool changes;
		bool istempfile;
		bool isquitting;
		BResources *openres;
		BFile *file;
		entry_ref fileref;
		BMenuBar *menus;
};

#endif /* RESOURCER_MAIN_RESWINDOW_H */
