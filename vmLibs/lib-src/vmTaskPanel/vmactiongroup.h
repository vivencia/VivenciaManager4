#ifndef VMACTIONGROUP_H
#define VMACTIONGROUP_H

#include <vmWidgets/vmwidget.h>

#include <QtGui/QIcon>
#include <QtWidgets/QWidget>
#include <QtWidgets/QFrame>
#include <QtWidgets/QVBoxLayout>

#include <functional>

class vmActionLabel;
class ActionPanelScheme;

class QLabel;
class QTimer;

class TaskGroup : public QFrame
{

public:
	TaskGroup ( QWidget* parent, const bool stretchContents = false );
	virtual ~TaskGroup () = default;

	void setScheme ( ActionPanelScheme *scheme );

	inline void setStretchContents ( const bool stretch ) {
		mbStretchContents = stretch;
	}

	inline QBoxLayout* groupLayout () {
		return static_cast<QVBoxLayout*> ( layout () );
	}

	void addQEntry ( QWidget* widget, QLayout* l, const bool addStretch );
	void addLayout ( QLayout* layout );

	QPixmap transparentRender ();

private:
	ActionPanelScheme* mScheme;
	bool mbStretchContents;
};

class TaskHeader : public QFrame
{

friend class vmActionGroup;

public:

	TaskHeader ( const QIcon& icon, const QString& title,
				 const bool expandable, const bool closable, QWidget* parent =  nullptr );
	virtual ~TaskHeader ();

	inline bool expandable () const {
		return mb_expandable;
	}
	void setExpandable ( const bool expandable );
	void setClosable ( const bool closable );
	void setScheme ( ActionPanelScheme* scheme );
	void setTitle ( const QString& new_title );
	void setIcon ( const QIcon& icon );

	inline void setCallbackForFoldButtonClicked ( const std::function<void ()>& func ) { funcFoldButtonClicked = func; }
	inline void setCallbackForCloseButtonClicked ( const std::function<void ()>& func ) { funcCloseButtonClicked = func; }

protected:
	void paintEvent ( QPaintEvent* event ) override;
	void keyPressEvent ( QKeyEvent* event ) override;
	void keyReleaseEvent ( QKeyEvent* event ) override;
	bool eventFilter ( QObject* obj, QEvent* event ) override;

private:
	void animate ();
	void fold ();
	void close ();
	void changeIcons ();

	ActionPanelScheme* mScheme;
	vmActionLabel* mTitle, *mExpandableButton, *mClosableButton;
	QTimer* timerSlide;

	bool mb_expandable, mb_closable, mb_over, mb_buttonOver, mb_fold;
	double md_opacity;

	std::function<void ()> funcFoldButtonClicked;
	std::function<void ()> funcCloseButtonClicked;
};

class vmActionGroup : public QWidget, public vmWidget
{

public:
	/** Constructor. Creates vmActionGroup with header's
		text set to \a title, but with no icon.

		If \a expandable set to \a true  ( default ) , the group can be expanded/collapsed by the user.
	  */
	explicit vmActionGroup ( const QString& title = QString (),
							 const bool expandable = true,
							 const bool stretchContents = true,
							 const bool closable = false,
							 QWidget* parent = nullptr );

	/** Constructor. Creates vmActionGroup with header's
		text set to \a title and icon set to \a icon.

		If \a expandable set to \a true  ( default ) , the group can be expanded/collapsed by the user.
	  */
	explicit vmActionGroup ( const QIcon& icon,
							 const QString& title = QString (),
							 const bool expandable = true,
							 const bool stretchContents = true,
							 const bool closable = false,
							 QWidget* parent = nullptr );

	virtual ~vmActionGroup ();

	inline void setCallbackForClosed ( const std::function<void ()>& func ) { mHeader->setCallbackForCloseButtonClicked ( func ); }
	inline void setTitle ( const QString& new_title ) { mHeader->setTitle ( new_title ); }
	inline void setIcon ( const QIcon& icon ) { mHeader->setIcon ( icon ); }

	/** Creates action item from the \a action and returns it.

	  If \a addToLayout is set to \a true  ( default ) ,
	  the action is added to the default vertical layout, i.e. subsequent
	  calls of this function will create several vmActionLabels arranged vertically,
	  one below another.

	  Set \a addToLayout to \a false if you want to add the action to the specified layout manually.
	  This allows to do custom actions arrangements, i.e. horizontal etc.

	  If \a addStretch is set to \a true  ( default ) ,
	  vmActionLabel will be automatically aligned to the left side of the vmActionGroup.
	  Set \a addStretch to \a false if you want vmActionLabel to occupy all the horizontal space.
	  */
	bool addEntry ( vmWidget* entry, QLayout* l = nullptr, const bool addStretch = false );
	void addQEntry ( QWidget* widget, QLayout* l = nullptr, const bool addStretch = false );
	void addLayout ( QLayout* layout );

	/** Returns group's layout  ( QVBoxLayout by default ) .
	  */
	QBoxLayout* groupLayout () const;

	/** Sets the scheme of the panel and all the child groups to \a scheme.

		By default, ActionPanelScheme::defaultScheme () is used.
	  */
	void setScheme ( ActionPanelScheme* scheme );

	/** Returns \a true if the group is expandable.

	  \sa setExpandable () .
	  */
	bool isExpandable () const;

	/** Returns \a true if the group has header.

	  \sa setHeader () .
	  */
	bool hasHeader () const;

	/** Returns text of the header.
		Only valid if the group has header  ( see hasHeader () ) .

	  \sa setHeaderText () .
	  */
	QString headerText () const;

	QSize minimumSizeHint () const override;

	/** Makes the group expandable if \a expandable is set to \a true.

	  \sa isExpandable () .
	  */
	void setExpandable ( const bool expandable = true );

	/** Enables/disables group's header according to \a enable.

	  \sa hasHeader () .
	  */
	void setHeader ( const bool enable = true );

	/** Sets text of the header to \a title.
		Only valid if the group has header  ( see hasHeader () ) .

	  \sa headerText () .
	  */
	void setHeaderText ( const QString& title );

protected:
	void init ();
	void paintEvent ( QPaintEvent* ) override;

private:
	/** Expands/collapses the group.
		Only valid if the group has header  ( see hasHeader () ) .
	  */
	void showHide ();
	void processHide ();
	void processShow ();

	double m_foldStep, m_foldDelta, m_fullHeight, m_tempHeight;
	int m_foldDirection;
	bool mbStretchContents;

	QPixmap m_foldPixmap;

	class TaskHeader* mHeader;
	class TaskGroup* mGroup;
	QWidget* mDummy;
	ActionPanelScheme *mScheme;
	QTimer* timerShow, *timerHide;
};

#endif // VMACTIONGROUP_H
