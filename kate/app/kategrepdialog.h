/* This file is part of the KDE project
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2001 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2001 Anders Lund <anders.lund@lund.tdcadsl.dk>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef _GREPDIALOG_H_
#define _GREPDIALOG_H_

#include <kdialog.h>
#include <tqstringlist.h>

class TQLineEdit;
class KComboBox;
class TQCheckBox;
class TQListBox;
class KPushButton;
class TQLabel;
class TDEProcess;
class TDEConfig;
class KURLRequester;
class TQEvent;

class GrepTool : public TQWidget
{
    TQ_OBJECT

public:
    GrepTool(TQWidget *parent, const char *name=0);
    ~GrepTool();

    // only updates if the dir you give to it differs from the last one given to it !
    void updateDirName(const TQString &);

    void setDirName(const TQString &);


signals:
    void itemSelected(const TQString &abs_filename, int line);

public slots:
    void slotSearchFor(const TQString &pattern);

protected:
    bool eventFilter( TQObject *, TQEvent * );
    void focusInEvent ( TQFocusEvent * ); 
    void showEvent( TQShowEvent * );
    bool m_fixFocus;

private slots:
    void templateActivated(int index);
    void childExited();
    void receivedOutput(TDEProcess *proc, char *buffer, int buflen);
    void receivedErrOutput(TDEProcess *proc, char *buffer, int buflen);
    void itemSelected(const TQString&);
    void slotSearch();
    void slotCancel();
    void slotClear();
    void patternTextChanged( const TQString &);
private:
    void processOutput();
    void finish();

    TQLineEdit *leTemplate;
    KComboBox *cmbFiles, *cmbPattern;
    KURLRequester *cmbDir;
    TQCheckBox *cbRecursive;
    TQCheckBox *cbCasesensitive;
    TQCheckBox *cbRegex;
    TQCheckBox *cbHideErrors;
    TQListBox *lbResult;
    KPushButton *btnSearch, *btnClear;
    TDEProcess *childproc;
    TQString buf;
    TQString errbuf;
    TDEConfig* config;
    TQStringList lastSearchItems;
    TQStringList lastSearchPaths;
    TQStringList lastSearchFiles;
    TQString m_lastUpdatedDir;
    TQString m_workingDir;
};


#endif
