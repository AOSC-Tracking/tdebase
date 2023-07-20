/*
 * Copyright 2015 Timothy Pearson <kb9vqf@pearsoncomputing.net>
 * 
 * This file is part of cryptocardwatcher, the TDE Cryptographic Card Session Monitor
 * 
 * cryptocardwatcher is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3
 * of the License, or (at your option) any later version.
 *
 * cryptocardwatcher is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with cryptocardwatcher. If not, see http://www.gnu.org/licenses/.
 */

#ifndef __TDECRYPTOCARDWATCHER_H__
#define __TDECRYPTOCARDWATCHER_H__

#include <tqobject.h>

class TDECryptographicCardDevice;

class CardWatcher : public TQObject
{
	TQ_OBJECT

	public:
		CardWatcher();
		~CardWatcher();

	public slots:
		void cryptographicCardInserted(TDECryptographicCardDevice*);
		void cryptographicCardRemoved(TDECryptographicCardDevice*);
};

#endif // __TDECRYPTOCARDWATCHER_H__