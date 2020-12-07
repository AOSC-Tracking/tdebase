#include "konq_historycomm.h"

bool KonqHistoryEntry::marshalURLAsStrings;

// TQDataStream operators (read and write a KonqHistoryEntry
// from/into a TQDataStream)
TQDataStream& operator<< (TQDataStream& s, const KonqHistoryEntry& e) {
    if (KonqHistoryEntry::marshalURLAsStrings)
	s << e.url.url();
    else
	s << e.url;

    s << e.typedURL;
    s << e.title;
    s << e.numberOfTimesVisited;
    s << e.firstVisited;
    s << e.lastVisited;

    return s;
}

TQDataStream& operator>> (TQDataStream& s, KonqHistoryEntry& e) {
    if (KonqHistoryEntry::marshalURLAsStrings)
    {
	TQString url;
	s >> url;
	e.url = url;
    }
    else
    {
	s>>e.url;
    }

    s >> e.typedURL;
    s >> e.title;
    s >> e.numberOfTimesVisited;
    s >> e.firstVisited;
    s >> e.lastVisited;

    return s;
}
