/*****************************************************************

Copyright (c) 1996-2000 the kicker authors. See file AUTHORS.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

******************************************************************/

#include <tqlabel.h>
#include <tqlayout.h>
#include <tqvbox.h>

#include <tdeglobal.h>
#include <tdelocale.h>
#include <kicondialog.h>
#include <tdefiledialog.h>
#include <klineedit.h>
#include <tdemessagebox.h>

#include "browser_dlg.h"
#include "browser_dlg.moc"

PanelBrowserDialog::PanelBrowserDialog( const TQString& path, const TQString &icon, TQWidget *parent, const char *name )
    :  KDialogBase( parent, name, true, i18n( "Quick Browser Configuration" ), Ok|Cancel, Ok, true )
{
    setMinimumWidth( 300 );

    TQVBox *page = makeVBoxMainWidget();

    TQHBox *hbox2 = new TQHBox( page );
    hbox2->setSpacing( KDialog::spacingHint() );
    TQLabel *label1 = new TQLabel( i18n( "Button icon:" ), hbox2 );

    iconBtn = new TDEIconButton( hbox2 );
    iconBtn->setFixedSize( 50, 50 );
    iconBtn->setIconType( TDEIcon::Panel, TDEIcon::Place );
    label1->setBuddy( iconBtn );

    TQHBox *hbox1 = new TQHBox( page );
    hbox1->setSpacing( KDialog::spacingHint() );
    TQLabel *label2 = new TQLabel( i18n ( "Path:" ), hbox1 );
    pathInput = new KLineEdit( hbox1 );
    connect( pathInput, TQT_SIGNAL( textChanged ( const TQString & )), this, TQT_SLOT( slotPathChanged( const TQString & )));

    pathInput->setText( path );
    pathInput->setFocus();
    label2->setBuddy( pathInput );
    browseBtn = new TQPushButton( i18n( "&Browse…" ), hbox1 );
    if ( icon.isEmpty() ) {
        KURL u;
        u.setPath( path );
        iconBtn->setIcon( KMimeType::iconForURL( u ) );
    }
    else
        iconBtn->setIcon( icon );

    connect( browseBtn, TQT_SIGNAL( clicked() ), this, TQT_SLOT( browse() ) );
}

PanelBrowserDialog::~PanelBrowserDialog()
{

}

void PanelBrowserDialog::slotPathChanged( const TQString &_text )
{
    enableButtonOK( !_text.isEmpty() );
}

void PanelBrowserDialog::browse()
{
    TQString dir = KFileDialog::getExistingDirectory( pathInput->text(), 0, i18n( "Select Folder" ) );
    if ( !dir.isEmpty() ) {
        pathInput->setText( dir );
        KURL u;
        u.setPath( dir );
        iconBtn->setIcon( KMimeType::iconForURL( u ) );
    }
}

void PanelBrowserDialog::slotOk()
{
    TQDir dir(path());
    if( !dir.exists() ) {
        KMessageBox::sorry( this, i18n("'%1' is not a valid folder.").arg(path()) );
        return;
    }
    KDialogBase::slotOk();
}

const TQString PanelBrowserDialog::icon()
{
    return iconBtn->icon();
}

TQString PanelBrowserDialog::path()
{
    return pathInput->text();
}
