#ifndef UPDATE_TDESTRINGMATCHER_H
#define UPDATE_TDESTRINGMATCHER_H

#include <tqstring.h>
#include <tdestringmatcher.h>

enum class UIresult: signed char
{
  NOCHANGE = -1, // No change
  APPLY    =  0, // Change applied
  SAVE     =  1, // Change applied, request save
  RELOAD   =  2  // No change, request reload
};

UIresult
getTDEStringMatcherPatternsFromUser(
  TDEStringMatcher *matcher,
  TQString dialogTitle
);

#endif
