#ifndef RESOURCER_RESLIB_CURSOR_H
#define RESOURCER_RESLIB_CURSOR_H

void setrescursor(int32 id);
void setrescursor(const char *name);

void setrescursor(int32 id) {
	size_t length;
	be_app->SetCursor(AppResource('CURS',id,&length));
}

void setrescursor(const char *name) {
	size_t length;
	be_app->SetCursor(AppResource('CURS',name,&length));
}

#endif /* RESOURCER_RESLIB_CURSOR_H */
