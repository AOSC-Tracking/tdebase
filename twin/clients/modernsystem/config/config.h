#ifndef __KDE_MODSYSTEMCONFIG_H
#define __KDE_MODSYSTEMCONFIG_H

#include <tqcheckbox.h>
#include <tqgroupbox.h>
#include <tqlayout.h>
#include <tqvbox.h>
#include <tqslider.h>
#include <tqlabel.h>

class ModernSysConfig : public TQObject
{
	TQ_OBJECT

	public:
		ModernSysConfig(TDEConfig* conf, TQWidget* parent);
		~ModernSysConfig();

	// These public signals/slots work similar to KCM modules
	signals:
		void		changed();

	public slots:
		void		load(TDEConfig* conf);	
		void		save(TDEConfig* conf);
		void		defaults();

	protected slots:
		void		slotSelectionChanged();	// Internal use

	private:
		TDEConfig   	*clientrc;
		TQWidget		*mainw;
		TQVBoxLayout	*vbox;
		TQWidget         *handleBox;
		TQCheckBox 	*cbShowHandle;
		TQVBox		*sliderBox;
		TQSlider		*handleSizeSlider;
		TQHBox		*hbox;
		TQLabel		*label1;
		TQLabel		*label2;
		TQLabel		*label3;
		
		unsigned  	handleWidth;
		unsigned  	handleSize;

};


#endif
