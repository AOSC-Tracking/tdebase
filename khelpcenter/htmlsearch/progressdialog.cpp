#include <tqlayout.h>
#include <tqlabel.h>


#include <tdelocale.h>
#include <kprogress.h>
#include <tdestandarddirs.h>


#include "progressdialog.moc"


ProgressDialog::ProgressDialog(TQWidget *parent, const char *name)
  : KDialogBase(KDialogBase::Plain, i18n("Generating Index"), Cancel, Cancel,
		parent, name, false)
{
  TQGridLayout *grid = new TQGridLayout(plainPage(), 5,3, spacingHint());
  
  TQLabel *l = new TQLabel(i18n("Scanning for files"), plainPage());
  grid->addMultiCellWidget(l, 0,0, 1,2);
  
  filesScanned = new TQLabel(plainPage());
  grid->addWidget(filesScanned, 1,2);
  setFilesScanned(0);

  check1 = new TQLabel(plainPage());
  grid->addWidget(check1, 0,0);

  l = new TQLabel(i18n("Extracting search terms"), plainPage());
  grid->addMultiCellWidget(l, 2,2, 1,2);
  
  bar = new KProgress(plainPage());
  grid->addWidget(bar, 3,2);

  check2 = new TQLabel(plainPage());
  grid->addWidget(check2, 2,0);

  l = new TQLabel(i18n("Generating index..."), plainPage());
  grid->addMultiCellWidget(l, 4,4, 1,2);

  check3 = new TQLabel(plainPage());
  grid->addWidget(check3, 4,0);

  setState(0);

  setMinimumWidth(300);
}


void ProgressDialog::setFilesScanned(int n)
{
  filesScanned->setText(i18n("Files processed: %1").arg(n));
}


void ProgressDialog::setFilesToDig(int n)
{
  bar->setTotalSteps(n);
}


void ProgressDialog::setFilesDigged(int n)
{
  bar->setProgress(n);
}


void ProgressDialog::setState(int n)
{
  TQPixmap unchecked = TQPixmap(locate("data", "khelpcenter/pics/unchecked.xpm"));
  TQPixmap checked = TQPixmap(locate("data", "khelpcenter/pics/checked.xpm"));

  check1->setPixmap( n > 0 ? checked : unchecked);  
  check2->setPixmap( n > 1 ? checked : unchecked);  
  check3->setPixmap( n > 2 ? checked : unchecked);  
}
