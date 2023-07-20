/* This file is part of the KDE project
   Copyright (C) 1999 David Faure <faure@kde.org>

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

//-----------------------------------------------------------------------------
//
// Konqueror/KDesktop Fonts & Colors Options (for icon/tree view)
//
// (c) Martin R. Jones 1996
// (c) Bernd Wuebben 1998
//
// KControl port & modifications
// (c) Torben Weis 1998
//
// End of the KControl port by David
// Port to KControl 2 by MHK
// konqy adaptations by David

#ifndef __KONQFONT_OPTIONS_H__
#define __KONQFONT_OPTIONS_H__

#include <tqstringlist.h>
#include <tqspinbox.h>

#include <tdecmodule.h>

class TQCheckBox;
class TQRadioButton;

class KColorButton;
class TDEConfig;
class TDEFontCombo;


//-----------------------------------------------------------------------------

class KonqFontOptions : public TDECModule
{
  TQ_OBJECT
public:
  KonqFontOptions(TDEConfig *config, TQString group, bool desktop, TQWidget *parent=0, const char *name=0);
  TQString quickHelp() const;

  virtual void load();
  virtual void load( bool readDefaults );
  virtual void save();
  virtual void defaults();
  virtual TQString handbookSection() const;

public slots:
  void slotFontSize(int i);
  void slotStandardFont(const TQString& n);
  void slotTextBackgroundClicked();

  void slotNormalTextColorChanged( const TQColor &col );
  //void slotHighlightedTextColorChanged( const TQColor &col );
  void slotTextBackgroundColorChanged( const TQColor &col );

private slots:
  void slotPNbLinesChanged(int value);
  void slotPNbWidthChanged(int value);

private:
  void updateGUI();

private:

  TDEConfig *g_pConfig;
  TQString groupname;
  bool m_bDesktop;

  /*
  TQRadioButton* m_pSmall;
  TQRadioButton* m_pMedium;
  TQRadioButton* m_pLarge;
  */
  TDEFontCombo* m_pStandard;
  TQSpinBox* m_pSize;

  int m_fSize;
  TQString m_stdName;

  KColorButton* m_pBg;
  KColorButton* m_pNormalText;
  //KColorButton* m_pHighlightedText;
  TQCheckBox* m_cbTextBackground;
  KColorButton* m_pTextBackground;
  TQColor normalTextColor;
  //TQColor highlightedTextColor;
  TQColor textBackgroundColor;

  TQSpinBox* m_pNbLines;
  TQSpinBox* m_pNbWidth;
  TQCheckBox* cbUnderline;
  TQCheckBox* m_pSizeInBytes;
};

#endif
