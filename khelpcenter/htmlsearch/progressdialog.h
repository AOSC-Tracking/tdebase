#ifndef __PROGRESS_DIALOG_H__
#define __PROGRESS_DIALOG_H__


#include <kdialogbase.h>


class TQLabel;
class KProgress;


class ProgressDialog : public KDialogBase
{

  TQ_OBJECT

public:

  ProgressDialog(TQWidget *parent=0, const char *name=0);

  void setFilesScanned(int s);
  void setFilesToDig(int d);
  void setFilesDigged(int d);

  void setState(int n);

private:

  TQLabel    *filesScanned, *check1, *check2, *check3;
  KProgress *bar;

};


#endif
