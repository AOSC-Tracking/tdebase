/*
    tdeshorturifilter.h

    This file is part of the KDE project
    Copyright (C) 2000 Dawit Alemayehu <adawit@kde.org>
    Copyright (C) 2000 Malte Starostik <starosti@zedat.fu-berlin.de>

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

#ifndef _TDESHORTURIFILTER_H_
#define _TDESHORTURIFILTER_H_

#include <dcopobject.h>
#include <kgenericfactory.h>
#include <kurifilter.h>

class TDEInstance;

/**
* This is short URL filter class.
*
* @short A filter that converts short URLs into fully qualified ones.
*
* @author Dawit Alemayehu <adawit@kde.org>
* @author Malte Starostik <starosti@zedat.fu-berlin.de>
*/
class TDEShortURIFilter : public KURIFilterPlugin , public DCOPObject
{
    K_DCOP
    TQ_OBJECT
public:

    /**
     * Creates a Short URI filter object
     *
     * @param parent the parent of this class.
     * @param name the internal name for this object.
     */
    TDEShortURIFilter( TQObject *parent = 0, const char *name = 0,
	             const TQStringList &args = TQStringList() );

    /**
     * Destructor
     */
    virtual ~TDEShortURIFilter() {};

    /**
     * Converts short URIs into fully qualified valid URIs
     * whenever possible.
     *
     * Parses any given invalid URI to determine whether it
     * is a known short URI and converts it to its fully
     * qualified version.
     *
     * @param data the data to be filtered
     * @return true if the url has been filtered
     */
    virtual bool filterURI( KURIFilterData &data ) const;

    /**
     * Returns the name of the config module for
     * this plugin.
     *
     * @return the name of the config module.
     */
    virtual TQString configName() const;

    /**
     * Returns an instance of the module used to configure
     * this object.
         *
         * @return the config module
         */
    virtual TDECModule* configModule( TQWidget*, const char* ) const;

public:
k_dcop:
    virtual void configure();

private:

    struct URLHint
    {
        URLHint() {}
        URLHint( TQString r, TQString p, 
                 KURIFilterData::URITypes t = KURIFilterData::NET_PROTOCOL )
               : regexp(r), prepend(p), type(t) {}
        TQString regexp; // if this matches, then...
        TQString prepend; // ...prepend this to the url
        KURIFilterData::URITypes type;
    };

    TQValueList<URLHint> m_urlHints;
    TQString m_strDefaultProtocol;
    bool m_bVerbose;
};

#endif
