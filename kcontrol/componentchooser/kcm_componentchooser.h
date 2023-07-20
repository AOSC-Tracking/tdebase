/***************************************************************************
                          kcm_componentchooser.h  -  description
                             -------------------
    copyright            : (C) 2002 by Joseph Wenninger
    email                : jowenn@kde.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License version 2 as     *
 *   published by the Free Software Foundation                             *
 *                                                                         *
 ***************************************************************************/

#ifndef _KCM_KTEXTEDITORCHOOSER_H_
#define _KCM_KTEXTEDITORCHOOSER_H_

#include <tdecmodule.h>

#include "componentchooser.h"

class TDEAboutData;

class KCMComponentChooser : public TDECModule
{
    TQ_OBJECT
public:
    KCMComponentChooser( TQWidget *parent = 0, const char *name = 0 );
 
    void load();
    void save();
    void defaults();

private:
    ComponentChooser  *m_chooser;
};

#endif
