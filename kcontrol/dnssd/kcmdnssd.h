/***************************************************************************
 *   Copyright (C) 2004,2005 by Jakub Stachowski                           *
 *   qbast@go2.pl                                                          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.             *
 ***************************************************************************/

#ifndef _KCMDNSSD_H_
#define _KCMDNSSD_H_

#include <tqmap.h>

#include <configdialog.h>
#include <tdeaboutdata.h>

class TDESimpleConfig;
class KCMDnssd: public ConfigDialog
{
	TQ_OBJECT

public:
	KCMDnssd( TQWidget *parent=0, const char *name=0, const TQStringList& = TQStringList() );
	~KCMDnssd();
	virtual void save();
	virtual void load();
	virtual TQString handbookSection() const;
private slots:
	void wdchanged();
	void enableZeroconfChanged(bool);
private: 
	void loadMdnsd();
	bool saveMdnsd();
	TQMap<TQString,TQString> mdnsdLines;
	bool m_wdchanged;
	TDESimpleConfig* domain;
	bool m_enableZeroconfChanged;
};

#endif
