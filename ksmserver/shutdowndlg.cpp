/*****************************************************************
ksmserver - the KDE session management server

Copyright (C) 2010 Timothy Pearson <kb9vqf@pearsoncomputing.net>
Copyright (C) 2000 Matthias Ettrich <ettrich@kde.org>
******************************************************************/

#include "shutdowndlg.h"
#include <tqapplication.h>
#include <tqlayout.h>
#include <tqgroupbox.h>
#include <tqvbuttongroup.h>
#include <tqlabel.h>
#include <tqvbox.h>
#include <tqtimer.h>
#include <tqstyle.h>
#include <tqcombobox.h>
#include <tqcursor.h>
#include <tqmessagebox.h>
#include <tqbuttongroup.h>
#include <tqiconset.h>
#include <tqpixmap.h>
#include <tqpopupmenu.h>
#include <tqtooltip.h>
#include <tqimage.h>
#include <tqpainter.h>
#include <tqfontmetrics.h>
#include <tqregexp.h>
#include <tqeventloop.h>

#include <klocale.h>
#include <kconfig.h>
#include <kapplication.h>
#include <kdebug.h>
#include <kpushbutton.h>
#include <kstdguiitem.h>
#include <kguiitem.h>
#include <kiconloader.h>
#include <kglobalsettings.h>
#include <kwin.h>
#include <kuser.h>
#include <kpixmap.h>
#include <kimageeffect.h>
#include <kdialog.h>
#include <kseparator.h>
#include <kconfig.h>

#include <dcopclient.h>
#include <dcopref.h>

#include <sys/types.h>
#include <sys/utsname.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>
#include <dmctl.h>
#include <kaction.h>
#include <netwm.h>

#include <X11/Xlib.h>

#include "shutdowndlg.moc"

KSMShutdownFeedback * KSMShutdownFeedback::s_pSelf = 0L;

KSMShutdownFeedback::KSMShutdownFeedback()
 : TQWidget( 0L, "feedbackwidget", WType_Popup ),
   m_currentY( 0 ),
   m_grayOpacity( 0.0f ),
   m_compensation( 0.0f ),
   m_fadeBackwards( FALSE ),
   m_unfadedImage(),
   m_grayImage(),
   m_fadeTime(),
   m_pmio(),
   m_greyImageCreated( FALSE )

{
    if (kapp->isX11CompositionAvailable()) {
        m_grayImage = TQImage( TQApplication::desktop()->width(), TQApplication::desktop()->height(), 32 );
        m_grayImage = m_grayImage.convertDepth(32);
        m_grayImage.setAlphaBuffer(false);
        m_grayImage.fill(0);	// Set the alpha buffer to 0 (fully transparent)
        m_grayImage.setAlphaBuffer(true);
    }
    else {
        // The hacks below aren't needed any more because Qt3 supports true transparency for the fading logout screen when composition is available
        DCOPRef("kicker", "KMenu").call("hideMenu");	// Make sure the K Menu is completely removed from the screen before taking a snapshot...
        m_grayImage = TQPixmap(TQPixmap::grabWindow(qt_xrootwin(), 0, 0, TQApplication::desktop()->width(), TQApplication::desktop()->height())).convertToImage();
    }
    m_unfadedImage = m_grayImage;
    resize(0, 0);
    setShown(true);
    TQTimer::singleShot( 500, this, TQT_SLOT( slotPaintEffect() ) );
}

// called after stopping shutdown-feedback -> smooth fade-back to color-mode
void KSMShutdownFeedback::fadeBack( void )
{
    m_fadeTime.restart();
    m_fadeBackwards = TRUE;
    // its possible that we have to fade back, before all is completely gray, so we cannot start
    // with completely gray when fading back...
    m_compensation = 1.0f - m_grayOpacity;
    // wait until we're completely back in color-mode...
    while ( m_grayOpacity > 0.0f )
    	    slotPaintEffect();
}

void KSMShutdownFeedback::slotPaintEffect()
{
    // determine which fade to use
   if (KConfigGroup(KGlobal::config(), "Logout").readBoolEntry("doFancyLogout", true))
    {

	float doFancyLogoutAdditionalDarkness  = (float)KConfigGroup(KGlobal::config(), "Logout").readDoubleNumEntry("doFancyLogoutAdditionalDarkness", 0.6);

	float doFancyLogoutFadeTime = (float)KConfigGroup(KGlobal::config(), "Logout").readDoubleNumEntry("doFancyLogoutFadeTime", 4000);

	float doFancyLogoutFadeBackTime = (float)KConfigGroup(KGlobal::config(), "Logout").readDoubleNumEntry("doFancyLogoutFadeBackTime", 1000);

	if (kapp->isX11CompositionAvailable()) {
		// We can do this in a different (simpler) manner because we have compositing support!
		// if slotPaintEffect() is called first time, we have to initialize the gray image
		// we also could do that in the constructor, but then the displaying of the
		// logout-UI would be too much delayed...
		if ( m_greyImageCreated == false )
		{
			m_greyImageCreated = true;

			// eliminate nasty flicker on first show
			m_root.resize( width(), height() );
			TQImage blendedImage = m_grayImage;
			TQPainter p;
                        p.begin( &m_root );
                        blendedImage.setAlphaBuffer(false);
                        p.drawImage( 0, 0, blendedImage );
                        p.end();

			setBackgroundPixmap( m_root );
			setGeometry( TQApplication::desktop()->geometry() );
			setBackgroundMode( TQWidget::NoBackground );

			m_unfadedImage = m_grayImage.copy();

			register uchar * r = m_grayImage.bits();
			uchar * end = m_grayImage.bits() + m_grayImage.numBytes();

			while ( r != end ) {
				*reinterpret_cast<TQRgb*>(r) = qRgba(0, 0, 0, 128);
				r += 4;
			}

			// start timer which is used for cpu-speed-independent fading
			m_fadeTime.start();
			m_rowsDone = 0;
		}

		// return if fading is completely done...
		if ( ( m_grayOpacity >= 1.0f && m_fadeBackwards == FALSE ) || ( m_grayOpacity <= 0.0f && m_fadeBackwards == TRUE ) )
			return;


		if ( m_fadeBackwards == FALSE )
		{
			m_grayOpacity = m_fadeTime.elapsed() / doFancyLogoutFadeTime;
			if ( m_grayOpacity > 1.0f )
			m_grayOpacity = 1.0f;
		}
		else
		{
			m_grayOpacity = 1.0f - m_fadeTime.elapsed() / doFancyLogoutFadeBackTime - m_compensation;
			if ( m_grayOpacity < 0.0f )
			m_grayOpacity = 0.0f;
		}

		const int imgWidth = m_unfadedImage.width();
		int imgHeight = m_unfadedImage.height();
		int heightUnit = imgHeight / 3;
		if( heightUnit < 1 )
			heightUnit = 1;

		int y1 = static_cast<int>( imgHeight*m_grayOpacity - heightUnit + m_grayOpacity*heightUnit*2.0f );
		if( y1 > imgHeight )
			y1 = imgHeight;

		int y2 = y1+heightUnit;
		if( y2 > imgHeight )
			y2 = imgHeight;

		if( m_fadeBackwards == FALSE )
		{
			if( y1 > 0 && y1 < imgHeight && y1-m_rowsDone > 0 && m_rowsDone < imgHeight )
			{
				TQImage img( imgWidth, y1-m_rowsDone, 32 );
				memcpy( img.bits(), m_grayImage.scanLine( m_rowsDone ), imgWidth*(y1-m_rowsDone)*4 );
				bitBlt( this, 0, m_rowsDone, &img );
				m_rowsDone = y1;
			}
		}
		else
		{
			// when fading back we have to blit area which isnt gray anymore to unfaded image
			if( y2 > 0 && y2 < imgHeight && m_rowsDone > y2 )
			{
				TQImage img( imgWidth, m_rowsDone-y2, 32 );
				memcpy( img.bits(), m_unfadedImage.scanLine( y2 ), imgWidth*(m_rowsDone-y2)*4 );
				bitBlt( this, 0, y2, &img );
				m_rowsDone = y2;
			}
		}

		int start_y1 = y1;
		if( start_y1 < 0 )
			start_y1 = 0;
		if( y2 > start_y1 )
		{
			TQImage img( imgWidth, y2-start_y1, 32 );
			memcpy( img.bits(), m_grayImage.scanLine( start_y1 ), ( y2-start_y1 ) * imgWidth * 4 );
			register uchar * rs = m_unfadedImage.scanLine( start_y1 );
			register uchar * rd = img.bits();
			for( int y = start_y1; y < y2; ++y )
			{
			// linear gradients look bad, so use cos-function
				short int opac = static_cast<short int>( 128 - cosf( M_PI*(y-y1)/heightUnit )*128.0f );
				for( short int x = 0; x < imgWidth; ++x )
				{
					*reinterpret_cast<TQRgb*>(rd) = qRgba(0, 0, 0, ((255.0-opac)/(255.0/127.0)));
					rs += 4; rd += 4;
				}
			}
			bitBlt( this, 0, start_y1, &img );
		}

		TQTimer::singleShot( 5, this, TQT_SLOT( slotPaintEffect() ) );
	}
	else {
		// if slotPaintEffect() is called first time, we have to initialize the gray image
		// we also could do that in the constructor, but then the displaying of the
		// logout-UI would be too much delayed...
		if ( m_greyImageCreated == false )
		{
			m_greyImageCreated = true;

			setBackgroundMode( TQWidget::NoBackground );
			setGeometry( TQApplication::desktop()->geometry() );
			m_root.resize( width(), height() ); // for the default logout

			m_unfadedImage = m_grayImage.copy();

			register uchar * r = m_grayImage.bits();
			register uchar * g = m_grayImage.bits() + 1;
			register uchar * b = m_grayImage.bits() + 2;
			uchar * end = m_grayImage.bits() + m_grayImage.numBytes();

			while ( r != end ) {
				*r = *g = *b = (uchar) ( ( (*r)*11 + ((*g)<<4) + (*b)*5 ) * doFancyLogoutAdditionalDarkness / 32.0f );
				r += 4;
				g += 4;
				b += 4;
			}

			// start timer which is used for cpu-speed-independent fading
			m_fadeTime.start();
			m_rowsDone = 0;
		}
		
		// return if fading is completely done...
		if ( ( m_grayOpacity >= 1.0f && m_fadeBackwards == FALSE ) || ( m_grayOpacity <= 0.0f && m_fadeBackwards == TRUE ) )
			return;
		
		
		if ( m_fadeBackwards == FALSE )
		{
			m_grayOpacity = m_fadeTime.elapsed() / doFancyLogoutFadeTime;
			if ( m_grayOpacity > 1.0f )
			m_grayOpacity = 1.0f;
		}
		else
		{
			m_grayOpacity = 1.0f - m_fadeTime.elapsed() / doFancyLogoutFadeBackTime - m_compensation;
			if ( m_grayOpacity < 0.0f )
			m_grayOpacity = 0.0f;
		}
		
		const int imgWidth = m_unfadedImage.width();
		int imgHeight = m_unfadedImage.height();
		int heightUnit = imgHeight / 3;
		if( heightUnit < 1 )
			heightUnit = 1;
	
		int y1 = static_cast<int>( imgHeight*m_grayOpacity - heightUnit + m_grayOpacity*heightUnit*2.0f );
		if( y1 > imgHeight )
			y1 = imgHeight;
		
		int y2 = y1+heightUnit;
		if( y2 > imgHeight )
			y2 = imgHeight;
		
		if( m_fadeBackwards == FALSE )
		{
			if( y1 > 0 && y1 < imgHeight && y1-m_rowsDone > 0 && m_rowsDone < imgHeight )
			{
				TQImage img( imgWidth, y1-m_rowsDone, 32 );
				memcpy( img.bits(), m_grayImage.scanLine( m_rowsDone ), imgWidth*(y1-m_rowsDone)*4 );
				// conversion is slow as hell if desktop-depth != 24bpp...
				//Pixmap pm = m_pmio.convertToPixmap( img );
				//bitBlt( this, 0, m_rowsDone, &pm );
				//TQImage pm = m_pmio.convertToImage( img );
				bitBlt( this, 0, m_rowsDone, &img );
				m_rowsDone = y1;
			}
		}
		else
		{
			// when fading back we have to blit area which isnt gray anymore to unfaded image
			if( y2 > 0 && y2 < imgHeight && m_rowsDone > y2 )
			{
				TQImage img( imgWidth, m_rowsDone-y2, 32 );
				memcpy( img.bits(), m_unfadedImage.scanLine( y2 ), imgWidth*(m_rowsDone-y2)*4 );
				// conversion is slow as hell if desktop-depth != 24bpp...
				//TQPixmap pm = m_pmio.convertToPixmap( img );
				//bitBlt( this, 0, y2, &pm );
				bitBlt( this, 0, y2, &img );
				m_rowsDone = y2;
			}
		}
		
		int start_y1 = y1;
		if( start_y1 < 0 )
			start_y1 = 0;
		if( y2 > start_y1 )
		{
			TQImage img( imgWidth, y2-start_y1, 32 );
			memcpy( img.bits(), m_grayImage.scanLine( start_y1 ), ( y2-start_y1 ) * imgWidth * 4 );
			register uchar * rs = m_unfadedImage.scanLine( start_y1 );
			register uchar * gs = rs + 1;
			register uchar * bs = gs + 1;
			register uchar * rd = img.bits();
			register uchar * gd = rd + 1;
			register uchar * bd = gd + 1;
			for( int y = start_y1; y < y2; ++y )
			{
				// linear gradients look bad, so use cos-function
				short int opac = static_cast<short int>( 128 - cosf( M_PI*(y-y1)/heightUnit )*128.0f );
				for( short int x = 0; x < imgWidth; ++x )
				{
					*rd += ( ( ( *rs - *rd ) * opac ) >> 8 );
					rs += 4; rd += 4;
					*gd += ( ( ( *gs - *gd ) * opac ) >> 8 );
					gs += 4; gd += 4;
					*bd += ( ( ( *bs - *bd ) * opac ) >> 8 );
					bs += 4; bd += 4;
				}
			}
			// conversion is slow as hell if desktop-depth != 24bpp...
			//TQPixmap pm = m_pmio.convertToPixmap( img );
			//bitBlt( this, 0, start_y1, &pm );
			bitBlt( this, 0, start_y1, &img );
		}

		TQTimer::singleShot( 5, this, TQT_SLOT( slotPaintEffect() ) );
	}

    }
    // standard logout fade
    else
    {
         if (kapp->isX11CompositionAvailable()) {
		// We can do this in a different (simpler) manner because we have compositing support!
		// The end effect will be very similar to the old style logout
		float doFancyLogoutFadeTime = 1000;
		float doFancyLogoutFadeBackTime = 0;
		if ( m_greyImageCreated == false )
		{
			m_greyImageCreated = true;

			// eliminate nasty flicker on first show
			m_root.resize( width(), height() );
			TQImage blendedImage = m_grayImage;
			TQPainter p;
			p.begin( &m_root );
			blendedImage.setAlphaBuffer(false);
			p.drawImage( 0, 0, blendedImage );
			p.end();

			setBackgroundPixmap( m_root );
			setGeometry( TQApplication::desktop()->geometry() );
			setBackgroundMode( TQWidget::NoBackground );

			m_unfadedImage = m_grayImage.copy();

			register uchar * r = m_grayImage.bits();
			uchar * end = m_grayImage.bits() + m_grayImage.numBytes();

			while ( r != end ) {
				*reinterpret_cast<TQRgb*>(r) = qRgba(0, 0, 0, 107);
				r += 4;
			}

			// start timer which is used for cpu-speed-independent fading
			m_fadeTime.start();
			m_rowsDone = 0;
		}
		
		// return if fading is completely done...
		if ( ( m_grayOpacity >= 1.0f && m_fadeBackwards == FALSE ) || ( m_grayOpacity <= 0.0f && m_fadeBackwards == TRUE ) )
			return;
		
		
		if ( m_fadeBackwards == FALSE )
		{
			m_grayOpacity = m_fadeTime.elapsed() / doFancyLogoutFadeTime;
			if ( m_grayOpacity > 1.0f )
			m_grayOpacity = 1.0f;
		}
		else
		{
			m_grayOpacity = 1.0f - m_fadeTime.elapsed() / doFancyLogoutFadeBackTime - m_compensation;
			if ( m_grayOpacity < 0.0f )
			m_grayOpacity = 0.0f;
		}
		
		const int imgWidth = m_unfadedImage.width();
		int imgHeight = m_unfadedImage.height();
		int heightUnit = imgHeight / 3;
		if( heightUnit < 1 )
			heightUnit = 1;
	
		int y1 = static_cast<int>( imgHeight*m_grayOpacity - heightUnit + m_grayOpacity*heightUnit*2.0f );
		if( y1 > imgHeight )
			y1 = imgHeight;
		
		int y2 = y1+heightUnit;
		if( y2 > imgHeight )
			y2 = imgHeight;
		
		if( m_fadeBackwards == FALSE )
		{
			if( y1 > 0 && y1 < imgHeight && y1-m_rowsDone > 0 && m_rowsDone < imgHeight )
			{
				TQImage img( imgWidth, y1-m_rowsDone, 32 );
				memcpy( img.bits(), m_grayImage.scanLine( m_rowsDone ), imgWidth*(y1-m_rowsDone)*4 );
				bitBlt( this, 0, m_rowsDone, &img );
				m_rowsDone = y1;
			}
		}
		else
		{
			// when fading back we have to blit area which isnt gray anymore to unfaded image
			if( y2 > 0 && y2 < imgHeight && m_rowsDone > y2 )
			{
				TQImage img( imgWidth, m_rowsDone-y2, 32 );
				memcpy( img.bits(), m_unfadedImage.scanLine( y2 ), imgWidth*(m_rowsDone-y2)*4 );
				bitBlt( this, 0, y2, &img );
				m_rowsDone = y2;
			}
		}

		int start_y1 = y1;
		if( start_y1 < 0 )
			start_y1 = 0;
		if( y2 > start_y1 )
		{
			TQImage img( imgWidth, y2-start_y1, 32 );
			memcpy( img.bits(), m_grayImage.scanLine( start_y1 ), ( y2-start_y1 ) * imgWidth * 4 );
			register uchar * rs = m_unfadedImage.scanLine( start_y1 );
			register uchar * rd = img.bits();
			for( int y = start_y1; y < y2; ++y )
			{
			// linear gradients look bad, so use cos-function
				for( short int x = 0; x < imgWidth; ++x )
				{
					*reinterpret_cast<TQRgb*>(rd) = qRgba(0, 0, 0, 107);
					rs += 4; rd += 4;
				}
			}
			bitBlt( this, 0, start_y1, &img );
		}

		TQTimer::singleShot( 1, this, TQT_SLOT( slotPaintEffect() ) );
         }
         else {
	    if ( m_currentY >= height() ) {
	        if ( backgroundMode() == TQWidget::NoBackground ) {
	            setBackgroundMode( TQWidget::NoBackground );
	            setBackgroundPixmap( m_root );
	        }
	        return;
	    }

	    if ( m_currentY == 0 ) {
		KPixmap pixmap;
		pixmap = TQPixmap(TQPixmap::grabWindow( qt_xrootwin(), 0, 0, width(), height() ));
		bitBlt( this, 0, 0, &pixmap );
		bitBlt( &m_root, 0, 0, &pixmap );
	    }

	    KPixmap pixmap;
	    pixmap = TQPixmap(TQPixmap::grabWindow( qt_xrootwin(), 0, m_currentY, width(), 10 ));
	    TQImage image = pixmap.convertToImage();
	    KImageEffect::blend( Qt::black, image, 0.4 );
	    KImageEffect::toGray( image, true );
	    pixmap.convertFromImage( image );
	    bitBlt( this, 0, m_currentY, &pixmap );
	    bitBlt( &m_root, 0, m_currentY, &pixmap );
	    m_currentY += 10;
	    TQTimer::singleShot( 1, this, TQT_SLOT( slotPaintEffect() ) );
        }
    }

}

//////

KSMShutdownIPFeedback * KSMShutdownIPFeedback::s_pSelf = 0L;

KSMShutdownIPFeedback::KSMShutdownIPFeedback()
: TQWidget( 0L, "systemmodaldialogclass", Qt::WStyle_Customize | Qt::WStyle_NoBorder | Qt::WStyle_StaysOnTop ), m_timeout(0), m_isPainted(false), m_sharedRootPixmap(NULL), mPixmapTimeout(0)

{
	m_sharedRootPixmap = new KRootPixmap(this);
	m_sharedRootPixmap->setCustomPainting(true);
	connect(m_sharedRootPixmap, TQT_SIGNAL(backgroundUpdated(const TQPixmap &)), this, TQT_SLOT(slotSetBackgroundPixmap(const TQPixmap &)));

	if (TQPaintDevice::x11AppDepth() == 32) {
		// The shared pixmap is 24 bits, but we are 32 bits
		// Therefore our only option is to use a 24-bit Xorg application to dump the shared pixmap in a common (png) format for loading later
		TQString filename = getenv("USER");
		filename.prepend("/tmp/kde-");
		filename.append("/krootbacking.png");
		remove(filename.ascii());
		system("krootbacking &"); 
	}

	// eliminate nasty flicker on first show
	m_root.resize( kapp->desktop()->width(), kapp->desktop()->height() );
	TQImage blendedImage = TQImage( kapp->desktop()->width(), kapp->desktop()->height(), 32 );
	TQPainter p;
	p.begin( &m_root );
	blendedImage.setAlphaBuffer(false);
	p.drawImage( 0, 0, blendedImage );
	p.end();

	setBackgroundPixmap( m_root );
	setGeometry( TQApplication::desktop()->geometry() );
	setBackgroundMode( TQWidget::NoBackground );

	setShown(true);
}

void KSMShutdownIPFeedback::showNow()
{
	TQTimer::singleShot( 0, this, SLOT(slotPaintEffect()) );
}

KSMShutdownIPFeedback::~KSMShutdownIPFeedback()
{
	if (m_sharedRootPixmap) {
		m_sharedRootPixmap->stop();
		delete m_sharedRootPixmap;
	}
}

void KSMShutdownIPFeedback::fadeBack( void )
{

}

void KSMShutdownIPFeedback::slotSetBackgroundPixmap(const TQPixmap &rpm) {
	m_rootPixmap = rpm;
}

void KSMShutdownIPFeedback::slotPaintEffect()
{
	TQPixmap pm = m_rootPixmap;
	if (mPixmapTimeout == 0) {
		if (TQPaintDevice::x11AppDepth() != 32) {
			m_sharedRootPixmap->start();
		}

		TQTimer::singleShot( 100, this, SLOT(slotPaintEffect()) );
		mPixmapTimeout++;
		return;
	}
	if (TQPaintDevice::x11AppDepth() == 32) {
		TQString filename = getenv("USER");
		filename.prepend("/tmp/kde-");
		filename.append("/krootbacking.png");
		bool success = pm.load(filename, "PNG");
		if (!success) {
			pm = TQPixmap();
		}
	}
	if ((pm.isNull()) || (pm.width() != kapp->desktop()->width()) || (pm.height() != kapp->desktop()->height())) {
		if (mPixmapTimeout < 10) {
			TQTimer::singleShot( 100, this, SLOT(slotPaintEffect()) );
			mPixmapTimeout++;
			return;
		}
		else {
			pm = TQPixmap(kapp->desktop()->width(), kapp->desktop()->height());
			pm.fill(Qt::black);
		}
	}

	if (TQPaintDevice::x11AppDepth() == 32) {
		// Remove the alpha components from the image
		TQImage correctedImage = pm.convertToImage();
		correctedImage = correctedImage.convertDepth(32);
		correctedImage.setAlphaBuffer(true);
		int w = correctedImage.width();
		int h = correctedImage.height();
		for (int y = 0; y < h; ++y) {
			TQRgb *ls = (TQRgb *)correctedImage.scanLine( y );
			for (int x = 0; x < w; ++x) {
				TQRgb l = ls[x];
				int r = int( tqRed( l ) );
				int g = int( tqGreen( l ) );
				int b = int( tqBlue( l ) );
				int a = int( 255 );
				ls[x] = tqRgba( r, g, b, a );
			}
		}
		pm.convertFromImage(correctedImage);
	}

	setBackgroundPixmap( pm );
	move(0,0);
	setWindowState(WindowFullScreen);
	setGeometry( TQApplication::desktop()->geometry() );

	repaint(true);
	tqApp->flushX();

	m_isPainted = true;
}

//////

KSMShutdownDlg::KSMShutdownDlg( TQWidget* parent,
                                bool maysd, KApplication::ShutdownType sdtype )
  : TQDialog( parent, 0, TRUE, (WFlags)WType_Popup ), targets(0)
    // this is a WType_Popup on purpose. Do not change that! Not
    // having a popup here has severe side effects.

{
    TQVBoxLayout* vbox = new TQVBoxLayout( this );


    TQFrame* frame = new TQFrame( this );
    frame->setFrameStyle( TQFrame::StyledPanel | TQFrame::Raised );
    frame->setLineWidth( style().pixelMetric( TQStyle::PM_DefaultFrameWidth, frame ) );
	// we need to set the minimum size for the logout box, since it
	// gets too small if there isn't all options available
	frame->setMinimumWidth(400);
    vbox->addWidget( frame );
    vbox = new TQVBoxLayout( frame, 2 * KDialog::marginHint(),
                            2 * KDialog::spacingHint() );

	// default factor
	bool doUbuntuLogout = KConfigGroup(KGlobal::config(), "Logout").readBoolEntry("doUbuntuLogout", false);	

	// slighty more space for the new logout    
	int factor = 2;

	if(doUbuntuLogout) 
	{ 	
		factor = 8;
	}
	else {
		TQLabel* label = new TQLabel( i18n("End Session for \"%1\"").arg(KUser().loginName()), frame );
		TQFont fnt = label->font();
		fnt.setBold( true );
		fnt.setPointSize( fnt.pointSize() * 3 / 2 );
		label->setFont( fnt );
		vbox->addWidget( label, 0, AlignHCenter );
	}

	// for the basic layout, within this box either the ubuntu dialog or
	// standard konqy+buttons will be placed.
	TQHBoxLayout* hbox = new TQHBoxLayout( vbox, factor * KDialog::spacingHint() );

	// from here on we have to adapt to the two different dialogs
	TQFrame* lfrm;
	TQVBoxLayout* buttonlay;
	TQHBoxLayout* hbuttonbox;
	TQFont btnFont;

	if(doUbuntuLogout)
	{
		// first line of buttons
		hbuttonbox = new TQHBoxLayout( hbox, factor * KDialog::spacingHint() );
		hbuttonbox->setAlignment( Qt::AlignHCenter );
		// End session
		FlatButton* btnLogout = new FlatButton( frame );
		btnLogout->setTextLabel( TQString("&") + i18n("Log out"), false );
		btnLogout->setPixmap( DesktopIcon( "back") );
		int i = btnLogout->textLabel().find( TQRegExp("\\&"), 0 );    // i == 1
		btnLogout->setAccel( "ALT+" + btnLogout->textLabel().lower()[i+1] ) ;
		hbuttonbox->addWidget ( btnLogout );
		connect(btnLogout, TQT_SIGNAL(clicked()), TQT_SLOT(slotLogout()));

	}
	else
	{

		// konqy
		lfrm = new TQFrame( frame );
		lfrm->setFrameStyle( TQFrame::Panel | TQFrame::Sunken );
		hbox->addWidget( lfrm, AlignCenter );

		buttonlay = new TQVBoxLayout( hbox, factor * KDialog::spacingHint() );
		buttonlay->setAlignment( Qt::AlignHCenter );

		TQLabel* icon = new TQLabel( lfrm );
		icon->setPixmap( UserIcon( "shutdownkonq" ) );
		lfrm->setFixedSize( icon->sizeHint());
		icon->setFixedSize( icon->sizeHint());

		buttonlay->addStretch( 1 );
		// End session
		KPushButton* btnLogout = new KPushButton( KGuiItem( i18n("&End Current Session"), "undo"), frame );
                TQToolTip::add( btnLogout, i18n( "<qt><h3>End Current Session</h3><p>Log out of the current session to login with a different user</p></qt>" ) );
		btnFont = btnLogout->font();
		buttonlay->addWidget( btnLogout );
		connect(btnLogout, TQT_SIGNAL(clicked()), TQT_SLOT(slotLogout()));
	}

	
#ifdef COMPILE_HALBACKEND
	m_halCtx = NULL;
#endif

	if (maysd) 	{

		// respect lock on resume & disable suspend/hibernate settings 
		// from power-manager
		KConfig config("power-managerrc");
		bool disableSuspend = config.readBoolEntry("disableSuspend", false);
		bool disableHibernate = config.readBoolEntry("disableHibernate", false);
		m_lockOnResume = config.readBoolEntry("lockOnResume", true);

		bool canSuspend = false;
		bool canHibernate = false;

#ifdef COMPILE_HALBACKEND
		// Query HAL for suspend/resume support
		m_halCtx = libhal_ctx_new();

		DBusError error;
		dbus_error_init(&error);
		m_dbusConn = dbus_connection_open_private(DBUS_SYSTEM_BUS, &error);
		if (!m_dbusConn)
		{
			dbus_error_free(&error);
			libhal_ctx_free(m_halCtx);
			m_halCtx = NULL;
		}
		else
		{
			dbus_bus_register(m_dbusConn, &error);
			if (dbus_error_is_set(&error))
			{
				dbus_error_free(&error);
				libhal_ctx_free(m_halCtx);
				m_dbusConn = NULL;
				m_halCtx = NULL;
			}
			else
			{
				libhal_ctx_set_dbus_connection(m_halCtx, m_dbusConn);
				if (!libhal_ctx_init(m_halCtx, &error))
				{
					if (dbus_error_is_set(&error))
						dbus_error_free(&error);
					libhal_ctx_free(m_halCtx);
					m_dbusConn = NULL;
					m_halCtx = NULL;
				}
			}
		}

		if (m_halCtx)
		{
			if (libhal_device_get_property_bool(m_halCtx,
												"/org/freedesktop/Hal/devices/computer",
												"power_management.can_suspend", 
												NULL))
			{
				canSuspend = true;
			}

			if (libhal_device_get_property_bool(m_halCtx, 
												"/org/freedesktop/Hal/devices/computer",
												"power_management.can_hibernate",
												NULL))
			{
				canHibernate = true;
			}
		}
#endif

		if(doUbuntuLogout) {

			if (canSuspend && !disableSuspend)
			{
				// Suspend
				FlatButton* btnSuspend = new FlatButton( frame );
				btnSuspend->setTextLabel( i18n("&Suspend"), false );
				btnSuspend->setPixmap( DesktopIcon( "suspend") );
			    int i = btnSuspend->textLabel().find( TQRegExp("\\&"), 0 );    // i == 1
				btnSuspend->setAccel( "ALT+" + btnSuspend->textLabel().lower()[i+1] ) ;
				hbuttonbox->addWidget ( btnSuspend);
				connect(btnSuspend, TQT_SIGNAL(clicked()), TQT_SLOT(slotSuspend()));
			}
		
			if (canHibernate && !disableHibernate)
			{
				// Hibernate
				FlatButton* btnHibernate = new FlatButton( frame );
		    	btnHibernate->setTextLabel( i18n("&Hibernate"), false );
				btnHibernate->setPixmap( DesktopIcon( "hibernate") );
				int i = btnHibernate->textLabel().find( TQRegExp("\\&"), 0 );    // i == 1
				btnHibernate->setAccel( "ALT+" + btnHibernate->textLabel().lower()[i+1] ) ;		
				hbuttonbox->addWidget ( btnHibernate);	
				connect(btnHibernate, TQT_SIGNAL(clicked()), TQT_SLOT(slotHibernate()));
			}

			// Separator (within buttonlay)
			vbox->addWidget( new KSeparator( frame ) );

			// bottom buttons
			TQHBoxLayout* hbuttonbox2 = new TQHBoxLayout( vbox, factor * KDialog::spacingHint() );
			hbuttonbox2->setAlignment( Qt::AlignHCenter );
			
			// Reboot
			FlatButton* btnReboot = new FlatButton( frame );
			btnReboot->setTextLabel( i18n("&Restart"), false );
			btnReboot->setPixmap( DesktopIcon( "reload") );
    		int i = btnReboot->textLabel().find( TQRegExp("\\&"), 0 );    // i == 1
			btnReboot->setAccel( "ALT+" + btnReboot->textLabel().lower()[i+1] ) ;
			hbuttonbox2->addWidget ( btnReboot);
			connect(btnReboot, TQT_SIGNAL(clicked()), TQT_SLOT(slotReboot()));
			if ( sdtype == KApplication::ShutdownTypeReboot )
				btnReboot->setFocus();

			// BAD CARMA .. this code is copied line by line from standard konqy dialog
			int def, cur;
			if ( DM().bootOptions( rebootOptions, def, cur ) ) {
			btnReboot->setPopupDelay(300); // visually add dropdown
			targets = new TQPopupMenu( frame );
			if ( cur == -1 )
				cur = def;
		
			int index = 0;
			for (TQStringList::ConstIterator it = rebootOptions.begin(); it != rebootOptions.end(); ++it, ++index)
				{
					TQString label = (*it);
					label=label.replace('&',"&&");
				if (index == cur)
				targets->insertItem( label + i18n("current option in boot loader", " (current)"), index);
				else
				targets->insertItem( label, index );
				}
		
			btnReboot->setPopup(targets);
			connect( targets, TQT_SIGNAL(activated(int)), TQT_SLOT(slotReboot(int)) );
			}
			// BAD CARMA .. this code is copied line by line from standard konqy dialog [EOF]
 		
			// Shutdown
			FlatButton* btnHalt = new FlatButton( frame );
			btnHalt->setTextLabel( i18n("&Turn Off"), false );
			btnHalt->setPixmap( DesktopIcon( "exit") );
			i = btnHalt->textLabel().find( TQRegExp("\\&"), 0 );    // i == 1
			btnHalt->setAccel( "ALT+" + btnHalt->textLabel().lower()[i+1] ) ;
			hbuttonbox2->addWidget ( btnHalt );
			connect(btnHalt, TQT_SIGNAL(clicked()), TQT_SLOT(slotHalt()));
				if ( sdtype == KApplication::ShutdownTypeHalt )
					btnHalt->setFocus();

			// cancel buttonbox
			TQHBoxLayout* hbuttonbox3 = new TQHBoxLayout( vbox, factor * KDialog::spacingHint() );
			hbuttonbox3->setAlignment( Qt::AlignRight );

			// Back to Desktop
			KSMPushButton* btnBack = new KSMPushButton( KStdGuiItem::cancel(), frame );
			hbuttonbox3->addWidget( btnBack );
			connect(btnBack, TQT_SIGNAL(clicked()), TQT_SLOT(reject()));
	
		}
		else 
		{
			// Shutdown
			KPushButton* btnHalt = new KPushButton( KGuiItem( i18n("&Turn Off Computer"), "exit"), frame );
        TQToolTip::add( btnHalt, i18n( "<qt><h3>Turn Off Computer</h3><p>Log out of the current session and turn off the computer</p></qt>" ) );
			btnHalt->setFont( btnFont );
			buttonlay->addWidget( btnHalt );
			connect(btnHalt, TQT_SIGNAL(clicked()), TQT_SLOT(slotHalt()));
        if ( sdtype == KApplication::ShutdownTypeHalt || getenv("KDM_AUTOLOGIN") ) 
				btnHalt->setFocus();
	
			// Reboot
			KSMDelayedPushButton* btnReboot = new KSMDelayedPushButton( KGuiItem( i18n("&Restart Computer"), "reload"), frame );
        TQToolTip::add( btnReboot, i18n( "<qt><h3>Restart Computer</h3><p>Log out of the current session and restart the computer</p><p>Hold the mouse button or the space bar for a short while to get a list of options what to boot</p></qt>" ) );
			btnReboot->setFont( btnFont );
			buttonlay->addWidget( btnReboot );
	
			connect(btnReboot, TQT_SIGNAL(clicked()), TQT_SLOT(slotReboot()));
			if ( sdtype == KApplication::ShutdownTypeReboot )
				btnReboot->setFocus();
	
			// this section is copied as-is into ubuntulogout as well
			int def, cur;
			if ( DM().bootOptions( rebootOptions, def, cur ) ) {
			targets = new TQPopupMenu( frame );
			if ( cur == -1 )
				cur = def;
		
			int index = 0;
			for (TQStringList::ConstIterator it = rebootOptions.begin(); it != rebootOptions.end(); ++it, ++index)
				{
					TQString label = (*it);
					label=label.replace('&',"&&");
				if (index == cur)
				targets->insertItem( label + i18n("current option in boot loader", " (current)"), index);
				else
				targets->insertItem( label, index );
				}
		
			btnReboot->setPopup(targets);
			connect( targets, TQT_SIGNAL(activated(int)), TQT_SLOT(slotReboot(int)) );
			}
	
	
			if (canSuspend && !disableSuspend)
			{
				KPushButton* btnSuspend = new KPushButton( KGuiItem( i18n("&Suspend Computer"), "suspend"), frame );
				btnSuspend->setFont( btnFont );
				buttonlay->addWidget( btnSuspend );
				connect(btnSuspend, TQT_SIGNAL(clicked()), TQT_SLOT(slotSuspend()));
			}
	
			if (canHibernate && !disableHibernate)
			{
				KPushButton* btnHibernate = new KPushButton( KGuiItem( i18n("&Hibernate Computer"), "hibernate"), frame );
				btnHibernate->setFont( btnFont );
				buttonlay->addWidget( btnHibernate );
				connect(btnHibernate, TQT_SIGNAL(clicked()), TQT_SLOT(slotHibernate()));
			}

			buttonlay->addStretch( 1 );
		
			// Separator
			buttonlay->addWidget( new KSeparator( frame ) );

			// Back to Desktop
			KPushButton* btnBack = new KPushButton( KStdGuiItem::cancel(), frame );
			buttonlay->addWidget( btnBack );
			connect(btnBack, TQT_SIGNAL(clicked()), TQT_SLOT(reject()));

		}

	}
	else {
		// finish the dialog correctly
		if(doUbuntuLogout)
		{
			// cancel buttonbox
			TQHBoxLayout* hbuttonbox3 = new TQHBoxLayout( vbox, factor * KDialog::spacingHint() );
			hbuttonbox3->setAlignment( Qt::AlignRight );

			// Back to Desktop
			KSMPushButton* btnBack = new KSMPushButton( KStdGuiItem::cancel(), frame );
			hbuttonbox3->addWidget( btnBack );

			connect(btnBack, TQT_SIGNAL(clicked()), TQT_SLOT(reject()));
		}
		else 
		{
			// Separator
			buttonlay->addWidget( new KSeparator( frame ) );

			// Back to Desktop
			KPushButton* btnBack = new KPushButton( KStdGuiItem::cancel(), frame );
			buttonlay->addWidget( btnBack );

			connect(btnBack, TQT_SIGNAL(clicked()), TQT_SLOT(reject()));
		}


	}


}


KSMShutdownDlg::~KSMShutdownDlg()
{
#ifdef COMPILE_HALBACKEND
    if (m_halCtx)
    {
        DBusError error;
        dbus_error_init(&error);
        libhal_ctx_shutdown(m_halCtx, &error);
        libhal_ctx_free(m_halCtx);
    }
#endif
}


void KSMShutdownDlg::slotLogout()
{
    m_shutdownType = KApplication::ShutdownTypeNone;
    accept();
}


void KSMShutdownDlg::slotReboot()
{
    // no boot option selected -> current
    m_bootOption = TQString::null;
    m_shutdownType = KApplication::ShutdownTypeReboot;
    accept();
}

void KSMShutdownDlg::slotReboot(int opt)
{
    if (int(rebootOptions.size()) > opt)
        m_bootOption = rebootOptions[opt];
    m_shutdownType = KApplication::ShutdownTypeReboot;
    accept();
}


void KSMShutdownDlg::slotHalt()
{
    m_bootOption = TQString::null;
    m_shutdownType = KApplication::ShutdownTypeHalt;
    accept();
}

void KSMShutdownDlg::slotSuspend()
{
#ifdef COMPILE_HALBACKEND
    if (m_lockOnResume) {
        DCOPRef("kdesktop", "KScreensaverIface").send("lock");
    }

    if (m_dbusConn) 
    {
        DBusMessage *msg = dbus_message_new_method_call(
                              "org.freedesktop.Hal",
                              "/org/freedesktop/Hal/devices/computer", 
                              "org.freedesktop.Hal.Device.SystemPowerManagement", 
                              "Suspend");

        int wakeup=0;
	    dbus_message_append_args(msg, DBUS_TYPE_INT32, &wakeup, DBUS_TYPE_INVALID);

        dbus_connection_send(m_dbusConn, msg, NULL);

        dbus_message_unref(msg);
    }

    reject(); // continue on resume
#endif
}

void KSMShutdownDlg::slotHibernate()
{
#ifdef COMPILE_HALBACKEND
    if (m_lockOnResume) {
        DCOPRef("kdesktop", "KScreensaverIface").send("lock");
    }

    if (m_dbusConn) 
    {
        DBusMessage *msg = dbus_message_new_method_call(
                              "org.freedesktop.Hal",
                              "/org/freedesktop/Hal/devices/computer", 
                              "org.freedesktop.Hal.Device.SystemPowerManagement", 
                              "Hibernate");

        dbus_connection_send(m_dbusConn, msg, NULL);

        dbus_message_unref(msg);
    }

    reject(); // continue on resume
#endif
}

bool KSMShutdownDlg::confirmShutdown( bool maysd, KApplication::ShutdownType& sdtype, TQString& bootOption )
{
    kapp->enableStyles();
    KSMShutdownDlg* l = new KSMShutdownDlg( 0,
                                            //KSMShutdownFeedback::self(),
                                            maysd, sdtype );

    // Show dialog (will save the background in showEvent)
    TQSize sh = l->sizeHint();
    TQRect rect = KGlobalSettings::desktopGeometry(TQCursor::pos());

    l->move(rect.x() + (rect.width() - sh.width())/2,
            rect.y() + (rect.height() - sh.height())/2);
    bool result = l->exec();
    sdtype = l->m_shutdownType;
    bootOption = l->m_bootOption;

    delete l;

    kapp->disableStyles();
    return result;
}

TQWidget* KSMShutdownIPDlg::showShutdownIP()
{
    kapp->enableStyles();
    KSMShutdownIPDlg* l = new KSMShutdownIPDlg( 0 );

    kapp->disableStyles();

    return l;
}

KSMShutdownIPDlg::KSMShutdownIPDlg(TQWidget* parent)
  : KSMModalDialog( parent )

{
	setStatusMessage(i18n("Saving your settings..."));

	show();
	setActiveWindow();
}

KSMShutdownIPDlg::~KSMShutdownIPDlg()
{
}

KSMDelayedPushButton::KSMDelayedPushButton( const KGuiItem &item,
					    TQWidget *parent,
					    const char *name)
  : KPushButton( item, parent, name), pop(0), popt(0)
{
  connect(this, TQT_SIGNAL(pressed()), TQT_SLOT(slotPressed()));
  connect(this, TQT_SIGNAL(released()), TQT_SLOT(slotReleased()));
  popt = new TQTimer(this);
  connect(popt, TQT_SIGNAL(timeout()), TQT_SLOT(slotTimeout()));
}

void KSMDelayedPushButton::setPopup(TQPopupMenu *p)
{
  pop = p;
  setIsMenuButton(p != 0);
}

void KSMDelayedPushButton::slotPressed()
{
  if (pop)
    popt->start(TQApplication::startDragTime());
}

void KSMDelayedPushButton::slotReleased()
{
  popt->stop();
}

void KSMDelayedPushButton::slotTimeout()
{
  TQPoint bl = mapToGlobal(rect().bottomLeft());
  TQWidget *par = (TQWidget*)parent();
  TQPoint br = par->mapToGlobal(par->rect().bottomRight());
  pop->popup( bl );
  popt->stop();
  setDown(false);
}

KSMDelayedMessageBox::KSMDelayedMessageBox( KApplication::ShutdownType sdtype, const TQString &bootOption, int confirmDelay )
    : TimedLogoutDlg( 0, 0, true, (WFlags)WType_Popup ), m_remaining(confirmDelay)
{
    if ( sdtype == KApplication::ShutdownTypeHalt )
    {
        m_title->setText( i18n( "Would you like to turn off your computer?" ) );
        m_template = i18n( "This computer will turn off automatically\n"
                           "after %1 seconds." );
        m_logo->setPixmap( BarIcon( "exit", 48 ) );
    } else if ( sdtype == KApplication::ShutdownTypeReboot )
    {
        if (bootOption.isEmpty())
            m_title->setText( i18n( "Would you like to reboot your computer?" ) );
        else
            m_title->setText( i18n( "Would you like to reboot to \"%1\"?" ).arg(bootOption) );
        m_template = i18n( "This computer will reboot automatically\n"
                           "after %1 seconds." );
        m_logo->setPixmap( BarIcon( "reload", 48 ) );
    } else {
        m_title->setText( i18n( "Would you like to end your current session?" ) );
        m_template = i18n( "This session will end\n"
                           "after %1 seconds automatically." );
        m_logo->setPixmap( BarIcon( "previous", 48 ) );
    }

    updateText();
    adjustSize();
    if (  double( height() ) / width() < 0.25 )
    {
        setFixedHeight( tqRound( width() * 0.3 ) );
        adjustSize();
    }
    TQTimer *timer = new TQTimer( this );
    timer->start( 1000 );
    connect( timer, TQT_SIGNAL( timeout() ), TQT_SLOT( updateText() ) );
    KDialog::centerOnScreen(this);
}

void KSMDelayedMessageBox::updateText()
{
    m_remaining--;
    if ( m_remaining == 0 )
    {
        accept();
        return;
    }
    m_text->setText( m_template.arg( m_remaining ) );
}

bool KSMDelayedMessageBox::showTicker( KApplication::ShutdownType sdtype, const TQString &bootOption, int confirmDelay )
{
    kapp->enableStyles();
    KSMDelayedMessageBox msg( sdtype, bootOption, confirmDelay );
    TQSize sh = msg.sizeHint();
    TQRect rect = KGlobalSettings::desktopGeometry(TQCursor::pos());

    msg.move(rect.x() + (rect.width() - sh.width())/2,
            rect.y() + (rect.height() - sh.height())/2);
    bool result = msg.exec();

    kapp->disableStyles();
    return result;
}

KSMPushButton::KSMPushButton( const KGuiItem &item,
					    TQWidget *parent,
					    const char *name)
  : KPushButton( item, parent, name),
    m_pressed(false)
{
	setDefault( false );
	setAutoDefault ( false );	
}


void KSMPushButton::keyPressEvent( TQKeyEvent* e )
{
switch ( e->key() ) 
  {
    case Key_Enter:
    case Key_Return:
    case Key_Space:
      m_pressed = TRUE;
	  setDown(true);
      emit pressed();
      break;
	case Key_Escape:
		e->ignore();
	break;
    default:
      e->ignore();
    }

	TQPushButton::keyPressEvent(e);
}


void KSMPushButton::keyReleaseEvent( TQKeyEvent* e )
{
	switch ( e->key() ) 
	{
		case Key_Space:
		case Key_Enter:
		case Key_Return:
			if ( m_pressed ) 
			{
			setDown(false);
			m_pressed = FALSE;
			emit released();
			emit clicked();
			}
		break;
		case Key_Escape:
			e->ignore();
		break;
		default:
			e->ignore();
	}

}




FlatButton::FlatButton( TQWidget *parent, const char *name )
  : TQToolButton( parent, name/*, TQt::WNoAutoErase*/ ),
    m_pressed(false)
{
  init();
}


FlatButton::~FlatButton() {}


void FlatButton::init()
{
	setUsesTextLabel(true);
	setUsesBigPixmap(true);
	setAutoRaise(true);
	setTextPosition( TQToolButton::Under );
	setFocusPolicy(TQ_StrongFocus);	
 }


void FlatButton::keyPressEvent( TQKeyEvent* e )
{
	switch ( e->key() ) 
	{
		case Key_Enter:
		case Key_Return:
		case Key_Space:
		m_pressed = TRUE;
		setDown(true);
		emit pressed();
		break;
		case Key_Escape:
			e->ignore();
		break;
		default:
			e->ignore();
	}

	TQToolButton::keyPressEvent(e);
}


void FlatButton::keyReleaseEvent( TQKeyEvent* e )
{
	switch ( e->key() ) 
	{
		case Key_Space:
		case Key_Enter:
		case Key_Return:
			if ( m_pressed ) 
			{
			setDown(false);
			m_pressed = FALSE;
			emit released();
			emit clicked();
			}
		break;
		case Key_Escape:
			e->ignore();
		break;
		default:
			e->ignore();
	}

}


