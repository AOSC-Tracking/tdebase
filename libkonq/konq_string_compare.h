#ifndef KONQ_STRING_COMPARE_H
#define KONQ_STRING_COMPARE_H

#include "konq_sort_constants.h"

static inline int stringCompare(
  const TextSortOrder sortorder,
  const TQString& a,
  const TQString& b
)
{
  // Our caller probably determined sortorder from KonqPropsView::getSortOrder()
  // but we have a reasonable fallback position for bogus values.

  switch(sortorder) {

    case UNICODE_UNMODIFIED:
      /*
       * Strictly character code(point) numeric comparison as defined
       * by the Unicode Standard that is backward compatible with the
       * the ASCII standard.
   . */
      return a.compare( b );
      break;

    case UNICODE_CASEINSENSITIVE:
      /*
       * This is the traditional "case-insensitive" variation on character
       * code order that ensures that ASCII lowercase and uppercase alphabetic
       * characters are are grouped together instead of being separated by
       * non-alphabetic ASCII characters [ \ ] ^ _ `
      */
      return a.lower().compare( b.lower() );
      break;

    case LOCALE_UNMODIFIED:
      /*
       * This is the pure locale-aware comparison as defined by ICU.
       * Note: if LC_COLLATE == 'C' or 'Posix', this will produce same
       * result as UNICODE_UNMODIFIED.
      */
      return a.localeAwareCompare( b );
      break;

    default: // Treat as UNICODE_UNMODIFIED
      return a.compare( b );
      break;
  }
}

#endif // KONQ_STRING_COMPARE_H
