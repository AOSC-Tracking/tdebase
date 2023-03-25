#include <tqfile.h>
#include <tqlabel.h>
#include <tqlayout.h>

#include <kiconloader.h>
#include <kpushbutton.h>
#include <kseparator.h>
#include <kstddirs.h>
#include <ktabwidget.h>
#include <ktextbrowser.h>
#include <tdelocale.h>

#include "TDELicenseDlg.h"

TDELicenseDlg::TDELicenseDlg(TQWidget *parent, const char *name)
        : KDialog(parent, name)
{
  setCaption(i18n("TDE License"));
  setIcon(TDEGlobal::iconLoader()->loadIcon("about_kde", TDEIcon::NoGroup, TDEIcon::SizeSmall));
  resize(850, 750);

  TQVBoxLayout *vbox = new TQVBoxLayout(this, marginHint(), spacingHint());

  // Top label
  TQLabel *topLabel = new TQLabel(this);
  topLabel->setText(i18n(
          "The Trinity Desktop Environment (TDE) project is a computer desktop\n"
          "environment for Unix-like operating systems with a primary goal of\n"
          "retaining the function and form of traditional desktop computers.\n\n"
          "Its components are provided under the following licenses, as applicable.\n\n"
          "Thanks for using TDE!"));
  vbox->addWidget(topLabel);

  TQSpacerItem *spacerV = new TQSpacerItem(10, 10, TQSizePolicy::Minimum,
          TQSizePolicy::Minimum);
  vbox->addItem(spacerV);

  // License tab widget
  KTabWidget *twLicense = new KTabWidget(this);
  twLicense->setTabReorderingEnabled(false);
  twLicense->setAutomaticResizeTabs(true);
  twLicense->setTabCloseActivatePrevious(true);
  twLicense->setMouseWheelScroll(true);
  twLicense->setTabPosition(TQTabWidget::Top);

  KTextBrowser *tbLicense = new KTextBrowser(twLicense);
  tbLicense->setText(readLicenseFile("GPL_V2"));
  tbLicense->moveCursor(TQTextEdit::MoveHome, false);
  twLicense->addTab(tbLicense, i18n("GPL v2"));

  tbLicense = new KTextBrowser(twLicense);
  tbLicense->setText(readLicenseFile("GPL_V3"));
  tbLicense->moveCursor(TQTextEdit::MoveHome, false);
  twLicense->addTab(tbLicense, i18n("GPL v3"));

  tbLicense = new KTextBrowser(twLicense);
  tbLicense->setText(readLicenseFile("LGPL_V2"));
  tbLicense->moveCursor(TQTextEdit::MoveHome, false);
  twLicense->addTab(tbLicense, i18n("LGPL v2"));

  tbLicense = new KTextBrowser(twLicense);
  tbLicense->setText(readLicenseFile("LGPL_V3"));
  tbLicense->moveCursor(TQTextEdit::MoveHome, false);
  twLicense->addTab(tbLicense, i18n("LGPL v3"));

  tbLicense = new KTextBrowser(twLicense);
  tbLicense->setText(readLicenseFile("BSD"));
  tbLicense->moveCursor(TQTextEdit::MoveHome, false);
  twLicense->addTab(tbLicense, i18n("BSD"));

  tbLicense = new KTextBrowser(twLicense);
  tbLicense->setText(readLicenseFile("ARTISTIC"));
  tbLicense->moveCursor(TQTextEdit::MoveHome, false);
  twLicense->addTab(tbLicense, i18n("Artistic"));

  tbLicense = new KTextBrowser(twLicense);
  tbLicense->setText(readLicenseFile("QPL_V1.0"));
  tbLicense->moveCursor(TQTextEdit::MoveHome, false);
  twLicense->addTab(tbLicense, i18n("QPL v1.0"));

  tbLicense = new KTextBrowser(twLicense);
  tbLicense->setText(readLicenseFile("MIT"));
  tbLicense->moveCursor(TQTextEdit::MoveHome, false);
  twLicense->addTab(tbLicense, i18n("MIT"));

  twLicense->setCurrentPage(0);
  vbox->addWidget(twLicense);

  KSeparator *sep = new KSeparator(KSeparator::HLine, this);
  vbox->addWidget(sep);

  // Close button
  TQHBoxLayout *hboxBottom = new TQHBoxLayout(vbox, 4);
  TQSpacerItem *spacerHBottom = new TQSpacerItem(10, 10, TQSizePolicy::Expanding,
          TQSizePolicy::Minimum);
  hboxBottom->addItem(spacerHBottom);
  KPushButton *okButton = new KPushButton(KStdGuiItem::ok(), this);
  connect(okButton, TQT_SIGNAL(clicked()), this, TQT_SLOT(accept()));
  okButton->setDefault(true);
  okButton->setFocus();
  hboxBottom->addWidget(okButton);
}

TQString TDELicenseDlg::readLicenseFile(const TQString &licenseName)
{
  TQString licensePath = locate("data", TQString("LICENSES/%1").arg(licenseName));
  if (licensePath.isEmpty())
  {
    return i18n("License file not found!");
  }

  TQFile licenseFile(licensePath);
  if (licenseFile.open(IO_ReadOnly))
  {
    TQTextStream txtstr(&licenseFile);
    return txtstr.read();
  }

  return i18n("Unable to open license file!");
}

#include "TDELicenseDlg.moc"
