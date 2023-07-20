#ifndef __MODIFIERS_MODULE_H
#define __MODIFIERS_MODULE_H

#include <tqwidget.h>

class TQCheckBox;
class TQLabel;
class KComboBox;
class TDEListView;

class ModifiersModule : public TQWidget
{
	TQ_OBJECT
 public:
	ModifiersModule( TQWidget *parent = 0, const char *name = 0 );

	void load( bool useDefaults );
	void save();
	void defaults();

	static void setupMacModifierKeys();

 signals:
	void changed( bool );

 protected:
	bool m_bMacKeyboardOrig, m_bMacSwapOrig;
	TQString m_sLabelCtrlOrig, m_sLabelAltOrig, m_sLabelWinOrig;

	TQLabel* m_plblCtrl, * m_plblAlt, * m_plblWin;
	TQLabel* m_plblWinModX;
	TQCheckBox* m_pchkMacKeyboard;
	TDEListView* m_plstXMods;
	TQCheckBox* m_pchkMacSwap;

	void initGUI();
	// Places the values in the *Orig variables into their
	//  respective widgets.
	void updateWidgetData();
	// Updates the labels according to the check-box settings
	//  and also reads in the X modifier map.
	void updateWidgets();

 protected slots:
	void slotMacKeyboardClicked();
	void slotMacSwapClicked();
};

#endif
