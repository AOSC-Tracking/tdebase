 /*
 *  This file is part of the Trinity Desktop Environment
 *
 *  Original file taken from the OpenSUSE kdebase builds
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

#include "module.h"

#include <assert.h>
#include <dcopclient.h>
#include <kapplication.h>
#include <kdebug.h>
#include <kfiledialog.h>
#include <kglobalsettings.h>
#include <klocale.h>
#include <krecentdocument.h>
#include <kwin.h>
#include <tqtimer.h>
#include <stdlib.h>
#include <unistd.h>
#include <kmessagebox.h>

#include <X11/Xutil.h>

extern "C"
{ 
    KDE_EXPORT KDEDModule *create_kdeintegration( const TQCString& obj )
        {
            return new KDEIntegration::Module( obj );
        }
};

namespace KDEIntegration
{

static void prepareDialog( TQWidget* w, long parent, const TQCString& wmclass1, const TQCString& wmclass2 )
    {
    XClassHint hints;
    hints.res_name = ( char* ) ( const char* ) wmclass1;
    hints.res_class = ( char* ) ( const char* ) wmclass2;
    XSetClassHint( qt_xdisplay(), w->winId(), &hints );
    KWin::setMainWindow( w, parent );
    KWin::setState( w->winId(), NET::Modal );
    KWin::WindowInfo info = KWin::windowInfo( parent, (unsigned long)NET::WMGeometry );
    if( info.valid())
        w->move( info.geometry().x() + ( info.geometry().width() - w->width())/2,
            info.geometry().y() + ( info.geometry().height()- w->height())/2 );
    }
    
// duped in qtkde
static TQString getHostname()
    {
    char hostname[ 256 ];
    if( gethostname( hostname, 255 ) == 0 )
        {
        hostname[ 255 ] = '\0';
        return hostname;
        }
    return "";
    }

bool Module::initializeIntegration( const TQString& hostname )
    {
    if( hostname != getHostname())
        return false;
    // multihead support in KDE is just a hack, it wouldn't work very well anyway
    if( KGlobalSettings::isMultiHead())
        return false;
    return true;
    }

void* Module::getOpenFileNames( const TQString& filter, TQString workingDirectory, long parent,
    const TQCString& name, const TQString& caption, TQString /*selectedFilter*/, bool multiple,
    const TQCString& wmclass1, const TQCString& wmclass2 )
    {
    KFileDialog* dlg = new KFileDialog( workingDirectory, filter, 0, name.isEmpty() ? "filedialog" : name, false);
    prepareDialog( dlg, parent, wmclass1, wmclass2 );
    dlg->setOperationMode( KFileDialog::Opening );
    dlg->setMode(( multiple ? KFile::Files : KFile::File ) | KFile::LocalOnly );
    dlg->setPlainCaption( caption.isNull() ? i18n("Open") : caption );
// TODO    dlg->ops->clearHistory();
    connect( dlg, TQT_SIGNAL( dialogDone( int )), TQT_SLOT( dialogDone( int )));
    dlg->show();
    return dlg;
    }

void* Module::getSaveFileName( const TQString& initialSelection, const TQString& filter,
    TQString workingDirectory, long parent, const TQCString& name, const TQString& caption, TQString /*selectedFilter*/,
    const TQCString& wmclass1, const TQCString& wmclass2 )
    {
    TQString initial = workingDirectory;
    if( !initialSelection.isEmpty())
        {
        if( initial.right( 1 ) != TQChar( '/' ))
            initial += '/';
        initial += initialSelection;
        }
    bool specialDir = initial.at(0) == ':';
    KFileDialog* dlg = new KFileDialog( specialDir ? initial : TQString(), filter, 0,
        name.isEmpty() ? "filedialog" : name, false);
    if ( !specialDir )
        dlg->setSelection( initial ); // may also be a filename
    prepareDialog( dlg, parent, wmclass1, wmclass2 );
    dlg->setOperationMode( KFileDialog::Saving );
    dlg->setPlainCaption( caption.isNull() ? i18n("Save As") : caption );
    connect( dlg, TQT_SIGNAL( dialogDone( int )), TQT_SLOT( dialogDone( int )));
    dlg->show();
    return dlg;
    }


void* Module::getExistingDirectory( const TQString& initialDirectory, long parent,
    const TQCString& name, const TQString& caption, const TQCString& wmclass1, const TQCString& wmclass2 )
    {
    KDirSelectDialog* dlg = new KDirSelectDialog( initialDirectory, true, 0,
        name.isEmpty() ? name : "kdirselect dialog", false );
    prepareDialog( dlg, parent, wmclass1, wmclass2 );
    dlg->setPlainCaption( caption.isNull() ? i18n( "Select Folder" ) : caption );
    connect( dlg, TQT_SIGNAL( dialogDone( int )), TQT_SLOT( dialogDone( int )));
    dlg->show();
    return dlg;
    }

void* Module::getColor( const TQColor& color, long parent, const TQCString& name,
    const TQCString& wmclass1, const TQCString& wmclass2 )
    {
    KColorDialog* dlg = new KColorDialog( NULL, name.isEmpty() ? name : "colordialog", true );
    dlg->setModal( false ); // KColorDialog creates its buttons depending on modality :(
    if( color.isValid())
        dlg->setColor( color );
    prepareDialog( dlg, parent, wmclass1, wmclass2 );
    dlg->setPlainCaption( i18n( "Select Color" ));
    connect( dlg, TQT_SIGNAL( dialogDone( int )), TQT_SLOT( dialogDone( int )));
    dlg->show();
    return dlg;
    }

void* Module::getFont( bool /*ok*/, const TQFont& def, long parent, const TQCString& name,
    const TQCString& wmclass1, const TQCString& wmclass2 )
    {
    KFontDialog* dlg = new KFontDialog( NULL, name.isEmpty() ? name : "Font Selector", false, false );
    dlg->setFont( def, false );
    prepareDialog( dlg, parent, wmclass1, wmclass2 );
    dlg->setPlainCaption( i18n( "Select Font" ));
    connect( dlg, TQT_SIGNAL( dialogDone( int )), TQT_SLOT( dialogDone( int )));
    dlg->show();
    return dlg;
    }

namespace
{
struct btns
    {
    int buttons[ 3 ];
    };
}
static TQMap< KDialogBase*, btns > msgbox1_buttons;

void* Module::messageBox1( int type, long parent, const TQString& caption, const TQString& text,
    int button0, int button1, int button2, const TQCString& wmclass1, const TQCString& wmclass2 )
    {
    static const char* const caps[ 4 ]
        = { I18N_NOOP( "Information" ), I18N_NOOP( "Question" ), I18N_NOOP( "Warning" ), I18N_NOOP( "Error" )};
    int buttons[ 3 ] = { button0 & TQMessageBox::ButtonMask,
        button1 & TQMessageBox::ButtonMask, button2 & TQMessageBox::ButtonMask };
    KGuiItem buttonItems[ 3 ];
    for( int i = 0;
         i < 3;
         ++i )
        switch( buttons[ i ] )
            {
            case TQMessageBox::Ok:
                buttonItems[ i ] = KStdGuiItem::ok();
              break;
            case TQMessageBox::Cancel:
                buttonItems[ i ] = KStdGuiItem::cancel();
              break;
            case TQMessageBox::Yes:
                buttonItems[ i ] = KStdGuiItem::yes();
              break;
            case TQMessageBox::No:
                buttonItems[ i ] = KStdGuiItem::no();
              break;
            case TQMessageBox::Abort:
                buttonItems[ i ] = KGuiItem( i18n( "&Abort" ));
              break;
            case TQMessageBox::Retry:
                buttonItems[ i ] = KGuiItem( "&Retry" );
              break;
            case TQMessageBox::Ignore:
                buttonItems[ i ] = KGuiItem( "&Ignore" );
              break;
            case TQMessageBox::YesAll:
                buttonItems[ i ] = KStdGuiItem::yes();
                buttonItems[ i ].setText( i18n( "Yes to &All" ));
              break;
            case TQMessageBox::NoAll:
                buttonItems[ i ] = KStdGuiItem::no();
                buttonItems[ i ].setText( i18n( "N&o to All" ));
              break;
            default:
              break;
            };
    KDialogBase::ButtonCode defaultButton = KDialogBase::NoDefault;
    if( button0 & TQMessageBox::Default )
        defaultButton = KDialogBase::Yes;
    else if( button1 & TQMessageBox::Default )
        defaultButton = KDialogBase::No;
    else if( button2 & TQMessageBox::Default )
        defaultButton = KDialogBase::Cancel;
    else // TODO KDialogBase's handling of NoDefault has strange focus effects
        defaultButton = KDialogBase::Yes;
    KDialogBase::ButtonCode escapeButton = KDialogBase::Cancel;
    if( button0 & TQMessageBox::Escape )
        escapeButton = KDialogBase::Yes;
    else if( button1 & TQMessageBox::Escape )
        escapeButton = KDialogBase::No;
    else if( button2 & TQMessageBox::Escape )
        escapeButton = KDialogBase::Cancel;
    KDialogBase *dialog= new KDialogBase(
                       caption.isEmpty() ? i18n( caps[ type ] ) : caption,
                       KDialogBase::Yes
                       | ( buttons[ 1 ] == TQMessageBox::NoButton ? 0 : int( KDialogBase::No ))
                       | ( buttons[ 2 ] == TQMessageBox::NoButton ? 0 : int( KDialogBase::Cancel )),
                       defaultButton, escapeButton,
                       NULL, "messageBox2", true, true,
                       buttonItems[ 0 ], buttonItems[ 1 ],buttonItems[ 2 ] );
    bool checkboxResult = false;
    KMessageBox::createKMessageBox(dialog, static_cast< TQMessageBox::Icon >( type ), text, TQStringList(),
                       TQString(),
                       &checkboxResult, KMessageBox::Notify | KMessageBox::NoExec);
    prepareDialog( dialog, parent, wmclass1, wmclass2 );
    dialog->setPlainCaption( caption );
    connect( dialog, TQT_SIGNAL( dialogDone( int )), TQT_SLOT( dialogDone( int )));
    btns b;
    b.buttons[ 0 ] = buttons[ 0 ];
    b.buttons[ 1 ] = buttons[ 1 ];
    b.buttons[ 2 ] = buttons[ 2 ];
    msgbox1_buttons[ dialog ] = b;
    dialog->show();
    return dialog;
    }

void* Module::messageBox2( int type, long parent, const TQString& caption, const TQString& text, const TQString& button0Text,
    const TQString& button1Text, const TQString& button2Text, int defaultButton, int escapeButton,
    const TQCString& wmclass1, const TQCString& wmclass2 )
    {
    static KDialogBase::ButtonCode map[ 4 ]
        = { KDialogBase::NoDefault, KDialogBase::Yes, KDialogBase::No, KDialogBase::Cancel };
    static const char* const caps[ 4 ]
        = { I18N_NOOP( "Information" ), I18N_NOOP( "Question" ), I18N_NOOP( "Warning" ), I18N_NOOP( "Error" )};
    KDialogBase *dialog= new KDialogBase(
                       caption.isEmpty() ? i18n( caps[ type ] ) : caption,
                       KDialogBase::Yes
                       | ( button1Text.isEmpty() ? 0 : int( KDialogBase::No ))
                       | ( button2Text.isEmpty() ? 0 : int( KDialogBase::Cancel )),
                       map[ defaultButton + 1 ], map[ escapeButton + 1 ],
                       NULL, "messageBox2", true, true,
                       button0Text.isEmpty() ? KStdGuiItem::ok() : KGuiItem( button0Text ), button1Text,button2Text);
    bool checkboxResult = false;
    KMessageBox::createKMessageBox(dialog, static_cast< TQMessageBox::Icon >( type ), text, TQStringList(),
                       TQString(),
                       &checkboxResult, KMessageBox::Notify | KMessageBox::NoExec);
    prepareDialog( dialog, parent, wmclass1, wmclass2 );
    dialog->setPlainCaption( caption );
    connect( dialog, TQT_SIGNAL( dialogDone( int )), TQT_SLOT( dialogDone( int )));
    dialog->show();
    return dialog;
    }

void Module::dialogDone( int result )
    {
    void* handle = (void*)sender(); // TODO?
    JobData job = jobs[ handle ];
    switch( job.type )
        {
        case JobData::GetOpenFileNames:
            {
            KFileDialog* dlg = static_cast< KFileDialog* >( handle );
            post_getOpenFileNames( dlg, result == TQDialog::Accepted ? dlg->selectedFiles() : TQStringList(),
                dlg->baseURL().path(), dlg->currentFilter());
            dlg->deleteLater();
          break;
            }
        case JobData::GetSaveFileName:
            {
            KFileDialog* dlg = static_cast< KFileDialog* >( handle );
            TQString filename = result == TQDialog::Accepted ? dlg->selectedFile() : TQString();
            if (!filename.isEmpty())
                KRecentDocument::add(filename);
            post_getSaveFileName( dlg, filename, dlg->baseURL().path(), dlg->currentFilter());
            dlg->deleteLater();
          break;
            }
        case JobData::GetExistingDirectory:
            {
            KDirSelectDialog* dlg = static_cast< KDirSelectDialog* >( handle );
            post_getExistingDirectory( dlg, result == TQDialog::Accepted ? dlg->url().path() : TQString());
            dlg->deleteLater();
          break;
            }
        case JobData::GetColor:
            {
            KColorDialog* dlg = static_cast< KColorDialog* >( handle );
            post_getColor( dlg, result == TQDialog::Accepted ? dlg->color() : TQColor());
            dlg->deleteLater();
          break;
            }
        case JobData::GetFont:
            {
            KFontDialog* dlg = static_cast< KFontDialog* >( handle );
            post_getFont( dlg, result == TQDialog::Accepted ? dlg->font() : TQFont(), result == TQDialog::Accepted );
            dlg->deleteLater();
          break;
            }
        case JobData::MessageBox1:
            {
            KDialogBase* dlg = static_cast< KDialogBase* >( handle );
            btns b = msgbox1_buttons[ dlg ];
            int res;
            if( result == KDialogBase::Cancel )
                res = b.buttons[ 2 ];
            else if( result == KDialogBase::Yes )
                res = b.buttons[ 0 ];
            else
                res = b.buttons[ 1 ];
            msgbox1_buttons.remove( dlg );
            post_messageBox1( dlg, res );
//            if (checkboxResult)
//                saveDontShowAgainYesNo(dontAskAgainName, res);
            dlg->deleteLater();
          break;
            }
        case JobData::MessageBox2:
            {
            KDialogBase* dlg = static_cast< KDialogBase* >( handle );
            int res;
            if( result == KDialogBase::Cancel )
                res = 2;
            else if( result == KDialogBase::Yes )
                res = 0;
            else if( result == KDialogBase::No )
                res = 1;
            else
                res = -1;
            post_messageBox2( dlg, res );
//            if (checkboxResult)
//                saveDontShowAgainYesNo(dontAskAgainName, res);
            dlg->deleteLater();
          break;
            }
        }
    }

Module::Module( const TQCString& obj )
    : KDEDModule( obj )
    {
    }

#include "module_functions.cpp"

} // namespace

#include "module.moc"
