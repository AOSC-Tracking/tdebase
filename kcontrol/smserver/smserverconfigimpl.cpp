/***************************************************************************
                          smserverconfigimpl.cpp  -  description
                             -------------------
    begin                : Thu May 17 2001
    copyright            : (C) 2001 by stulle
    email                : stulle@tux
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "smserverconfigimpl.h"
#include "smserverconfigimpl.moc"

SMServerConfigImpl::SMServerConfigImpl(TQWidget *parent, const char *name ) : SMServerConfigDlg(parent,name) {}

SMServerConfigImpl::~SMServerConfigImpl() {}

void SMServerConfigImpl::configChanged()
{
  emit changed();
}

void SMServerConfigImpl::fadeAwayConfigChanged()
{
  // Update showFadeAway and showFancyFadeAway status correctly
  showFadeAway->setEnabled(confirmLogoutCheck->isChecked());
  showFancyFadeAway->setEnabled(confirmLogoutCheck->isChecked() && showFadeAway->isChecked());

  configChanged();
}
