//------------------------------------------------------------------------------
//	BViewEditor.h
//
//------------------------------------------------------------------------------

#ifndef BVIEWEDITOR_H
#define BVIEWEDITOR_H

// Standard Includes -----------------------------------------------------------
#include <vector>

// System Includes -------------------------------------------------------------
#include <app/Messenger.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

class BStringView;
class BTextControl;
class BView;
class BWindow;

class ControlKeeper;

class BViewEditor
{
	public:
		BViewEditor();
		virtual ~BViewEditor();

		virtual void SetParent(BView* parent);
		virtual void SetTarget(BView* target);

		virtual void		SetTo(BMessage* archive);
		virtual bool		MessageReceived(BMessage* msg);
		virtual const char*	Supports();

		// True when the editor's controls are placed by a layout (the panel
		// then gives them one), false when they are placed by hand.
		virtual bool		UsesLayout() const;

		BWindow* Window();

		BMessenger&	Messenger();

		// TODO: Remove when control pool is done
		void SetBottom(int bottom) { fBottom = bottom; }
	protected:

		int	Bottom();
		// The width of the labels of the text fields, wide enough for the
		// longest so none is cut short and the fields line up.
		static float LabelDivider();
		// Label divider for one label: its width plus room so it isn't cut short
		static float DividerFor(const char* label, float padding = 10);
		void AddControl(BView* control);
		BView* Parent();
		BView* Target();

	private:
		void Init();

		BMessenger		fControl;
		BStringView*	fClassName;
		BTextControl*	fViewName;
		BView*			fParent;
		BView*			fTarget;
		int				fBottom;

		class ControlKeeper
		{
			public:
				ControlKeeper();
				~ControlKeeper();
		
				void AddControl(BView* control);
		
			private:
				std::vector<BView*>	fControls;
		};
		static ControlKeeper	fControlKeeper;
};

#endif	// BVIEWEDITOR_H

/*
 * $Log $
 *
 * $Id  $
 *
 */

