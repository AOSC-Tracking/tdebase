/* This file is part of the KDE project
   Copyright (C) 1998, 1999 David Faure <faure@kde.org>
                 2003       Sven Leiber <s.leiber@web.de>

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

#include <tqdir.h>

#include <kdebug.h>
#include <kdesktopfile.h>
#include <kdirwatch.h>
#include <kinstance.h>
#include <kinputdialog.h>
#include <tdelocale.h>
#include <tdemessagebox.h>
#include <kstandarddirs.h>
#include <kprotocolinfo.h>
#include <tdepopupmenu.h>
#include <krun.h>

#include <tdeio/job.h>
#include <tdeio/renamedlg.h>

#include <kpropertiesdialog.h>
#include "konq_operations.h"
#include "konq_undo.h"
#include "knewmenu.h"
#include <utime.h>

// For KURLDesktopFileDlg
#include <tqlayout.h>
#include <tqhbox.h>
#include <klineedit.h>
#include <kurlrequester.h>
#include <tqlabel.h>
#include <tqpopupmenu.h>

TQValueList<KNewMenu::Entry> * KNewMenu::s_templatesList = 0L;
int KNewMenu::s_templatesVersion = 0;
bool KNewMenu::s_filesParsed = false;
KDirWatch * KNewMenu::s_pDirWatch = 0L;

class KNewMenu::KNewMenuPrivate
{
public:
    KNewMenuPrivate() : m_parentWidget(0) {}
    TDEActionCollection * m_actionCollection;
    TQString m_destPath;
    TQWidget *m_parentWidget;
    TDEActionMenu *m_menuDev;
};

KNewMenu::KNewMenu( TDEActionCollection * _collec, const char *name ) :
  TDEActionMenu( i18n( "Create New" ), "document-new", _collec, name ),
  menuItemsVersion( 0 )
{
    //kdDebug(1203) << "KNewMenu::KNewMenu " << this << endl;
    // Don't fill the menu yet
    // We'll do that in slotCheckUpToDate (should be connected to abouttoshow)
    d = new KNewMenuPrivate;
    d->m_actionCollection = _collec;
    makeMenus();
}

KNewMenu::KNewMenu( TDEActionCollection * _collec, TQWidget *parentWidget, const char *name ) :
  TDEActionMenu( i18n( "Create New" ), "document-new", _collec, name ),
  menuItemsVersion( 0 )
{
    d = new KNewMenuPrivate;
    d->m_actionCollection = _collec;
    d->m_parentWidget = parentWidget;
    makeMenus();
}

KNewMenu::~KNewMenu()
{
    //kdDebug(1203) << "KNewMenu::~KNewMenu " << this << endl;
    delete d;
}

void KNewMenu::makeMenus()
{
    d->m_menuDev = new TDEActionMenu( i18n( "Link to Device" ), "kcmdevices", d->m_actionCollection, "devnew" );
}

void KNewMenu::slotCheckUpToDate( )
{
    //kdDebug(1203) << "KNewMenu::slotCheckUpToDate() " << this
    //              << " : menuItemsVersion=" << menuItemsVersion
    //              << " s_templatesVersion=" << s_templatesVersion << endl;
    if (menuItemsVersion < s_templatesVersion || s_templatesVersion == 0)
    {
        //kdDebug(1203) << "KNewMenu::slotCheckUpToDate() : recreating actions" << endl;
        // We need to clean up the action collection
        // We look for our actions using the group
        TQValueList<TDEAction*> actions = d->m_actionCollection->actions( "KNewMenu" );
        for( TQValueListIterator<TDEAction*> it = actions.begin(); it != actions.end(); ++it )
        {
            remove( *it );
            d->m_actionCollection->remove( *it );
        }

        if (!s_templatesList) { // No templates list up to now
            s_templatesList = new TQValueList<Entry>();
            slotFillTemplates();
            parseFiles();
        }

        // This might have been already done for other popupmenus,
        // that's the point in s_filesParsed.
        if ( !s_filesParsed )
            parseFiles();

        fillMenu();

        menuItemsVersion = s_templatesVersion;
    }
}

void KNewMenu::parseFiles()
{
    //kdDebug(1203) << "KNewMenu::parseFiles()" << endl;
    s_filesParsed = true;
    TQValueList<Entry>::Iterator templ = s_templatesList->begin();
    for ( /*++templ*/; templ != s_templatesList->end(); ++templ)
    {
        TQString iconname;
        TQString filePath = (*templ).filePath;
        if ( !filePath.isEmpty() )
        {
            TQString text;
            TQString templatePath;
            // If a desktop file, then read the name from it.
            // Otherwise (or if no name in it?) use file name
            if ( KDesktopFile::isDesktopFile( filePath ) ) {
                KSimpleConfig config( filePath, true );
                config.setDesktopGroup();
                text = config.readEntry("Name");
                (*templ).icon = config.readEntry("Icon");
                (*templ).comment = config.readEntry("Comment");
                TQString type = config.readEntry( "Type" );
                if ( type == "Link" )
                {
                    templatePath = config.readPathEntry("URL");
                    if ( templatePath[0] != '/' )
                    {
                        if ( templatePath.startsWith("file:/") )
                            templatePath = KURL(templatePath).path();
                        else
                        {
                            // A relative path, then (that's the default in the files we ship)
                            TQString linkDir = filePath.left( filePath.findRev( '/' ) + 1 /*keep / */ );
                            //kdDebug(1203) << "linkDir=" << linkDir << endl;
                            templatePath = linkDir + templatePath;
                        }
                    }
                }
                if ( templatePath.isEmpty() )
                {
                    // No dest, this is an old-style template
                    (*templ).entryType = TEMPLATE;
                    (*templ).templatePath = (*templ).filePath; // we'll copy the file
                } else {
                    (*templ).entryType = LINKTOTEMPLATE;
                    (*templ).templatePath = templatePath;
                }

            }
            if (text.isEmpty())
            {
                text = KURL(filePath).fileName();
                if ( text.endsWith(".desktop") )
                    text.truncate( text.length() - 8 );
                else if ( text.endsWith(".kdelnk") )
                    text.truncate( text.length() - 7 );
            }
            (*templ).text = text;
            /*kdDebug(1203) << "Updating entry with text=" << text
                          << " entryType=" << (*templ).entryType
                          << " templatePath=" << (*templ).templatePath << endl;*/
        }
        else {
            (*templ).entryType = SEPARATOR;
        }
    }
}

void KNewMenu::fillMenu()
{
    //kdDebug(1203) << "KNewMenu::fillMenu()" << endl;
    popupMenu()->clear();
    d->m_menuDev->popupMenu()->clear();

    TDEAction *linkURL = 0, *linkApp = 0;  // these shall be put at special positions

    int i = 1; // was 2 when there was Folder
    TQValueList<Entry>::Iterator templ = s_templatesList->begin();
    for ( ; templ != s_templatesList->end(); ++templ, ++i)
    {
        if ( (*templ).entryType != SEPARATOR )
        {
            // There might be a .desktop for that one already, if it's a kdelnk
            // This assumes we read .desktop files before .kdelnk files ...

            // In fact, we skip any second item that has the same text as another one.
            // Duplicates in a menu look bad in any case.

            bool bSkip = false;

            TQValueList<TDEAction*> actions = d->m_actionCollection->actions();
            TQValueListIterator<TDEAction*> it = actions.begin();
            for( ; it != actions.end() && !bSkip; ++it )
            {
                if ( (*it)->text() == (*templ).text )
                {
                    kdDebug(1203) << "KNewMenu: skipping " << (*templ).filePath << endl;
                    bSkip = true;
                }
            }

            if ( !bSkip )
            {
                Entry entry = *(s_templatesList->at( i-1 ));

                // The best way to identify the "Create Directory", "Link to Location", "Link to Application" was the template
                if ( (*templ).templatePath.endsWith( "emptydir" ) )
                {
                    TDEAction * act = new TDEAction( (*templ).text, (*templ).icon, 0, this, TQT_SLOT( slotNewDir() ),
                                     d->m_actionCollection, TQCString().sprintf("newmenu%d", i ) );
                    act->setGroup( "KNewMenu" );
                    act->plug( popupMenu() );

                    TDEActionSeparator *sep = new TDEActionSeparator();
                    sep->plug( popupMenu() );
                }
                else
                {
                    TDEAction * act = new TDEAction( (*templ).text, (*templ).icon, 0, this, TQT_SLOT( slotNewFile() ),
                                             d->m_actionCollection, TQCString().sprintf("newmenu%d", i ) );
                    act->setGroup( "KNewMenu" );

                    if ( (*templ).templatePath.endsWith( "URL.desktop" ) )
                    {
                        linkURL = act;
                    }
                    else if ( (*templ).templatePath.endsWith( "Program.desktop" ) )
                    {
                        linkApp = act;
                    }
                    else if ( KDesktopFile::isDesktopFile( entry.templatePath ) )
                    {
                        KDesktopFile df( entry.templatePath );
                        if(df.readType() == "FSDevice")
                            act->plug( d->m_menuDev->popupMenu() );
                        else
                          act->plug( popupMenu() );
                    }
                    else
                    {
                        act->plug( popupMenu() );
                    }
                }
            }
        } else { // Separate system from personal templates
            Q_ASSERT( (*templ).entryType != 0 );

            TDEActionSeparator * act = new TDEActionSeparator();
            act->plug( popupMenu() );
        }
    }

    TDEActionSeparator * act = new TDEActionSeparator();
    act->plug( popupMenu() );
    if ( linkURL ) linkURL->plug( popupMenu() );
    if ( linkApp ) linkApp->plug( popupMenu() );
    d->m_menuDev->plug( popupMenu() );
}

void KNewMenu::slotFillTemplates()
{
    //kdDebug(1203) << "KNewMenu::slotFillTemplates()" << endl;
    // Ensure any changes in the templates dir will call this
    if ( ! s_pDirWatch )
    {
        s_pDirWatch = new KDirWatch;
        TQStringList dirs = d->m_actionCollection->instance()->dirs()->resourceDirs("templates");
        for ( TQStringList::Iterator it = dirs.begin() ; it != dirs.end() ; ++it )
        {
            //kdDebug(1203) << "Templates resource dir: " << *it << endl;
            s_pDirWatch->addDir( *it );
        }
        connect ( s_pDirWatch, TQT_SIGNAL( dirty( const TQString & ) ),
                  this, TQT_SLOT ( slotFillTemplates() ) );
        connect ( s_pDirWatch, TQT_SIGNAL( created( const TQString & ) ),
                  this, TQT_SLOT ( slotFillTemplates() ) );
        connect ( s_pDirWatch, TQT_SIGNAL( deleted( const TQString & ) ),
                  this, TQT_SLOT ( slotFillTemplates() ) );
        // Ok, this doesn't cope with new dirs in TDEDIRS, but that's another story
    }
    s_templatesVersion++;
    s_filesParsed = false;

    s_templatesList->clear();

    // Look into "templates" dirs.
    TQStringList files = d->m_actionCollection->instance()->dirs()->findAllResources("templates");
    KSortableValueList<Entry,TQString> slist;
    for ( TQStringList::Iterator it = files.begin() ; it != files.end() ; ++it )
    {
        //kdDebug(1203) << *it << endl;
        if ( (*it)[0] != '.' )
        {
            Entry e;
            e.filePath = *it;
            e.entryType = 0; // not parsed yet
            // put Directory etc. with special order (see fillMenu()) first in the list (a bit hacky)
            if ( (*it).endsWith( "Directory.desktop" ) ||
                 (*it).endsWith( "linkProgram.desktop" ) ||
                 (*it).endsWith( "linkURL.desktop" ) )
                s_templatesList->prepend( e );
            else
            {
                KSimpleConfig config( *it, true );
                config.setDesktopGroup();

                // tricky solution to ensure that TextFile is at the beginning
                // because this filetype is the most used (according kde-core discussion)
                TQString key = config.readEntry("Name");
                if ( (*it).endsWith( "TextFile.desktop" ) )
                    key = "1_" + key;
                else
                    key = "2_" + key;

                slist.insert( key, e );
            }
        }
    }
    slist.sort();
    for(KSortableValueList<Entry, TQString>::ConstIterator it = slist.begin(); it != slist.end(); ++it)
    {
        s_templatesList->append( (*it).value() );
    }

}

void KNewMenu::slotNewDir()
{
    emit activated(); // for KDIconView::slotNewMenuActivated()

    if (popupFiles.isEmpty())
       return;

    KonqOperations::newDir(d->m_parentWidget, popupFiles.first());
}

void KNewMenu::slotNewFile()
{
    int id = TQString( TQT_TQOBJECT_CONST(sender())->name() + 7 ).toInt(); // skip "newmenu"
    if (id == 0)
    {
	// run the command for the templates
	KRun::runCommand(TQString(TQT_TQOBJECT_CONST(sender())->name()));
	return;
    }

    emit activated(); // for KDIconView::slotNewMenuActivated()

    Entry entry = *(s_templatesList->at( id - 1 ));
    //kdDebug(1203) << TQString("sFile = %1").arg(sFile) << endl;

    if ( !TQFile::exists( entry.templatePath ) ) {
        kdWarning(1203) << entry.templatePath << " doesn't exist" << endl;
        KMessageBox::sorry( 0L, i18n("<qt>The template file <b>%1</b> does not exist.</qt>").arg(entry.templatePath));
        return;
    }
    m_isURLDesktopFile = false;
    TQString name;
    if ( KDesktopFile::isDesktopFile( entry.templatePath ) )
    {
	KDesktopFile df( entry.templatePath );
    	//kdDebug(1203) <<  df.readType() << endl;
    	if ( df.readType() == "Link" )
    	{
    	    m_isURLDesktopFile = true;
    	    // entry.comment contains i18n("Enter link to location (URL):"). JFYI :)
    	    KURLDesktopFileDlg dlg( i18n("File name:"), entry.comment, d->m_parentWidget );
    	    // TODO dlg.setCaption( i18n( ... ) );
    	    if ( dlg.exec() )
    	    {
                name = dlg.fileName();
                m_linkURL = dlg.url();
                if ( name.isEmpty() || m_linkURL.isEmpty() )
        	    return;
            	if ( !name.endsWith( ".desktop" ) )
            	    name += ".desktop";
    	    }
    	    else
                return;
    	}
    	else // any other desktop file (Device, App, etc.)
    	{
    	    KURL::List::Iterator it = popupFiles.begin();
    	    for ( ; it != popupFiles.end(); ++it )
    	    {
                //kdDebug(1203) << "first arg=" << entry.templatePath << endl;
                //kdDebug(1203) << "second arg=" << (*it).url() << endl;
                //kdDebug(1203) << "third arg=" << entry.text << endl;
                TQString text = entry.text;
                text.replace( "...", TQString() ); // the ... is fine for the menu item but not for the default filename
                
		KURL defaultFile( *it );
		defaultFile.addPath( TDEIO::encodeFileName( text ) );
		if ( defaultFile.isLocalFile() && TQFile::exists( defaultFile.path() ) )
		    text = TDEIO::RenameDlg::suggestName( *it, text);

                KURL templateURL;
                templateURL.setPath( entry.templatePath );
                (void) new KPropertiesDialog( templateURL, *it, text, d->m_parentWidget );
    	    }
    	    return; // done, exit.
    	}
    }
    else
    {
        // The template is not a desktop file
        // Show the small dialog for getting the destination filename
        bool ok;
        TQString text = entry.text;
        text.replace( "...", TQString() ); // the ... is fine for the menu item but not for the default filename
        
	KURL defaultFile( *(popupFiles.begin()) );
	defaultFile.addPath( TDEIO::encodeFileName( text ) );
	if ( defaultFile.isLocalFile() && TQFile::exists( defaultFile.path() ) )
	    text = TDEIO::RenameDlg::suggestName( *(popupFiles.begin()), text);

        name = KInputDialog::getText( TQString::null, entry.comment,
    	text, &ok, d->m_parentWidget );
        if ( !ok )
	    return;
    }

    // The template is not a desktop file [or it's a URL one]
    // Copy it.
    KURL::List::Iterator it = popupFiles.begin();

    TQString src = entry.templatePath;
    for ( ; it != popupFiles.end(); ++it )
    {
        KURL dest( *it );
        dest.addPath( TDEIO::encodeFileName(name) ); // Chosen destination file name
        d->m_destPath = dest.path(); // will only be used if m_isURLDesktopFile and dest is local

        KURL uSrc;
        uSrc.setPath( src );
        //kdDebug(1203) << "KNewMenu : TDEIO::copyAs( " << uSrc.url() << ", " << dest.url() << ")" << endl;
        TDEIO::CopyJob * job = TDEIO::copyAs( uSrc, dest );
        job->setDefaultPermissions( true );
        connect( job, TQT_SIGNAL( result( TDEIO::Job * ) ),
                TQT_SLOT( slotResult( TDEIO::Job * ) ) );
        if ( m_isURLDesktopFile )
		connect( job, TQT_SIGNAL( renamed( TDEIO::Job *, const KURL&, const KURL& ) ),
        	     TQT_SLOT( slotRenamed( TDEIO::Job *, const KURL&, const KURL& ) ) );
    	KURL::List lst;
    	lst.append( uSrc );
    	(void)new KonqCommandRecorder( KonqCommand::COPY, lst, dest, job );
    }
}

// Special case (filename conflict when creating a link=url file)
// We need to update m_destURL
void KNewMenu::slotRenamed( TDEIO::Job *, const KURL& from , const KURL& to )
{
    if ( from.isLocalFile() )
    {
        kdDebug() << k_funcinfo << from.prettyURL() << " -> " << to.prettyURL() << " ( m_destPath=" << d->m_destPath << ")" << endl;
        Q_ASSERT( from.path() == d->m_destPath );
        d->m_destPath = to.path();
    }
}

void KNewMenu::slotResult( TDEIO::Job * job )
{
    if (job->error())
        job->showErrorDialog();
    else
    {
        KURL destURL = static_cast<TDEIO::CopyJob*>(job)->destURL();
        if ( destURL.isLocalFile() )
        {
            if ( m_isURLDesktopFile )
            {
                // destURL is the original destination for the new file.
                // But in case of a renaming (due to a conflict), the real path is in m_destPath
                kdDebug(1203) << " destURL=" << destURL.path() << " " << " d->m_destPath=" << d->m_destPath << endl;
                KDesktopFile df( d->m_destPath );
                df.writeEntry( "Icon", KProtocolInfo::icon( KURL(m_linkURL).protocol() ) );
                df.writePathEntry( "URL", m_linkURL );
                df.sync();
            }
            else
            {
                // Normal (local) file. Need to "touch" it, tdeio_file copied the mtime.
                (void) ::utime( TQFile::encodeName( destURL.path() ), 0 );
            }
        }
    }
}

//////////

KURLDesktopFileDlg::KURLDesktopFileDlg( const TQString& textFileName, const TQString& textUrl )
    : KDialogBase( Plain, TQString::null, Ok|Cancel|User1, Ok, 0L /*parent*/, 0L, true,
                   true, KStdGuiItem::clear() )
{
    initDialog( textFileName, TQString::null, textUrl, TQString::null );
}

KURLDesktopFileDlg::KURLDesktopFileDlg( const TQString& textFileName, const TQString& textUrl, TQWidget *parent )
    : KDialogBase( Plain, TQString::null, Ok|Cancel|User1, Ok, parent, 0L, true,
                   true, KStdGuiItem::clear() )
{
    initDialog( textFileName, TQString::null, textUrl, TQString::null );
}

void KURLDesktopFileDlg::initDialog( const TQString& textFileName, const TQString& defaultName, const TQString& textUrl, const TQString& defaultUrl )
{
    TQVBoxLayout * topLayout = new TQVBoxLayout( plainPage(), 0, spacingHint() );

    // First line: filename
    TQHBox * fileNameBox = new TQHBox( plainPage() );
    topLayout->addWidget( fileNameBox );

    TQLabel * label = new TQLabel( textFileName, fileNameBox );
    m_leFileName = new KLineEdit( fileNameBox, 0L );
    m_leFileName->setMinimumWidth(m_leFileName->sizeHint().width() * 3);
    label->setBuddy(m_leFileName);  // please "scheck" style
    m_leFileName->setText( defaultName );
    m_leFileName->setSelection(0, m_leFileName->text().length()); // autoselect
    connect( m_leFileName, TQT_SIGNAL(textChanged(const TQString&)),
             TQT_SLOT(slotNameTextChanged(const TQString&)) );

    // Second line: url
    TQHBox * urlBox = new TQHBox( plainPage() );
    topLayout->addWidget( urlBox );
    label = new TQLabel( textUrl, urlBox );
    m_urlRequester = new KURLRequester( defaultUrl, urlBox, "urlRequester" );
    m_urlRequester->setMode( KFile::File | KFile::Directory );

    m_urlRequester->setMinimumWidth( m_urlRequester->sizeHint().width() * 3 );
    connect( m_urlRequester->lineEdit(), TQT_SIGNAL(textChanged(const TQString&)),
             TQT_SLOT(slotURLTextChanged(const TQString&)) );
    label->setBuddy(m_urlRequester);  // please "scheck" style

    m_urlRequester->setFocus();
    enableButtonOK( !defaultName.isEmpty() && !defaultUrl.isEmpty() );
    connect( this, TQT_SIGNAL(user1Clicked()), this, TQT_SLOT(slotClear()) );
    m_fileNameEdited = false;
}

TQString KURLDesktopFileDlg::url() const
{
    if ( result() == TQDialog::Accepted )
        return m_urlRequester->url();
    else
        return TQString::null;
}

TQString KURLDesktopFileDlg::fileName() const
{
    if ( result() == TQDialog::Accepted )
        return m_leFileName->text();
    else
        return TQString::null;
}

void KURLDesktopFileDlg::slotClear()
{
    m_leFileName->setText( TQString::null );
    m_urlRequester->clear();
    m_fileNameEdited = false;
}

void KURLDesktopFileDlg::slotNameTextChanged( const TQString& )
{
    kdDebug() << k_funcinfo << endl;
    m_fileNameEdited = true;
    enableButtonOK( !m_leFileName->text().isEmpty() && !m_urlRequester->url().isEmpty() );
}

void KURLDesktopFileDlg::slotURLTextChanged( const TQString& )
{
    if ( !m_fileNameEdited )
    {
        // use URL as default value for the filename
        // (we copy only its filename if protocol supports listing,
        // but for HTTP we don't want tons of index.html links)
        KURL url( m_urlRequester->url() );
        if ( KProtocolInfo::supportsListing( url ) )
            m_leFileName->setText( url.fileName() );
        else
            m_leFileName->setText( url.url() );
        m_fileNameEdited = false; // slotNameTextChanged set it to true erroneously
    }
    enableButtonOK( !m_leFileName->text().isEmpty() && !m_urlRequester->url().isEmpty() );
}


#include "knewmenu.moc"
