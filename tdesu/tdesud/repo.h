/*
 *
 * This file is part of the KDE project, module tdesu.
 * Copyright (C) 1999,2000 Geert Jansen <jansen@kde.org>
 */

#ifndef __Repo_h_included__
#define __Repo_h_included__


#include <tqmap.h>
#include <tqcstring.h>


/**
 * Used internally.
 */
struct Data_entry 
{
    TQCString value;
    TQCString group;
    unsigned int timeout;
};


/**
 * String repository.
 *
 * This class implements a string repository with expiration.
 */
class Repository {
public:
    Repository();
    ~Repository();

    /** Remove data elements which are expired. */
    int expire();

    /** Add a data element */
    void add(const TQCString& key, Data_entry& data);

    /** Delete a data element. */
    int remove(const TQCString& key);

    /** Delete all data entries having the given group.  */
    int removeGroup(const TQCString& group);

    /** Delete all data entries based on key. */
    int removeSpecialKey(const TQCString& key );

    /** Checks for the existence of the specified group. */
    int hasGroup(const TQCString &group) const;

    /** Return a data value.  */
    TQCString find(const TQCString& key) const;

    /** Returns the key values for the given group. */
    TQCString findKeys(const TQCString& group, const char *sep= "-") const;

private:

    TQMap<TQCString,Data_entry> repo;
    typedef TQMap<TQCString,Data_entry>::Iterator RepoIterator;
    typedef TQMap<TQCString,Data_entry>::ConstIterator RepoCIterator;
    unsigned head_time;
};

#endif
