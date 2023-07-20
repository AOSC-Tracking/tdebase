/*  This file is part of the KDE libraries
    Copyright (C) 2000 Malte Starostik <malte@kde.org>
    Copyright (C) 2006 Roberto Cappuccio <roberto.cappuccio@gmail.com>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#ifndef _HTMLCREATOR_H_
#define _HTMLCREATOR_H_

#include <tdeio/thumbcreator.h>

class TDEHTMLPart;

class HTMLCreator : public TQObject, public ThumbCreator
{
    TQ_OBJECT
public:
    HTMLCreator();
    virtual ~HTMLCreator();
    virtual bool create(const TQString &path, int width, int height, TQImage &img);
    virtual Flags flags() const;

protected:
    virtual void timerEvent(TQTimerEvent *);

private slots:
    void slotCompleted();

private:
    TDEHTMLPart *m_html;
};

#endif
