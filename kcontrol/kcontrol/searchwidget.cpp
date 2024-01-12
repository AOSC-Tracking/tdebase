/*
  Copyright (c) 2000 Matthias Elter <elter@kde.org>
  Copyright (c) 2004 Daniel Molkentin <molkentin@kde.org>

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
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/

#include <tqlabel.h>
#include <tqlayout.h>
#include <tqregexp.h>

#include <klineedit.h>
#include <kiconloader.h>
#include <tdelocale.h>
#include <tdelistbox.h>

#include "searchwidget.h"
#include "searchwidget.moc"

/**
 * Helper class for sorting icon modules by name without losing the fileName ID
 */
class ModuleItem : public TQListBoxPixmap
{
public:
 ModuleItem(ConfigModule *module, TQListBox * listbox = 0) :
	TQListBoxPixmap(listbox,
      TDEGlobal::iconLoader()->loadIcon(module->icon(), TDEIcon::Desktop, TDEIcon::SizeSmall),
      module->moduleName())
  , m_module(module)
 { 
 
 }

 ConfigModule *module() const { return m_module; };

protected:
 ConfigModule *m_module;

};

KeywordListEntry::KeywordListEntry(const TQString& name, ConfigModule* module)
  : _name(name)
{
  if(module)
    _modules.append(module);
}

void KeywordListEntry::addModule(ConfigModule* module)
{
  if(module)
    _modules.append(module);
}

SearchWidget::SearchWidget(TQWidget *parent , const char *name)
  : TQWidget(parent, name)
{
  _keywords.setAutoDelete(true);

  TQVBoxLayout * l = new TQVBoxLayout(this, 0, 2);

  // keyword list
  _keyList = new TDEListBox(this);
  TQLabel *keyl = new TQLabel(_keyList, i18n("&Keywords:"), this);

  l->addWidget(keyl);
  l->addWidget(_keyList);

  // result list
  _resultList = new TDEListBox(this);
  TQLabel *resultl = new TQLabel(_resultList, i18n("&Results:"), this);

  l->addWidget(resultl);
  l->addWidget(_resultList);


  connect(_keyList, TQ_SIGNAL(highlighted(const TQString&)),
          this, TQ_SLOT(slotKeywordSelected(const TQString&)));

  connect(_resultList, TQ_SIGNAL(selected(TQListBoxItem*)),
          this, TQ_SLOT(slotModuleSelected(TQListBoxItem *)));
  connect(_resultList, TQ_SIGNAL(clicked(TQListBoxItem *)),
          this, TQ_SLOT(slotModuleClicked(TQListBoxItem *)));
}

void SearchWidget::populateKeywordList(ConfigModuleList *list)
{
  ConfigModule *module;

  // loop through all control modules
  for (module=list->first(); module != 0; module=list->next())
    {
      if (module->library().isEmpty())
        continue;

      // get the modules keyword list
      TQStringList kw = module->keywords();
      kw << module->moduleName();

      // loop through the keyword list to populate _keywords
      for(TQStringList::ConstIterator it = kw.begin(); it != kw.end(); ++it)
        {
          TQString name = (*it).lower();
          bool found = false;

          // look if _keywords already has an entry for this keyword
          for(KeywordListEntry *k = _keywords.first(); k != 0; k = _keywords.next())
            {
              // if there is an entry for this keyword, add the module to the entries modul list
              if (k->moduleName() == name)
                {
                  k->addModule(module);
                  found = true;
                  break;
                }
            }

          // if there is entry for this keyword, create a new one
          if (!found)
            {
              KeywordListEntry *k = new KeywordListEntry(name, module);
              _keywords.append(k);
            }
        }
    }
  populateKeyListBox("*");
}

void SearchWidget::populateKeyListBox(const TQString& s)
{
  _keyList->clear();

  TQStringList matches;

  for(KeywordListEntry *k = _keywords.first(); k != 0; k = _keywords.next())
    {
      if ( TQRegExp(s, false, true).search(k->moduleName()) >= 0)
        matches.append(k->moduleName().stripWhiteSpace());
    }

  for(TQStringList::ConstIterator it = matches.begin(); it != matches.end(); it++)
    _keyList->insertItem(*it);

  _keyList->sort();
}

void SearchWidget::populateResultListBox(const TQString& s)
{
  _resultList->clear();

  TQPtrList<ModuleItem> results;

  for(KeywordListEntry *k = _keywords.first(); k != 0; k = _keywords.next())
    {
      if (k->moduleName() == s)
        {
          TQPtrList<ConfigModule> modules = k->modules();

          for(ConfigModule *m = modules.first(); m != 0; m = modules.next())
              new ModuleItem(m, _resultList);
        }
    }

  _resultList->sort();
}

void SearchWidget::searchTextChanged(const TQString & s)
{
  TQString regexp = s;
  regexp += "*";
  populateKeyListBox(regexp);
  if (_keyList->count()==1)
    _keyList->setSelected(0,true);
}

void SearchWidget::slotKeywordSelected(const TQString & s)
{
  populateResultListBox(s);
}

void SearchWidget::slotModuleSelected(TQListBoxItem *item)
{
  if (item)
    emit moduleSelected( static_cast<ModuleItem*>(item)->module() );
}

void SearchWidget::slotModuleClicked(TQListBoxItem *item)
{
  if (item)
    emit moduleSelected( static_cast<ModuleItem*>(item)->module() );
}
