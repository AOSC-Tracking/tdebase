// -*- Mode: C++; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4; -*-
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

#include "ktheme.h"

#include <tqfile.h>
#include <tqfileinfo.h>
#include <tqimage.h>
#include <tqpixmap.h>
#include <tqregexp.h>
#include <tqtextstream.h>
#include <tqdir.h>

#include <dcopclient.h>
#include <tdeapplication.h>
#include <tdeconfig.h>
#include <kdatastream.h>
#include <kdebug.h>
#include <tdeglobal.h>
#include <kiconloader.h>
#include <tdeio/job.h>
#include <tdeio/netaccess.h>
#include <kipc.h>
#include <tdelocale.h>
#include <kservice.h>
#include <ksimpleconfig.h>
#include <kstandarddirs.h>
#include <ktar.h>
#include <tdestyle.h>

KTheme::KTheme( TQWidget *parent, const TQString & xmlFile )
	: m_parent(parent)
{
    TQFile file( xmlFile );
    file.open( IO_ReadOnly );
    m_dom.setContent( TQByteArray(file.readAll()) );
    file.close();

    //kdDebug() << m_dom.toString( 2 ) << endl;

    setName( TQFileInfo( file ).baseName() );
    m_kgd = TDEGlobal::dirs();
}

KTheme::KTheme( TQWidget *parent, bool create )
    : m_parent(parent)
{
    if ( create )
    {
        m_dom = TQDomDocument( "ktheme" );

        m_root = m_dom.createElement( "ktheme" );
        m_root.setAttribute( "version", SYNTAX_VERSION );
        m_dom.appendChild( m_root );

        m_general = m_dom.createElement( "general" );
        m_root.appendChild( m_general );
    }

    m_kgd = TDEGlobal::dirs();
}

KTheme::~KTheme()
{
}

void KTheme::setName( const TQString & name )
{
    m_name = name;
}

bool KTheme::load( const KURL & url )
{
    kdDebug() << "Loading theme from URL: " << url << endl;

    TQString tmpFile;
    if ( !TDEIO::NetAccess::download( url, tmpFile, 0L ) )
        return false;

    kdDebug() << "Theme is in temp file: " << tmpFile << endl;

    // set theme's name
    setName( TQFileInfo( url.fileName() ).baseName() );

    // unpack the tarball
    TQString location = m_kgd->saveLocation( "themes",  m_name + "/" );
    KTar tar( tmpFile );
    tar.open( IO_ReadOnly );
    tar.directory()->copyTo( location );
    tar.close();

    // create the DOM
    TQFile file( location + m_name + ".xml" );
    file.open( IO_ReadOnly );
    m_dom.setContent( TQByteArray(file.readAll()) );
    file.close();

    // remove the temp file
    TDEIO::NetAccess::removeTempFile( tmpFile );

    return true;
}

TQString KTheme::createYourself( bool pack )
{
    // start with empty dir for orig theme
    if ( !pack )
        KTheme::remove( name() );

    // 1. General stuff set by methods setBlah()

    // 2. Background theme
    TDEConfig * globalConf = TDEGlobal::config();

    TDEConfig twinConf( "twinrc", true );
    twinConf.setGroup( "Desktops" );
    uint numDesktops = twinConf.readUnsignedNumEntry( "Number", 4 );

    TDEConfig desktopConf( "kdesktoprc", true );
    desktopConf.setGroup( "Background Common" );
    bool common = desktopConf.readBoolEntry( "CommonDesktop", true );

    for ( uint i=0; i < numDesktops-1; i++ )
    {
        TQDomElement desktopElem = m_dom.createElement( "desktop" );
        desktopElem.setAttribute( "number", i );
        desktopElem.setAttribute( "common", common );

        desktopConf.setGroup( "Desktop" + TQString::number( i ) );

        TQDomElement modeElem = m_dom.createElement( "mode" );
        modeElem.setAttribute( "id", desktopConf.readEntry( "BackgroundMode", "Flat" ) );
        desktopElem.appendChild( modeElem );

        TQDomElement c1Elem = m_dom.createElement( "color1" );
        c1Elem.setAttribute( "rgb", desktopConf.readColorEntry( "Color1" ).name() );
        desktopElem.appendChild( c1Elem );

        TQDomElement c2Elem = m_dom.createElement( "color2" );
        c2Elem.setAttribute( "rgb", desktopConf.readColorEntry( "Color2" ).name() );
        desktopElem.appendChild( c2Elem );

        TQDomElement blendElem = m_dom.createElement( "blending" );
        blendElem.setAttribute( "mode", desktopConf.readEntry( "BlendMode", TQString( "NoBlending" ) ) );
        blendElem.setAttribute( "balance", desktopConf.readEntry( "BlendBalance", TQString::number( 100 ) ) );
        blendElem.setAttribute( "reverse", desktopConf.readBoolEntry( "ReverseBlending", false ) );
        desktopElem.appendChild( blendElem );

        TQDomElement patElem = m_dom.createElement( "pattern" );
        patElem.setAttribute( "name", desktopConf.readEntry( "Pattern" ) );
        desktopElem.appendChild( patElem );

        TQDomElement wallElem = m_dom.createElement( "wallpaper" );
        wallElem.setAttribute( "url", processFilePath( "desktop", desktopConf.readPathEntry( "Wallpaper" ) ) );
        wallElem.setAttribute( "mode", desktopConf.readEntry( "WallpaperMode" ) );
        desktopElem.appendChild( wallElem );

        // TODO handle multi wallpapers (aka slideshow)

        m_root.appendChild( desktopElem );

        if ( common )           // generate only one node
            break;
    }

    // 11. Screensaver
    desktopConf.setGroup( "ScreenSaver" );
    TQDomElement saverElem = m_dom.createElement( "screensaver" );
    saverElem.setAttribute( "name", desktopConf.readEntry( "Saver" ) );
    m_root.appendChild( saverElem );

    // 3. Icons
    globalConf->setGroup( "Icons" );
    TQDomElement iconElem = m_dom.createElement( "icons" );
    iconElem.setAttribute( "name", globalConf->readEntry( "Theme",TDEIconTheme::current() ) );
    createIconElems( "DesktopIcons", "desktop", iconElem, globalConf );
    createIconElems( "MainToolbarIcons", "mainToolbar", iconElem, globalConf );
    createIconElems( "PanelIcons", "panel", iconElem, globalConf );
    createIconElems( "SmallIcons", "small", iconElem, globalConf );
    createIconElems( "ToolbarIcons", "toolbar", iconElem, globalConf );
    m_root.appendChild( iconElem );

    // 4. Sounds
    // 4.1 Global sounds
    TDEConfig * soundConf = new TDEConfig( "knotify.eventsrc", true );
    TQStringList stdEvents;
    stdEvents << "cannotopenfile" << "catastrophe" << "exittde" << "fatalerror"
              << "notification" << "printerror" << "starttde" << "warning"
              << "messageCritical" << "messageInformation" << "messageWarning"
              << "messageboxQuestion";

    // 4.2 KWin sounds
    TDEConfig * twinSoundConf = new TDEConfig( "twin.eventsrc", true );
    TQStringList twinEvents;
    twinEvents << "activate" << "close" << "delete" <<
        "desktop1" << "desktop2" << "desktop3" << "desktop4" <<
        "desktop5" << "desktop6" << "desktop7" << "desktop8" <<
        "maximize" << "minimize" << "moveend" << "movestart" <<
        "new" << "not_on_all_desktops" << "on_all_desktops" <<
        "resizeend" << "resizestart" << "shadedown" << "shadeup" <<
        "transdelete" << "transnew" << "unmaximize" << "unminimize";

    TQDomElement soundsElem = m_dom.createElement( "sounds" );
    createSoundList( stdEvents, "global", soundsElem, soundConf );
    createSoundList( twinEvents, "twin", soundsElem, twinSoundConf );
    m_root.appendChild( soundsElem );
    delete soundConf;
    delete twinSoundConf;


    // 5. Colors
    TQDomElement colorsElem = m_dom.createElement( "colors" );
    globalConf->setGroup( "KDE" );
    colorsElem.setAttribute( "contrast", globalConf->readNumEntry( "contrast", 7 ) );
    TQStringList stdColors;
    stdColors << "background" << "selectBackground" << "foreground" << "windowForeground"
              << "windowBackground" << "selectForeground" << "buttonBackground"
              << "buttonForeground" << "linkColor" << "visitedLinkColor" << "alternateBackground";

    globalConf->setGroup( "General" );
    for ( TQStringList::Iterator it = stdColors.begin(); it != stdColors.end(); ++it )
        createColorElem( ( *it ), "global", colorsElem, globalConf );

    TQStringList twinColors;
    twinColors << "activeForeground" << "inactiveBackground" << "inactiveBlend" << "activeBackground"
               << "activeBlend" << "inactiveForeground" << "activeTitleBtnBg" << "inactiveTitleBtnBg"
               << "frame" << "inactiveFrame" << "handle" << "inactiveHandle";
    globalConf->setGroup( "WM" );
    for ( TQStringList::Iterator it = twinColors.begin(); it != twinColors.end(); ++it )
        createColorElem( ( *it ), "twin", colorsElem, globalConf );

    m_root.appendChild( colorsElem );

    // 6. Cursors
    TDEConfig* mouseConf = new TDEConfig( "kcminputrc", true );
    mouseConf->setGroup( "Mouse" );
    TQDomElement cursorsElem = m_dom.createElement( "cursors" );
    cursorsElem.setAttribute( "name", mouseConf->readEntry( "cursorTheme" ) );
    m_root.appendChild( cursorsElem );
    delete mouseConf;
    // TODO copy the cursor theme?

    // 7. KWin
    twinConf.setGroup( "Style" );
    TQDomElement wmElem = m_dom.createElement( "wm" );
    wmElem.setAttribute( "name", twinConf.readEntry( "PluginLib" ) );
    wmElem.setAttribute( "type", "builtin" );    // TODO support pixmap themes when the twin client gets ported
    if ( twinConf.readBoolEntry( "CustomButtonPositions" )  )
    {
        TQDomElement buttonsElem = m_dom.createElement( "buttons" );
        buttonsElem.setAttribute( "left", twinConf.readEntry( "ButtonsOnLeft" ) );
        buttonsElem.setAttribute( "right", twinConf.readEntry( "ButtonsOnRight" ) );
        wmElem.appendChild( buttonsElem );
    }
    TQDomElement borderElem = m_dom.createElement( "border" );
    borderElem.setAttribute( "size", twinConf.readNumEntry( "BorderSize", 1 ) );
    wmElem.appendChild( borderElem );
    m_root.appendChild( wmElem );

    // 8. Konqueror
    TDEConfig konqConf( "konquerorrc", true );
    konqConf.setGroup( "Settings" );
    TQDomElement konqElem = m_dom.createElement( "konqueror" );
    TQDomElement konqWallElem = m_dom.createElement( "wallpaper" );
    TQString bgImagePath = konqConf.readPathEntry( "BgImage" );
    konqWallElem.setAttribute( "url", processFilePath( "konqueror", bgImagePath ) );
    konqElem.appendChild( konqWallElem );
    TQDomElement konqBgColorElem = m_dom.createElement( "bgcolor" );
    konqBgColorElem.setAttribute( "rgb", konqConf.readColorEntry( "BgColor" ).name() );
    konqElem.appendChild( konqBgColorElem );
    m_root.appendChild( konqElem );

    // 9. Kicker (aka TDE Panel)
    TDEConfig kickerConf( "kickerrc", true );
    kickerConf.setGroup( "General" );

    TQDomElement panelElem = m_dom.createElement( "panel" );

    if ( kickerConf.readBoolEntry( "UseBackgroundTheme" ) )
    {
        TQDomElement backElem = m_dom.createElement( "background" );
        TQString kbgPath = kickerConf.readPathEntry( "BackgroundTheme" );
        backElem.setAttribute( "url", processFilePath( "panel", kbgPath ) );
        backElem.setAttribute( "colorize", kickerConf.readBoolEntry( "ColorizeBackground" ) );
        panelElem.appendChild( backElem );
    }

    TQDomElement transElem = m_dom.createElement( "transparent" );
    transElem.setAttribute( "value", kickerConf.readBoolEntry( "Transparent" ) );
    panelElem.appendChild( transElem );

    TQDomElement posElem = m_dom.createElement( "position" );
    posElem.setAttribute( "value", kickerConf.readEntry( "Position" ) );
    panelElem.appendChild( posElem );


    TQDomElement showLeftHideButtonElem = m_dom.createElement( "showlefthidebutton" );
    showLeftHideButtonElem.setAttribute( "value", kickerConf.readBoolEntry( "ShowLeftHideButton" ) );
    panelElem.appendChild( showLeftHideButtonElem );

    TQDomElement showRightHideButtonElem = m_dom.createElement( "showrighthidebutton" );
    showRightHideButtonElem.setAttribute( "value", kickerConf.readBoolEntry( "ShowRightHideButton" ) );
    panelElem.appendChild( showRightHideButtonElem );



    m_root.appendChild( panelElem );

    // 10. Widget style
    globalConf->setGroup( "General" );
    TQDomElement widgetsElem = m_dom.createElement( "widgets" );
    widgetsElem.setAttribute( "name", globalConf->readEntry( "widgetStyle",TDEStyle::defaultStyle()  ) );
    m_root.appendChild( widgetsElem );

    // 12.
    TQDomElement fontsElem = m_dom.createElement( "fonts" );
    TQStringList fonts;
    fonts   << "General"    << "font"
            << "General"    << "fixed"
            << "General"    << "toolBarFont"
            << "General"    << "menuFont"
            << "WM"         << "activeFont"
            << "General"    << "taskbarFont"
            << "FMSettings" << "StandardFont";

    for ( TQStringList::Iterator it = fonts.begin(); it != fonts.end(); ++it ) {
        TQString group = *it; ++it;
        TQString key   = *it;
        TQString value;

        if ( group == "FMSettings" ) {
            desktopConf.setGroup( group );
            value = desktopConf.readEntry( key );
        }
        else {
            globalConf->setGroup( group );
            value = globalConf->readEntry( key );
        }
        TQDomElement fontElem = m_dom.createElement( key );
        fontElem.setAttribute( "object", group );
        fontElem.setAttribute( "value", value );
        fontsElem.appendChild( fontElem );
    }
    m_root.appendChild( fontsElem );

    // Save the XML
    TQFile file( m_kgd->saveLocation( "themes", m_name + "/" ) + m_name + ".xml" );
    if ( file.open( IO_WriteOnly ) ) {
        TQTextStream stream( &file );
        m_dom.save( stream, 2 );
        file.close();
    }

    TQString result;
    if ( pack )
    {
        // Pack the whole theme
        KTar tar( m_kgd->saveLocation( "themes" ) + m_name + ".kth", "application/x-gzip" );
        tar.open( IO_WriteOnly );

        kdDebug() << "Packing everything under: " << m_kgd->saveLocation( "themes", m_name + "/" ) << endl;

        if ( tar.addLocalDirectory( m_kgd->saveLocation( "themes", m_name + "/" ), TQString::null ) )
            result = tar.fileName();

        tar.close();
    }

    //kdDebug() << m_dom.toString( 2 ) << endl;

    return result;
}

void KTheme::apply()
{
    kdDebug() << "Going to apply theme: " << m_name << endl;

    TQString themeDir = m_kgd->findResourceDir( "themes", m_name + "/" + m_name + ".xml") + m_name + "/";
    kdDebug() << "Theme dir: " << themeDir << endl;

    // 2. Background theme

    TQDomNodeList desktopList = m_dom.elementsByTagName( "desktop" );
    TDEConfig desktopConf( "kdesktoprc" );
    desktopConf.setGroup( "Background Common" );

    for ( uint i = 0; i <= desktopList.count(); i++ )
    {
        TQDomElement desktopElem = desktopList.item( i ).toElement();
        if ( !desktopElem.isNull() )
        {
            // TODO optimize, don't write several times the common section
            bool common = static_cast<bool>( desktopElem.attribute( "common", "true" ).toUInt() );
            desktopConf.writeEntry( "CommonDesktop", common );
            desktopConf.writeEntry( "DeskNum", desktopElem.attribute( "number", "0" ).toUInt() );

            desktopConf.setGroup( TQString( "Desktop%1" ).arg( i ) );
            if (getProperty( desktopElem, "mode", "id" ) != TQString::null)          desktopConf.writeEntry( "BackgroundMode", getProperty( desktopElem, "mode", "id" ) );
            if (getProperty( desktopElem, "color1", "rgb" ) != TQString::null)       desktopConf.writeEntry( "Color1", TQColor( getProperty( desktopElem, "color1", "rgb" ) ) );
            if (getProperty( desktopElem, "color2", "rgb" ) != TQString::null)       desktopConf.writeEntry( "Color2", TQColor( getProperty( desktopElem, "color2", "rgb" ) ) );
            if (getProperty( desktopElem, "blending", "mode" ) != TQString::null)    desktopConf.writeEntry( "BlendMode", getProperty( desktopElem, "blending", "mode" ) );
            if (getProperty( desktopElem, "blending", "balance" ) != TQString::null) desktopConf.writeEntry( "BlendBalance", getProperty( desktopElem, "blending", "balance" ) );
            if (getProperty( desktopElem, "blending", "reverse" ) != TQString::null) desktopConf.writeEntry( "ReverseBlending", static_cast<bool>( getProperty( desktopElem, "blending", "reverse" ).toUInt() ) );
            if (getProperty( desktopElem, "pattern", "name" ) != TQString::null)     desktopConf.writeEntry( "Pattern", getProperty( desktopElem, "pattern", "name" ) );
            if (getProperty( desktopElem, "wallpaper", "url" ) != TQString::null)    desktopConf.writeEntry( "Wallpaper", unprocessFilePath( "desktop", getProperty( desktopElem, "wallpaper", "url" ) ) );
            if (getProperty( desktopElem, "wallpaper", "mode" ) != TQString::null)   desktopConf.writeEntry( "WallpaperMode", getProperty( desktopElem, "wallpaper", "mode" ) );

            if ( common ) {
                break;          // stop here
            }
        }
    }

    // 11. Screensaver
    TQDomElement saverElem = m_dom.elementsByTagName( "screensaver" ).item( 0 ).toElement();

    if ( !saverElem.isNull() )
    {
        desktopConf.setGroup( "ScreenSaver" );
        desktopConf.writeEntry( "Saver", saverElem.attribute( "name" ) );
    }

    desktopConf.sync();         // TODO sync and signal only if <desktop> elem present
    // reconfigure kdesktop. kdesktop will notify all clients
    DCOPClient *client = kapp->dcopClient();
    if ( !client->isAttached() ) {
        client->attach();
    }
    client->send("kdesktop", "KBackgroundIface", "configure()", TQString(""));
    // FIXME Xinerama

    // 3. Icons
    TQDomElement iconElem = m_dom.elementsByTagName( "icons" ).item( 0 ).toElement();
    if ( !iconElem.isNull() )
    {
        TDEConfig * iconConf = TDEGlobal::config();
        iconConf->setGroup( "Icons" );
        iconConf->writeEntry( "Theme", iconElem.attribute( "name", "crystalsvg" ), true, true );

        TQDomNodeList iconList = iconElem.childNodes();
        for ( uint i = 0; i < iconList.count(); i++ )
        {
            TQDomElement iconSubElem = iconList.item( i ).toElement();
            TQString object = iconSubElem.attribute( "object" );
            if ( object == "desktop" ) {
                iconConf->setGroup( "DesktopIcons" );
            }
            else if ( object == "mainToolbar" ) {
                iconConf->setGroup( "MainToolbarIcons" );
            }
            else if ( object == "panel" ) {
                iconConf->setGroup( "PanelIcons" );
            }
            else if ( object == "small" ) {
                iconConf->setGroup( "SmallIcons" );
            }
            else if ( object == "toolbar" ) {
                iconConf->setGroup( "ToolbarIcons" );
            }

            TQString iconName = iconSubElem.tagName();
            if ( iconName.contains( "Color" ) ) {
                TQColor iconColor = TQColor( iconSubElem.attribute( "rgb" ) );
                iconConf->writeEntry( iconName, iconColor, true, true );
            }
            else if ( iconName.contains( "Value" ) || iconName == "Size" ) {
                iconConf->writeEntry( iconName, iconSubElem.attribute( "value" ).toUInt(), true, true );
            }
            else if ( iconName.contains( "Effect" ) ) {
                iconConf->writeEntry( iconName, iconSubElem.attribute( "name" ), true, true );
            }
            else {
                iconConf->writeEntry( iconName, static_cast<bool>( iconSubElem.attribute( "value" ).toUInt() ), true, true );
            }
        }
        iconConf->sync();

        for ( int i = 0; i < TDEIcon::LastGroup; i++ )
            KIPC::sendMessageAll( KIPC::IconChanged, i );
        KService::rebuildKSycoca( m_parent );
    }

    // 4. Sounds
    TQDomElement soundsElem = m_dom.elementsByTagName( "sounds" ).item( 0 ).toElement();
    if ( !soundsElem.isNull() )
    {
        TDEConfig soundConf( "knotify.eventsrc" );
        TDEConfig twinSoundConf( "twin.eventsrc" );
        TQDomNodeList eventList = soundsElem.elementsByTagName( "event" );
        for ( uint i = 0; i < eventList.count(); i++ )
        {
            TQDomElement eventElem = eventList.item( i ).toElement();
            TQString object = eventElem.attribute( "object" );

            if ( object == "global" )
            {
                soundConf.setGroup( eventElem.attribute( "name" ) );
                soundConf.writeEntry( "soundfile", unprocessFilePath( "sounds", eventElem.attribute( "url" ) ) );
                soundConf.writeEntry( "presentation", soundConf.readNumEntry( "presentation" ) | 1 );
            }
            else if ( object == "twin" )
            {
                twinSoundConf.setGroup( eventElem.attribute( "name" ) );
                twinSoundConf.writeEntry( "soundfile", unprocessFilePath( "sounds", eventElem.attribute( "url" ) ) );
                twinSoundConf.writeEntry( "presentation", soundConf.readNumEntry( "presentation" ) | 1 );
            }
        }

        soundConf.sync();
        twinSoundConf.sync();
        client->send("knotify", "", "reconfigure()", TQString(""));
        // TODO signal twin sounds change?
    }

    // 5. Colors
    TQDomElement colorsElem = m_dom.elementsByTagName( "colors" ).item( 0 ).toElement();

    if ( !colorsElem.isNull() )
    {
        TQDomNodeList colorList = colorsElem.childNodes();
        TDEConfig * colorConf = TDEGlobal::config();

        TQString sCurrentScheme = locateLocal("data", "tdedisplay/color-schemes/thememgr.kcsrc");
        KSimpleConfig *colorScheme = new KSimpleConfig( sCurrentScheme );
        colorScheme->setGroup("Color Scheme" );

        for ( uint i = 0; i < colorList.count(); i++ )
        {
            TQDomElement colorElem = colorList.item( i ).toElement();
            TQString object = colorElem.attribute( "object" );
            if ( object == "global" )
                colorConf->setGroup( "General" );
            else if ( object == "twin" )
                colorConf->setGroup( "WM" );

            TQString colName = colorElem.tagName();
            TQColor curColor = TQColor( colorElem.attribute( "rgb" ) );
            colorConf->writeEntry( colName, curColor, true, true ); // kdeglobals
            colorScheme->writeEntry( colName, curColor ); // thememgr.kcsrc
        }

        colorConf->setGroup( "KDE" );
        colorConf->writeEntry( "colorScheme", "thememgr.kcsrc", true, true );
        colorConf->writeEntry( "contrast", colorsElem.attribute( "contrast", "7" ), true, true );
        colorScheme->writeEntry( "contrast", colorsElem.attribute( "contrast", "7" ) );
        colorConf->sync();
        delete colorScheme;

        KIPC::sendMessageAll( KIPC::PaletteChanged );
    }

    // 6.Cursors
    TQDomElement cursorsElem = m_dom.elementsByTagName( "cursors" ).item( 0 ).toElement();

    if ( !cursorsElem.isNull() )
    {
        TDEConfig mouseConf( "kcminputrc" );
        mouseConf.setGroup( "Mouse" );
        mouseConf.writeEntry( "cursorTheme", cursorsElem.attribute( "name" ));
        // FIXME is there a way to notify KDE of cursor changes?
    }

    // 7. KWin
    TQDomElement wmElem = m_dom.elementsByTagName( "wm" ).item( 0 ).toElement();

    if ( !wmElem.isNull() )
    {
        TDEConfig twinConf( "twinrc" );
        twinConf.setGroup( "Style" );
        TQString type = wmElem.attribute( "type" );
        if ( type == "builtin" ) {
            twinConf.writeEntry( "PluginLib", wmElem.attribute( "name" ) );
        }
        //else // TODO support custom themes
        TQDomNodeList buttons = wmElem.elementsByTagName ("buttons");
        if ( buttons.count() > 0 )
        {
            twinConf.writeEntry( "CustomButtonPositions", true );
            if (getProperty( wmElem, "buttons", "left" ) != TQString::null)  twinConf.writeEntry( "ButtonsOnLeft", getProperty( wmElem, "buttons", "left" ) );
            if (getProperty( wmElem, "buttons", "right" ) != TQString::null) twinConf.writeEntry( "ButtonsOnRight", getProperty( wmElem, "buttons", "right" ) );
        }
        else
        {
            twinConf.writeEntry( "CustomButtonPositions", false );
        }
        if (getProperty( wmElem, "border", "size" ) != TQString::null) twinConf.writeEntry( "BorderSize", getProperty( wmElem, "border", "size" ) );

        twinConf.sync();
        client->send( "twin", "", "reconfigure()", TQString("") );
    }

    // 8. Konqueror
    TQDomElement konqElem = m_dom.elementsByTagName( "konqueror" ).item( 0 ).toElement();

    if ( !konqElem.isNull() )
    {
        TDEConfig konqConf( "konquerorrc" );
        konqConf.setGroup( "Settings" );
        if (getProperty( konqElem, "wallpaper", "url" ) != TQString::null) konqConf.writeEntry( "BgImage", unprocessFilePath( "konqueror", getProperty( konqElem, "wallpaper", "url" ) ) );
        if (getProperty( konqElem, "bgcolor", "rgb" ) != TQString::null)   konqConf.writeEntry( "BgColor", TQColor( getProperty( konqElem, "bgcolor", "rgb" ) ) );

        konqConf.sync();
        client->send("konqueror*", "KonquerorIface", "reparseConfiguration()", TQString("")); // FIXME seems not to work :(
    }

    // 9. Kicker
    TQDomElement panelElem = m_dom.elementsByTagName( "panel" ).item( 0 ).toElement();

    if ( !panelElem.isNull() )
    {
        TDEConfig kickerConf( "kickerrc" );
        kickerConf.setGroup( "General" );
        TQString kickerBgUrl = getProperty( panelElem, "background", "url" );
        if ( !kickerBgUrl.isEmpty() )
        {
            kickerConf.writeEntry( "UseBackgroundTheme", true );
            if (getProperty( panelElem, "panel", kickerBgUrl ) != TQString::null)       kickerConf.writeEntry( "BackgroundTheme", unprocessFilePath( "panel", kickerBgUrl ) );
            if (getProperty( panelElem, "background", "colorize" ) != TQString::null)   kickerConf.writeEntry( "ColorizeBackground", static_cast<bool>( getProperty( panelElem, "background", "colorize" ).toUInt() ) );
        }
        if (getProperty( panelElem, "transparent", "value" ) != TQString::null)         kickerConf.writeEntry( "Transparent", static_cast<bool>( getProperty( panelElem, "transparent", "value" ).toUInt() ) );
        if (getProperty( panelElem, "position", "value" ) != TQString::null)            kickerConf.writeEntry( "Position", static_cast<int> (getProperty( panelElem, "position", "value" ).toUInt() ));
        if (getProperty( panelElem, "showlefthidebutton", "value" ) != TQString::null)  kickerConf.writeEntry( "ShowLeftHideButton", static_cast<bool>( getProperty( panelElem, "showlefthidebutton", "value").toInt()));
        if (getProperty( panelElem, "showrighthidebutton", "value" ) != TQString::null) kickerConf.writeEntry( "ShowRightHideButton", static_cast<bool>( getProperty( panelElem, "showrighthidebutton", "value").toInt()));

        kickerConf.sync();
        client->send("kicker", "Panel", "configure()", TQString(""));
    }

    // 10. Widget style
    TQDomElement widgetsElem = m_dom.elementsByTagName( "widgets" ).item( 0 ).toElement();

    if ( !widgetsElem.isNull() )
    {
        TDEConfig * widgetConf = TDEGlobal::config();
        widgetConf->setGroup( "General" );
        widgetConf->writeEntry( "widgetStyle", widgetsElem.attribute( "name" ), true, true );
        widgetConf->sync();
        KIPC::sendMessageAll( KIPC::StyleChanged );
    }

    // 12. Fonts
    TQDomElement fontsElem = m_dom.elementsByTagName( "fonts" ).item( 0 ).toElement();
    if ( !fontsElem.isNull() )
    {
        TDEConfig * fontsConf = TDEGlobal::config();
        TDEConfig * kde1xConf = new KSimpleConfig( TQDir::homeDirPath() + "/.tderc" );
        kde1xConf->setGroup( "General" );

        TQDomNodeList fontList = fontsElem.childNodes();
        for ( uint i = 0; i < fontList.count(); i++ )
        {
            TQDomElement fontElem = fontList.item( i ).toElement();
            TQString fontName  = fontElem.tagName();
            TQString fontValue = fontElem.attribute( "value" );
            TQString fontObject = fontElem.attribute( "object" );

            if ( fontObject == "FMSettings" ) {
                desktopConf.setGroup( fontObject );
                desktopConf.writeEntry( fontName, fontValue, true, true );
                desktopConf.sync();
            }
            else {
                fontsConf->setGroup( fontObject );
                fontsConf->writeEntry( fontName, fontValue, true, true );
            }
            kde1xConf->writeEntry( fontName, fontValue, true, true );
        }

        fontsConf->sync();
        kde1xConf->sync();
        KIPC::sendMessageAll( KIPC::FontChanged );
    }

}

bool KTheme::remove( const TQString & name )
{
    kdDebug() << "Going to remove theme: " << name << endl;
    return TDEIO::NetAccess::del( TDEGlobal::dirs()->saveLocation( "themes", name + "/" ), 0L );
}

void KTheme::setProperty( const TQString & name, const TQString & value, TQDomElement parent )
{
    TQDomElement tmp = m_dom.createElement( name );
    tmp.setAttribute( "value", value );
    parent.appendChild( tmp );
}

TQString KTheme::getProperty( const TQString & name ) const
{
    TQDomNodeList _list = m_dom.elementsByTagName( name );
    if ( _list.count() != 0 )
        return _list.item( 0 ).toElement().attribute( "value" );
    else
    {
        kdWarning() << "Found no such property: " << name << endl;
        return TQString::null;
    }
}

TQString KTheme::getProperty( TQDomElement parent, const TQString & tag,
                             const TQString & attr ) const
{
    TQDomNodeList _list = parent.elementsByTagName( tag );

    if ( _list.count() != 0 )
        return _list.item( 0 ).toElement().attribute( attr );
    else
    {
        kdWarning() << TQString( "No such property found: %1->%2->%3" )
            .arg( parent.tagName() ).arg( tag ).arg( attr ) << endl;
        return TQString::null;
    }
}

void KTheme::createIconElems( const TQString & group, const TQString & object,
                              TQDomElement parent, TDEConfig * cfg )
{
    cfg->setGroup( group );
    TQStringList elemNames;
    elemNames << "Animated" << "DoublePixels" << "Size"
              << "ActiveColor" << "ActiveColor2" << "ActiveEffect"
              << "ActiveSemiTransparent" << "ActiveValue"
              << "DefaultColor" << "DefaultColor2" << "DefaultEffect"
              << "DefaultSemiTransparent" << "DefaultValue"
              << "DisabledColor" << "DisabledColor2" << "DisabledEffect"
              << "DisabledSemiTransparent" << "DisabledValue";
    for ( TQStringList::ConstIterator it = elemNames.begin(); it != elemNames.end(); ++it ) {
        if ( (*it).contains( "Color" ) )
            createColorElem( *it, object, parent, cfg );
        else
        {
            TQDomElement tmpCol = m_dom.createElement( *it );
            tmpCol.setAttribute( "object", object );

            if ( (*it).contains( "Value" ) || *it == "Size" )
                tmpCol.setAttribute( "value", cfg->readNumEntry( *it, 1 ) );
            else if ( (*it).contains( "DisabledEffect" ) )
                tmpCol.setAttribute( "name", cfg->readEntry( *it, "togray" ) );
	    else if ( (*it).contains( "Effect" ) )
                tmpCol.setAttribute( "name", cfg->readEntry( *it, "none" ) );
            else
                tmpCol.setAttribute( "value", cfg->readBoolEntry( *it, false ) );
            parent.appendChild( tmpCol );
        }
    }
}

void KTheme::createColorElem( const TQString & name, const TQString & object,
                              TQDomElement parent, TDEConfig * cfg )
{
    TQColor color = cfg->readColorEntry( name );
    if ( color.isValid() )
    {
        TQDomElement tmpCol = m_dom.createElement( name );
        tmpCol.setAttribute( "rgb", color.name() );
        tmpCol.setAttribute( "object", object );
        parent.appendChild( tmpCol );
    }
}

void KTheme::createSoundList( const TQStringList & events, const TQString & object,
                              TQDomElement parent, TDEConfig * cfg )
{
    for ( TQStringList::ConstIterator it = events.begin(); it != events.end(); ++it )
    {
        TQString group = ( *it );
        if ( cfg->hasGroup( group ) )
        {
            cfg->setGroup( group );
            TQString soundURL = cfg->readPathEntry( "soundfile" );
            int pres = cfg->readNumEntry( "presentation", 0 );
            if ( !soundURL.isEmpty() && ( ( pres & 1 ) == 1 ) )
            {
                TQDomElement eventElem = m_dom.createElement( "event" );
                eventElem.setAttribute( "object", object );
                eventElem.setAttribute( "name", group );
                eventElem.setAttribute( "url", processFilePath( "sounds", soundURL ) );
                parent.appendChild( eventElem );
            }
        }
    }
}

TQString KTheme::processFilePath( const TQString & section, const TQString & path )
{
    TQFileInfo fi( path );

    if ( fi.isRelative() )
        fi.setFile( findResource( section, path ) );

    kdDebug() << "Processing file: " << fi.absFilePath() << ", " << fi.fileName() << endl;

    if ( section == "desktop" )
    {
        if ( copyFile( fi.absFilePath(), m_kgd->saveLocation( "themes", m_name + "/wallpapers/desktop/" ) + "/" + fi.fileName() ) )
            return "theme:/wallpapers/desktop/" + fi.fileName();
    }
    else if ( section == "sounds" )
    {
        if ( copyFile( fi.absFilePath(), m_kgd->saveLocation( "themes", m_name + "/sounds/" ) + "/" + fi.fileName() ) )
            return "theme:/sounds/" + fi.fileName();
    }
    else if ( section == "konqueror" )
    {
        if ( copyFile( fi.absFilePath(), m_kgd->saveLocation( "themes", m_name + "/wallpapers/konqueror/" ) + "/" + fi.fileName() ) )
            return "theme:/wallpapers/konqueror/" + fi.fileName();
    }
    else if ( section == "panel" )
    {
        if ( copyFile( fi.absFilePath(), m_kgd->saveLocation( "themes", m_name + "/wallpapers/panel/" ) + "/" + fi.fileName() ) )
            return "theme:/wallpapers/panel/" + fi.fileName();
    }
    else
        kdWarning() << "Unsupported theme resource type" << endl;

    return TQString::null;       // an error occured or the resource doesn't exist
}

TQString KTheme::unprocessFilePath( const TQString & section, TQString path )
{
    if ( path.startsWith( "theme:/" ) )
        return path.replace( TQRegExp( "^theme:/" ), m_kgd->findResourceDir( "themes", m_name + "/" + m_name + ".xml") + m_name + "/" );

    if ( TQFile::exists( path ) )
        return path;
    else  // try to find it in the system
        return findResource( section, path );
}

void KTheme::setAuthor( const TQString & author )
{
    setProperty( "author", author, m_general );
}

void KTheme::setEmail( const TQString & email )
{
    setProperty( "email", email, m_general );
}

void KTheme::setHomepage( const TQString & homepage )
{
    setProperty( "homepage", homepage, m_general );
}

void KTheme::setComment( const TQString & comment )
{
    setProperty( "comment", comment, m_general );
}

void KTheme::setVersion( const TQString & version )
{
    setProperty( "version", version, m_general );
}

void KTheme::addPreview()
{
    TQString file = m_kgd->saveLocation( "themes", m_name + "/" ) + m_name + ".preview.png";
    kdDebug() << "Adding preview: " << file << endl;
    TQPixmap snapshot = TQPixmap::grabWindow( tqt_xrootwin() );
    snapshot.save( file, "PNG" );
}

bool KTheme::copyFile( const TQString & from, const TQString & to )
{
    // we overwrite b/c of restoring the "original" theme
    return TDEIO::NetAccess::file_copy( from, to, -1, true /*overwrite*/ );
}

TQString KTheme::findResource( const TQString & section, const TQString & path )
{
    if ( section == "desktop" )
        return m_kgd->findResource( "wallpaper", path );
    else if ( section == "sounds" )
        return m_kgd->findResource( "sound", path );
    else if ( section == "konqueror" )
        return m_kgd->findResource( "data", "konqueror/tiles/" + path );
    else if ( section == "panel" )
        return m_kgd->findResource( "data", "kicker/wallpapers/" + path );
    else
    {
        kdWarning() << "Requested unknown resource: " << section << endl;
        return TQString::null;
    }
}
