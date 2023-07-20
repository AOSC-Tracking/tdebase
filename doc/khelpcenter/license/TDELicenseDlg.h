#ifndef __TDELICENSE_DLG_H__
#define __TDELICENSE_DLG_H__

#include <kdialog.h>

class TDELicenseDlg : public KDialog
{
  TQ_OBJECT

public:
  TDELicenseDlg(TQWidget *parent = 0, const char *name = 0);

protected:
  static TQString readLicenseFile(const TQString &licenseName);
};

#endif
