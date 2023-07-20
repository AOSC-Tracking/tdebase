/*
 *
 *	KDE2 Default configuration widget
 *
 *	Copyright (c) 2001
 *		Karol Szwed <gallium@kde.org>
 *		http://gallium.n3.net/
 */

#ifndef _KDE_DEFAULT_CONFIG_H
#define _KDE_DEFAULT_CONFIG_H

#include <tqcheckbox.h>
#include <tqgroupbox.h>
#include <tdeconfig.h>
#include <tqhbox.h>
#include <tqlabel.h>
#include <tqvbox.h>

class KDEDefaultConfig: public TQObject
{
	TQ_OBJECT

	public:
		KDEDefaultConfig( TDEConfig* conf, TQWidget* parent );
		~KDEDefaultConfig();

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
		TQCheckBox* cbShowStipple;
		TQCheckBox* cbShowGrabBar;
		TQCheckBox* cbUseGradients;
		TQVBox* gb;
		bool 	   highcolor;
};

#endif
