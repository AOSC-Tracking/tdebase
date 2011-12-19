/***********************************************************************
 *
 *  kdatecombo.cpp
 *
 ***********************************************************************/

#include <tqtimer.h>

#include <kglobal.h>
#include <klocale.h>
#include <kdatepicker.h>
#include <kdatetbl.h>
#include <kdebug.h>

#include "kdatecombo.h"

#include "kdatecombo.moc"

KDateCombo::KDateCombo(TQWidget *parent, const char *name ) : TQComboBox(FALSE, parent,name)
{
  TQDate date = TQDate::currentDate();
  initObject(date, parent, name);
}

KDateCombo::KDateCombo(const TQDate & date, TQWidget *parent, const char *name) : TQComboBox(FALSE, parent,name)
{
  initObject(date, parent, name);
}

void KDateCombo::initObject(const TQDate & date, TQWidget *, const char *)
{
  clearValidator();
  popupFrame = new KPopupFrame(this, "popupFrame");
  popupFrame->installEventFilter(this);
  datePicker = new KDatePicker(popupFrame, date, "datePicker");
  datePicker->setMinimumSize(datePicker->sizeHint());
  datePicker->installEventFilter(this);
  popupFrame->setMainWidget(datePicker);
  setDate(date);

  connect(datePicker, TQT_SIGNAL(dateSelected(TQDate)), this, TQT_SLOT(dateEnteredEvent(TQDate)));
}

KDateCombo::~KDateCombo()
{
  delete datePicker;
  delete popupFrame;
}

TQString KDateCombo::date2String(const TQDate & date)
{
  return(KGlobal::locale()->formatDate(date, true));
}

TQDate & KDateCombo::string2Date(const TQString & str, TQDate *qd)
{
  return *qd = KGlobal::locale()->readDate(str);
}

TQDate & KDateCombo::getDate(TQDate *currentDate)
{
  return string2Date(currentText(), currentDate);
}

bool KDateCombo::setDate(const TQDate & newDate)
{
  if (newDate.isValid())
  {
    if (count())
      clear();
    insertItem(date2String(newDate));
    return TRUE;
  }
  return FALSE;
}

void KDateCombo::dateEnteredEvent(TQDate newDate)
{
  if (!newDate.isValid())
     newDate = datePicker->date();
  popupFrame->hide();
  setDate(newDate);
}

void KDateCombo::mousePressEvent (TQMouseEvent * e)
{
  if (e->button() & Qt::LeftButton)
  {
    if  (TQT_TQRECT_OBJECT(rect()).contains( e->pos()))
    {
      TQDate tempDate;
      getDate(& tempDate);
      datePicker->setDate(tempDate);
      popupFrame->popup(mapToGlobal(TQPoint(0, height())));
      //datePicker->setFocus();
    }
  }
}

bool KDateCombo::eventFilter (TQObject*, TQEvent* e)
{
  if ( e->type() == TQEvent::MouseButtonPress )
  {
      TQMouseEvent *me = (TQMouseEvent *)e;
      TQPoint p = mapFromGlobal( me->globalPos() );
      if (TQT_TQRECT_OBJECT(rect()).contains( p ) )
      {
        TQTimer::singleShot(10, this, TQT_SLOT(dateEnteredEvent()));
        return true;
      }
  }
  else if ( e->type() == TQEvent::KeyRelease )
  {
      TQKeyEvent *k = (TQKeyEvent *)e;
      //Press return == pick selected date and close the combo
      if((k->key()==Qt::Key_Return)||(k->key()==Qt::Key_Enter))
      {
        dateEnteredEvent(datePicker->date());
        return true;
      }
      else if (k->key()==Qt::Key_Escape)
      {
        popupFrame->hide();
        return true;
      }
      else
        return false;
  }

  return false;
}
