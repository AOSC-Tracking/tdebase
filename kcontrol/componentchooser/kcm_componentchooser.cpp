/***************************************************************************
                          kcm_componentchooser.cpp  -  description
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

#include <tqlayout.h>

#include <tdeaboutdata.h>
#include <tdeglobal.h>

#include "kcm_componentchooser.h"
#include "kcm_componentchooser.moc"


KCMComponentChooser::KCMComponentChooser( TQWidget *parent, const char *name ):
	TDECModule(parent,name) {

	(new TQVBoxLayout(this))->setAutoAdd(true);
	m_chooser=new ComponentChooser(this,"ComponentChooser");
	connect(m_chooser,TQ_SIGNAL(changed(bool)),this,TQ_SIGNAL(changed(bool)));
	setButtons( Help | Apply );

	TDEAboutData *about =
	new TDEAboutData(I18N_NOOP("kcmcomponentchooser"), I18N_NOOP("Component Chooser"),
			0, 0, TDEAboutData::License_GPL,
			I18N_NOOP("(c), 2002 Joseph Wenninger"));

	about->addAuthor("Joseph Wenninger", 0 , "jowenn@kde.org");
	setAboutData( about );

}

void KCMComponentChooser::load(){
	m_chooser->load();
}

void KCMComponentChooser::save(){
	m_chooser->save();
}

void KCMComponentChooser::defaults(){
	m_chooser->restoreDefault();
}


extern "C"
{
    KDE_EXPORT TDECModule *create_componentchooser( TQWidget *parent, const char * )
    {
        TDEGlobal::locale()->insertCatalogue("kcmcomponentchooser");
        return new KCMComponentChooser( parent, "kcmcomponentchooser" );
    }
}

