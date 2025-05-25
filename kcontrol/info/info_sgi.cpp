/* 	info_sgi.cpp
	
	!!!!! this file will be included by info.cpp !!!!!
*/

#define INFO_CPU_AVAILABLE
#define INFO_IRQ_AVAILABLE
#define INFO_DMA_AVAILABLE
#define INFO_PCI_AVAILABLE
#define INFO_IOPORTS_AVAILABLE
#define INFO_SOUND_AVAILABLE
#define INFO_DEVICES_AVAILABLE
#define INFO_SCSI_AVAILABLE
#define INFO_PARTITIONS_AVAILABLE
#define INFO_XSERVER_AVAILABLE


/*  all following functions should return true, when the Information 
    was filled into the lBox-Widget.
    returning false indicates, that information was not available.
*/
       

#include <sys/systeminfo.h>

bool GetInfo_CPU( TQListView *lBox )
{
      TQString str;
      char buf[256];

      sysinfo(SI_ARCHITECTURE, buf, sizeof(buf));
      str = TQString::fromLocal8Bit(buf);
      new TQListViewItem(lBox, str);
      return true;
}


bool GetInfo_IRQ( TQListView * )
{
	return false;
}

bool GetInfo_DMA( TQListView * )
{
	return false;
}

bool GetInfo_PCI( TQListView * )
{
	return false;
}

bool GetInfo_IO_Ports( TQListView * )
{
	return false;
}

bool GetInfo_Sound( TQListView * )
{
	return false;
}

bool GetInfo_Devices( TQListView * )
{
	return false;
}

bool GetInfo_SCSI( TQListView * )
{
	return false;
}

bool GetInfo_Partitions( TQListView * )
{
	return false;
}

bool GetInfo_XServer_and_Video( TQListView *lBox )
{
	return GetInfo_XServer_Generic( lBox );
}

