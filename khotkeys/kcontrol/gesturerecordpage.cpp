/****************************************************************************

 KHotKeys
 
 Copyright (C) 2003 Mike Pilone <mpilone@slac.com>
 Copyright (C) 2003 Lubos Lunak <l.lunak@kde.org>

 Distributed under the terms of the GNU General Public License version 2.
 
****************************************************************************/

#include <tqwidget.h>
#include <tqlabel.h>
#include <tqpushbutton.h>

#include <klocale.h>
#include <kmessagebox.h>

#include "gesturerecordpage.h"
#include "gesturerecorder.h"
#include "gesturedrawer.h"

namespace KHotKeys
{

GestureRecordPage::GestureRecordPage(const TQString &gesture,
                                     TQWidget *parent, const char *name)
  : TQVBox(parent, name),
    _recorder(NULL), _resetButton(NULL),
    _tryOne(NULL), _tryTwo(NULL), _tryThree(NULL), _gest(TQString::null),
    _tryCount(1)
    {
    TQString message;

    message = i18n("Draw the gesture you would like to record below. Press "
                   "and hold the left mouse button while drawing, and release "
                   "when you have finished.\n\n"
                   "You will be required to draw the gesture 3 times. After "
                   "each drawing, if they match, the indicators below will "
                   "change to represent which step you are on.\n\n"
                   "If at any point they do not match, you will be required to "
                   "restart. If you want to force a restart, use the reset "
                   "button below.\n\nDraw here:");

    TQLabel *label = new TQLabel(message, this, "label");
    label->setAlignment(TQLabel::AlignLeft | TQLabel::WordBreak |
                        TQLabel::AlignVCenter);

    _recorder = new GestureRecorder(this, "recorder");
    _recorder->setMinimumHeight(150);
    setStretchFactor(_recorder, 1);
    connect(_recorder, TQT_SIGNAL(recorded(const TQString &)),
            this, TQT_SLOT(slotRecorded(const TQString &)));

    TQHBox *hBox = new TQHBox(this, "hbox");

    _tryOne = new GestureDrawer(hBox, "tryOne");
    _tryTwo = new GestureDrawer(hBox, "tryTwo");
    _tryThree = new GestureDrawer(hBox, "tryThree");

    TQWidget *spacer = new TQWidget(hBox, "spacer");
    hBox->setStretchFactor(spacer, 1);

    _resetButton = new TQPushButton(i18n("&Reset"), hBox, "resetButton");
    connect(_resetButton, TQT_SIGNAL(clicked()),
            this, TQT_SLOT(slotResetClicked()));



  // initialize
    if (!gesture.isNull())
        {
        slotRecorded(gesture);
        slotRecorded(gesture);
        slotRecorded(gesture);
        }
    else
        emit gestureRecorded(false);
    }

GestureRecordPage::~GestureRecordPage()
    {
    }

void GestureRecordPage::slotRecorded(const TQString &data)
    {
    switch (_tryCount)
        {
        case 1:
            {
            _gest = data;
            _tryOne->setData(_gest);
            _tryCount++;
            }
        break;

    case 2:
            {
            if (_gest == data)
                {
                _tryTwo->setData(data);
                _tryCount++;
                }
            else
                {
                KMessageBox::sorry(this, i18n("Your gestures did not match."));
                slotResetClicked();
                }
            break;
            }

        case 3:
            {
            if (_gest == data)
                {
                _tryThree->setData(data);
                _tryCount++;
                emit gestureRecorded(true);
                }
            else
                {
                KMessageBox::sorry(this, i18n("Your gestures did not match."));
                slotResetClicked();
                }
            break;
            }
        default:
            KMessageBox::information(this, i18n("You have already completed the three required drawings. Either press 'Ok' to save or 'Reset' to try again."));
        }
    }

void GestureRecordPage::slotResetClicked()
    {
    _gest = TQString::null;

    _tryOne->setData(TQString::null);
    _tryTwo->setData(TQString::null);
    _tryThree->setData(TQString::null);

    _tryCount = 1;

    emit gestureRecorded(false);
    }

} // namespace KHotKeys

#include "gesturerecordpage.moc"
