/***************************************************************************
 *   Copyright Ravikiran Rajagopal 2003                                    *
 *   ravi@ee.eng.ohio-state.edu                                            *
 *   Copyright (c) 1998 Stefan Taferner <taferner@kde.org>                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License (version 2) as   *
 *   published by the Free Software Foundation.                            *
 *                                                                         *
 ***************************************************************************/
#ifndef SPLASHINSTALLER_H
#define SPLASHINSTALLER_H

#include <tqmap.h>
#include <tqpoint.h>

#include <tdelistbox.h>
#include <kurl.h>

class TQLabel;
class TQTextEdit;
class TQPushButton;
class ThemeListBox;

class SplashInstaller : public TQWidget
{
  TQ_OBJECT
public:
  SplashInstaller(TQWidget *parent=0, const char *aName=0, bool aInit=false);
  ~SplashInstaller();

  virtual void load();
  virtual void load( bool useDefaults );
  virtual void save();
  virtual void defaults();

signals:
  void changed( bool state );

protected slots:
  virtual void slotAdd();
  virtual void slotRemove();
  virtual void slotTest();
  virtual void slotSetTheme(int);
  void slotFilesDropped(const KURL::List &urls);

protected:
  /** Scan Themes directory for available theme packages */
  virtual void readThemesList();
  /** add a theme to the list, returns the list index */
  int addTheme(const TQString &path, const TQString &name);
  void addNewTheme(const KURL &srcURL);
  int findTheme( const TQString &theme );

private:
  bool mGui;
  ThemeListBox *mThemesList;
  TQPushButton *mBtnAdd, *mBtnRemove, *mBtnTest;
  TQTextEdit *mText;
  TQLabel *mPreview;
};

class ThemeListBox: public TDEListBox
{
  TQ_OBJECT
public:
  ThemeListBox(TQWidget *parent);
  TQMap<TQString, TQString> text2path;

signals:
  void filesDropped(const KURL::List &urls);

protected:
  void dragEnterEvent(TQDragEnterEvent* event);
  void dropEvent(TQDropEvent* event);
  void mouseMoveEvent(TQMouseEvent *e);

protected slots:
  void slotMouseButtonPressed(int button, TQListBoxItem *item, const TQPoint &p);

private:
  TQString mDragFile;
  TQPoint mOldPos;

};

#endif
