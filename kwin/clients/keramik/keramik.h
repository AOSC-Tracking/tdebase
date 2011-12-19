/*
 *
 * Keramik KWin client (version 0.8)
 *
 * Copyright (C) 2002 Fredrik H�glund <fredrik@kde.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the license, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */


#ifndef __KERAMIK_H
#define __KERAMIK_H

#include <tqbutton.h>
#include <kdecoration.h>
#include <kdecorationfactory.h>

#include "tiles.h"

class TQSpacerItem;

namespace Keramik {

	enum TilePixmap  { TitleLeft=0, TitleCenter, TitleRight,
	                   CaptionSmallLeft, CaptionSmallCenter, CaptionSmallRight,
	                   CaptionLargeLeft, CaptionLargeCenter, CaptionLargeRight,
					   GrabBarLeft, GrabBarCenter, GrabBarRight,
	                   BorderLeft, BorderRight, NumTiles };

	enum Button      { MenuButton=0, OnAllDesktopsButton, HelpButton, MinButton,
	                   MaxButton, CloseButton, AboveButton, BelowButton, ShadeButton,
	                   NumButtons };

	enum ButtonDeco  { Menu=0, OnAllDesktops, NotOnAllDesktops, Help, Minimize, Maximize,
	                   Restore, Close, AboveOn, AboveOff, BelowOn, BelowOff, ShadeOn, ShadeOff,
                           NumButtonDecos };

	struct SettingsCache
	{
		bool largeGrabBars:1;
		bool smallCaptionBubbles:1;
	};

	class KeramikHandler : public KDecorationFactory
	{
		public:
			KeramikHandler();
			~KeramikHandler();

			virtual TQValueList< BorderSize > borderSizes() const;
			virtual bool reset( unsigned long changed );
                        virtual KDecoration* createDecoration( KDecorationBridge* );
			virtual bool supports( Ability ability );

			bool showAppIcons() const        { return showIcons; }
			bool useShadowedText() const     { return shadowedText; }
			bool largeCaptionBubbles() const { return !smallCaptionBubbles; }

			int titleBarHeight( bool large ) const {
				return ( large ? activeTiles[CaptionLargeCenter]->height()
						: activeTiles[CaptionSmallCenter]->height() );
			}

			int grabBarHeight() const
				{ return activeTiles[GrabBarCenter]->height(); }

			const TQPixmap *roundButton() const  { return titleButtonRound; }
			const TQPixmap *squareButton() const { return titleButtonSquare; }
			const TQBitmap *buttonDeco( ButtonDeco deco ) const
				{ return buttonDecos[ deco ]; }

			inline const TQPixmap *tile( TilePixmap tilePix, bool active ) const;

		private:
			void readConfig();
			void createPixmaps();
			void destroyPixmaps();

			void addWidth  (int width,  TQPixmap *&pix, bool left, TQPixmap *bottomPix);
			void addHeight (int height, TQPixmap *&pix);
			void flip( TQPixmap *&, TQPixmap *& );
			void pretile( TQPixmap *&, int, Qt::Orientation );
			TQPixmap *composite( TQImage *, TQImage * );
			TQImage  *loadImage( const TQString &, const TQColor & );
			TQPixmap *loadPixmap( const TQString &, const TQColor & );

			bool showIcons:1, shadowedText:1,
				smallCaptionBubbles:1, largeGrabBars:1;
			SettingsCache *settings_cache;
			KeramikImageDb *imageDb;

			TQPixmap *activeTiles[ NumTiles ];
			TQPixmap *inactiveTiles[ NumTiles ];
			TQBitmap *buttonDecos[ NumButtonDecos ];

			TQPixmap *titleButtonRound, *titleButtonSquare;

	}; // class KeramikHandler

	class KeramikClient;
	class KeramikButton : public TQButton
	{
		public:
			KeramikButton( KeramikClient *, const char *, Button, const TQString &, const int realizeBtns = Qt::LeftButton );
			~KeramikButton();

			ButtonState lastButton() const { return lastbutton; }

		private:
			void enterEvent( TQEvent * );
			void leaveEvent( TQEvent * );
			void mousePressEvent( TQMouseEvent * );
			void mouseReleaseEvent( TQMouseEvent * );
			void drawButton( TQPainter * );

		private:
			KeramikClient *client;
			Button button;
			bool hover;
			ButtonState lastbutton;
			int realizeButtons;
	}; // class KeramikButton


	class KeramikClient : public KDecoration
	{
		Q_OBJECT

		public:

			KeramikClient( KDecorationBridge* bridge, KDecorationFactory* factory );
			~KeramikClient();
                        virtual void init();
			virtual void reset( unsigned long changed );
			virtual Position mousePosition( const TQPoint& p ) const;
		    	virtual void borders( int& left, int& right, int& top, int& bottom ) const;
			virtual void resize( const TQSize& s );
			virtual TQSize minimumSize() const;
			virtual bool eventFilter( TQObject* o, TQEvent* e );
			virtual void activeChange();
			virtual void captionChange();
                        virtual void maximizeChange();
                        virtual void desktopChange();
                        virtual void shadeChange();

		private:
			void createLayout();
			void addButtons( TQBoxLayout*, const TQString & );
			void updateMask(); // FRAME
			void updateCaptionBuffer();
			void iconChange();
			void resizeEvent( TQResizeEvent *); // FRAME
			void paintEvent( TQPaintEvent *); // FRAME
			void mouseDoubleClickEvent( TQMouseEvent * ); // FRAME
			void wheelEvent( TQWheelEvent *); //FRAME
			int width() const { return widget()->width(); }
			int height() const { return widget()->height(); }

			void calculateCaptionRect();

			inline bool maximizedVertical() const {
				return ( maximizeMode() & MaximizeVertical );
			}
			bool isModalSystemNotification();

		private slots:
			void menuButtonPressed();
			void slotMaximize();
			void slotAbove();
			void slotBelow();
			void slotShade();
			void keepAboveChange( bool );
			void keepBelowChange( bool );

		private:
			TQSpacerItem   *topSpacer, *titlebar;
			KeramikButton *button[ NumButtons ];
			TQRect          captionRect;
			TQPixmap        captionBuffer;
			TQPixmap       *activeIcon, *inactiveIcon;
			bool           captionBufferDirty:1, maskDirty:1;
			bool           largeCaption:1, largeTitlebar:1;
	}; // class KeramikClient

} // namespace Keramik

#endif // ___KERAMIK_H

// vim: set noet ts=4 sw=4:
