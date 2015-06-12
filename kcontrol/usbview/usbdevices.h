/***************************************************************************
 *   Copyright (C) 2001 by Matthias Hoelzer-Kluepfel <mhk@caldera.de>      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#ifndef __USB_DEVICES_H__
#define __USB_DEVICES_H__


#include <tqstring.h>
#include <tqptrlist.h>

#ifdef Q_OS_FREEBSD
#include <libusb20.h>
#include <dev/usb/usb_ioctl.h>
#endif

class USBDB;


class USBDevice
{
public:

  USBDevice();

  void parseLine(TQString line);
  void parseSysDir(int bus, int parent, int level, TQString line);

  int level() { return _level; };
  int device() { return _device; };
  int parent() { return _parent; };
  int bus() { return _bus; };
  TQString product();

  TQString dump();

  static TQPtrList<USBDevice> &devices() { return _devices; };
  static USBDevice *find(int bus, int device);
  static bool parse(TQString fname);
  static bool parseSys(TQString fname);


private:

  static TQPtrList<USBDevice> _devices;

  static USBDB *_db;

  int _bus, _level, _parent, _port, _count, _device, _channels, _power;
  float _speed;

  TQString _manufacturer, _product, _serial;

  int _bwTotal, _bwUsed, _bwPercent, _bwIntr, _bwIso;
  bool _hasBW;

  unsigned int _verMajor, _verMinor, _class, _sub, _prot, _maxPacketSize, _configs;
  TQString _className;

  unsigned int _vendorID, _prodID, _revMajor, _revMinor;

#ifdef Q_OS_FREEBSD
  void collectData(struct libusb20_backend *, struct libusb20_device *);
  TQStringList _devnodes;
#endif
};


#endif
