/*
 *
 * This file is part of the KDE project, module tdesu.
 * Copyright (C) 1999,2000 Geert Jansen <g.t.jansen@stud.tue.nl>
 */

#include <time.h>
#include <assert.h>

#include <tqcstring.h>
#include <tqmap.h>
#include <tqvaluestack.h>
#include <kdebug.h>

#include "repo.h"


Repository::Repository()
{
    head_time = (unsigned) -1;
}


Repository::~Repository()
{}


void Repository::add(const TQCString &key, Data_entry &data)
{
    RepoIterator it = repo.find(key);
    if (it != repo.end())
        remove(key);
    if (data.timeout == 0)
        data.timeout = (unsigned) -1;
    else
        data.timeout += time(0L);
    head_time = TQMIN(head_time, data.timeout);
    repo.insert(key, data);
}

int Repository::remove(const TQCString &key)
{
    if( key.isEmpty() )
        return -1;

     RepoIterator it = repo.find(key);
     if (it == repo.end())
        return -1;
     it.data().value.fill('x');
     it.data().group.fill('x');
     repo.remove(it);
     return 0;
}

int Repository::removeSpecialKey(const TQCString &key)
{
    int found = -1;
    if ( !key.isEmpty() )
    {
        TQValueStack<TQCString> rm_keys;
        for (RepoCIterator it=repo.begin(); it!=repo.end(); ++it)
        {
            if (  key.find( static_cast<const char *>(it.data().group) ) == 0 &&
                  it.key().find( static_cast<const char *>(key) ) >= 0 )
            {
                rm_keys.push(it.key());
                found = 0;
            }
        }
        while (!rm_keys.isEmpty())
        {
            kdDebug(1205) << "Removed key: " << rm_keys.top() << endl;
            remove(rm_keys.pop());
        }
    }
    return found;
}

int Repository::removeGroup(const TQCString &group)
{
    int found = -1;
    if ( !group.isEmpty() )
    {
        TQValueStack<TQCString> rm_keys;
        for (RepoCIterator it=repo.begin(); it!=repo.end(); ++it)
        {
            if (it.data().group == group)
            {
                rm_keys.push(it.key());
                found = 0;
            }
        }
        while (!rm_keys.isEmpty())
        {
            kdDebug(1205) << "Removed key: " << rm_keys.top() << endl;
            remove(rm_keys.pop());
        }
    }
    return found;
}

int Repository::hasGroup(const TQCString &group) const
{
    if ( !group.isEmpty() )
    {
        RepoCIterator it;
        for (it=repo.begin(); it!=repo.end(); ++it)
        {
            if (it.data().group == group)
                return 0;
        }
    }
    return -1;
}

TQCString Repository::findKeys(const TQCString &group, const char *sep ) const
{
    TQCString list="";
    if( !group.isEmpty() )
    {
        kdDebug(1205) << "Looking for matching key with group key: " << group << endl;
        int pos;
        TQCString key;
        RepoCIterator it;
        for (it=repo.begin(); it!=repo.end(); ++it)
        {
            if (it.data().group == group)
            {
                key = it.key().copy();
                kdDebug(1205) << "Matching key found: " << key << endl;
                pos = key.findRev(sep);
                key.truncate( pos );
                key.remove(0, 2);
                if (!list.isEmpty())
                {
                    // Add the same keys only once please :)
                    if( !list.contains(static_cast<const char *>(key)) )
                    {
                        kdDebug(1205) << "Key added to list: " << key << endl;
                        list += '\007'; // I do not know
                        list.append(key);
                    }
                }
                else
                    list = key;
            }
        }
    }
    return list;
}

TQCString Repository::find(const TQCString &key) const
{
    if( key.isEmpty() )
        return 0;

    RepoCIterator it = repo.find(key);
    if (it == repo.end())
        return 0;
    return it.data().value;
}


int Repository::expire()
{
    unsigned current = time(0L);
    if (current < head_time)
	return 0;

    unsigned t;
    TQValueStack<TQCString> keys;
    head_time = (unsigned) -1;
    RepoIterator it;
    for (it=repo.begin(); it!=repo.end(); ++it)
    {
	t = it.data().timeout;
	if (t <= current)
	    keys.push(it.key());
	else
	    head_time = TQMIN(head_time, t);
    }

    int n = keys.count();
    while (!keys.isEmpty())
	remove(keys.pop());
    return n;
}

