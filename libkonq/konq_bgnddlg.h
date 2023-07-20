/* This file is part of the KDE project
   Copyright (C) 1998, 1999 Torben Weis <weis@kde.org>
   Copyright (c) 1999 David Faure <fauren@kde.org>

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

#ifndef __konq_bgnd_h
#define __konq_bgnd_h

#include <tqstring.h>
#include <tqpixmap.h>

#include <kdialogbase.h>

class KColorButton;
class KURLRequester;
class TQButtonGroup;
class TQRadioButton;

/**
 * Dialog for configuring the background
 * Currently it defines and shows the pixmaps under the tiles resource
 */
class KonqBgndDialog : public KDialogBase
{
  TQ_OBJECT
public:
  /**
   * Constructor
   */
  KonqBgndDialog( TQWidget* parent, const TQString& pixmapFile,
                  const TQColor& theColor, const TQColor& defaultColor );
  ~KonqBgndDialog();

  TQColor color() const;
  const TQString& pixmapFile() const { return m_pixmapFile; }

private slots:
  void slotBackgroundModeChanged();
  void slotPictureChanged();
  void slotColorChanged();
  
private:
  void initPictures();
  void loadPicture( const TQString& fileName );

  TQColor m_color;
  TQPixmap m_pixmap;
  TQString m_pixmapFile;
  TQFrame* m_preview;
  
  TQButtonGroup* m_buttonGroup;
  TQRadioButton* m_radioColor;
  TQRadioButton* m_radioPicture;
  KURLRequester* m_comboPicture;
  KColorButton* m_buttonColor;
  
};

#endif
