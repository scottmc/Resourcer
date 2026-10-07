//------------------------------------------------------------------------------
//	WIND.h
//
//------------------------------------------------------------------------------

#ifndef WIND_H
#define WIND_H

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------
#include <support/SupportDefs.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------

// Local Defines ---------------------------------------------------------------
#define MSG_MENU_FIELD_SET_LABEL		((uint32)-1100)
#define MSG_MENU_SET_DEFAULT_TEXT		((uint32)-1101)
#define	MSG_MENU_NEW_ITEM				((uint32)-4000)
#define MSG_MENU_SET_ITEM_LABEL			((uint32)-4001)
#define MSG_MENU_SET_ITEM_MSG			((uint32)-4002)
#define MSG_MENU_SET_ITEM_SHORTCUT		((uint32)-4003)
#define MSG_MENU_SET_ITEM_MARKED		((uint32)-4004)
#define MSG_MENU_SET_ITEM_ENABLED		((uint32)-4005)
#define MSG_MENU_ADD_SEPARATOR			((uint32)-4006)
#define MSG_MENU_SELECTION_CHANGED		((uint32)-4007)

#define MSG_VIEW_SET_CHECKED			((uint32)-2000)

#define MSG_VIEW_SET_NAME				((uint32)-400)

#define MSG_CONTROL_SET_LABEL			((uint32)-500)
#define MSG_CONTROL_SET_MSG				((uint32)-510)
#define MSG_CONTROL_SET_ENABLED			((uint32)-520)

#define MSG_STRING_VIEW_SET_TEXT		((uint32)-800)
#define MSG_STRING_VIEW_ALIGN_LEFT		((uint32)-825)
#define MSG_STRING_VIEW_ALIGN_CENTER	((uint32)-850)
#define MSG_STRING_VIEW_ALIGN_RIGHT		((uint32)-875)

#define MSG_BOX_SET_LABEL				((uint32)-900)
#define MSG_BOX_FANCY_BORDER			((uint32)-901)
#define MSG_BOX_PLAIN_BORDER			((uint32)-902)
#define MSG_BOX_NO_BORDER				((uint32)-903)

#define MSG_RES_IMAGE_VIEW_SET_ID		((uint32)-1000)
#define MSG_RES_STRING_VIEW_SET_ID		((uint32)-1300)

#define MSG_WINDOW_SET_TITLE			((uint32)-100)
#define MSG_WINDOW_SET_LOOK				((uint32)-110)
#define MSG_WINDOW_SET_FLAG				((uint32)-120)
#define MSG_WINDOW_ADD_MENU				((uint32)-138)
#define MSG_WINDOW_SET_FEEL				((uint32)-140)
#define MSG_WINDOW_ARCHIVE				((uint32)-150)
#define MSG_WINDOW_QUIT					((uint32)-165)

#define MSG_COLOR_CONTROL_SET_ALIGN		((uint32)-1200)
#define MSG_COLOR_CONTROL_SET_CELL_SIZE	((uint32)-1250)


#define DEBUG_ARCHIVE(__id__)	\
	debugger("Couldn't find " __id__ " in archive")
// Globals ---------------------------------------------------------------------
#if __POWERPC__
	#define flipcode(o) o
#else
	int32 flipcode(int32 original);
#endif

int32 track_over(void *view);

#endif	// WIND_H

/*
 * $Log $
 *
 * $Id  $
 *
 */

