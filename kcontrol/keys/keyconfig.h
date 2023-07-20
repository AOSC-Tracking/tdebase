//
// KDE Shortcut config module
//
// Copyright (c)  Mark Donohoe 1998
// Copyright (c)  Matthias Ettrich 1998
// Converted to generic key configuration module, Duncan Haldane 1998.

#ifndef __KEYCONFIG_H__
#define __KEYCONFIG_H__

#include <tqpushbutton.h>
#include <tqlistbox.h>

#include <tdeaccel.h>
#include <kkeydialog.h>
//#include <tdecmodule.h>
#include <tqdict.h>

class TQCheckBox;

class KeyChooserSpec;

class KKeyModule : public TQWidget
{
	TQ_OBJECT
public:
	TDEAccelActions actions;
        //TDEAccelActions dict;
        KeyChooserSpec *kc;

	KKeyModule( TQWidget *parent, bool isGlobal, bool bSeriesOnly, bool bSeriesNone, const char *name = 0 );
	KKeyModule( TQWidget *parent, bool isGlobal, const char *name = 0 );
	~KKeyModule ();

protected:
	void init( bool isGlobal, bool bSeriesOnly, bool bSeriesNone );

public:
        virtual void load();
        //virtual void save();
        virtual void defaults();
        static void init();

	bool writeSettings( const TQString& sGroup, TDEConfig* pConfig );
	bool writeSettingsGlobal( const TQString& sGroup );

public slots:
	//void slotPreviewScheme( int );
	//void slotAdd();
	//void slotSave();
	//void slotRemove();
	void slotKeyChange();
	void slotPreferMeta();
        //void updateKeys( const TDEAccelActions* map_P );
	//void readSchemeNames();

signals:
	void keyChange();
        //void keysChanged( const TDEAccelActions* map_P );

protected:
	TQListBox *sList;
	TQStringList *sFileList;
	TQPushButton *addBt;
	TQPushButton *removeBt;
	TQCheckBox *preferMetaBt;
	int nSysSchemes;
	bool bSeriesOnly;

	void readScheme( int index=0 );

	TQString KeyType;
	TQString KeyScheme;
	TQString KeySet;

};

class KeyChooserSpec : public KKeyChooser
{
        TQ_OBJECT
public:
        KeyChooserSpec( TDEAccelActions& actions, TQWidget* parent,
                 bool bGlobal );
        //void updateKeys( const TDEAccelActions* map_P );
protected:
        bool m_bGlobal;
};

#endif

