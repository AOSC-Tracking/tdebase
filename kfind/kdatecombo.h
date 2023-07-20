/***********************************************************************
 *
 *  kdatecombo.h
 *
 ***********************************************************************/

#ifndef KDATECOMBO_H
#define KDATECOMBO_H

#include <tqwidget.h>
#include <tqcombobox.h>
#include <tqdatetime.h>

/**
  *@author Beppe Grimaldi
  */

class KDatePicker;
class TDEPopupFrame;

class KDateCombo : public TQComboBox  {
   TQ_OBJECT

public:
	KDateCombo(TQWidget *parent=0, const char *name=0);
	KDateCombo(const TQDate & date, TQWidget *parent=0, const char *name=0);
	~KDateCombo();

	TQDate & getDate(TQDate *currentDate);
	bool setDate(const TQDate & newDate);

private:
   TDEPopupFrame * popupFrame;
   KDatePicker * datePicker;

   void initObject(const TQDate & date, TQWidget *parent, const char *name);

   TQString date2String(const TQDate &);
   TQDate & string2Date(const TQString &, TQDate * );

protected:
  bool eventFilter (TQObject*, TQEvent*);
  virtual void mousePressEvent (TQMouseEvent * e);
   
protected slots:
   void dateEnteredEvent(TQDate d=TQDate());
};

#endif
