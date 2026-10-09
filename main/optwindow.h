#ifndef RESOURCER_MAIN_OPTWINDOW_H
#define RESOURCER_MAIN_OPTWINDOW_H

class optwindow : public BWindow {
	public:
		optwindow(bool add,DoubleItem* to,type_code typex,reswindow *x);
		// The list of editors (file name and description) is read from the
		// editors folder once and kept in itemLista; every dialog builds a
		// fresh menu from it.
		void init_typemenu(BPopUpMenu *menu);
		char * parse_size(size_t sizer);
		void add(void);
		void changeinfo(void);
		void MessageReceived(BMessage *message);
	private:
		BView *gray;
		BTextControl *type;
		BTextControl *id;
		BTextControl *name;
		bool whattodo;
		bool willberes;
		int32 sel;
		int32 sela;
		DoubleItem* toop;
		type_code typeb;
		reswindow *win;
};

#endif /* RESOURCER_MAIN_OPTWINDOW_H */
