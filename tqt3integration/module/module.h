 /*
 *  This file is part of the Trinity Desktop Environment
 *
 *  Original file taken from the OpenSUSE tdebase builds
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef _INTEGRATION_MODULE_H_
#define _INTEGRATION_MODULE_H_

#include <kcolordialog.h>
#include <kdedmodule.h>
#include <kdirselectdialog.h>
#include <tdefiledialog.h>
#include <tdefontdialog.h>
#include <kdialogbase.h>

class DCOPClientTransaction;

namespace TDEIntegration
{

class Module
    : public KDEDModule
    {
    Q_OBJECT
  
    public:
        Module( const TQCString& obj );
        // DCOP
        virtual bool process(const TQCString &fun, const TQByteArray &data,
            TQCString &replyType, TQByteArray &replyData);
        virtual QCStringList functions();
        virtual QCStringList interfaces();
    private slots:
        void dialogDone( int result );
    private:
        struct JobData
            {
            DCOPClientTransaction* transaction;
            enum
                {
                GetOpenFileNames,
                GetSaveFileName,
                GetExistingDirectory,
                GetColor,
                GetFont,
                MessageBox1,
                MessageBox2
                } type;
            };
        TQMap< void*, JobData > jobs;
#include "module_functions.h"
    };

class KFileDialog
    : public ::KFileDialog
    {
    Q_OBJECT
  
    public:
        KFileDialog(const TQString& startDir, const TQString& filter,
            TQWidget *parent, const char *name, bool modal)
            : ::KFileDialog( startDir, filter, parent, name, modal )
            {}
    signals:
        void dialogDone( int result );
    protected:
        virtual void done( int r ) { ::KFileDialog::done( r ); emit dialogDone( r ); }
    };


class KDirSelectDialog
    : public ::KDirSelectDialog
    {
    Q_OBJECT
  
    public:
        KDirSelectDialog(const TQString& startDir, bool localOnly,
            TQWidget *parent, const char *name, bool modal)
            : ::KDirSelectDialog( startDir, localOnly, parent, name, modal )
            {}
    signals:
        void dialogDone( int result );
    protected:
        virtual void done( int r ) { ::KDirSelectDialog::done( r ); emit dialogDone( r ); }
    };


class KColorDialog
    : public ::KColorDialog
    {
    Q_OBJECT
  
    public:
        KColorDialog( TQWidget *parent, const char *name, bool modal )
            : ::KColorDialog( parent, name, modal )
            {}
    signals:
        void dialogDone( int result );
    protected:
        virtual void done( int r ) { ::KColorDialog::done( r ); emit dialogDone( r ); } // hmm?
    };

class TDEFontDialog
    : public ::TDEFontDialog
    {
    Q_OBJECT
  
    public:
        TDEFontDialog( TQWidget *parent, const char *name, bool onlyFixed, bool modal,
            const TQStringList &fontlist = TQStringList(), bool makeFrame = true,
            bool diff = false, TQButton::ToggleState *sizeIsRelativeState = 0L )
            : ::TDEFontDialog( parent, name, onlyFixed, modal, fontlist, makeFrame, diff, sizeIsRelativeState )
            {}
    signals:
        void dialogDone( int result );
    protected:
        virtual void done( int r ) { ::TDEFontDialog::done( r ); emit dialogDone( r ); }
    };

class KDialogBase
    : public ::KDialogBase
    {
    Q_OBJECT
  
    public:
        KDialogBase( const TQString &caption, int buttonMask=Yes|No|Cancel,
		 ButtonCode defaultButton=Yes, ButtonCode escapeButton=Cancel,
		 TQWidget *parent=0, const char *name=0,
		 bool modal=true, bool separator=false,
		 const KGuiItem &yes = KStdGuiItem::yes(), // i18n("&Yes")
		 const KGuiItem &no = KStdGuiItem::no(), // i18n("&No"),
		 const KGuiItem &cancel = KStdGuiItem::cancel()) // i18n("&Cancel")
                 : ::KDialogBase( caption, buttonMask, defaultButton, escapeButton, parent, name, modal, separator,
                     yes, no, cancel )
                {}
    signals:
        void dialogDone( int result );
    protected:
        virtual void done( int r ) { ::KDialogBase::done( r ); emit dialogDone( r ); }
    };

    
} // namespace

#endif
