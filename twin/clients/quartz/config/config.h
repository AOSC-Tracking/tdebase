/* 	
 *
 *	This file contains the quartz configuration widget
 *
 *	Copyright (c) 2001
 *		Karol Szwed <gallium@kde.org>
 *		http://gallium.n3.net/
 */

#ifndef __KDE_QUARTZCONFIG_H
#define __KDE_QUARTZCONFIG_H

#include <tqcheckbox.h>
#include <tqvbox.h>
#include <tdeconfig.h>

class QuartzConfig: public TQObject
{
	Q_OBJECT

	public:
		QuartzConfig( TDEConfig* conf, TQWidget* parent );
		~QuartzConfig();

	// These public signals/slots work similar to KCM modules
	signals:
		void changed();

	public slots:
		void load( TDEConfig* conf );	
		void save( TDEConfig* conf );
		void defaults();

	protected slots:
		void slotSelectionChanged();	// Internal use

	private:
		TDEConfig*   quartzConfig;
		TQCheckBox* cbColorBorder;
		TQCheckBox* cbExtraSmall;
		TQVBox* gb;
};


#endif
