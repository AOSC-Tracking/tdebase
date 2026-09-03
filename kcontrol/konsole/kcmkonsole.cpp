/***************************************************************************
                          kcmkonsole.cpp - control module for konsole
                             -------------------
    begin                : mar apr 17 16:44:59 CEST 2001
    copyright            : (C) 2001 by Andrea Rizzi
    email                : rizzi@kde.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <tqcheckbox.h>
#include <tqlayout.h>
#include <tqstringlist.h>
#include <tqtabwidget.h>

#include <dcopclient.h>

#include <tdeaboutdata.h>
#include <tdeconfig.h>
#include <kdebug.h>
#include <tdefontdialog.h>
#include <kgenericfactory.h>
#include <tdemessagebox.h>

#include "schemaeditor.h"
#include "sessioneditor.h"
#include "kcmkonsole.h"

typedef KGenericFactory<KCMKonsole, TQWidget> ModuleFactory;
K_EXPORT_COMPONENT_FACTORY( kcm_konsole, ModuleFactory("kcmkonsole") )

KCMKonsole::KCMKonsole(TQWidget * parent, const char *name, const TQStringList&)
:TDECModule(ModuleFactory::instance(), parent, name)
{
    
    setQuickHelp( i18n("<h1>Konsole</h1> With this module you can configure Konsole, the TDE terminal"
		" application. You can configure the generic Konsole options (which can also be "
		"configured using the RMB) and you can edit the schemas and sessions "
		"available to Konsole."));

    TQVBoxLayout *topLayout = new TQVBoxLayout(this);
    dialog = new KCMKonsoleDialog(this);
    dialog->line_spacingSB->setRange(0, 8, 1, false);
    dialog->line_spacingSB->setSpecialValueText(i18n("normal line spacing", "Normal"));
    dialog->show();
    topLayout->add(dialog);
    load();

    TDEAboutData *ab=new TDEAboutData( "kcmkonsole", I18N_NOOP("KCM Konsole"),
       "0.2",I18N_NOOP("KControl module for Konsole configuration"), TDEAboutData::License_GPL,
       "(c) 2001, Andrea Rizzi", 0, 0, "rizzi@kde.org");

    ab->addAuthor("Andrea Rizzi",0, "rizzi@kde.org");
    setAboutData( ab );

    connect(dialog->terminalSizeHintCB,TQ_SIGNAL(toggled(bool)), TQ_SLOT( changed() ));
    connect(dialog->warnCB,TQ_SIGNAL(toggled(bool)), TQ_SLOT( changed() ));
    connect(dialog->ctrldragCB,TQ_SIGNAL(toggled(bool)), TQ_SLOT( changed() ));
    connect(dialog->cutToBeginningOfLineCB,TQ_SIGNAL(toggled(bool)), TQ_SLOT( changed() ));
    connect(dialog->selectAllHistoryCB,TQ_SIGNAL(toggled(bool)), TQ_SLOT( changed() ));
    connect(dialog->allowResizeCB,TQ_SIGNAL(toggled(bool)), TQ_SLOT( changed() ));
    connect(dialog->bidiCB,TQ_SIGNAL(toggled(bool)), TQ_SLOT( changed() ));
    connect(dialog->xonXoffCB,TQ_SIGNAL(toggled(bool)), TQ_SLOT( changed() ));
    connect(dialog->blinkingCB,TQ_SIGNAL(toggled(bool)), TQ_SLOT( changed() ));
    connect(dialog->frameCB,TQ_SIGNAL(toggled(bool)), TQ_SLOT( changed() ));
    connect(dialog->line_spacingSB,TQ_SIGNAL(valueChanged(int)), TQ_SLOT( changed() ));
    connect(dialog->matchTabWinTitleCB,TQ_SIGNAL(toggled(bool)), TQ_SLOT( changed() ));
    connect(dialog->tabsCycleWheelCB,TQ_SIGNAL(toggled(bool)), TQ_SLOT( changed() ));
    connect(dialog->menuAcceleratorsCB,TQ_SIGNAL(toggled(bool)), TQ_SLOT( changed() ));
    connect(dialog->metaAsAltModeCB,TQ_SIGNAL(toggled(bool)), TQ_SLOT( changed() ));
    connect(dialog->realTransparency,TQ_SIGNAL(toggled(bool)), TQ_SLOT( changed() ));
    connect(dialog->silence_secondsSB,TQ_SIGNAL(valueChanged(int)), TQ_SLOT( changed() ));
    connect(dialog->word_connectorLE,TQ_SIGNAL(textChanged(const TQString &)), TQ_SLOT( changed() ));
    connect(dialog->SchemaEditor1, TQ_SIGNAL(changed()), TQ_SLOT( changed() ));
    connect(dialog->SessionEditor1, TQ_SIGNAL(changed()), TQ_SLOT( changed() ));
    connect(dialog->SchemaEditor1, TQ_SIGNAL(schemaListChanged(const TQStringList &,const TQStringList &)),
            dialog->SessionEditor1, TQ_SLOT(schemaListChanged(const TQStringList &,const TQStringList &)));
    connect(dialog->SessionEditor1, TQ_SIGNAL(getList()), dialog->SchemaEditor1, TQ_SLOT(getList()));
}

void KCMKonsole::load()
{
    load(false);
}

void KCMKonsole::load(bool useDefaults)
{
    TDEConfig config("konsolerc", true);
    config.setDesktopGroup();
    config.setReadDefaults(useDefaults);

    dialog->terminalSizeHintCB->setChecked(config.readBoolEntry("TerminalSizeHint",false));
    bidiOrig = config.readBoolEntry("EnableBidi",false);
    dialog->bidiCB->setChecked(bidiOrig);
    dialog->matchTabWinTitleCB->setChecked(config.readBoolEntry("MatchTabWinTitle",false));
    dialog->tabsCycleWheelCB->setChecked(config.readBoolEntry("TabsCycleWheel",true));
    dialog->menuAcceleratorsCB->setChecked(config.readBoolEntry("MenuAccelerators",false));
    dialog->warnCB->setChecked(config.readBoolEntry("WarnQuit",true));
    dialog->ctrldragCB->setChecked(config.readBoolEntry("CtrlDrag",true));
    dialog->cutToBeginningOfLineCB->setChecked(config.readBoolEntry("CutToBeginningOfLine",false));
    dialog->selectAllHistoryCB->setChecked(config.readBoolEntry("SelectAllHistory",true));
    dialog->allowResizeCB->setChecked(config.readBoolEntry("AllowResize",false));
    xonXoffOrig = config.readBoolEntry("XonXoff",false);
    dialog->xonXoffCB->setChecked(xonXoffOrig);
    dialog->blinkingCB->setChecked(config.readBoolEntry("BlinkingCursor",false));
    dialog->frameCB->setChecked(config.readBoolEntry("has frame",true));
    dialog->line_spacingSB->setValue(config.readUnsignedNumEntry( "LineSpacing", 0 ));
    dialog->silence_secondsSB->setValue(config.readUnsignedNumEntry( "SilenceSeconds", 10 ));
    dialog->word_connectorLE->setText(config.readEntry("wordseps",":@-./_~"));
    dialog->metaAsAltModeCB->setChecked(config.readBoolEntry("metaAsAltMode",false));
    realTransparencyOrig = config.readBoolEntry("RealTransparency",false);
    dialog->realTransparency->setChecked(realTransparencyOrig);

    dialog->SchemaEditor1->setSchema(config.readEntry("schema"));

    emit changed(useDefaults);
}

void KCMKonsole::save()
{
    if (dialog->SchemaEditor1->isModified())
    {
       dialog->TabWidget2->showPage(dialog->tab_2);
       dialog->SchemaEditor1->querySave();
    }

    if (dialog->SessionEditor1->isModified())
    {
       dialog->TabWidget2->showPage(dialog->tab_3);
       dialog->SessionEditor1->querySave();
    }

    TDEConfig config("konsolerc");
    config.setDesktopGroup();

    config.writeEntry("TerminalSizeHint", dialog->terminalSizeHintCB->isChecked());
    bool bidiNew = dialog->bidiCB->isChecked();
    config.writeEntry("EnableBidi", bidiNew);
    config.writeEntry("MatchTabWinTitle", dialog->matchTabWinTitleCB->isChecked());
    config.writeEntry("TabsCycleWheel", dialog->tabsCycleWheelCB->isChecked());
    config.writeEntry("MenuAccelerators", dialog->menuAcceleratorsCB->isChecked());
    config.writeEntry("WarnQuit", dialog->warnCB->isChecked());
    config.writeEntry("CtrlDrag", dialog->ctrldragCB->isChecked());
    config.writeEntry("CutToBeginningOfLine", dialog->cutToBeginningOfLineCB->isChecked());
    config.writeEntry("SelectAllHistory", dialog->selectAllHistoryCB->isChecked());
    config.writeEntry("AllowResize", dialog->allowResizeCB->isChecked());
    bool xonXoffNew = dialog->xonXoffCB->isChecked();
    config.writeEntry("XonXoff", xonXoffNew);
    config.writeEntry("BlinkingCursor", dialog->blinkingCB->isChecked());
    config.writeEntry("has frame", dialog->frameCB->isChecked());
    config.writeEntry("LineSpacing" , dialog->line_spacingSB->value());
    config.writeEntry("SilenceSeconds" , dialog->silence_secondsSB->value());
    config.writeEntry("wordseps", dialog->word_connectorLE->text());
    config.writeEntry("metaAsAltMode", dialog->metaAsAltModeCB->isChecked());
    bool realTransparencyNew = dialog->realTransparency->isChecked();
    config.writeEntry("RealTransparency", realTransparencyNew);

    config.writeEntry("schema", dialog->SchemaEditor1->schema());

    config.sync();

    emit changed(false);

    DCOPClient *dcc = tdeApp->dcopClient();
    dcc->send("konsole-*", "konsole", "reparseConfiguration()", TQByteArray());
    dcc->send("kdesktop", "default", "configure()", TQByteArray());
    dcc->send("tdelauncher", "tdelauncher", "reparseConfiguration()", TQByteArray());

    if (xonXoffOrig != xonXoffNew)
    {
       xonXoffOrig = xonXoffNew;
       KMessageBox::information(this, i18n("The Ctrl+S/Ctrl+Q flow control setting will only affect "
                                           "newly started Konsole sessions.\n"
                                           "The 'stty' command can be used to change the flow control "
                                           "settings of existing Konsole sessions."));
    }

    if (realTransparencyOrig != realTransparencyNew)
    {
       KMessageBox::information(this, i18n("The real transparency setting will only affect "
                                           "newly started Konsole sessions.\n"));
    }

    if (bidiNew && !bidiOrig)
    {
       KMessageBox::information(this, i18n("You have chosen to enable "
                                           "bidirectional text rendering by "
                                           "default.\n"
                                           "Note that bidirectional text may "
                                           "not always be shown correctly, "
                                           "especially when selecting parts of "
                                           "text written right-to-left. This "
                                           "is a known issue which cannot be "
                                           "resolved at the moment due to the "
                                           "nature of text handling in "
                                           "console-based applications."));
    }
    bidiOrig = bidiNew;

}

void KCMKonsole::defaults()
{
    load(true);
}

#include "kcmkonsole.moc"
