#ifndef RESOURCER_MAIN_PLUGINWINDOW_H
#define RESOURCER_MAIN_PLUGINWINDOW_H

class pluginwindow : public BWindow {
	public:
		pluginwindow(pluginwindow **reg,plug_in plugin1,char *name,int32 resid,type_code type,char *xname,bool isattr);
		void RemoveChildren(void);
		bool QuitRequested(void);
		void MessageReceived(BMessage *message);
		BView *gray;
		plug_in plugin;
	private:
		int32 id;
		type_code typea;
		char *yname;
		pluginwindow **registration;
		bool attr;
		BView *oldfocus;
};

#endif /* RESOURCER_MAIN_PLUGINWINDOW_H */
