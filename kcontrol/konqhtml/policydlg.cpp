// (C) < 2002 to whoever created and edited this file before
// (C) 2002 Leo Savernik <l.savernik@aon.at>
//	Generalizing the policy dialog

#include <tqlayout.h>
#include <tqlabel.h>
#include <tqwhatsthis.h>
#include <tqcombobox.h>

#include <tdelocale.h>
#include <kbuttonbox.h>
#include <tdemessagebox.h>

#include <tqpushbutton.h>

#include "policydlg.h"
#include "policies.h"

PolicyDialog::PolicyDialog( Policies *policies, TQWidget *parent, const char *name )
    : KDialogBase(parent, name, true, TQString::null, Ok|Cancel, Ok, true), 
      policies(policies)
{
  TQFrame *main = makeMainWidget();

  insertIdx = 1;	// index where to insert additional panels
  topl = new TQVBoxLayout(main, 0, spacingHint());

  TQGridLayout *grid = new TQGridLayout(topl, 2, 2);
  grid->setColStretch(1, 1);

  TQLabel *l = new TQLabel(i18n("&Host or domain name:"), main);
  grid->addWidget(l, 0, 0);

  le_domain = new TQLineEdit(main);
  l->setBuddy( le_domain );
  grid->addWidget(le_domain, 0, 1);
  connect( le_domain,TQ_SIGNAL(textChanged( const TQString & )),
      TQ_SLOT(slotTextChanged( const TQString &)));

  TQWhatsThis::add(le_domain, i18n("Enter the name of a host (like www.trinitydesktop.org) "
                                  "or a domain, starting with a dot (like .trinitydesktop.org or .org)") );

  l_feature_policy = new TQLabel(main);
  grid->addWidget(l_feature_policy, 1, 0);

  cb_feature_policy = new TQComboBox(main);
  l_feature_policy->setBuddy( cb_feature_policy );
  policy_values << i18n("Use Global") << i18n("Accept") << i18n("Reject");
  cb_feature_policy->insertStringList(policy_values);
  grid->addWidget(cb_feature_policy, 1, 1);

  le_domain->setFocus();

  enableButtonOK(!le_domain->text().isEmpty());
}

PolicyDialog::FeatureEnabledPolicy PolicyDialog::featureEnabledPolicy() const {
    return (FeatureEnabledPolicy)cb_feature_policy->currentItem();
}

void PolicyDialog::slotTextChanged( const TQString &text)
{
    enableButtonOK(!text.isEmpty());
}

void PolicyDialog::setDisableEdit( bool state, const TQString& text )
{
    le_domain->setText( text );

    le_domain->setEnabled( state );

    if( state )
        cb_feature_policy->setFocus();
}

void PolicyDialog::refresh() {
    FeatureEnabledPolicy pol;

    if (policies->isFeatureEnabledPolicyInherited())
      pol = InheritGlobal;
    else if (policies->isFeatureEnabled())
      pol = Accept;
    else
      pol = Reject;
    cb_feature_policy->setCurrentItem(pol);
}

void PolicyDialog::setFeatureEnabledLabel(const TQString &text) {
  l_feature_policy->setText(text);
}

void PolicyDialog::setFeatureEnabledWhatsThis(const TQString &text) {
  TQWhatsThis::add(cb_feature_policy, text);
}

void PolicyDialog::addPolicyPanel(TQWidget *panel) {
  topl->insertWidget(insertIdx++,panel);
}

TQString PolicyDialog::featureEnabledPolicyText() const {
  int pol = cb_feature_policy->currentItem();
  if (pol >= 0 && pol < 3) // Keep in sync with FeatureEnabledPolicy
    return policy_values[pol];
  else
    return TQString::null;
}

void PolicyDialog::accept()
{
    if( le_domain->text().isEmpty() )
    {
        KMessageBox::information( 0, i18n("You must first enter a domain name.") );
        return;
    }

    FeatureEnabledPolicy pol = (FeatureEnabledPolicy)
    		cb_feature_policy->currentItem();
    if (pol == InheritGlobal) {
      policies->inheritFeatureEnabledPolicy();
    } else if (pol == Reject) {
      policies->setFeatureEnabled(false);
    } else {
      policies->setFeatureEnabled(true);
    }
    TQDialog::accept();
}

#include "policydlg.moc"
