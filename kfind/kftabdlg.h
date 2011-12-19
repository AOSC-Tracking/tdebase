/***********************************************************************
 *
 *  kftabdlg.h
 *
 ***********************************************************************/

#ifndef KFTABDLG_H
#define KFTABDLG_H

#include <tqtabwidget.h>
#include <tqvalidator.h> // for KDigitValidator

#include <kurl.h>
#include <kmimetype.h>

#include "kdatecombo.h"

class TQButtonGroup;
class TQPushButton;
class TQRadioButton;
class TQCheckBox;
class TQLineEdit;
class TQString;
class TQDate;
class TQRegExp;
class TQDialog;
class TQComboBox;
class TQSpinBox;

class KfDirDialog;

class KfindTabWidget: public TQTabWidget
{
  Q_OBJECT

public:
  KfindTabWidget(TQWidget * parent = 0, const char *name=0);
  virtual ~KfindTabWidget();
  void initMimeTypes();
  void initSpecialMimeTypes();
  void setQuery(class KQuery * query);
  void setDefaults();

  void beginSearch();
  void endSearch();
  void loadHistory();
  void saveHistory();
  bool isSearchRecursive();

  void setURL( const KURL & url );

  virtual TQSize sizeHint() const;

public slots:
  void setFocus();

private slots:
  void getDirectory();
  void fixLayout();
  void slotSizeBoxChanged(int);
  void slotEditRegExp();

signals:
    void startSearch();

protected:
public:
  TQComboBox   *nameBox;
  TQComboBox   *dirBox;
  // for first page
  TQCheckBox   *subdirsCb;
  TQCheckBox *useLocateCb;
  // for third page
  TQComboBox *typeBox;
  TQLineEdit * textEdit;
  TQCheckBox *caseSensCb;
  TQComboBox *m_usernameBox;
  TQComboBox *m_groupBox;
  //for fourth page
  TQLineEdit *metainfoEdit;
  TQLineEdit *metainfokeyEdit;

private:
  bool isDateValid();

  TQString date2String(const TQDate &);
  TQDate &string2Date(const TQString &, TQDate * );

  TQWidget *pages[3];

  //1st page
  TQPushButton *browseB;

  KfDirDialog *dirselector;

  //2nd page
  TQCheckBox   *findCreated;
  TQComboBox   *betweenType;
  TQButtonGroup *bg;
  TQRadioButton *rb[2];
  KDateCombo * fromDate;
  KDateCombo * toDate;
  TQSpinBox *timeBox;

  //3rd page
  TQComboBox *sizeBox;
  TQComboBox *sizeUnitBox;
  TQSpinBox *sizeEdit;
  TQCheckBox *caseContextCb;
  TQCheckBox *binaryContextCb;
  TQCheckBox *regexpContentCb;
  TQDialog *regExpDialog;

  KURL m_url;

  KMimeType::List m_types;
  TQStringList m_ImageTypes;
  TQStringList m_VideoTypes;
  TQStringList m_AudioTypes;
};

class KDigitValidator : public TQValidator
{
  Q_OBJECT

public:
  KDigitValidator(TQWidget * parent, const char *name = 0 );
  ~KDigitValidator();

  TQValidator::State validate(TQString & input, int &) const;

 private:
  TQRegExp *r;
};

#endif
