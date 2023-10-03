#include "update_tdestringmatcher.h"
#include "update_tdestringmatcher_dialog.h"

#include <kdebug.h>

UIresult
getTDEStringMatcherPatternsFromUser(
  TDEStringMatcher *matcher,
  TQString dialogTitle
)
{
  TDEStringMatcher::MatchSpecList matchSpecs = matcher->getMatchSpecs();
  TDEStringMatcher_UI *tsmDialog = new TDEStringMatcher_UI( matchSpecs, dialogTitle );
  tsmDialog->exec();
  UIresult requested_action;
  if ( tsmDialog->result() ) {
    requested_action = tsmDialog->getDialogResult();
  }
  else {
    requested_action = UIresult::NOCHANGE;
  }

  switch ( requested_action ) {
    case UIresult::NOCHANGE :
      TSMTRACE
        << "TDEStringMatcherUI::getMatchPropertiesFromUser: user edit cancelled" << endl;
      return requested_action;
      break;
    case UIresult::RELOAD   :
      TSMTRACE
        << "TDEStringMatcherUI::getMatchPropertiesFromUser: user asking caller to reload stored pattern" << endl;
      return requested_action;
      break;
  }

  if ( matcher->setMatchSpecs( tsmDialog->getMatchSpecs() ) ) {
    TSMTRACE
      << "TDEStringMatcherUI::getMatchPropertiesFromUser: user edits applied:  '"
      << matcher->getMatchSpecString() << "'" << endl;
  }

  else {
    TSMTRACE
      << "TDEStringMatcherUI::getMatchPropertiesFromUser: user edits rejected"  << endl;
    return UIresult::NOCHANGE;
  }


  if ( requested_action == UIresult::RELOAD ) {
    TSMTRACE
      << "TDEStringMatcherUI::getMatchPropertiesFromUser: user asking caller to save updated criteria string" << endl;
  }

  return requested_action;
}
