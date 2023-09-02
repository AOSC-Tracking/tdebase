/*  Copyright (C) 2003 Lukas Tinkl <lukas@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <tqlabel.h>
#include <tqlayout.h>
#include <tqpixmap.h>
#include <tqpushbutton.h>
#include <tqtoolbutton.h>
#include <tqtooltip.h>

#include <tdeaboutdata.h>
#include <tdeapplication.h>
#include <kdebug.h>
#include <kdialog.h>
#include <tdeemailsettings.h>
#include <tdefiledialog.h>
#include <tdeglobal.h>
#include <kiconloader.h>
#include <tdelistview.h>
#include <tdelocale.h>
#include <tdemessagebox.h>
#include <kservicegroup.h>
#include <kstandarddirs.h>
#include <kurldrag.h>

#include "kthememanager.h"
#include "knewthemedlg.h"
#include "config.h"

kthememanager::kthememanager( TQWidget *parent, const char *name )
    : TDECModule( parent, name ), m_theme( 0 ), m_origTheme( 0 )
{

    TDEAboutData *about = new TDEAboutData("kthememanager", I18N_NOOP("TDE Theme Manager"),
                                       "0.4", I18N_NOOP("This control module handles installing, removing and "
                                                        "creating visual TDE themes."),
                                       TDEAboutData::License_GPL, "(c) 2003, 2004, 2006 Lukáš Tinkl", 0);
    setAboutData( about );

    setQuickHelp( i18n("This control module handles installing, removing and "
                "creating visual TDE themes."));

    setButtons( TDECModule::Default|TDECModule::Apply|TDECModule::Help );

    setAcceptDrops( true );
    init();

    TQBoxLayout *top = new TQVBoxLayout(this, 0, KDialog::spacingHint());

    dlg = new KThemeDlg(this);
    top->addWidget( dlg );

    dlg->lvThemes->setColumnWidthMode( 0, TQListView::Maximum );

    connect( ( TQObject * )dlg->btnInstall, TQT_SIGNAL( clicked() ),
             this, TQT_SLOT( slotInstallTheme() ) );

    connect( ( TQObject * )dlg->btnRemove, TQT_SIGNAL( clicked() ),
             this, TQT_SLOT( slotRemoveTheme() ) );

    connect( ( TQObject * )dlg->btnCreate, TQT_SIGNAL( clicked() ),
             this, TQT_SLOT( slotCreateTheme() ) );

    connect( ( TQObject * )dlg->lvThemes, TQT_SIGNAL( clicked( TQListViewItem * ) ),
             this, TQT_SLOT( slotThemeChanged( TQListViewItem * ) ) );

    connect( ( TQObject * )dlg->lvThemes, TQT_SIGNAL( currentChanged( TQListViewItem * ) ),
             this, TQT_SLOT( slotThemeChanged( TQListViewItem * ) ) );

    connect( this, TQT_SIGNAL( filesDropped( const KURL::List& ) ),
             this, TQT_SLOT( updateButton() ) );

    connect( ( TQObject * )dlg->lvThemes, TQT_SIGNAL( clicked( TQListViewItem * ) ),
             this, TQT_SLOT( updateButton() ) );

    m_origTheme = new KTheme( this, true ); // stores the defaults to get back to
    m_origTheme->setName( ORIGINAL_THEME );
    m_origTheme->createYourself();

    load();
    queryLNFModules();
    updateButton();
}

kthememanager::~kthememanager()
{
    delete m_theme;
    delete m_origTheme;
}

void kthememanager::init()
{
    TDEGlobal::dirs()->addResourceType( "themes", TDEStandardDirs::kde_default("data") +
                                      "kthememanager/themes/" );
}

void kthememanager::updateButton()
{
    TQListViewItem * cur = dlg->lvThemes->currentItem();
    bool enable = (cur != 0);
    if (enable) {
        enable = TQFile(TDEGlobal::dirs()->saveLocation( "themes",  cur->text( 0 ) + "/"+ cur->text( 0 )+ ".xml" ,false)).exists() ;
    }
    dlg->btnRemove->setEnabled(enable);
}

void kthememanager::load()
{
	load( false );
}

void kthememanager::load(bool useDefaults)
{
    listThemes();

    // Load the current theme name
    TDEConfig conf("kcmthememanagerrc", false, false);

	 conf.setReadDefaults( useDefaults );

    conf.setGroup( "General" );
    TQString themeName = conf.readEntry( "CurrentTheme" );
    TQListViewItem * cur =  dlg->lvThemes->findItem( themeName, 0 );
    if ( cur )
    {
        dlg->lvThemes->setSelected( cur, true );
        dlg->lvThemes->ensureItemVisible( cur );
        slotThemeChanged( cur );
    }

	 emit changed( true );
}

void kthememanager::defaults()
{
	load( true );
}

void kthememanager::save()
{
    TQListViewItem * cur = dlg->lvThemes->currentItem();

    if ( cur )
    {
        TQString themeName = cur->text( 0 );

        m_theme = new KTheme( this, TDEGlobal::dirs()->findResource( "themes", themeName + "/" + themeName + ".xml") );
        m_theme->apply();

        // Save the current theme name
        TDEConfig conf("kcmthememanagerrc", false, false);
        conf.setGroup( "General" );
        conf.writeEntry( "CurrentTheme", themeName );
        conf.sync();

        delete m_theme;
        m_theme = 0;

    }
}

void kthememanager::listThemes()
{
    dlg->lvThemes->clear();
    dlg->lbPreview->setPixmap( TQPixmap() );

    TQStringList themes = TDEGlobal::dirs()->findAllResources( "themes", "*.xml", true /*recursive*/ );

    TQStringList::const_iterator it;

    for ( it = themes.begin(); it != themes.end(); ++it )
    {
        KTheme theme( this, ( *it ) );
        TQString name = theme.name();
        if ( name != ORIGINAL_THEME ) // skip the "original" theme
            ( void ) new TQListViewItem( dlg->lvThemes, name, theme.comment() );
    }

    kdDebug() << "Available themes: " << themes << endl;
}

float kthememanager::getThemeVersion( const TQString & themeName )
{
    TQStringList themes = TDEGlobal::dirs()->findAllResources( "themes", "*.xml", true /*recursive*/ );

    TQStringList::const_iterator it;

    for ( it = themes.begin(); it != themes.end(); ++it )
    {
        KTheme theme( 0L, ( *it ) );
        TQString name = theme.name();
        bool ok = false;
        float version = theme.version().toFloat( &ok );
        if ( name == themeName && ok )
            return version;
    }

    return -1;
}

void kthememanager::slotInstallTheme()
{
    addNewTheme( KFileDialog::getOpenURL( ":themes", "*.kth|" + i18n("Theme Files"), this,
                                          i18n( "Select Theme File" ) ) );
}

void kthememanager::addNewTheme( const KURL & url )
{
    if ( url.isValid() )
    {
        TQString themeName = TQFileInfo( url.fileName() ).baseName();
        if ( getThemeVersion( themeName ) != -1 ) // theme exists already
        {
            KTheme::remove( themeName  ); // remove first
        }

        m_theme = new KTheme(this);
        if ( m_theme->load( url ) )
        {
            listThemes();
            emit changed( true );
        }

        delete m_theme;
        m_theme = 0;
        updateButton();
    }
}

void kthememanager::slotRemoveTheme()
{
    // get the selected item from the listview
    TQListViewItem * cur = dlg->lvThemes->currentItem();
    // ask and remove it
    if ( cur )
    {
        TQString themeName = cur->text( 0 );
        if ( KMessageBox::warningContinueCancel( this, "<qt>" + i18n( "Do you really want to remove the theme <b>%1</b>?" ).arg( themeName ),
                                                 i18n( "Remove Theme" ), KGuiItem( i18n( "&Remove" ), "edit-delete" ) )
             == KMessageBox::Continue )
        {
            KTheme::remove( themeName );
            listThemes();
        }
        updateButton();
    }
}

bool kthememanager::themeExist(const TQString &_themeName)
{
    return ( dlg->lvThemes->findItem( _themeName, 0 )!=0 );
}

void kthememanager::slotCreateTheme()
{
    KNewThemeDlg dlg( this );

    KEMailSettings es;
    es.setProfile( es.defaultProfileName() );

    dlg.setName( i18n( "My Theme" ) );
    dlg.setAuthor( es.getSetting( KEMailSettings::RealName ) ) ;
    dlg.setEmail( es.getSetting( KEMailSettings::EmailAddress ) );
    dlg.setVersion( "0.1" );

    if ( dlg.exec() == TQDialog::Accepted )
    {

        TQString themeName = dlg.getName();
        if ( themeExist(themeName) )
        {
            KMessageBox::information( this, i18n( "Theme %1 already exists." ).arg( themeName ) );
        }
        else
        {
            if ( getThemeVersion( themeName ) != -1 ) // remove the installed theme first
            {
                KTheme::remove( themeName );
            }
            m_theme = new KTheme( this, true );
            m_theme->setName( dlg.getName() );
            m_theme->setAuthor( dlg.getAuthor() );
            m_theme->setEmail( dlg.getEmail() );
            m_theme->setHomepage( dlg.getHomepage() );
            m_theme->setComment( dlg.getComment().replace( "\n", "" ) );
            m_theme->setVersion( dlg.getVersion() );

            TQString result = m_theme->createYourself( true );
	    m_theme->addPreview();

            if ( !result.isEmpty() )
                KMessageBox::information( this, i18n( "Your theme has been successfully created in %1." ).arg( result ),
                                          i18n( "Theme Created" ), "theme_created_ok" );
            else
                KMessageBox::error( this, i18n( "An error occurred while creating your theme." ),
                                    i18n( "Theme Not Created" ) );
            delete m_theme;
            m_theme = 0;

            listThemes();
        }
    }
}

void kthememanager::slotThemeChanged( TQListViewItem * item )
{
    if ( item )
    {
        TQString themeName = item->text(0);
        kdDebug() << "Activated theme: " << themeName  << endl;

        TQString themeDir = TDEGlobal::dirs()->findResourceDir( "themes", themeName + "/" + themeName + ".xml") + themeName + "/";

        TQString pixFile = themeDir + themeName + ".preview.png";

        if ( TQFile::exists( pixFile ) )
        {
            updatePreview( pixFile );
        }
        else
        {
            dlg->lbPreview->setPixmap( TQPixmap() );
            dlg->lbPreview->setText( i18n( "This theme does not contain a preview." ) );
        }

        KTheme theme( this, themeDir + themeName + ".xml" );
        TQToolTip::remove( dlg->lbPreview );
        TQToolTip::add( dlg->lbPreview, "<qt>" + i18n( "Author: %1<br>Email: %2<br>Version: %3<br>Homepage: %4" )
                       .arg( theme.author() ).arg( theme.email() )
                       .arg( theme.version() ).arg( theme.homepage() ) + "</qt>");

        emit changed( true );
    }
}

void kthememanager::dragEnterEvent( TQDragEnterEvent * ev )
{
    ev->accept( KURLDrag::canDecode( ev ) );
}

void kthememanager::dropEvent( TQDropEvent * ev )
{
    KURL::List urls;
    if ( KURLDrag::decode( ev, urls ) )
    {
        emit filesDropped( urls );
    }
}

void kthememanager::slotFilesDropped( const KURL::List & urls )
{
    for ( KURL::List::ConstIterator it = urls.begin(); it != urls.end(); ++it )
        addNewTheme( *it );
}

void kthememanager::queryLNFModules()
{
    /*KServiceGroup::Ptr settings = KServiceGroup::group( "Settings/LookNFeel/" );
    if ( !settings || !settings->isValid() )
        return;

    KServiceGroup::List list = settings->entries();

    // Iterate over all entries in the group
    for( KServiceGroup::List::ConstIterator it = list.begin();
         it != list.end(); it++ )
    {
        KSycocaEntry *p = ( *it );
        if ( p->isType( KST_KService ) )
        {
            KService *s = static_cast<KService *>( p );
            ( void ) new KThemeDetailsItem( dlg->lvDetails, s->name(), s->pixmap( TDEIcon::Desktop ), s->exec() );
        }
    }

    dlg->lvDetails->sort();*/

    // For now use a static list
    TDEIconLoader * il = TDEGlobal::iconLoader();
    dlg->btnBackground->setIconSet( il->loadIconSet( "background", TDEIcon::Desktop, 32 ) );
    dlg->btnColors->setIconSet( il->loadIconSet( "application-x-kcsrc", TDEIcon::Desktop, 32 ) );
    dlg->btnStyle->setIconSet( il->loadIconSet( "style", TDEIcon::Desktop, 32 ) );
    dlg->btnIcons->setIconSet( il->loadIconSet( "icons", TDEIcon::Desktop, 32 ) );
    dlg->btnFonts->setIconSet( il->loadIconSet( "fonts", TDEIcon::Desktop, 32 ) );
    dlg->btnSaver->setIconSet( il->loadIconSet( "tdescreensaver", TDEIcon::Desktop, 32 ) );
}

void kthememanager::updatePreview( const TQString & pixFile )
{
     kdDebug() << "Preview is in file: " << pixFile << endl;
     TQImage preview( pixFile, "PNG" );
     if (preview.width()>dlg->lbPreview->contentsRect().width() ||
         preview.height()>dlg->lbPreview->contentsRect().height() )
         preview = preview.smoothScale( dlg->lbPreview->contentsRect().size(), TQImage::ScaleMin );
     TQPixmap pix;
     pix.convertFromImage( preview );
     dlg->lbPreview->setPixmap( pix );
}

extern "C"
{
    KDE_EXPORT TDECModule *create_kthememanager(TQWidget *parent, const char *)
    {
        TDEGlobal::locale()->insertCatalogue( "kthememanager" );
        return new kthememanager( parent, "kthememanager" );
    }
}

#include "kthememanager.moc"
