/*
 * Copyright (c) 2004 Lubos Lunak <l.lunak@kde.org>
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "ruleslist.h"

#include <tdelistbox.h>
#include <kpushbutton.h>
#include <assert.h>
#include <kdebug.h>
#include <tdeconfig.h>

#include "ruleswidget.h"

namespace KWinInternal
{

KCMRulesList::KCMRulesList( TQWidget* parent, const char* name )
: KCMRulesListBase( parent, name )
    {
    // connect both current/selected, so that current==selected (stupid TQListBox :( )
    connect( rules_listbox, TQ_SIGNAL( currentChanged( TQListBoxItem* )),
        TQ_SLOT( activeChanged( TQListBoxItem*)));
    connect( rules_listbox, TQ_SIGNAL( selectionChanged( TQListBoxItem* )),
        TQ_SLOT( activeChanged( TQListBoxItem*)));
    connect( new_button, TQ_SIGNAL( clicked()),
        TQ_SLOT( newClicked()));
    connect( modify_button, TQ_SIGNAL( clicked()),
        TQ_SLOT( modifyClicked()));
    connect( delete_button, TQ_SIGNAL( clicked()),
        TQ_SLOT( deleteClicked()));
    connect( moveup_button, TQ_SIGNAL( clicked()),
        TQ_SLOT( moveupClicked()));
    connect( movedown_button, TQ_SIGNAL( clicked()),
        TQ_SLOT( movedownClicked()));
    connect( rules_listbox, TQ_SIGNAL( doubleClicked ( TQListBoxItem * ) ),
            TQ_SLOT( modifyClicked()));
    load();
    }

KCMRulesList::~KCMRulesList()
    {
    for( TQValueVector< Rules* >::Iterator it = rules.begin();
         it != rules.end();
         ++it )
        delete *it;
    rules.clear();
    }

void KCMRulesList::activeChanged( TQListBoxItem* item )
    {
    if( item != NULL )
        rules_listbox->setSelected( item, true ); // make current==selected
    modify_button->setEnabled( item != NULL );
    delete_button->setEnabled( item != NULL );
    moveup_button->setEnabled( item != NULL && item->prev() != NULL );
    movedown_button->setEnabled( item != NULL && item->next() != NULL );
    }

void KCMRulesList::newClicked()
    {
    RulesDialog dlg;
    Rules* rule = dlg.edit( NULL, 0, false );
    if( rule == NULL )
        return;
    int pos = rules_listbox->currentItem() + 1;
    rules_listbox->insertItem( rule->description, pos );
    rules_listbox->setSelected( pos, true );
    rules.insert( rules.begin() + pos, rule );
    emit changed( true );
    }

void KCMRulesList::modifyClicked()
    {
    int pos = rules_listbox->currentItem();
    if ( pos == -1 )
        return;
    RulesDialog dlg;
    Rules* rule = dlg.edit( rules[ pos ], 0, false );
    if( rule == rules[ pos ] )
        return;
    delete rules[ pos ];
    rules[ pos ] = rule;
    rules_listbox->changeItem( rule->description, pos );
    emit changed( true );
    }

void KCMRulesList::deleteClicked()
    {
    int pos = rules_listbox->currentItem();
    assert( pos != -1 );
    rules_listbox->removeItem( pos );
    rules.erase( rules.begin() + pos );
    emit changed( true );
    }

void KCMRulesList::moveupClicked()
    {
    int pos = rules_listbox->currentItem();
    assert( pos != -1 );
    if( pos > 0 )
        {
        TQString txt = rules_listbox->text( pos );
        rules_listbox->removeItem( pos );
        rules_listbox->insertItem( txt, pos - 1 );
        rules_listbox->setSelected( pos - 1, true );
        Rules* rule = rules[ pos ];
        rules[ pos ] = rules[ pos - 1 ];
        rules[ pos - 1 ] = rule;
        }
    emit changed( true );
    }

void KCMRulesList::movedownClicked()
    {
    int pos = rules_listbox->currentItem();
    assert( pos != -1 );
    if( pos < int( rules_listbox->count()) - 1 )
        {
        TQString txt = rules_listbox->text( pos );
        rules_listbox->removeItem( pos );
        rules_listbox->insertItem( txt, pos + 1 );
        rules_listbox->setSelected( pos + 1, true );
        Rules* rule = rules[ pos ];
        rules[ pos ] = rules[ pos + 1 ];
        rules[ pos + 1 ] = rule;
        }
    emit changed( true );
    }

void KCMRulesList::load()
    {
    rules_listbox->clear();
    for( TQValueVector< Rules* >::Iterator it = rules.begin();
         it != rules.end();
         ++it )
        delete *it;
    rules.clear();
    TDEConfig cfg( "twinrulesrc", true );
    cfg.setGroup( "General" );
    int count = cfg.readNumEntry( "count" );
    rules.reserve( count );
    for( int i = 1;
         i <= count;
         ++i )
        {
        cfg.setGroup( TQString::number( i ));
        Rules* rule = new Rules( cfg );
        rules.append( rule );
        rules_listbox->insertItem( rule->description );
        }
    if( rules.count() > 0 )
        rules_listbox->setSelected( 0, true );
    else
        activeChanged( NULL );
    }

void KCMRulesList::save()
    {
    TDEConfig cfg( "twinrulesrc" );
    TQStringList groups = cfg.groupList();
    for( TQStringList::ConstIterator it = groups.begin();
         it != groups.end();
         ++it )
        cfg.deleteGroup( *it );
    cfg.setGroup( "General" );
    cfg.writeEntry( "count", rules.count());
    int i = 1;
    for( TQValueVector< Rules* >::ConstIterator it = rules.begin();
         it != rules.end();
         ++it )
        {
        cfg.setGroup( TQString::number( i ));
        (*it)->write( cfg );
        ++i;
        }
    }

void KCMRulesList::defaults()
    {
    load();
    }

} // namespace

#include "ruleslist.moc"
