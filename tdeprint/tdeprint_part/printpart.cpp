/*
 *  This file is part of the KDE libraries
 *  Copyright (c) 2001 Michael Goffioul <tdeprint@swing.be>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License version 2 as published by the Free Software Foundation.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 **/

#include "printpart.h"

#include <tdeprint/kmmainview.h>
#include <tdeprint/kiconselectaction.h>
#include <tdeaction.h>
#include <tdelocale.h>
#include <tdeinstance.h>
#include <kiconloader.h>
#include <tdeaboutdata.h>
#include <kdebug.h>
#include <tdeparts/genericfactory.h>
#include <tqwidget.h>

typedef KParts::GenericFactory<PrintPart> PrintPartFactory;
K_EXPORT_COMPONENT_FACTORY( libtdeprint_part, PrintPartFactory )

PrintPart::PrintPart(TQWidget *parentWidget, const char * /*widgetName*/ ,
	             TQObject *parent, const char *name,
		     const TQStringList & /*args*/ )
: KParts::ReadOnlyPart(parent, name)
{
	setInstance(PrintPartFactory::instance());
    instance()->iconLoader()->addAppDir("tdeprint");
	m_extension = new PrintPartExtension(this);

	m_view = new KMMainView(parentWidget, "MainView", actionCollection());
	m_view->setFocusPolicy(TQWidget::ClickFocus);
	m_view->enableToolbar(false);
	setWidget(m_view);

	initActions();
}

PrintPart::~PrintPart()
{
}

TDEAboutData *PrintPart::createAboutData()
{
	return new TDEAboutData(I18N_NOOP("tdeprint_part"), I18N_NOOP("A Konqueror Plugin for Print Management"), "0.1");
}

bool PrintPart::openFile()
{
	return true;
}

void PrintPart::initActions()
{
	setXMLFile("tdeprint_part.rc");
}

PrintPartExtension::PrintPartExtension(PrintPart *parent)
: KParts::BrowserExtension(parent, "PrintPartExtension")
{
}

PrintPartExtension::~PrintPartExtension()
{
}

#include "printpart.moc"
