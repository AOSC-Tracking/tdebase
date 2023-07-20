/***************************************************************************
                          sessioneditor.h  -  description
                             -------------------
    begin                : oct 28 2001
    copyright            : (C) 2001 by Stephan Binner
    email                : binner@kde.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef SESSIONEDITOR_H
#define SESSIONEDITOR_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <tqptrlist.h>
#include <tqstringlist.h>
#include <tdeapplication.h>
#include <tqwidget.h>

#include "sessiondialog.h"

class SessionEditor : public SessionDialog
{
  TQ_OBJECT 
  public:
    SessionEditor(TQWidget* parent=0, const char *name=0);
    ~SessionEditor();
 
    bool isModified() const { return sesMod; }
    void querySave();

  signals:
    void changed();
    void getList();

  public slots:
    void schemaListChanged(const TQStringList &titles, const TQStringList &filenames);

  private slots:
    void readSession(int);
    void saveCurrent();
    void removeCurrent();
    void sessionModified();

  private: 
    void show();
    void loadAllKeytab();
    void loadAllSession(TQString currentFile="");
    TQString readKeymapTitle(const TQString& filename);

    bool sesMod;
    int oldSession;
    bool loaded;
    TQPtrList<TQString> keytabFilename;
    TQPtrList<TQString> schemaFilename;
};

#endif
