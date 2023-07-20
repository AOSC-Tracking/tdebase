#ifndef ADVANCEDDIALOG_H
#define ADVANCEDDIALOG_H

#include <kdialogbase.h>
#include <tqwidget.h>
#include <tdeconfig.h>
#include <tqlabel.h>
#include <tqlayout.h>
#include <tqwhatsthis.h>
#include <tqgroupbox.h>
#include <tqobject.h>
#include <tqcheckbox.h>
#include <tqslider.h>

#include "advanceddialogimpl.h"

class AdvancedDialog : public AdvancedDialogImpl
{
public:
	AdvancedDialog(TQWidget *parent = 0, const char *name = 0);
	~AdvancedDialog();
	void setMode(TQComboBox *box, int i);
	int mode(TQComboBox *box);
};

/* =================================================================================================== */

class KScreenSaverAdvancedDialog : public KDialogBase
{
    TQ_OBJECT
public:
    KScreenSaverAdvancedDialog(TQWidget *parent, const char* name = 0);
      
public slots:
    void slotOk();
         
protected slots:
    void slotPriorityChanged(int val);
    void slotChangeBottomRightCorner(int);
    void slotChangeBottomLeftCorner(int);
    void slotChangeTopRightCorner(int);
    void slotChangeTopLeftCorner(int);
                        
private:
    void readSettings();
                     
    TQCheckBox *m_topLeftCorner;
    TQCheckBox *m_bottomLeftCorner;
    TQCheckBox *m_topRightCorner;
    TQCheckBox *m_bottomRightCorner;
    TQSlider   *mPrioritySlider;
                                          
    bool mChanged;
    int  mPriority;
    AdvancedDialog *dialog;

};


#endif

