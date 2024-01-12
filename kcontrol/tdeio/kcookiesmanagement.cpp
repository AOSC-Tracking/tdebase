/**
 * kcookiesmanagement.cpp - Cookies manager
 *
 * Copyright 2000-2001 Marco Pinelli <pinmc@orion.it>
 * Copyright (c) 2000-2001 Dawit Alemayehu <adawit@kde.org>
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

#include <tqapplication.h>
#include <tqlayout.h>
#include <tqpushbutton.h>
#include <tqgroupbox.h>
#include <tqhbox.h>
#include <tqlabel.h>
#include <tqtimer.h>
#include <tqdatetime.h>
#include <tqtoolbutton.h>

#include <kidna.h>
#include <kdebug.h>
#include <tdelocale.h>
#include <kdialog.h>
#include <kiconloader.h>
#include <klineedit.h>
#include <tdelistview.h>
#include <tdelistviewsearchline.h>
#include <tdemessagebox.h>
#include <dcopref.h>

#include "kcookiesmain.h"
#include "kcookiespolicies.h"
#include "kcookiesmanagement.h"
#include "kcookiesmanagementdlg_ui.h"

#include <assert.h>

struct CookieProp
{
    TQString host;
    TQString name;
    TQString value;
    TQString domain;
    TQString path;
    TQString expireDate;
    TQString secure;
    bool allLoaded;
};

CookieListViewItem::CookieListViewItem(TQListView *parent, TQString dom)
                   :TQListViewItem(parent)
{
    init( 0, dom );
}

CookieListViewItem::CookieListViewItem(TQListViewItem *parent, CookieProp *cookie)
                   :TQListViewItem(parent)
{
    init( cookie );
}

CookieListViewItem::~CookieListViewItem()
{
    delete mCookie;
}

void CookieListViewItem::init( CookieProp* cookie, TQString domain,
                               bool cookieLoaded )
{
    mCookie = cookie;
    mDomain = domain;
    mCookiesLoaded = cookieLoaded;
}

CookieProp* CookieListViewItem::leaveCookie()
{
    CookieProp *ret = mCookie;
    mCookie = 0;
    return ret;
}

TQString CookieListViewItem::text(int f) const
{
    if (mCookie)
        return f == 0 ? TQString() : KIDNA::toUnicode(mCookie->host);
    else
        return f == 0 ? KIDNA::toUnicode(mDomain) : TQString();
}

KCookiesManagement::KCookiesManagement(TQWidget *parent)
                   : TDECModule(parent, "kcmtdeio")
{
  // Toplevel layout
  TQVBoxLayout* mainLayout = new TQVBoxLayout(this, KDialog::marginHint(),
                                            KDialog::spacingHint());

  dlg = new KCookiesManagementDlgUI (this);

  dlg->tbClearSearchLine->setIconSet(SmallIconSet(TQApplication::reverseLayout() ? "clear_left" : "locationbar_erase"));
  dlg->kListViewSearchLine->setListView(dlg->lvCookies);

  mainLayout->addWidget(dlg);
  dlg->lvCookies->setSorting(0);

  connect(dlg->lvCookies, TQ_SIGNAL(expanded(TQListViewItem*)), TQ_SLOT(getCookies(TQListViewItem*)) );
  connect(dlg->lvCookies, TQ_SIGNAL(selectionChanged(TQListViewItem*)), TQ_SLOT(showCookieDetails(TQListViewItem*)) );

  connect(dlg->pbDelete, TQ_SIGNAL(clicked()), TQ_SLOT(deleteCookie()));
  connect(dlg->pbDeleteAll, TQ_SIGNAL(clicked()), TQ_SLOT(deleteAllCookies()));
  connect(dlg->pbReload, TQ_SIGNAL(clicked()), TQ_SLOT(getDomains()));
  connect(dlg->pbPolicy, TQ_SIGNAL(clicked()), TQ_SLOT(doPolicy()));

  connect(dlg->lvCookies, TQ_SIGNAL(doubleClicked (TQListViewItem *)), TQ_SLOT(doPolicy()));
  deletedCookies.setAutoDelete(true);
  m_bDeleteAll = false;
  mainWidget = parent;

  load();
}

KCookiesManagement::~KCookiesManagement()
{
}

void KCookiesManagement::load()
{
  reset();
  getDomains();
}

void KCookiesManagement::save()
{
  // If delete all cookies was requested!
  if(m_bDeleteAll)
  {
    if(!DCOPRef("kded", "kcookiejar").send("deleteAllCookies"))
    {
      TQString caption = i18n ("DCOP Communication Error");
      TQString message = i18n ("Unable to delete all the cookies as requested.");
      KMessageBox::sorry (this, message,caption);
      return;
    }
    m_bDeleteAll = false; // deleted[Cookies|Domains] have been cleared yet    
  }

  // Certain groups of cookies were deleted...
  TQStringList::Iterator dIt = deletedDomains.begin();
  while( dIt != deletedDomains.end() )
  {
    TQByteArray call;
    TQByteArray reply;
    TQCString replyType;
    TQDataStream callStream(call, IO_WriteOnly);
    callStream << *dIt;

    if( !DCOPRef("kded", "kcookiejar").send("deleteCookiesFromDomain", (*dIt)) )
    {
      TQString caption = i18n ("DCOP Communication Error");
      TQString message = i18n ("Unable to delete cookies as requested.");
      KMessageBox::sorry (this, message,caption);
      return;
    }

    dIt = deletedDomains.remove(dIt);
  }

  // Individual cookies were deleted...
  bool success = true; // Maybe we can go on...
  TQDictIterator<CookiePropList> cookiesDom(deletedCookies);

  while(cookiesDom.current())
  {
    CookiePropList *list = cookiesDom.current();
    TQPtrListIterator<CookieProp> cookie(*list);

    while(*cookie)
    {
      if( !DCOPRef("kded", "kcookiejar").send("deleteCookie",(*cookie)->domain,
                                              (*cookie)->host, (*cookie)->path,
                                              (*cookie)->name) )
      {
        success = false;
        break;
      }

      list->removeRef(*cookie);
    }

    if(!success)
      break;

    deletedCookies.remove(cookiesDom.currentKey());
  }

  emit changed( false );
}

void KCookiesManagement::defaults()
{
  reset();
  getDomains();
  emit changed (false);
}

void KCookiesManagement::reset()
{
  m_bDeleteAll = false;
  clearCookieDetails();
  dlg->lvCookies->clear();
  deletedDomains.clear();
  deletedCookies.clear();
  dlg->pbDelete->setEnabled(false);
  dlg->pbDeleteAll->setEnabled(false);
  dlg->pbPolicy->setEnabled(false);
}

void KCookiesManagement::clearCookieDetails()
{
  dlg->leName->clear();
  dlg->leValue->clear();
  dlg->leDomain->clear();
  dlg->lePath->clear();
  dlg->leExpires->clear();
  dlg->leSecure->clear();
}

TQString KCookiesManagement::quickHelp() const
{
  return i18n("<h1>Cookies Management Quick Help</h1>" );
}

void KCookiesManagement::getDomains()
{
  DCOPReply reply = DCOPRef("kded", "kcookiejar").call("findDomains");

  if( !reply.isValid() )
  {
    TQString caption = i18n ("Information Lookup Failure");
    TQString message = i18n ("Unable to retrieve information about the "
                            "cookies stored on your computer.");
    KMessageBox::sorry (this, message, caption);
    return;
  }

  TQStringList domains = reply;

  if ( dlg->lvCookies->childCount() )
  {
    reset();
    dlg->lvCookies->setCurrentItem( 0L );
  }

  CookieListViewItem *dom;
  for(TQStringList::Iterator dIt = domains.begin(); dIt != domains.end(); dIt++)
  {
    dom = new CookieListViewItem(dlg->lvCookies, *dIt);
    dom->setExpandable(true);
  }

  // are ther any cookies?
  dlg->pbDeleteAll->setEnabled(dlg->lvCookies->childCount());
}

void KCookiesManagement::getCookies(TQListViewItem *cookieDom)
{
  CookieListViewItem* ckd = static_cast<CookieListViewItem*>(cookieDom);
  if ( ckd->cookiesLoaded() )
    return;

  TQValueList<int> fields;
  fields << 0 << 1 << 2 << 3;

  DCOPReply reply = DCOPRef ("kded", "kcookiejar").call ("findCookies",
                                                         DCOPArg(fields, "TQValueList<int>"),
                                                         ckd->domain(),
                                                         TQString(),
                                                         TQString(),
                                                         TQString());
  if(reply.isValid())
  {
    TQStringList fieldVal = reply;
    TQStringList::Iterator fIt = fieldVal.begin();

    while(fIt != fieldVal.end())
    {
      CookieProp *details = new CookieProp;
      details->domain = *fIt++;
      details->path = *fIt++;
      details->name = *fIt++;
      details->host = *fIt++;
      details->allLoaded = false;
      new CookieListViewItem(cookieDom, details);
    }

    static_cast<CookieListViewItem*>(cookieDom)->setCookiesLoaded();
  }
}

bool KCookiesManagement::cookieDetails(CookieProp *cookie)
{
  TQValueList<int> fields;
  fields << 4 << 5 << 7;

  DCOPReply reply = DCOPRef ("kded", "kcookiejar").call ("findCookies",
                                                         DCOPArg(fields, "TQValueList<int>"),
                                                         cookie->domain,
                                                         cookie->host,
                                                         cookie->path,
                                                         cookie->name);
  if( !reply.isValid() )
    return false;

  TQStringList fieldVal = reply;

  TQStringList::Iterator c = fieldVal.begin();
  cookie->value = *c++;
  unsigned tmp = (*c++).toUInt();

  if( tmp == 0 )
    cookie->expireDate = i18n("End of session");
  else
  {
    TQDateTime expDate;
    expDate.setTime_t(tmp);
    cookie->expireDate = TDEGlobal::locale()->formatDateTime(expDate);
  }

  tmp = (*c).toUInt();
  cookie->secure = i18n(tmp ? "Yes" : "No");
  cookie->allLoaded = true;
  return true;
}

void KCookiesManagement::showCookieDetails(TQListViewItem* item)
{
  kdDebug () << "::showCookieDetails... " << endl;
  CookieProp *cookie = static_cast<CookieListViewItem*>(item)->cookie();
  if( cookie )
  {
    if( cookie->allLoaded || cookieDetails(cookie) )
    {
      dlg->leName->validateAndSet(cookie->name,0,0,0);
      dlg->leValue->validateAndSet(cookie->value,0,0,0);
      dlg->leDomain->validateAndSet(cookie->domain,0,0,0);
      dlg->lePath->validateAndSet(cookie->path,0,0,0);
      dlg->leExpires->validateAndSet(cookie->expireDate,0,0,0);
      dlg->leSecure->validateAndSet(cookie->secure,0,0,0);
    }

    dlg->pbPolicy->setEnabled (true);
  }
  else
  {
    clearCookieDetails();
    dlg->pbPolicy->setEnabled(false);
  }

  dlg->pbDelete->setEnabled(true);
}

void KCookiesManagement::doPolicy()
{
  // Get current item
  CookieListViewItem *item = static_cast<CookieListViewItem*>( dlg->lvCookies->currentItem() );

  if( item && item->cookie())
  {
    CookieProp *cookie = item->cookie();

    TQString domain = cookie->domain;

    if( domain.isEmpty() )
    {
      CookieListViewItem *parent = static_cast<CookieListViewItem*>( item->parent() );

      if ( parent )
        domain = parent->domain ();
    }

    KCookiesMain* mainDlg =static_cast<KCookiesMain*>( mainWidget );
    // must be present or something is really wrong.
    assert (mainDlg);

    KCookiesPolicies* policyDlg = mainDlg->policyDlg();
    // must be present unless someone rewrote the widget in which case
    // this needs to be re-written as well.
    assert (policyDlg);
    policyDlg->addNewPolicy(domain);
  }
}


void KCookiesManagement::deleteCookie(TQListViewItem* deleteItem)
{
  CookieListViewItem *item = static_cast<CookieListViewItem*>( deleteItem );
  if( item->cookie() )
  {
    CookieListViewItem *parent = static_cast<CookieListViewItem*>(item->parent());
    CookiePropList *list = deletedCookies.find(parent->domain());
    if(!list)
    {
      list = new CookiePropList;
      list->setAutoDelete(true);
      deletedCookies.insert(parent->domain(), list);
    }

    list->append(item->leaveCookie());
    delete item;

    if(parent->childCount() == 0)
      delete parent;
  }
  else
  {
    deletedDomains.append(item->domain());
    delete item;
  }
}

void KCookiesManagement::deleteCookie()
{
  deleteCookie(dlg->lvCookies->currentItem());

  TQListViewItem* currentItem = dlg->lvCookies->currentItem();

  if ( currentItem )
  {
    dlg->lvCookies->setSelected( currentItem, true );
    showCookieDetails( currentItem );
  }
  else
    clearCookieDetails();

  dlg->pbDeleteAll->setEnabled(dlg->lvCookies->childCount());

  const bool hasSelectedItem = dlg->lvCookies->selectedItem();
  dlg->pbDelete->setEnabled(hasSelectedItem);
  dlg->pbPolicy->setEnabled(hasSelectedItem);

  emit changed( true );
}

void KCookiesManagement::deleteAllCookies()
{
  if ( dlg->kListViewSearchLine->text().isEmpty())
  {
    reset();
    m_bDeleteAll = true;
  }
  else
  {
    TQListViewItem* item = dlg->lvCookies->firstChild();

    while (item)
    {
      if (item->isVisible())
      {
        deleteCookie(item);
        item = dlg->lvCookies->currentItem();
      }
      else
        item = item->nextSibling();
    }

    const int count = dlg->lvCookies->childCount();
    m_bDeleteAll = (count == 0);
    dlg->pbDeleteAll->setEnabled(count);

    const bool hasSelectedItem = dlg->lvCookies->selectedItem();
    dlg->pbDelete->setEnabled(hasSelectedItem);
    dlg->pbPolicy->setEnabled(hasSelectedItem);
  }

  emit changed( true );
}

#include "kcookiesmanagement.moc"
