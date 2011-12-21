/* This file is part of the KDE project
   Copyright (C) 1998, 1999 Torben Weis <weis@kde.org>
   Copyright (c) 1999 David Faure <faure@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include <tqbuttongroup.h>
#include <tqlabel.h>
#include <tqlayout.h>
#include <tqradiobutton.h>

#include <kcolorbutton.h>
#include <kcombobox.h>
#include <kdebug.h>
#include <kimagefilepreview.h>
#include <klocale.h>
//#include <krecentdocument.h>
#include <kstandarddirs.h>
#include <kurlrequester.h>

#include "konq_bgnddlg.h"


KonqBgndDialog::KonqBgndDialog( TQWidget* parent,
                                const TQString& pixmapFile,
                                const TQColor& theColor,
                                const TQColor& defaultColor )
 : KDialogBase( parent, "KonqBgndDialog", false,
                i18n("Background Settings"), Ok|Cancel, Ok, true )
{
    TQWidget* page = new TQWidget( this );
    setMainWidget( page );
    TQVBoxLayout* mainLayout = new TQVBoxLayout( page, 0, KDialog::spacingHint() );

    m_buttonGroup = new TQButtonGroup( i18n("Background"), page );
    m_buttonGroup->setColumnLayout( 0, Qt::Vertical );
    m_buttonGroup->layout()->setMargin( KDialog::marginHint() );
    m_buttonGroup->layout()->setSpacing( KDialog::spacingHint() );
    TQGridLayout* groupLayout = new TQGridLayout( m_buttonGroup->layout() );
    groupLayout->setAlignment( Qt::AlignTop );
    mainLayout->addWidget( m_buttonGroup );

    connect( m_buttonGroup, TQT_SIGNAL( clicked(int) ),
             this, TQT_SLOT( slotBackgroundModeChanged() ) );

    // color
    m_radioColor = new TQRadioButton( i18n("Co&lor:"), m_buttonGroup );
    groupLayout->addWidget( m_radioColor, 0, 0 );
    m_buttonColor = new KColorButton( theColor, defaultColor, m_buttonGroup );
    m_buttonColor->setSizePolicy( TQSizePolicy::Preferred,
                                TQSizePolicy::Minimum );
    groupLayout->addWidget( m_buttonColor, 0, 1 );

    connect( m_buttonColor, TQT_SIGNAL( changed( const TQColor& ) ),
             this, TQT_SLOT( slotColorChanged() ) );

    // picture
    m_radioPicture = new TQRadioButton( i18n("&Picture:"), m_buttonGroup );
    groupLayout->addWidget( m_radioPicture, 1, 0 );
    m_comboPicture = new KURLComboRequester( m_buttonGroup );
    groupLayout->addMultiCellWidget( m_comboPicture, 1, 1, 1, 2 );
    initPictures();

    connect( m_comboPicture->comboBox(), TQT_SIGNAL( activated( int ) ),
	     this, TQT_SLOT( slotPictureChanged() ) );
    connect( m_comboPicture, TQT_SIGNAL( urlSelected(const TQString &) ),
             this, TQT_SLOT( slotPictureChanged() ) );

    TQSpacerItem* spacer1 = new TQSpacerItem( 0, 0, TQSizePolicy::Expanding,
                                            TQSizePolicy::Minimum );
    groupLayout->addItem( spacer1, 0, 2 );

    // preview title
    TQHBoxLayout* hlay = new TQHBoxLayout( mainLayout, KDialog::spacingHint() );
    //mainLayout->addLayout( hlay );
    TQLabel* lbl = new TQLabel( i18n("Preview"), page );
    hlay->addWidget( lbl );
    TQFrame* frame = new TQFrame( page );
    frame->setSizePolicy( TQSizePolicy::Expanding, TQSizePolicy::Minimum );
    frame->setFrameShape( TQFrame::HLine );
    frame->setFrameShadow( TQFrame::Sunken );
    hlay->addWidget( frame );

    // preview frame
    m_preview = new TQFrame( page );
    m_preview->setSizePolicy( TQSizePolicy::Expanding, TQSizePolicy::Expanding );
    m_preview->setMinimumSize( 370, 180 );
    m_preview->setFrameShape( TQFrame::Panel );
    m_preview->setFrameShadow( TQFrame::Raised );
    mainLayout->addWidget( m_preview );

    if ( !pixmapFile.isEmpty() ) {
        loadPicture( pixmapFile );
        m_buttonColor->setColor( defaultColor );
        m_radioPicture->setChecked( true );
    }
    else {
        m_buttonColor->setColor( theColor );
        m_comboPicture->comboBox()->setCurrentItem( 0 );
        m_radioColor->setChecked( true );
    }
    slotBackgroundModeChanged();
}

KonqBgndDialog::~KonqBgndDialog()
{
}

TQColor KonqBgndDialog::color() const
{
    if ( m_radioColor->isChecked() )
        return m_buttonColor->color();

    return TQColor();
}

void KonqBgndDialog::initPictures()
{
    KGlobal::dirs()->addResourceType( "tiles",
        KGlobal::dirs()->kde_default("data") + "konqueror/tiles/");
    kdDebug(1203) << KGlobal::dirs()->kde_default("data") + "konqueror/tiles/" << endl;

    TQStringList list = KGlobal::dirs()->findAllResources("tiles");

    if ( list.isEmpty() )
        m_comboPicture->comboBox()->insertItem( i18n("None") );
    else {
        TQStringList::ConstIterator it;
        for ( it = list.begin(); it != list.end(); it++ )
            m_comboPicture->comboBox()->insertItem(
                ( (*it).at(0) == '/' ) ?    // if absolute path
                KURL( *it ).fileName() :  // then only fileName
                *it );
    }
}

void KonqBgndDialog::loadPicture( const TQString& fileName )
{
    int i ;
    for ( i = 0; i < m_comboPicture->comboBox()->count(); i++ ) {
        if ( fileName == m_comboPicture->comboBox()->text( i ) ) {
            m_comboPicture->comboBox()->setCurrentItem( i );
            return;
        }
    }

    if ( !fileName.isEmpty() ) {
        m_comboPicture->comboBox()->insertItem( fileName );
        m_comboPicture->comboBox()->setCurrentItem( i );
    }
    else
        m_comboPicture->comboBox()->setCurrentItem( 0 );
}

void KonqBgndDialog::slotPictureChanged()
{
    m_pixmapFile = m_comboPicture->comboBox()->currentText();
    TQString file = locate( "tiles", m_pixmapFile );
    if ( file.isEmpty() )
        file = locate("wallpaper", m_pixmapFile); // add fallback for compatibility
    if ( file.isEmpty() ) {
        kdWarning(1203) << "Couldn't locate wallpaper " << m_pixmapFile << endl;
        m_preview->unsetPalette();
        m_pixmap = TQPixmap();
        m_pixmapFile = "";
    }
    else {
        m_pixmap.load( file );

        if ( m_pixmap.isNull() )
            kdWarning(1203) << "Could not load wallpaper " << file << endl;
    }
    m_preview->setPaletteBackgroundPixmap( m_pixmap );
}

void KonqBgndDialog::slotColorChanged()
{
    m_preview->setPaletteBackgroundColor( m_buttonColor->color() );
}

void KonqBgndDialog::slotBackgroundModeChanged()
{
    if ( m_radioColor->isChecked() ) {
        m_buttonColor->setEnabled( true );
        m_comboPicture->setEnabled( false );
        m_pixmapFile = "";
        slotColorChanged();
    }
    else {  // m_comboPicture->isChecked() == true
        m_comboPicture->setEnabled( true );
        m_buttonColor->setEnabled( false );
        slotPictureChanged();
    }
}


#include "konq_bgnddlg.moc"
