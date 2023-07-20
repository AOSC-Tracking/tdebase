//-----------------------------------------------------------------------------
//
// HTML Options
//
// (c) Martin R. Jones 1996
//
// Port to KControl
// (c) Torben Weis 1998
//
// Redesign and cleanup
// (c) Daniel Molkentin 2000
//
//-----------------------------------------------------------------------------

#ifndef __JAVAOPTS_H__
#define __JAVAOPTS_H__

#include <tdecmodule.h>

#include "domainlistview.h"
#include "policies.h"

class KColorButton;
class TDEConfig;
class TDEListView;
class KURLRequester;
class KIntNumInput;

class TQCheckBox;
class TQComboBox;
class TQLineEdit;
class TQListViewItem;
class TQRadioButton;

class KJavaOptions;

/** policies with java-specific constructor
  */
class JavaPolicies : public Policies {
public:
  /**
   * constructor
   * @param config configuration to initialize this instance from
   * @param group config group to use if this instance contains the global
   *	policies (global == true)
   * @param global true if this instance contains the global policy settings,
   *	false if this instance contains policies specific for a domain.
   * @param domain name of the domain this instance is used to configure the
   *	policies for (case insensitive, ignored if global == true)
   */
  JavaPolicies(TDEConfig* config, const TQString &group, bool global,
  		const TQString &domain = TQString::null);

  /** empty constructur to make TQMap happy
   * don't use for constructing a policies instance.
   * @internal
   */
  JavaPolicies();

  virtual ~JavaPolicies();
};

/** Java-specific enhancements to the domain list view
  */
class JavaDomainListView : public DomainListView {
  TQ_OBJECT
public:
  JavaDomainListView(TDEConfig *config,const TQString &group,KJavaOptions *opt,
  		TQWidget *parent,const char *name = 0);
  virtual ~JavaDomainListView();

  /** remnant for importing pre KDE 3.2 settings
    */
  void updateDomainListLegacy(const TQStringList &domainConfig);

protected:
  virtual JavaPolicies *createPolicies();
  virtual JavaPolicies *copyPolicies(Policies *pol);
  virtual void setupPolicyDlg(PushButton trigger,PolicyDialog &pDlg,
  		Policies *copy);

private:
  TQString group;
  KJavaOptions *options;
};

class KJavaOptions : public TDECModule
{
    TQ_OBJECT

public:
    KJavaOptions( TDEConfig* config, TQString group, TQWidget* parent = 0, const char* name = 0 );

    virtual void load();
    virtual void load( bool useDefaults );
    virtual void save();
    virtual void defaults();

    bool _removeJavaScriptDomainAdvice;

private slots:
    void slotChanged();
    void toggleJavaControls();

private:

    TDEConfig* m_pConfig;
    TQString  m_groupname;
    JavaPolicies java_global_policies;

    TQCheckBox*     enableJavaGloballyCB;
    TQCheckBox*     javaSecurityManagerCB;
    TQCheckBox*     useKioCB;
    TQCheckBox*     enableShutdownCB;
    KIntNumInput*  serverTimeoutSB;
    TQLineEdit*     addArgED;
    KURLRequester* pathED;
    bool           _removeJavaDomainSettings;

    JavaDomainListView *domainSpecific;

    friend class JavaDomainListView;
};

#endif		// __HTML_OPTIONS_H__

