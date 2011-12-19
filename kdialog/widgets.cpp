//
//  Copyright (C) 1998 Matthias Hoelzer <hoelzer@kde.org>
//  Copyright (C) 2002 David Faure <faure@kde.org>
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the7 implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//


#include <klocale.h>
#include <kmessagebox.h>

#include "widgets.h"
#include "klistboxdialog.h"
#include "progressdialog.h"
#include <kinputdialog.h>
#include <kpassdlg.h>
#include <kcombobox.h>
#include <kdebug.h>
#include <kapplication.h>

#include <tqlabel.h>
#include <ktextedit.h>
#include <tqvbox.h>
#include <tqfile.h>

#if defined Q_WS_X11 && ! defined K_WS_QTONLY
#include <netwm.h>
#endif

void Widgets::handleXGeometry(TQWidget * dlg)
{
#ifdef Q_WS_X11
    if ( ! kapp->geometryArgument().isEmpty()) {
	int x, y;
	int w, h;
	int m = XParseGeometry( kapp->geometryArgument().latin1(), &x, &y, (unsigned int*)&w, (unsigned int*)&h);
	if ( (m & XNegative) )
	    x = KApplication::desktop()->width()  + x - w;
	if ( (m & YNegative) )
	    y = KApplication::desktop()->height() + y - h;
	dlg->setGeometry(x, y, w, h);
	// kdDebug() << "x: " << x << "  y: " << y << "  w: " << w << "  h: " << h << endl;
    }
#endif
}

bool Widgets::inputBox(TQWidget *parent, const TQString& title, const TQString& text, const TQString& init, TQString &result)
{
  bool ok;
  TQString str = KInputDialog::text( title, text, init, &ok, parent, 0, 0, TQString::null );
  if ( ok )
    result = str;
  return ok;
}

bool Widgets::passwordBox(TQWidget *parent, const TQString& title, const TQString& text, TQCString &result)
{
  KPasswordDialog dlg( KPasswordDialog::Password, false, 0, parent );

  kapp->setTopWidget( &dlg );
  dlg.setCaption(title);
  dlg.setPrompt(text);

  handleXGeometry(&dlg);

  bool retcode = (dlg.exec() == TQDialog::Accepted);
  if ( retcode )
    result = dlg.password();
  return retcode;
}

int Widgets::textBox(TQWidget *parent, int width, int height, const TQString& title, const TQString& file)
{
//  KTextBox dlg(parent, 0, TRUE, width, height, file);
  KDialogBase dlg( parent, 0, true, title, KDialogBase::Ok, KDialogBase::Ok );

  kapp->setTopWidget( &dlg );
  KTextEdit *edit = new KTextEdit( dlg.makeVBoxMainWidget() );
  edit->setReadOnly(TRUE);

  TQFile f(file);
  if (!f.open(IO_ReadOnly))
  {
    kdError() << i18n("kdialog: could not open file ") << file << endl;
    return -1;
  }
  TQTextStream s(&f);

  while (!s.eof())
    edit->append(s.readLine());

  edit->moveCursor(TQTextEdit::MoveHome, false);

  f.close();

  if ( width > 0 && height > 0 )
      dlg.setInitialSize( TQSize( width, height ) );

  handleXGeometry(&dlg);
  dlg.setCaption(title);
  dlg.exec();
  return 0;
}

int Widgets::textInputBox(TQWidget *parent, int width, int height, const TQString& title, const TQStringList& args, TQCString &result)
{
//  KTextBox dlg(parent, 0, TRUE, width, height, file);
  KDialogBase dlg( parent, 0, true, title, KDialogBase::Ok, KDialogBase::Ok );

  kapp->setTopWidget( &dlg );
  TQVBox* vbox = dlg.makeVBoxMainWidget();

  if( args.count() > 0 )
  {
    TQLabel *label = new TQLabel(vbox);
    label->setText(args[0]);
  }

  KTextEdit *edit = new KTextEdit( vbox );
  edit->setReadOnly(FALSE);
  edit->setTextFormat( TQt::PlainText );
  edit->setFocus();

  if( args.count() > 1 )
    edit->setText( args[1] );

  if ( width > 0 && height > 0 )
    dlg.setInitialSize( TQSize( width, height ) );

  handleXGeometry(&dlg);
  dlg.setCaption(title);
  dlg.exec();
  result = edit->text().local8Bit();
  return 0;
}

bool Widgets::comboBox(TQWidget *parent, const TQString& title, const TQString& text, const TQStringList& args, 
		       const TQString& defaultEntry, TQString &result)
{
  KDialogBase dlg( parent, 0, true, title, KDialogBase::Ok|KDialogBase::Cancel,
                   KDialogBase::Ok );

  kapp->setTopWidget( &dlg );
  dlg.setCaption(title);
  TQVBox* vbox = dlg.makeVBoxMainWidget();

  TQLabel label (vbox);
  label.setText (text);
  KComboBox combo (vbox);
  combo.insertStringList (args);
  combo.setCurrentItem( defaultEntry, false );

  handleXGeometry(&dlg);

  bool retcode = (dlg.exec() == TQDialog::Accepted);

  if (retcode)
    result = combo.currentText();

  return retcode;
}

bool Widgets::listBox(TQWidget *parent, const TQString& title, const TQString& text, const TQStringList& args, 
		      const TQString& defaultEntry, TQString &result)
{
  KListBoxDialog box(text,parent);

  kapp->setTopWidget( &box );
  box.setCaption(title);

  for (unsigned int i = 0; i+1<args.count(); i += 2) {
    box.insertItem(args[i+1]);
  }
  box.setCurrentItem( defaultEntry );

  handleXGeometry(&box);

  bool retcode = (box.exec() == TQDialog::Accepted);
  if ( retcode )
    result = args[ box.currentItem()*2 ];
  return retcode;
}


bool Widgets::checkList(TQWidget *parent, const TQString& title, const TQString& text, const TQStringList& args, bool separateOutput, TQStringList &result)
{
  TQStringList entries, tags;
  TQString rs;

  result.clear();

  KListBoxDialog box(text,parent);

  TQListBox &table = box.getTable();

  kapp->setTopWidget( &box );
  box.setCaption(title);

  for (unsigned int i=0; i+2<args.count(); i += 3) {
    tags.append(args[i]);
    entries.append(args[i+1]);
  }

  table.insertStringList(entries);
  table.setMultiSelection(TRUE);
  table.setCurrentItem(0); // This is to circumvent a Qt bug

  for (unsigned int i=0; i+2<args.count(); i += 3) {
    table.setSelected( i/3, args[i+2] == TQString::fromLatin1("on") );
  }

  handleXGeometry(&box);

  bool retcode = (box.exec() == TQDialog::Accepted);

  if ( retcode ) {
    if (separateOutput) {
      for (unsigned int i=0; i<table.count(); i++)
        if (table.isSelected(i))
          result.append(tags[i]);
    } else {
      for (unsigned int i=0; i<table.count(); i++)
        if (table.isSelected(i))
          rs += TQString::fromLatin1("\"") + tags[i] + TQString::fromLatin1("\" ");
      result.append(rs);
    }
  }
  return retcode;
}


bool Widgets::radioBox(TQWidget *parent, const TQString& title, const TQString& text, const TQStringList& args, TQString &result)
{
  TQStringList entries, tags;

  KListBoxDialog box(text,parent);

  TQListBox &table = box.getTable();

  kapp->setTopWidget( &box );
  box.setCaption(title);

  for (unsigned int i=0; i+2<args.count(); i += 3) {
    tags.append(args[i]);
    entries.append(args[i+1]);
  }

  table.insertStringList(entries);

  for (unsigned int i=0; i+2<args.count(); i += 3) {
    table.setSelected( i/3, args[i+2] == TQString::fromLatin1("on") );
  }

  handleXGeometry(&box);

  bool retcode = (box.exec() == TQDialog::Accepted);
  if ( retcode )
    result = tags[ table.currentItem() ];
  return retcode;
}

bool Widgets::progressBar(TQWidget *parent, const TQString& title, const TQString& text, int totalSteps)
{
  ProgressDialog dlg( parent, title, text, totalSteps );
  kapp->setTopWidget( &dlg );
  dlg.setCaption( title );
  handleXGeometry(&dlg);
  dlg.exec();
  return dlg.wasCancelled();
}
