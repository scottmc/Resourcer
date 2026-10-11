#ifndef RESOURCER_MAIN_CLASS_H
#define RESOURCER_MAIN_CLASS_H

#include <FindDirectory.h>
#include <Path.h>
#define RESOURCER_SIGNATURE "application/x-vnd.NW-resourcer"

extern bool launched;
extern bool alredopen;
extern bool panelopen;
extern bool openpan;

class BRApplication : public BApplication {
	public:
		BRApplication(void);
		void RefsReceived(BMessage *message);
		void getready(BMessage *message,bool istempfile = false);
		void dissectwindow(BMessage *msg);
		void MessageReceived(BMessage *message);
		void AboutRequested(void);
		void ReadyToRun(void);
		bool savepan;
		char *retrieve;
		bool saveas;
};

#endif /* RESOURCER_MAIN_CLASS_H */
