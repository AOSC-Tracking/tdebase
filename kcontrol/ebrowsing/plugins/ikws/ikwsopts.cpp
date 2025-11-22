/*
 * Copyright (c) 2000 Yves Arrouye <yves@realnames.com>
 * Copyright (c) 2001, 2002 Dawit Alemayehu <adawit@kde.org>
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

#include <tqcheckbox.h>
#include <tqfile.h>
#include <tqgroupbox.h>
#include <tqheader.h>
#include <tqlabel.h>
#include <tqlayout.h>
#include <tqpushbutton.h>
#include <tqvbox.h>
#include <tqwhatsthis.h>

#include <kdebug.h>
#include <tdeglobal.h>
#include <dcopref.h>
#include <tdeapplication.h>
#include <kcombobox.h>
#include <tdeconfig.h>
#include <kiconloader.h>
#include <tdelistview.h>
#include <tdelocale.h>
#include <tdemessagebox.h>
#include <kservice.h>
#include <tdesimpleconfig.h>
#include <tdestandarddirs.h>
#include <ktrader.h>

#include "ikwsopts.h"
#include "ikwsopts_ui.h"
#include "kuriikwsfiltereng.h"
#include "searchprovider.h"
#include "searchproviderdlg.h"


class CategoryItem : public TQListViewItem
{
  public:
    CategoryItem(TQListView *parent, TQString category)
    : TQListViewItem(parent), m_category(category)
    {
      if (category.isNull())
          m_category = "Misc";

      setText(0, displayName());
    }

    const TQString& key() const { return m_category; }

    const TQString& displayName() const
    {
        return SearchProvider::searchCategoryName(m_category);
    }

  private:
    TQString m_category;
};


class SearchProviderItem : public TQCheckListItem
{
public:
    SearchProviderItem(TQListView *parent, SearchProvider *provider)
    :TQCheckListItem(parent, provider->name(), CheckBox), m_provider(provider)
    {
      update();
    }

    virtual ~SearchProviderItem()
    {
      delete m_provider;
    }

    void update()
    {
      setText(0, m_provider->name());
      setText(1, m_provider->keys().join(","));
    }

    SearchProvider *provider() const { return m_provider; }

private:
    SearchProvider *m_provider;
};

FilterOptions::FilterOptions(TDEInstance *instance, TQWidget *parent, const char *name)
              :TDECModule(instance, parent, name)
{
    TQVBoxLayout *mainLayout = new TQVBoxLayout( this, KDialog::marginHint(),
        KDialog::spacingHint());

    m_dlg = new FilterOptionsUI (this);
    mainLayout->addWidget(m_dlg);

    m_dlg->lvSearchProviders->header()->setLabel(0, SmallIconSet("bookmark"), i18n("Name"));
    m_dlg->lvSearchProviders->setSorting(0);

    // Load the options
    load();
}

TQString FilterOptions::quickHelp() const
{
    return i18n("In this module you can configure the web shortcuts feature. "
                "Web shortcuts allow you to quickly search or lookup words on "
                "the Internet. For example, to search for information about the "
                "TDE project using the Google engine, you simply type <b>gg:TDE</b> "
                "or <b>google:TDE</b>."
                "<p>If you select a default search engine, normal words or phrases "
                "will be looked up at the specified search engine by simply typing "
                "them into applications, such as Konqueror, that have built-in support "
                "for such a feature.");
}

void FilterOptions::load()
{
   load( false );
}

void FilterOptions::load( bool useDefaults )
{
    // Clear state first.
    m_dlg->lvSearchProviders->clear();

    TDEConfig config( KURISearchFilterEngine::self()->name() + "rc", false, false );

    config.setReadDefaults( useDefaults );

    config.setGroup("General");

    TQString defaultSearchEngine = config.readEntry("DefaultSearchEngine");

    m_favoriteEngines.clear();
    m_favoriteEngines << "duckduckgo" << "wikipedia_en";
    m_favoriteEngines = config.readListEntry("FavoriteSearchEngines", m_favoriteEngines);

    const TDETrader::OfferList services = TDETrader::self()->query("SearchProvider");

    for (TDETrader::OfferList::ConstIterator it = services.begin();
         it != services.end(); ++it)
    {
      displaySearchProvider(new SearchProvider(*it),
                            ((*it)->desktopEntryName() == defaultSearchEngine));
    }

    bool webShortcutsEnabled = config.readBoolEntry("EnableWebShortcuts", true);
    m_dlg->cbEnableShortcuts->setChecked( webShortcutsEnabled );

    setDelimiter (config.readNumEntry ("KeywordDelimiter", ':'));

    // Update the GUI to reflect the config options read above...
    setWebShortcutState();

    if (m_dlg->lvSearchProviders->childCount())
      m_dlg->lvSearchProviders->setSelected(m_dlg->lvSearchProviders->firstChild(), true);

    // Connect all the signals/slots...
    connect(m_dlg->cbEnableShortcuts, TQ_SIGNAL(clicked()), this,
            TQ_SLOT(setWebShortcutState()));
    connect(m_dlg->cbEnableShortcuts, TQ_SIGNAL(clicked()), this,
            TQ_SLOT(configChanged()));
    connect(m_dlg->cbCategories, TQ_SIGNAL(toggled(bool)), this,
            TQ_SLOT(setCategoriesShown(bool)));

    connect(m_dlg->lvSearchProviders, TQ_SIGNAL(selectionChanged(TQListViewItem *)),
           this, TQ_SLOT(updateSearchProvider()));
    connect(m_dlg->lvSearchProviders, TQ_SIGNAL(doubleClicked(TQListViewItem *)),
           this, TQ_SLOT(changeSearchProvider()));
    connect(m_dlg->lvSearchProviders, TQ_SIGNAL(returnPressed(TQListViewItem *)),
           this, TQ_SLOT(changeSearchProvider()));
    connect(m_dlg->lvSearchProviders, TQ_SIGNAL(executed(TQListViewItem *)),
           this, TQ_SLOT(checkFavoritesChanged()));
    connect(m_dlg->lvSearchProviders, TQ_SIGNAL(spacePressed(TQListViewItem *)),
           this, TQ_SLOT(checkFavoritesChanged()));
    connect(m_dlg->lvSearchProviders, TQ_SIGNAL(pressed(TQListViewItem *)),
           this, TQ_SLOT(checkFavoritesChanged()));
    connect(m_dlg->lvSearchProviders, TQ_SIGNAL(clicked(TQListViewItem *)),
           this, TQ_SLOT(checkFavoritesChanged()));
    connect(m_dlg->lvSearchProviders, TQ_SIGNAL(expanded(TQListViewItem *)),
           this, TQ_SLOT(adjustColumns()));
    connect(m_dlg->lvSearchProviders, TQ_SIGNAL(collapsed(TQListViewItem *)),
           this, TQ_SLOT(adjustColumns()));


    connect(m_dlg->cmbDefaultEngine, TQ_SIGNAL(activated(const TQString &)), this,
            TQ_SLOT(configChanged()));
    connect(m_dlg->cmbDelimiter, TQ_SIGNAL(activated(const TQString &)), this,
            TQ_SLOT(configChanged()));

    connect(m_dlg->pbNew, TQ_SIGNAL(clicked()), this, TQ_SLOT(addSearchProvider()));
    connect(m_dlg->pbChange, TQ_SIGNAL(clicked()), this, TQ_SLOT(changeSearchProvider()));
    connect(m_dlg->pbDelete, TQ_SIGNAL(clicked()), this, TQ_SLOT(deleteSearchProvider()));

    adjustColumns();
    updateSearchProvider();
    emit changed( useDefaults );
}

char FilterOptions::delimiter ()
{
  switch (m_dlg->cmbDelimiter->currentItem())
  {
    case 1:
      return ' ';
    case 0:
    default:
      return ':';
  };
}

void FilterOptions::setDelimiter (char sep)
{
  switch (sep)
  {
    case ' ':
      m_dlg->cmbDelimiter->setCurrentItem (1);
      break;
    case ':':
    default:
      m_dlg->cmbDelimiter->setCurrentItem (0);
  };
}

void FilterOptions::save()
{
  TDEConfig config( KURISearchFilterEngine::self()->name() + "rc", false, false );

  config.setGroup("General");
  config.writeEntry("EnableWebShortcuts", m_dlg->cbEnableShortcuts->isChecked());
  config.writeEntry("KeywordDelimiter", delimiter() );

  TQString engine;

  if (m_dlg->cmbDefaultEngine->currentItem() != 0)
    engine = m_dlg->cmbDefaultEngine->currentText();

  config.writeEntry("DefaultSearchEngine", m_defaultEngineMap[engine]);

  // kdDebug () << "Engine: " << m_defaultEngineMap[engine] << endl;

  int changedProviderCount = 0;
  TQString path = tdeApp->dirs()->saveLocation("services", "searchproviders/");

  m_favoriteEngines.clear();

  for (TQListViewItemIterator it(m_dlg->lvSearchProviders); it.current(); ++it)
  {
    SearchProviderItem *item = dynamic_cast<SearchProviderItem *>(it.current());
    if (!item) continue;

    SearchProvider *provider = item->provider();

    TQString name = provider->desktopEntryName();

    if (item->isOn())
      m_favoriteEngines << name;

    if (provider->isDirty())
    {
      changedProviderCount++;

      if (name.isEmpty())
      {
        // New provider
        // Take the longest search shortcut as filename,
        // if such a file already exists, append a number and increase it
        // until the name is unique
        for (TQStringList::ConstIterator it = provider->keys().begin(); it != provider->keys().end(); ++it)
        {
            if ((*it).length() > name.length())
                name = (*it).lower();
        }
        for (int suffix = 0; ; ++suffix)
        {
            TQString located, check = name;
            if (suffix)
                check += TQString().setNum(suffix);
            if ((located = locate("services", "searchproviders/" + check + ".desktop")).isEmpty())
            {
                name = check;
                break;
            }
            else if (located.left(path.length()) == path)
            {
                // If it's a deleted (hidden) entry, overwrite it
                if (KService(located).isDeleted())
                    break;
            }
        }
      }

      TDESimpleConfig service(path + name + ".desktop");
      service.setGroup("Desktop Entry");
      service.writeEntry("Type", "Service");
      service.writeEntry("X-TDE-ServiceTypes", "SearchProvider");
      service.writeEntry("Name", provider->name());
      service.writeEntry("Query", provider->query(), true, false, true);
      service.writeEntry("Keys", provider->keys());
      service.writeEntry("Category", provider->category());
      service.writeEntry("Charset", provider->charset());

      // we might be overwriting a hidden entry
      service.writeEntry("Hidden", false);
    }
  }

  for (TQStringList::ConstIterator it = m_deletedProviders.begin();
      it != m_deletedProviders.end(); ++it)
  {
      TQStringList matches = tdeApp->dirs()->findAllResources("services", "searchproviders/" + *it + ".desktop");

      // Shouldn't happen
      if (!matches.count())
          continue;

      if (matches.count() == 1 && matches[0].left(path.length()) == path)
      {
          // If only the local copy existed, unlink it
          // TODO: error handling
          TQFile::remove(matches[0]);
          continue;
      }
      TDESimpleConfig service(path + *it + ".desktop");
      service.setGroup("Desktop Entry");
      service.writeEntry("Type", "Service");
      service.writeEntry("X-TDE-ServiceTypes", "SearchProvider");
      service.writeEntry("Hidden", true);
  }

  config.writeEntry("FavoriteSearchEngines", m_favoriteEngines);
  config.sync();

  emit changed(false);

  // Update filters in running applications...
  (void) DCOPRef("*", "KURIIKWSFilterIface").send("configure");
  (void) DCOPRef("*", "KURISearchFilterIface").send("configure");

  // If the providers changed, tell sycoca to rebuild its database...
  if (changedProviderCount)
    KService::rebuildKSycoca(this);
}

void FilterOptions::defaults()
{
   load( true );
}

void FilterOptions::configChanged()
{
  // kdDebug () << "FilterOptions::configChanged: true" << endl;
  emit changed(true);
}

void FilterOptions::checkFavoritesChanged()
{
  TQStringList currentFavoriteEngines;
  for (TQListViewItemIterator it(m_dlg->lvSearchProviders); it.current(); ++it)
  {
    SearchProviderItem *item = dynamic_cast<SearchProviderItem *>(it.current());
    if (!item) continue;

    if (item->isOn())
      currentFavoriteEngines << item->provider()->desktopEntryName();
  }

  if (!(currentFavoriteEngines==m_favoriteEngines)) {
    m_favoriteEngines=currentFavoriteEngines;
    configChanged();
  }
}

void FilterOptions::setWebShortcutState()
{
  bool use_keywords = m_dlg->cbEnableShortcuts->isChecked();
  m_dlg->lvSearchProviders->setEnabled(use_keywords);
  m_dlg->pbNew->setEnabled(use_keywords);
  m_dlg->pbChange->setEnabled(use_keywords);
  m_dlg->pbDelete->setEnabled(use_keywords);
  m_dlg->lbDelimiter->setEnabled (use_keywords);
  m_dlg->cmbDelimiter->setEnabled (use_keywords);
  m_dlg->lbDefaultEngine->setEnabled (use_keywords);
  m_dlg->cmbDefaultEngine->setEnabled (use_keywords);
}

void FilterOptions::adjustColumns()
{
    m_dlg->lvSearchProviders->adjustColumn(1);
    m_dlg->lvSearchProviders->adjustColumn(2);
}

void FilterOptions::setCategoriesShown(bool shown)
{
    if (shown)
    {
        m_dlg->lvSearchProviders->removeColumn(2);
    }
    else
    {
        m_dlg->lvSearchProviders->addColumn(i18n("Category"));
        adjustColumns();
    }

    // Move search provider items between the list root and the appropriate
    // category items.
    TQListViewItemIterator it(m_dlg->lvSearchProviders);
    while (it.current())
    {
        SearchProviderItem *sp = dynamic_cast<SearchProviderItem*>(it.current());
        ++it;

        if (!sp) continue;

        TQString category = sp->provider()->category();

        if (shown)
        {
            if (sp->depth() > 0) continue;

            if (!m_categories.contains(category))
            {
                m_categories[category] = new CategoryItem(m_dlg->lvSearchProviders, category);
            }

            m_dlg->lvSearchProviders->takeItem(sp);
            m_categories[category]->insertItem(sp);
        }
        else
        {
            if (sp->depth() < 1) continue;

            sp->parent()->takeItem(sp);
            m_dlg->lvSearchProviders->insertItem(sp);

            sp->setText(2, category);
        }
    }

    // Delete category items if no longer needed
    if (!shown)
    {
        TQListViewItem *item = m_dlg->lvSearchProviders->firstChild();
        TQListViewItem *next;
        do
        {
            next = item->nextSibling();
            if (dynamic_cast<CategoryItem *>(item))
                delete item;
        }
        while (item = next);
        m_categories.clear();
    }

    m_dlg->lvSearchProviders->adjustColumn(0);
    m_dlg->lvSearchProviders->setTreeStepSize(shown ? 20 : 0);
    m_dlg->lvSearchProviders->sort();
}

const TQString& FilterOptions::getCurrentCategory()
{
    TQListViewItem *item = m_dlg->lvSearchProviders->currentItem();
    if (!item) return TQString::null;

    CategoryItem *cat = dynamic_cast<CategoryItem*>(item);
    if (cat) return cat->key();

    SearchProviderItem *sp = dynamic_cast<SearchProviderItem*>(item);
    if (sp) return sp->provider()->category();

    return TQString::null;
}

void FilterOptions::addSearchProvider()
{
    SearchProviderDialog dlg(0, getCurrentCategory(), this);
    if (dlg.exec())
    {
        m_dlg->lvSearchProviders->setSelected(displaySearchProvider(dlg.provider()), true);
        configChanged();
    }
}

void FilterOptions::changeSearchProvider()
{
  SearchProviderItem *item = dynamic_cast<SearchProviderItem *>(m_dlg->lvSearchProviders->currentItem());
  if (!item) return;

  SearchProviderDialog dlg(item->provider(), item->provider()->category(), this);

  if (dlg.exec())
  {
    m_dlg->lvSearchProviders->setSelected(displaySearchProvider(dlg.provider()), true);

    if (m_dlg->cbCategories->isChecked())
    {
        // check if we need to move the item to another category
        CategoryItem *category = static_cast<CategoryItem*>(item->parent());

        if (category->key() != dlg.provider()->category())
        {
            TQString c = dlg.provider()->category();
            if (!m_categories.contains(c))
            {
                m_categories[c] = new CategoryItem(m_dlg->lvSearchProviders, c);
            }
            CategoryItem *newCategory = m_categories[c];

            category->takeItem(item);
            newCategory->insertItem(item);

            if (category->childCount() < 1)
            {
                delete category;
            }
        }
    }

    configChanged();
  }
}

void FilterOptions::deleteSearchProvider()
{
  SearchProviderItem *item = dynamic_cast<SearchProviderItem *>(m_dlg->lvSearchProviders->currentItem());
  if (!item) return;

  // Update the combo box to go to None if the fallback was deleted.
  int current = m_dlg->cmbDefaultEngine->currentItem();
  for (int i = 1, count = m_dlg->cmbDefaultEngine->count(); i < count; ++i)
  {
    if (m_dlg->cmbDefaultEngine->text(i) == item->provider()->name())
    {
      m_dlg->cmbDefaultEngine->removeItem(i);
      if (i == current)
        m_dlg->cmbDefaultEngine->setCurrentItem(0);
      else if (current > i)
        m_dlg->cmbDefaultEngine->setCurrentItem(current - 1);

      break;
    }
  }

  if (item->nextSibling())
      m_dlg->lvSearchProviders->setSelected(item->nextSibling(), true);
  else if (item->itemAbove())
      m_dlg->lvSearchProviders->setSelected(item->itemAbove(), true);

  if (!item->provider()->desktopEntryName().isEmpty())
      m_deletedProviders.append(item->provider()->desktopEntryName());

  delete item;
  updateSearchProvider();
  configChanged();
}

void FilterOptions::updateSearchProvider()
{
  SearchProviderItem *item = dynamic_cast<SearchProviderItem*>(m_dlg->lvSearchProviders->currentItem());
  m_dlg->pbChange->setEnabled(item);
  m_dlg->pbDelete->setEnabled(item);
}

SearchProviderItem *FilterOptions::displaySearchProvider(SearchProvider *p, bool fallback)
{
  // Show the provider in the list.
  SearchProviderItem *item = nullptr;

  TQListViewItemIterator it(m_dlg->lvSearchProviders);

  for (; it.current(); ++it)
  {
    if (it.current()->text(0) == p->name())
    {
      item = dynamic_cast<SearchProviderItem *>(it.current());
      break;
    }
  }

  if (item)
    item->update ();
  else
  {
    // Put the name in the default search engine combo box.
    int itemCount;
    int totalCount = m_dlg->cmbDefaultEngine->count();

    item = new SearchProviderItem(m_dlg->lvSearchProviders, p);

    if (m_dlg->cbCategories->isChecked())
    {
        if (!m_categories.contains(p->category()))
        {
            m_categories[p->category()] = new CategoryItem(m_dlg->lvSearchProviders, p->category());
        }

        m_dlg->lvSearchProviders->takeItem(item);
        m_categories[p->category()]->insertItem(item);
    }

    if (m_favoriteEngines.find(p->desktopEntryName())!=m_favoriteEngines.end())
       item->setOn(true);

    for (itemCount = 1; itemCount < totalCount; itemCount++)
    {
      if (m_dlg->cmbDefaultEngine->text(itemCount) > p->name())
      {
        int currentItem = m_dlg->cmbDefaultEngine->currentItem();
        m_dlg->cmbDefaultEngine->insertItem(p->name(), itemCount);
        m_defaultEngineMap[p->name ()] = p->desktopEntryName ();
        if (currentItem >= itemCount)
          m_dlg->cmbDefaultEngine->setCurrentItem(currentItem+1);
        break;
      }
    }

    // Append it to the end of the list...
    if (itemCount == totalCount)
    {
      m_dlg->cmbDefaultEngine->insertItem(p->name(), itemCount);
      m_defaultEngineMap[p->name ()] = p->desktopEntryName ();
    }

    if (fallback)
      m_dlg->cmbDefaultEngine->setCurrentItem(itemCount);
  }

  if (!it.current())
    m_dlg->lvSearchProviders->sort();

  return item;
}

#include "ikwsopts.moc"
