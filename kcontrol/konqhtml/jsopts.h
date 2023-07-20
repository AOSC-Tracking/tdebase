//-----------------------------------------------------------------------------
//
// HTML Options
//
// (c) Martin R. Jones 1996
//
// Port to KControl
// (c) Torben Weis 1998

#ifndef __JSOPTS_H__
#define __JSOPTS_H__

#include <tdecmodule.h>

#include "domainlistview.h"
#include "jspolicies.h"

class KColorButton;
class TDEConfig;
class KURLRequester;
class TQCheckBox;
class TQComboBox;
class TQLineEdit;
class TQListViewItem;
class TQRadioButton;
class TQSpinBox;
class TQButtonGroup;

class PolicyDialog;

class KJavaScriptOptions;

/** JavaScript-specific enhancements to the domain list view
  */
class JSDomainListView : public DomainListView {
  TQ_OBJECT
public:
  JSDomainListView(TDEConfig *config,const TQString &group,KJavaScriptOptions *opt,
  		TQWidget *parent,const char *name = 0);
  virtual ~JSDomainListView();

  /** remnant for importing pre KDE 3.2 settings
    */
  void updateDomainListLegacy(const TQStringList &domainConfig);

protected:
  virtual JSPolicies *createPolicies();
  virtual JSPolicies *copyPolicies(Policies *pol);
  virtual void setupPolicyDlg(PushButton trigger,PolicyDialog &pDlg,
  		Policies *copy);

private:
  TQString group;
  KJavaScriptOptions *options;
};

class KJavaScriptOptions : public TDECModule
{
  TQ_OBJECT
public:
  KJavaScriptOptions( TDEConfig* config, TQString group, TQWidget* parent = 0, const char* name = 0 );

  virtual void load();
  virtual void load( bool useDefaults );
  virtual void save();
  virtual void defaults();

  bool _removeJavaScriptDomainAdvice;

private slots:
  void slotChangeJSEnabled();

private:

  TDEConfig *m_pConfig;
  TQString m_groupname;
  JSPolicies js_global_policies;
  TQCheckBox *enableJavaScriptGloballyCB;
  TQCheckBox *reportErrorsCB;
  TQCheckBox *jsDebugWindow;
  JSPoliciesFrame *js_policies_frame;
  bool _removeECMADomainSettings;

  JSDomainListView* domainSpecific;
  
  friend class JSDomainListView;
};

#endif		// __JSOPTS_H__

