#ifndef RESOURCER_MAIN_RESWINDOW_H
#define RESOURCER_MAIN_RESWINDOW_H

class FillerView : public BView {
	public:
		FillerView(void) : BView(BRect(0,400-B_H_SCROLL_BAR_HEIGHT,350-B_V_SCROLL_BAR_WIDTH,400),"BottomFiller",B_FOLLOW_LEFT_RIGHT | B_FOLLOW_BOTTOM,B_WILL_DRAW | B_FRAME_EVENTS) {
			Draw(Bounds());
		}
		void Draw(BRect toUpdate) {
			BRect temp = toUpdate;
			temp.top = 2;
			SetHighColor(216,216,216);
			FillRect(temp);
			SetHighColor(255,255,255);
			temp.top = 2;
			temp.bottom = 2;
			FillRect(temp);
			SetHighColor(152,152,152);
			temp.top = 1;
			temp.bottom = 1;
			FillRect(temp);
		}
};

class reswindow : public BWindow {
	public:
	BRect gettiledrect(void) {
		BRect work = find_center(350,400);
		work.top += openmwindows * 30;
		work.left += openmwindows * 30;
		work.right += openmwindows * 30;
		work.bottom += openmwindows * 30;
		return work;
	}
	reswindow(const char *title,entry_ref ref) : BWindow(gettiledrect(),title,B_DOCUMENT_WINDOW,0/*B_ASYNCHRONOUS_CONTROLS*/) {
		istempfile = false;
		isquitting = false;
		changes = false;
		fileref = ref;
		file = new BFile(&ref,B_READ_WRITE);
		openres = new BResources;
		if (openres->SetTo(file) != B_NO_ERROR) {
			return;
		}
		openmwindows++;
		Show(); 
		menus = new BMenuBar(BRect(0,0,600,5),"Menus");
		AddChild(menus);
		Lock();
		mbheight = (int32)(menus->Bounds().bottom);
		Unlock();
		AddChild(new FillerView);
		BMenu *file = new BMenu("File");
		menus->AddItem(file);
		file->AddItem(new BMenuItem("About Resourcer...",new BMessage(B_ABOUT_REQUESTED)));
		file->AddItem(new BSeparatorItem);
		file->AddItem(new BMenuItem("New",new BMessage('newf'),'N'));
		file->AddItem(new BMenuItem("Open...",new BMessage('open'),'O'));
		file->AddItem(new BSeparatorItem);
		file->AddItem(new BMenuItem("Save",new BMessage('save'),'S'));
		file->AddItem(new BMenuItem("Save As...",new BMessage('svas'),'S',B_SHIFT_KEY));
		file->AddItem(new BSeparatorItem);
		file->AddItem(new BMenuItem("Revert Resources",new BMessage('rvrt'),'R'));
		file->AddItem(new BMenuItem("Edit File Type...",new BMessage('mime'),'M'));
		file->AddItem(new BSeparatorItem);
		BMenuItem *quit = new BMenuItem("Quit",new BMessage(B_QUIT_REQUESTED),'Q');
		quit->SetTarget(be_app);
		file->AddItem(quit);
		BMenu *edit = new BMenu("Edit");
		menus->AddItem(edit);
		edit->AddItem(new BMenuItem("Cut",new BMessage(-400),'X'));
		edit->AddItem(new BMenuItem("Copy",new BMessage(-500),'C'));
		edit->AddItem(new BMenuItem("Paste",new BMessage(B_PASTE),'V'));
		BMenu *res = new BMenu("Resource");
		menus->AddItem(res);
		res->AddItem(new BMenuItem("Add Resource...",new BMessage(-100),'A'));
		res->AddItem(new BMenuItem("Delete Resource",new BMessage(-200),'D'));
		res->AddItem(new BMenuItem("Resource Info...",new BMessage(-300),'I'));
		res->AddItem(new BSeparatorItem);
		res->AddItem(new BMenuItem("Copy to Attribute/Resource",new BMessage(-900)));
		res->AddItem(new BSeparatorItem);
		res->AddItem(new BMenuItem("Open with Generic Editor",new BMessage(-800)));
		this->res = new restypeview(openres);
		BScrollView *resa = new BScrollView("resa",this->res,B_FOLLOW_ALL_SIDES,0,false,true,B_NO_BORDER);
		AddChild(resa);
		this->res->SelectionChanged();
		Lock();
		this->res->MakeFocus(true);
		if (IsLocked())
			Unlock();
	}
	void CloseWindows(void);
	void WindowActivated (bool active) {
		if (active == true)
			be_app->SetCursor(B_HAND_CURSOR);
	}
	bool QuitRequested(void) {
		char text[800];
		sprintf(text,"Save changes to \"%s\"?\n\nChanges to attributes are saved automatically.",Title());
		CloseWindows();
		isquitting = false;
		if (changes == true) {
			BAlert *alert = new BAlert("save",text,"Cancel","Don't Save","Save",B_WIDTH_AS_USUAL,B_OFFSET_SPACING,B_WARNING_ALERT);
			alert->SetShortcut(0,B_ESCAPE);
			switch(alert->Go()) {
				case 0:
					return false;
					break;
				case 1:
					if (istempfile)
						BEntry(&fileref).Remove();
					break;
				case 2:
					openres->Sync();
					file->Sync();
					changes = false;
					if (istempfile) {
						MessageReceived(new BMessage('svas'));
						isquitting = true;
						return false;
					}
					break;
			}
		} else {
			delete openres;
			delete file;
		}
		openmwindows--;
		if (openmwindows <= 0) {
			BNode node("/boot");
			list_state rescol = collapsed;
			if (res->resources->IsExpanded())
				rescol = expanded;
			node.WriteAttr("rescol",'STAT',0,&rescol,sizeof(list_state));
			rescol = collapsed;
			if (res->attributes->IsExpanded())
				rescol = expanded;
			node.WriteAttr("attrcol",'STAT',0,&rescol,sizeof(list_state));
			be_app->PostMessage(B_QUIT_REQUESTED);
		}
		return true;
	}
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
