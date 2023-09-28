#include "actionpanelscheme.h"
#include "vmlibs.h"

const QString ActionPanelScheme::PanelStyleStr[6] =
{
	QStringLiteral ( "PANEL_NONE" ),
	QStringLiteral ( "PANEL_DEFAULT" ),
	QStringLiteral ( "PANEL_DEFAULT_2" ),
	QStringLiteral ( "PANEL_VISTA" ),
	QStringLiteral ( "PANEL_XP_1" ),
	QStringLiteral ( "PANEL_XP_2" )
};

const char* const ActionPanelNoStyle ( "" );

const char* const colorDefault1 ( "#88b6b6" );
const char* const colorDefault2 ( "#cad6dc" );
const char* const ActionPanelDefaultStyle (

	"QFrame[class='panel'] {"
		"background-color: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 0, stop: 0 #88b6b6, stop: 1 #cad6dc);"
	"}"

	"vmActionGroup QFrame[class='header'] {"
		"background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #cbcfd0, stop: 1 #b2bcc1);"
		"border: 1px solid #00aa99;"
		"border-bottom: 1px solid #8ebdbd;"
		"border-top-left-radius: 3px;"
		"border-top-right-radius: 3px;"
	"}"

	"vmActionGroup QFrame[class='header']:hover {"
		//"background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #EAF7FF, stop: 1 #F9FDFF);"
		"background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #b2bcc1, stop: 1 #cbcfd0);"
	"}"

	"vmActionGroup QToolButton[class='header'] {"
		"text-align: left;"
		"font: 14px;"
		"color: #006600;"
		"background-color: transparent;"
		"border: 1px solid transparent;"
	"}"

	"vmActionGroup QToolButton[class='header']:hover {"
		"color: #00cc00;"
		"text-decoration: underline;"
	"}"

	"vmActionGroup QFrame[class='content'] {"
		"background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #cbcfd0, stop: 1 #b2bcc1);"
		"border: 1px solid #00aa99;"
	"}"

	"vmActionGroup QFrame[class='content'][header='true'] {"
		"border-top: none;"
	"}"

	"vmActionGroup QToolButton[class='action'] {"
		"background-color: transparent;"
		"border: 1px solid transparent;"
		"color: #0033ff;"
		"text-align: left;"
		"font: 11px;"
	"}"

	"vmActionGroup QToolButton[class='action']:!enabled {"
		"color: #999999;"
	"}"

	"vmActionGroup QToolButton[class='action']:hover {"
		"color: #0099ff;"
		"text-decoration: underline;"
	"}"

	"vmActionGroup QToolButton[class='action']:focus {"
		"border: 1px dotted black;"
	"}"

	"vmActionGroup QToolButton[class='action']:on {"
		"background-color: #ddeeff;"
		"color: #006600;"
	"}"
);

const char* const colorVista1 ( "#5C9EEC" );
const char* const colorVista2 ( "#86E78A" );
const char* const ActionPanelVistaStyle (

	"QFrame[class='panel'] {"
	"background: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 0, stop: 0 #5C9EEC, stop: 1 #86E78A);"
	"}"

	"vmActionGroup QFrame[class='header'] {"
	"background: transparent;"
	"border: 1px solid transparent;"
	"}"

	"vmActionGroup QFrame[class='header']:hover {"
	"background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 rgba(249,253,255,100), stop: 0.5 rgba(234,247,255,20), stop: 1 rgba(249,253,255,100));"
	"border: 1px solid transparent;"
	"}"

	"vmActionGroup QToolButton[class='header'] {"
	"text-align: left;"
	"color: #ffffff;"
	"background-color: transparent;"
	"border: 1px solid transparent;"
	"font-size: 12px;"
	"}"

	"vmActionGroup QFrame[class='content'] {"
	"background-color: transparent;"
	"color: #ffffff;"
	"}"

	"vmActionGroup QComboBox[class='content'] {"
	"background-color: transparent;"
	"color: #ffffff;"
	"}"

	"vmActionGroup QToolButton[class='action'] {"
	"background-color: transparent;"
	"border: 1px solid transparent;"
	"color: #ffffff;"
	"text-align: left;"
	"}"

	"vmActionGroup QToolButton[class='action']:!enabled {"
	"color: #666666;"
	"}"

	"vmActionGroup QToolButton[class='action']:hover {"
	"color: #DAF2FC;"
	"text-decoration: underline;"
	"}"

	"vmActionGroup QToolButton[class='action']:focus {"
	"border: 1px dotted black;"
	"}"

	"vmActionGroup QToolButton[class='action']:on {"
	"background-color: #ddeeff;"
	"color: #006600;"
	"}"
);

const char* const colorXP1 ( "#7ba2e7" );
const char* const colorXP2 ( "#6375d6" );
const char* const ActionPanelWinXPBlueStyle1 (

	"QFrame[class='panel'] {"
		"background-color: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 0, stop: 0 #7ba2e7, stop: 1 #6375d6);"
	"}"

	"vmActionGroup QFrame[class='header'] {"
		"background-color: #225aca;"
		"border: 1px solid #225aca;"
		"border-top-left-radius: 4px;"
		"border-top-right-radius: 4px;"
	"}"

	"vmActionGroup QToolButton[class='header'] {"
		"text-align: left;"
		"color: #ffffff;"
		"background-color: transparent;"
		"border: 1px solid transparent;"
		"font-weight: bold;"
	"}"

	"vmActionGroup QToolButton[class='header']:hover {"
		"color: #428eff;"
	"}"

	"vmActionGroup QFrame[class='content'] {"
		"background-color: #eff3ff;"
		"border: 1px solid #ffffff;"
	"}"

	"vmActionGroup QFrame[class='content'][header='true'] {"
		"border-top: none;"
	"}"

	"vmActionGroup QToolButton[class='action'] {"
		"background-color: transparent;"
		"border: 1px solid transparent;"
		"color: #215dc6;"
		"text-align: left;"
	"}"

	"vmActionGroup QToolButton[class='action']:!enabled {"
		"color: #999999;"
	"}"

	"vmActionGroup QToolButton[class='action']:hover {"
		"color: #428eff;"
	"}"

	"vmActionGroup QToolButton[class='action']:focus {"
		"border: 1px dotted black;"
	"}"

	"vmActionGroup QToolButton[class='action']:on {"
		"background-color: #ddeeff;"
		"color: #006600;"
	"}"
);

const char* const ActionPanelWinXPBlueStyle2 (

	"QFrame[class='panel'] {"
		"background-color: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 0, stop: 0 #7ba2e7, stop: 1 #6375d6);"
	"}"

	"vmActionGroup QFrame[class='header'] {"
		"border: 1px solid #ffffff;"
		"border-top-left-radius: 4px;"
		"border-top-right-radius: 4px;"
		"background-color: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 0, stop: 0 #ffffff, stop: 1 #c6d3f7);"
	"}"

	"vmActionGroup QToolButton[class='header'] {"
		"text-align: left;"
		"color: #215dc6;"
		"background-color: transparent;"
		"border: 1px solid transparent;"
		"font-weight: bold;"
	"}"

	"vmActionGroup QToolButton[class='header']:hover {"
		"color: #428eff;"
	"}"

	"vmActionGroup QFrame[class='content'] {"
		"background-color: #d6dff7;"
		"border: 1px solid #ffffff;"
	"}"

	"vmActionGroup QFrame[class='content'][header='true'] {"
		"border-top: none;"
	"}"

	"vmActionGroup QToolButton[class='action'] {"
		"background-color: transparent;"
		"border: 1px solid transparent;"
		"color: #215dc6;"
		"text-align: left;"
	"}"

	"vmActionGroup QToolButton[class='action']:!enabled {"
		"color: #999999;"
	"}"

	"vmActionGroup QToolButton[class='action']:hover {"
		"color: #428eff;"
	"}"

	"vmActionGroup QToolButton[class='action']:focus {"
		"border: 1px dotted black;"
	"}"

	"vmActionGroup QToolButton[class='action']:on {"
		"background-color: #ddeeff;"
		"color: #006600;"
	"}"
);

const char* const colorDefault21 ( "#99cccc" );
const char* const colorDefault22 ( "#EAF7FF" );
const char* const ActionPanelDefaultStyle2 (
		"QFrame[class='panel'] {"
			"background-color: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 0, stop: 0 #99cccc, stop: 1 #EAF7FF);"
		"}"
	
		"vmActionGroup QFrame[class='header'] {"
			"background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #F9FDFF, stop: 1 #EAF7FF);"
			"border: 1px solid #00aa99;"
			"border-bottom: 1px solid #99cccc;"
			"border-top-left-radius: 3px;"
			"border-top-right-radius: 3px;"
		"}"
	
		"vmActionGroup QFrame[class='header']:hover {"
			"background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #EAF7FF, stop: 1 #F9FDFF);"
		"}"
	
		"vmActionGroup QToolButton[class='header'] {"
			"text-align: left;"
			"font: 14px;"
			"color: #006600;"
			"background-color: transparent;"
			"border: 1px solid transparent;"
		"}"
	
		"vmActionGroup QToolButton[class='header']:hover {"
			"color: #00cc00;"
			"text-decoration: underline;"
		"}"
	
		"vmActionGroup QFrame[class='content'] {"
			"background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #F9FDFF, stop: 1 #EAF7FF);"
			"border: 1px solid #00aa99;"
		"}"
	
		"vmActionGroup QFrame[class='content'][header='true'] {"
			"border-top: none;"
		"}"
	
		"vmActionGroup QToolButton[class='action'] {"
			"background-color: transparent;"
			"border: 1px solid transparent;"
			"color: #0033ff;"
			"text-align: left;"
			"font: 11px;"
		"}"
	
		"vmActionGroup QToolButton[class='action']:!enabled {"
			"color: #999999;"
		"}"
	
		"vmActionGroup QToolButton[class='action']:hover {"
			"color: #0099ff;"
			"text-decoration: underline;"
		"}"
	
		"vmActionGroup QToolButton[class='action']:focus {"
			"border: 1px dotted black;"
		"}"
	
		"vmActionGroup QToolButton[class='action']:on {"
			"background-color: #ddeeff;"
			"color: #006600;"
		"}"
);

ActionPanelScheme::ActionPanelScheme ( const PanelStyle style )
{
	headerSize = 26;
	headerAnimation = true;

	headerButtonFold = ICON ( "itmages-upload" );
	headerButtonFoldOver = ICON ( "upload" );
	headerButtonUnfold = ICON ( "go-down-search" );
	headerButtonUnfoldOver = ICON ( "vm-download" );
	headerButtonClose = ICON ( "dialog-close" );
	headerButtonCloseOver = ICON ( "tab-close" );
	headerButtonSize = QSize ( 24, 24 );

	groupFoldSteps = 20;
	groupFoldDelay = 15;
	groupFoldEffect = SlideFolding;
	groupFoldThaw = true;
	styleName = PanelStyleStr[static_cast<int>(style)];

	switch ( style )
	{
		case PANEL_NONE:
			actionStyle = ActionPanelNoStyle;
			colorStyle1 = emptyString;
			colorStyle2 = emptyString;
		break;
		case PANEL_DEFAULT:
			actionStyle = ActionPanelDefaultStyle;
			colorStyle1 = colorDefault1;
			colorStyle2 = colorDefault2;
		break;
		case PANEL_DEFAULT_2:
			actionStyle = ActionPanelDefaultStyle2;
			colorStyle1 = colorDefault21;
			colorStyle2 = colorDefault22;
		break;
		case PANEL_VISTA:
			actionStyle = ActionPanelVistaStyle;
			colorStyle1 = colorVista1;
			colorStyle2 = colorVista2;
		break;
		case PANEL_XP_1:
			actionStyle = ActionPanelWinXPBlueStyle1;
			colorStyle1 = colorXP1;
			colorStyle2 = colorXP2;
		break;
		case PANEL_XP_2:
			actionStyle = ActionPanelWinXPBlueStyle2;
			colorStyle1 = colorXP1;
			colorStyle2 = colorXP2;
		break;
	}
}

ActionPanelScheme* ActionPanelScheme::defaultScheme ()
{
	static ActionPanelScheme scheme ( PANEL_NONE );
	return &scheme;
}

// No pointer management here. Must be inplementes somewhere else
ActionPanelScheme* ActionPanelScheme::newScheme ( const QString& scheme_name )
{
	if ( scheme_name == PanelStyleStr[PANEL_NONE] )
		return new ActionPanelScheme ( PANEL_NONE );
	if ( scheme_name == PanelStyleStr[PANEL_DEFAULT] )
		return new ActionPanelScheme ( PANEL_DEFAULT );
	if ( scheme_name == PanelStyleStr[PANEL_DEFAULT_2] )
		return new ActionPanelScheme ( PANEL_DEFAULT_2 );
	if ( scheme_name == PanelStyleStr[PANEL_VISTA] )
		return new ActionPanelScheme ( PANEL_VISTA );
	if ( scheme_name == PanelStyleStr[PANEL_XP_1] )
		return new ActionPanelScheme ( PANEL_XP_1 );
	if ( scheme_name == PanelStyleStr[PANEL_XP_2] )
		return new ActionPanelScheme ( PANEL_XP_2 );
	
	return new ActionPanelScheme ( PANEL_NONE );
}
