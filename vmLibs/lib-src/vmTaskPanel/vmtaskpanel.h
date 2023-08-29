#ifndef VMTASKPANEL_H
#define VMTASKPANEL_H

#include "vmlist.h"

#include <QtWidgets/QFrame>

class ActionPanelScheme;
class vmActionGroup;
class vmActionLabel;

class QVBoxLayout;

class vmTaskPanel : public QFrame
{

public:
	explicit vmTaskPanel ( const QString& title = nullptr, QWidget* parent = nullptr );
	virtual ~vmTaskPanel () = default;

	void setTitle ( const QString& new_title );
	/** Adds a widget \a w to the vmTaskPanel's vertical layout.
	  */
	void addWidget ( QWidget* w );

	/** Adds a spacer with width \a s to the vmTaskPanel's vertical layout.
		Normally you should do this after all the vmActionGroups were added, in order to
		maintain some space below.
	  */
	void addStretch ( const int s = 0 );

	/** Creates and adds to the vmTaskPanel's vertical layout an empty vmActionGroup with header's
		text set to \a title, but with no icon.

		If \a expandable set to \a true (default), the group can be expanded/collapsed by the user.
	  */
	vmActionGroup* createGroup ( const QString& title,
								 const bool expandable = true,
								 const bool stretchContents = true,
								 const bool closable = false );

	/** Creates and adds to the vmTaskPanel's vertical layout an empty vmActionGroup with header's
		text set to \a title and icon set to \a icon.

		If \a expandable set to \a true (default), the group can be expanded/collapsed by the user.
	  */
	vmActionGroup* createGroup (const QIcon& icon,
								 const QString& title,
								 const bool expandable = true,
								 const bool stretchContents = true,
								 const bool closable = false );

	void removeGroup ( vmActionGroup* group, const bool bDelete = false );

	/** Sets the scheme of the panel and all the child groups to \a scheme.

		By default, vmTaskPanelScheme::defaultScheme() is used.
	  */
	void setScheme ( const QString& style );

	QSize minimumSizeHint () const;

protected:
	ActionPanelScheme* mScheme;
	vmActionLabel* mTitle;
	QVBoxLayout* mLayout;
	
private:
	pointersList<vmActionGroup*> mGroups;
};

#endif // VMTASKPANEL_H
