#ifndef TDESTRINGMATCHER_DIALOG_H
#define TDESTRINGMATCHER_DIALOG_H

#include <tqobject.h>
#include <tqdialog.h>

#include <tdestringmatcher.h>
#include "update_tdestringmatcher.h"

class TQDialog;
class TQBoxLayout;
class TQVBoxLayout;
class TQHBoxLayout;
class TQGroupBox;
class TQButtonGroup;
class TQSpacerItem;
class TQLabel;
class TQListBox;
class TQListBoxItem;
class TQLineEdit;
class TQComboBox;
class TQPushButton;
class TQRadioButton;

class TDEStringMatcher_UI : public TQDialog
{
 TQ_OBJECT

public:

  TDEStringMatcher_UI(
    TDEStringMatcher::MatchSpecList& data,
    TQString title = ""
  );
  ~TDEStringMatcher_UI();

  UIresult            getDialogResult();
  TDEStringMatcher::MatchSpecList& getMatchSpecs();

public slots:

protected slots:

  virtual void displayItemText();
  virtual void editItemText();
  virtual void updateItemText();
  virtual void lineEditReturn();
  virtual void addItem();
  virtual void removeItem();

  virtual void updatePatternType(int index );
  virtual void updateANCHandling(int index );
  virtual void updateWantMatch(int index );

  virtual void setExitDisposition(int buttonNum );

  virtual void accept();
  virtual void reject();

protected:

  // Keyboard event handlers

  bool event( TQEvent * e );
  class DialogEventHandler : public TQObject
  {
  protected:
    bool eventFilter( TQObject *o, TQEvent *e );
  };

  // Internal data

  TDEStringMatcher::MatchSpecList matchSpecs;
  UIresult           dialogResult;

  // Primary widgets

  TQLabel       *patterns_label;
  TQLabel       *lbl_patternListHeader;

  TQListBox*    lb_patternList;
  TQLineEdit*   le_editPattern;

  TQPushButton* pb_addPattern;
  TQPushButton* pb_delPattern;

  TQComboBox*   cb_patternType;
  TQComboBox*   cb_ancHandling;
  TQComboBox*   cb_expectMatch;

  TQButtonGroup *bg_Disposition;
  TQRadioButton *rb_ApplyNoSave;
  TQRadioButton *rb_ApplyAndSave;
  TQRadioButton *rb_RestoreDefault;

  TQPushButton  *pb_OK;
  TQPushButton  *pb_Cancel;

  // Layout, grouping, and spacing widgets

  TQVBoxLayout* main_vbl;

  TQHBoxLayout* sec1X_hbl;
  TQVBoxLayout* sec1A_vbl;

  TQBoxLayout*  sec1B_vbl;
  TQGroupBox*   sec1B1_vbx;
  TQVBoxLayout *sec1B1_vbl;
  TQGroupBox   *sec1B2_vbx;
  TQVBoxLayout *sec1B2_vbl;
  TQSpacerItem* pad1B3;
  TQVBoxLayout *sec1B4_vbl;

  TQHBoxLayout* sec2X_hbl;
  TQSpacerItem* pad2_hb;
};

#endif
