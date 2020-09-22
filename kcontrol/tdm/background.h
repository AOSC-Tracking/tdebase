/*
 *
 * This file is part of the KDE project, module kcmdisplay.
 * Copyright (C) 1999 Geert Jansen <g.t.jansen@stud.tue.nl>
 *
 * You can Freely distribute this program under the GNU General Public
 * License. See the file "COPYING" for the exact licensing terms.
 */

#ifndef __Bgnd_h_Included__
#define __Bgnd_h_Included__

#include <tqobject.h>
#include <tqwidget.h>


class KSimpleConfig;
class BGDialog;
class TDEGlobalBackgroundSettings;
class TQCheckBox;
class TQLabel;

class KBackground: public TQWidget
{
    Q_OBJECT
public:
    KBackground(TQWidget *parent=0, const char *name=0);
    ~KBackground();

    void load();
    void save();
    void defaults();
    void makeReadOnly();
signals:
    void changed(bool);

private slots:
    void slotEnableChanged();
private:
    void init();
    void apply();

    TQCheckBox *m_pCBEnable;
    TQLabel *m_pMLabel;
    KSimpleConfig *m_simpleConf;
    BGDialog *m_background;
};


#endif // __Bgnd_h_Included__
