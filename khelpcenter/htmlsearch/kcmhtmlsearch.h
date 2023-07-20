/**
 * kcmhtmlsearch.h
 *
 * Copyright (c) 2000 Matthias Hölzer-Klüpfel <hoelzer@kde.org>
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

#ifndef __kcmhtmlsearch_h__
#define __kcmhtmlsearch_h__


#include <tdecmodule.h>


class TQLineEdit;
class TQCheckBox;
class TQPushButton;
class TDEListBox;
class TDEProcess;
class KLanguageCombo;
class KURLRequester;

class TDEHTMLSearchConfig : public TDECModule
{
  TQ_OBJECT

public:

  TDEHTMLSearchConfig(TQWidget *parent = 0L, const char *name = 0L);
  virtual ~TDEHTMLSearchConfig();
  
  void load();
  void save();
  void defaults();

  TQString quickHelp() const;
  
  int buttons();

  
protected slots:

  void configChanged();
  void addClicked(); 
  void delClicked();
  void pathSelected(const TQString &);
  void urlClicked(const TQString&);
  void generateIndex();

  void indexTerminated(TDEProcess *proc);

      
private:

  void checkButtons();
  void loadLanguages();

  KURLRequester *htdigBin, *htsearchBin, *htmergeBin;
  TQCheckBox *indexKDE, *indexMan, *indexInfo;
  TQPushButton *addButton, *delButton, *runButton;
  TDEListBox *searchPaths;
  KLanguageCombo *language;

  TDEProcess *indexProc;

};

#endif
