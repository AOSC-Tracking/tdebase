#ifndef KONQ_SORT_CONSTANTS_H
#define KONQ_SORT_CONSTANTS_H

typedef unsigned short TextSortOrder;
  // Can't use name 'SortOrder' because that's part of TQt

enum TextSortOrders {
  UNICODE_UNMODIFIED      = 0,
  LOCALE_UNMODIFIED       = 1,
  UNICODE_CASEINSENSITIVE = 2,
};

#endif // KONQ_SORT_CONSTANTS_H
