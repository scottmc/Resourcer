#ifndef RESOURCER_MAIN_RESWINDOW_H
#define RESOURCER_MAIN_RESWINDOW_H

class FillerView : public BView {
	public:
		FillerView(void);
		void Draw(BRect toUpdate);
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
