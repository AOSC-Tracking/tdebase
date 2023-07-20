/***************************************************************************
  Copyright:
  (C) 2002 by George Russell <george.russell@clara.net>
  (C) 2003-2004 by Olaf Schmidt <ojschmidt@kde.org>
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#ifndef TDEHTMLKTTSD_H
#define TDEHTMLKTTSD_H

#include <tdeparts/plugin.h>

class KURL;
class TDEInstance;

/**
 * TDEHTML KParts Plugin
 */
class TDEHTMLPluginKTTSD : public KParts::Plugin
{
    TQ_OBJECT
public:

    /**
     * Construct a new KParts plugin.
     */
    TDEHTMLPluginKTTSD( TQObject* parent, const char* name, const TQStringList& );

    /**
     * Destructor.
     */
    virtual ~TDEHTMLPluginKTTSD();
public slots:
    void slotReadOut();
};
 
#endif
