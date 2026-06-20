/*
    This file is part of Konsole, an X terminal.
    Copyright (C) 2001 by Stephan Binner <binner@kde.org>

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
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301  USA.
*/

#ifndef SESSIONIFACE_H
#define SESSIONIFACE_H

#include <dcopobject.h>

class SessionIface : virtual public DCOPObject
{
    K_DCOP
    k_dcop:

    virtual bool closeSession() =0;
    virtual bool sendSignal(int signal) =0;

    virtual void clearHistory() =0;
    virtual void renameSession(const TQString &name) =0;
    virtual TQString sessionName() =0;
    virtual int sessionPID() =0;
    
    virtual TQString schema() =0;
    virtual void setSchema(const TQString &schema) =0;
    virtual TQString encoding() =0;
    virtual void setEncoding(const TQString &encoding) =0;
    virtual TQString keytab() =0;
    virtual void setKeytab(const TQString &keyboard) =0;
    virtual TQSize size() =0;
    virtual void setSize(TQSize size) =0;
    virtual TQString font() =0;
    virtual void setFont(const TQString &font) =0;
};

#endif // SESSIONIFACE_H
