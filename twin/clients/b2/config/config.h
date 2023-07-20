/* 	
 *	This file contains the B2 configuration widget
 *
 *	Copyright (c) 2001
 *		Karol Szwed <gallium@kde.org>
 *		http://gallium.n3.net/
 */

#ifndef _KDE_B2CONFIG_H
#define _KDE_B2CONFIG_H

#include <tqcheckbox.h>
#include <tqgroupbox.h>
#include <tqhgroupbox.h>
#include <tqlabel.h>
#include <tqcombobox.h>
#include <tdeconfig.h>

class B2Config: public TQObject
{
	TQ_OBJECT

	public:
		B2Config( TDEConfig* conf, TQWidget* parent );
		~B2Config();

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
		TDEConfig*   b2Config;
		TQCheckBox* cbColorBorder;
		TQCheckBox*  showGrabHandleCb;
		TQHGroupBox* actionsGB;
		TQComboBox*  menuDblClickOp;
		TQWidget* gb;
};

#endif
