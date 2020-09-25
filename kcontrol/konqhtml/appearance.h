// (c) Martin R. Jones 1996
// (c) Bernd Wuebben 1998
// KControl port & modifications
// (c) Torben Weis 1998
// End of the KControl port, added 'kfmclient configure' call.
// (c) David Faure 1998
// Cleanup and modifications for KDE 2.1
// (c) Daniel Molkentin 2000

#ifndef __APPEARANCE_H__
#define __APPEARANCE_H__

#include <tqwidget.h>
#include <tqmap.h>

#include <tdecmodule.h>

class TQSpinBox;
class TDEFontCombo;

class KAppearanceOptions : public TDECModule
{
  Q_OBJECT
public:
  KAppearanceOptions(TDEConfig *config, TQString group, TQWidget *parent=0, const char *name=0);
  ~KAppearanceOptions();

  virtual void load();
  virtual void load( bool useDefaults );
  virtual void save();
  virtual void defaults();

public slots:
  void slotFontSize( int );
  void slotMinimumFontSize( int );
  void slotStandardFont(const TQString& n);
  void slotFixedFont(const TQString& n);
  void slotSerifFont( const TQString& n );
  void slotSansSerifFont( const TQString& n );
  void slotCursiveFont( const TQString& n );
  void slotFantasyFont( const TQString& n );
  void slotEncoding( const TQString& n);
  void slotFontSizeAdjust( int value );

private:
  void updateGUI();

private:

  TDEConfig *m_pConfig;
  TQString m_groupname;
  TQStringList m_families;

  KIntNumInput* m_minSize;
  KIntNumInput* m_MedSize;
  KIntNumInput* m_pageDPI;
  TDEFontCombo* m_pFonts[6];
  TQComboBox* m_pEncoding;
  TQSpinBox *m_pFontSizeAdjust;

  int fSize;
  int fMinSize;
  TQStringList encodings;
  TQStringList fonts;
  TQStringList defaultFonts;
  TQString encodingName;
};

#endif // __APPEARANCE_H__
