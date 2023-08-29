#ifndef ACTIONPANELSCHEME_H
#define ACTIONPANELSCHEME_H

#include <QtGui/QIcon>
#include <QtCore/QSize>
#include <QtCore/QString>

class ActionPanelScheme
{
public:
	/** \enum TaskPanelFoldEffect
		\brief Animation effect during expanding/collapsing of the ActionGroup's contents.
	  */
	enum TaskPanelFoldEffect
	{
		/// No folding effect
		NoFolding,
		/// Folding by scaling group's contents
		ShrunkFolding,
		/// Folding by sliding group's contents
		SlideFolding
	};

	enum PanelStyle
	{
		PANEL_NONE,
		PANEL_DEFAULT,
		PANEL_DEFAULT_2,
		PANEL_VISTA,
		PANEL_XP_1, PANEL_XP_2
	};

	static const QString PanelStyleStr[6];
	
	explicit ActionPanelScheme ( const PanelStyle style = PANEL_DEFAULT );

	/** Returns a pointer to the default scheme object.
	Must be reimplemented in the own schemes.
	*/
	static ActionPanelScheme* defaultScheme ();
	
	static ActionPanelScheme* newScheme ( const QString& scheme_name );

	/// Height of the header in pixels.
	int headerSize;
	/// If set to \a true, moving mouse over the header results in changing its opacity slowly.
	bool headerAnimation;

	/// Image of folding button when the group is expanded.
	QIcon headerButtonFold;
	/// Image of folding button when the group is expanded and mouse cursor is over the button.
	QIcon headerButtonFoldOver;
	/// Image of folding button when the group is collapsed.
	QIcon headerButtonUnfold;
	/// Image of folding button when the group is collapsed and mouse cursor is over the button.
	QIcon headerButtonUnfoldOver;

	QIcon headerButtonClose;
	QIcon headerButtonCloseOver;

	QSize headerButtonSize;

	/// Number of steps made for expanding/collapsing animation (default 20).
	int groupFoldSteps;
	/// Delay in ms between steps made for expanding/collapsing animation (default 15).
	int groupFoldDelay;
	/// Sets folding effect during expanding/collapsing.
	TaskPanelFoldEffect groupFoldEffect;
	/// If set to \a true, changes group's opacity slowly during expanding/collapsing.
	bool groupFoldThaw;

	/// The CSS for the ActionPanel/ActionGroup elements.
	QString actionStyle;
};

#endif // ACTIONPANELSCHEME_H
