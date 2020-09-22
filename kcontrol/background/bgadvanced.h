/*

   This file is part of the KDE project, module kcmbackground.

   Copyright (C) 1999 Geert Jansen <g.t.jansen@stud.tue.nl>
   Copyright (C) 2003 Waldo Bastian <bastian@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
 */

#ifndef BGADVANCED_H
#define BGADVANCED_H

#include <tqdict.h>
#include <tqlistview.h>
#include <tqstringlist.h>

#include <kdialogbase.h>

class TQLineEdit;
class TQSpinBox;

class BGAdvancedBase;
class KBackgroundRenderer;
class KBackgroundProgram;

class BGAdvancedDialog : public KDialogBase
{
   Q_OBJECT
public:
   BGAdvancedDialog(KBackgroundRenderer *_r, TQWidget *parent, bool m_multidesktop);

   void setCacheSize(int s);
   int cacheSize();
   TQColor textColor();
   void setTextColor(const TQColor &color);
   TQColor textBackgroundColor();
   void setTextBackgroundColor(const TQColor &color);
   bool shadowEnabled();
   void setShadowEnabled(bool enabled);
   void setTextLines(int lines);
   int textLines() const;
   void setTextWidth(int width);
   int textWidth() const;

   void updateUI();

   void makeReadOnly();

   TQString backgroundProgram() const;
   int backgroundMode() const;

public slots:
   void slotAdd();
   void slotRemove();
   void slotModify();

protected:
   void addProgram(const TQString &name);
   void removeProgram(const TQString &name);
   void selectProgram(const TQString &name);

protected slots:
   void slotProgramItemClicked(TQListViewItem *item);
   void slotProgramItemDoubleClicked(TQListViewItem *item);
   void slotProgramChanged();
   void slotEnableProgram(bool b);

private:
   KBackgroundRenderer *r;

   BGAdvancedBase *dlg;

   TQWidget *m_pMonitor;
   TQDict<TQListViewItem> m_programItems;
   TQString m_selectedProgram;
   int m_oldBackgroundMode;
   int m_backgroundMode;
};

/**
 * Dialog to edit a background program.
 */
class KProgramEditDialog: public KDialogBase
{
    Q_OBJECT

public:
    KProgramEditDialog(const TQString &program=TQString::null, TQWidget *parent=0L,
	    char *name=0L);

    /** The program name is here in case the user changed it */
    TQString program()const;

public slots:
    void slotOk();

private:
    TQString m_Program;
    TQLineEdit *m_NameEdit, *m_CommentEdit;
    TQLineEdit *m_ExecEdit, *m_CommandEdit;
    TQLineEdit *m_PreviewEdit;
    TQSpinBox *m_RefreshEdit;
    KBackgroundProgram *m_Prog;
};


#endif

