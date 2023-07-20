//
//
// "Misc Options" Tab for KFM configuration
//
// (c) Sven Radej 1998
// (c) David Faure 1998
//

#ifndef __KMISCHTML_OPTIONS_H
#define __KMISCHTML_OPTIONS_H

#include <tqstrlist.h>
#include <tqcheckbox.h>
#include <tqlineedit.h>
#include <tqcombobox.h>


//-----------------------------------------------------------------------------
// The "Misc Options" Tab for the HTML view contains :

// Change cursor over links
// Underline links
// AutoLoad Images
// Smooth Scrolling
// ... there is room for others :))


#include <tqstring.h>
#include <tdeconfig.h>
#include <tdecmodule.h>
class TQRadioButton;
class KIntNumInput;

class KMiscHTMLOptions : public TDECModule
{
    TQ_OBJECT

public:
    KMiscHTMLOptions(TDEConfig *config, TQString group, TQWidget *parent = 0L, const char *name = 0L );
	~KMiscHTMLOptions();
    virtual void load();
    virtual void load( bool useDefaults );
    virtual void save();
    virtual void defaults();

private slots:
    void slotChanged();

private:
    TDEConfig* m_pConfig;
    TQString  m_groupname;

    TQComboBox* m_pUnderlineCombo;
    TQComboBox* m_pAnimationsCombo;
    TQComboBox* m_pSmoothScrollingCombo;
    TQCheckBox* m_cbCursor;
    TQCheckBox* m_pAutoLoadImagesCheckBox;
    TQCheckBox* m_pEnableFavIconCheckBox;
    TQCheckBox* m_pUnfinishedImageFrameCheckBox;
    TQCheckBox* m_pAutoRedirectCheckBox;
    TQCheckBox* m_pOpenMiddleClick;
    TQCheckBox* m_pBackRightClick;
    TQCheckBox* m_pFormCompletionCheckBox;
    TQCheckBox* m_pAdvancedAddBookmarkCheckBox;
    TQCheckBox* m_pOnlyMarkedBookmarksCheckBox;
    KIntNumInput* m_pMaxFormCompletionItems;
};

#endif
