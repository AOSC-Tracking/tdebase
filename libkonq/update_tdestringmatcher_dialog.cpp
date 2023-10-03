#include <tqapplication.h>
#include <tqlayout.h>
#include <tqlistbox.h>
#include <tqbuttongroup.h>
#include <tqlineedit.h>
#include <tqcombobox.h>
#include <tqpushbutton.h>
#include <tqradiobutton.h>
#include <tqlabel.h>
#include <tdelocale.h>
#include <tqtooltip.h>

#include "update_tdestringmatcher.h"
#include "update_tdestringmatcher_dialog.h"

#include <kdebug.h>

TDEStringMatcher_UI::TDEStringMatcher_UI(
  TDEStringMatcher::MatchSpecList& data,
  TQString title
)
{
  int fontHeight = this->fontMetrics().height();
  int fontLeading = this->fontMetrics().leading();
  int defaultLineSpacing = this->fontMetrics().lineSpacing();
  //int defaultLineSpacing = this->fontMetrics().height() ;
  int defaultMargin = this->fontMetrics().maxWidth() / 4;
  //int defaultMargin = this->fontMetrics().width("W");

  setName( "TDEStringMatcher_UI" );
  main_vbl = new TQVBoxLayout( this, 11, 6, "main_vbl"); 

  //-------------------------------------------------------------------------
  //  Section 1 (horizontal): 2-parts
  //-------------------------------------------------------------------------

  sec1X_hbl = new TQHBoxLayout(  0, 0, 0, "sec1X_hbl" );
  sec1X_hbl->setAlignment( TQt::AlignTop );
  sec1X_hbl->setSpacing( defaultLineSpacing );
  sec1X_hbl->setMargin( defaultMargin );

  //-------------------------------------------------------------------------
  //  Section 1A (vertical): 2 parts: label, listbox
  //-------------------------------------------------------------------------

  sec1A_vbl = new TQVBoxLayout( 0, 0, 0, "sec1A_vbl");
  sec1A_vbl->setAlignment( TQt::AlignTop );
  sec1X_hbl->addLayout( sec1A_vbl );

  lbl_patternListHeader = new TQLabel( this, "&Pattern List" );
  lbl_patternListHeader->setText( i18n( "Match Patterns" ) );
  lbl_patternListHeader->setAlignment( TQt::AlignHCenter );
  sec1A_vbl->addWidget( lbl_patternListHeader ) ;

  lb_patternList = new TQListBox( this, "lb_patternList" );
  sec1A_vbl->addWidget( lb_patternList );

  //-------------------------------------------------------------------------
  //  Section 1B (vertical): 4 parts
  //-------------------------------------------------------------------------

  sec1B_vbl = new TQVBoxLayout( 0, 0, 0, "sec1B_vbl");
  sec1B_vbl->setAlignment( TQt::AlignCenter );
  sec1B_vbl->setSpacing( defaultLineSpacing );
  sec1B_vbl->setMargin( defaultMargin );
  sec1X_hbl->addLayout( sec1B_vbl );

  //-------------------------------------------------------------------------
  //  Section 1B, part 1: lineedit + 2 buttons
  //-------------------------------------------------------------------------

  sec1B1_vbx = new TQGroupBox( this, "sec1B1_vbx");
  sec1B1_vbx->setTitle( i18n( "Modify Patterns" ) );
  sec1B1_vbx->setAlignment( TQGroupBox::AlignHCenter );
  sec1B1_vbx ->setColumnLayout(0, TQt::Vertical );

  sec1B1_vbl = new TQVBoxLayout( sec1B1_vbx->layout() );
  sec1B1_vbl->setMargin( defaultMargin );
  sec1B_vbl->addWidget( sec1B1_vbx );

  le_editPattern = new TQLineEdit( sec1B1_vbx, "le_editPattern" );
  TQToolTip::add( le_editPattern, i18n( "Modify highlighted pattern" ) );
  sec1B1_vbl->addWidget( le_editPattern );

  pb_addPattern = new TQPushButton( sec1B1_vbx, "pb_addPattern" );
  pb_addPattern->setText( i18n( "&New pattern" ) );
  TQToolTip::add( pb_addPattern, i18n( "Add new pattern to list" ) );
  pb_addPattern->setAutoDefault( false );
  pb_addPattern->setDefault( false );
  sec1B1_vbl->addWidget( pb_addPattern );

  pb_delPattern = new TQPushButton( sec1B1_vbx, "pb_delPattern" );
  pb_delPattern->setText( i18n( "&Delete pattern" ) );
  TQToolTip::add( pb_delPattern, i18n( "Delete highlighted pattern from list" ) );
  pb_addPattern->setAutoDefault( false );
  pb_addPattern->setDefault( false );
  sec1B1_vbl->addWidget( pb_delPattern );

  //-------------------------------------------------------------------------
  //  Section 1B, part 2: 3 comboboxes
  //-------------------------------------------------------------------------

  sec1B2_vbx = new TQGroupBox( this, "sec1B2_vbx");
  sec1B2_vbx->setTitle( i18n( "Pattern Match &Options" ) );
  sec1B2_vbx->setAlignment( TQGroupBox::AlignHCenter );
  sec1B2_vbx ->setColumnLayout(0, TQt::Vertical );

  sec1B2_vbl = new TQVBoxLayout( sec1B2_vbx->layout() );
  sec1B2_vbl->setMargin( defaultMargin );
  sec1B_vbl->addWidget( sec1B2_vbx );

  cb_patternType = new TQComboBox( FALSE, sec1B2_vbx, "cb_patternType" );
  TQToolTip::add( cb_patternType, i18n( "Highlighted pattern type" ) );
  cb_patternType->insertItem( i18n( "Pattern is a Regex"     ) ); // PatternType::REGEX
  cb_patternType->insertItem( i18n( "Pattern is a POSIX Wildcard expression"  ) ); // PatternType::WILDCARD
  cb_patternType->insertItem( i18n( "Pattern is a simple substring"  ) ); // PatternType::SUBSTRING
  cb_patternType->setCurrentItem( 0 );
  sec1B2_vbl->addWidget( cb_patternType ) ;

  cb_ancHandling = new TQComboBox( FALSE, sec1B2_vbx, "cb_ancHandling" );
  TQToolTip::add( cb_ancHandling, i18n( "Alphabetic case handling" ) );
  cb_ancHandling->insertItem( i18n( "Alphabetic letter variants distinct"      ) ); // ANCHandling::CASE_SENSITIVE
  cb_ancHandling->insertItem( i18n( "Upper and lower case letters are same"    ) ); // ANCHandling::CASE_INSENSITIVE
  cb_ancHandling->insertItem( i18n( "Alphanumeric character variants are same" ) ); // ANCHandling::IGNORE_DIACRITICS
  cb_ancHandling->setCurrentItem( 0 );
  sec1B2_vbl->addWidget( cb_ancHandling ) ;

  cb_expectMatch = new TQComboBox( FALSE, sec1B2_vbx, "cb_expectMatch" );
  TQToolTip::add( cb_expectMatch, i18n( "Match pattern or not?" ) );
  cb_expectMatch->insertItem( i18n( "Does NOT match pattern" ) );
  cb_expectMatch->insertItem( i18n( "Matches pattern" ) );
  cb_expectMatch->setCurrentItem( 1 );
  sec1B2_vbl->addWidget( cb_expectMatch ) ;

  //-------------------------------------------------------------------------
  //  Section 1B, part 3: spacer
  //-------------------------------------------------------------------------

  pad1B3 = new TQSpacerItem( 0, 0, TQSizePolicy::Minimum, TQSizePolicy::Expanding );
  sec1B_vbl->addItem( pad1B3 );

  //-------------------------------------------------------------------------
  //  Section 1B, part 4: radio button group
  //-------------------------------------------------------------------------

  // Create a group of radio buttons
  bg_Disposition = new TQButtonGroup( this, "bg_Disposition" );
  bg_Disposition->setTitle( i18n( "Disposition of &Changes" ) );
  bg_Disposition->setAlignment( TQGroupBox::AlignHCenter );
  //bg_Disposition->setFlat( true );
  bg_Disposition->setRadioButtonExclusive(true);
  sec1B_vbl->addWidget( bg_Disposition );
	
  // Create a layout for the radio buttons
  TQVBoxLayout *sec1B4_vbl = new TQVBoxLayout(bg_Disposition, -1 );
  sec1B4_vbl->setMargin( defaultMargin * 2 );
  sec1B4_vbl->insertSpacing( 0, defaultLineSpacing / 2 );

  // Set up radio button 0
  rb_ApplyNoSave = new TQRadioButton( bg_Disposition );
  rb_ApplyNoSave->setText( i18n( "&Apply but do not save" ) );
  rb_ApplyNoSave->setChecked( TRUE );
  TQToolTip::add( rb_ApplyNoSave, i18n( "Changes will be applied temporarily" ) );
  sec1B4_vbl->addWidget( rb_ApplyNoSave );

  // Set up radio button 1
  rb_ApplyAndSave = new TQRadioButton( bg_Disposition );
  rb_ApplyAndSave->setText( i18n( "Apply and &save as default" ) );
  TQToolTip::add( rb_ApplyAndSave, i18n( "Changes will be applied and saved to disk" ) );
  sec1B4_vbl->addWidget( rb_ApplyAndSave );

  // Set up radio button 2
  rb_RestoreDefault = new TQRadioButton( bg_Disposition );
  rb_RestoreDefault->setText( i18n( "Ignore and restore &default" ) );
  TQToolTip::add( rb_RestoreDefault, i18n( "Changes will be discarded and saved default restored" ) );
  sec1B4_vbl->addWidget( rb_RestoreDefault );

  //-------------------------------------------------------------------------
  //  Section 2 (horizontal): horizontal filler + pushbuttone
  //-------------------------------------------------------------------------

  sec2X_hbl = new TQHBoxLayout( 0, 0, 6, "sec2X_hbl"); 

  pad2_hb = new TQSpacerItem( 111, 0, TQSizePolicy::Expanding, TQSizePolicy::Minimum );
  sec2X_hbl->addItem( pad2_hb );

  pb_OK = new TQPushButton( this, "pb_OK" );
  pb_OK->setDefault( false );
  pb_OK->setText( i18n( "&OK" ) );
  TQToolTip::add( pb_OK, i18n( "Save settings as default" ) );
  //pb_OK->setAccel( TQKeySequence( "Return" ) );
  sec2X_hbl->addWidget( pb_OK );

  pb_Cancel = new TQPushButton( this, "pb_Cancel" );
  pb_Cancel->setDefault( false );
  pb_Cancel->setText( i18n( "&Cancel" ) );
  // pb_Cancel->setAccel( TQKeySequence( "Esc" ) );
  sec2X_hbl->addWidget( pb_Cancel );

  //-------------------------------------------------------------------------
  //  Final widget assembly & preparation for display
  //-------------------------------------------------------------------------

  main_vbl->addLayout( sec1X_hbl );
  main_vbl->addLayout( sec2X_hbl );

  resize( TQSize(0, 0).expandedTo(minimumSizeHint() * 1.1 ) );
  clearWState( WState_Polished );

  //-------------------------------------------------------------------------
  //  Connect UI actions with functions
  //-------------------------------------------------------------------------

  connect( lb_patternList,  SIGNAL( highlighted(int) ), this, SLOT( displayItemText() ) );
  connect( lb_patternList,  SIGNAL( selected(int)    ), this, SLOT( editItemText()    ) );

  connect( le_editPattern,  SIGNAL( lostFocus()      ), this, SLOT( updateItemText() ) );
  connect( le_editPattern,  SIGNAL( returnPressed()  ), this, SLOT( lineEditReturn() ) );

  connect( pb_addPattern,   SIGNAL( clicked() ), this, SLOT( addItem()    ) );
  connect( pb_delPattern,   SIGNAL( clicked() ), this, SLOT( removeItem() ) );

  connect( cb_patternType,  SIGNAL( activated(int) ), this, SLOT( updatePatternType(int) ) );
  connect( cb_ancHandling,  SIGNAL( activated(int) ), this, SLOT( updateANCHandling(int) ) );
  connect( cb_expectMatch,    SIGNAL( activated(int) ), this, SLOT( updateWantMatch(int)   ) );
  connect( bg_Disposition,  SIGNAL(clicked(int)), this, SLOT(setExitDisposition(int)) );

  connect( pb_OK,           SIGNAL( clicked() ), this, SLOT( accept() ) );
  connect( pb_Cancel,       SIGNAL( clicked() ), this, SLOT( reject() ) );

  DialogEventHandler *le_returnKeyFilter = new DialogEventHandler;
  le_editPattern->installEventFilter( le_returnKeyFilter );

  // tab order
  setTabOrder( lb_patternList, le_editPattern );
  setTabOrder( le_editPattern, pb_addPattern );
  setTabOrder( pb_addPattern, pb_delPattern );
  setTabOrder( pb_delPattern, cb_patternType );
  setTabOrder( cb_patternType, cb_ancHandling );
  setTabOrder( cb_ancHandling, cb_expectMatch );
  setTabOrder( cb_expectMatch, rb_ApplyNoSave );
  setTabOrder( rb_ApplyNoSave, rb_ApplyAndSave );
  setTabOrder( rb_ApplyAndSave, rb_RestoreDefault );
  setTabOrder( rb_RestoreDefault, lb_patternList );

  //-------------------------------------------------------------------------
  //  Initialize dialog data
  //-------------------------------------------------------------------------

  matchSpecs = data;
  lb_patternList->clear();
  for (int i = 0 ; i < matchSpecs.count(); i++ ) {
    lb_patternList->insertItem(matchSpecs[i].pattern.latin1() );
    //-Debug: fprintf(stdout, "Received spec # %d (%s)\n", i, matchSpecs[i].pattern.latin1() );
  }

  if ( lb_patternList->count() > 0 ) {
    lb_patternList->setFocus();
    lb_patternList->setSelected( 0, true );
  }
  else {
    //-Debug: kdWarning() << "Adding item to othewise empty pattern list" << endl;
    addItem();
  }

  dialogResult = UIresult::APPLY;

  if ( title.isEmpty() ) title = i18n( "Update string matching specifications" );
  setCaption( title );
}

TDEStringMatcher_UI::~TDEStringMatcher_UI()
{
}

//=== Signal handlers ===/

void TDEStringMatcher_UI::displayItemText()
{
  /*
     Display current TQListBox entry in the TQLineEdit widget
  */
  int currentIndex = lb_patternList->currentItem();
  if ( currentIndex < 0 ) return;
  le_editPattern->setText( lb_patternList->currentText() );
  cb_patternType->setCurrentItem( (uint) matchSpecs[currentIndex].patternType );
  cb_ancHandling->setCurrentItem( (uint) matchSpecs[currentIndex].ancHandling );
  cb_expectMatch->setCurrentItem(   (uint) matchSpecs[currentIndex].expectMatch   );
  //-Debug: fprintf( stdout, "Current item # %d ('%s')\n", currentIndex, lb_patternList->currentText().latin1() ) ;
}

void TDEStringMatcher_UI::editItemText()
{
  /*
     Focus on TQLineEdit widget for user editing
  */
  le_editPattern->setFocus();
  le_editPattern->selectAll();
  //-Debug: fprintf( stdout, "Editing item # %d ('%s')\n", lb_patternList->currentItem(), lb_patternList->currentText().latin1() ) ;
}

void TDEStringMatcher_UI::lineEditReturn()
{
  /*
    Configure <Return> to trigger updateItemText(), effectively
    returning focus to corresponding item in the TQListBox widget.
    FIXME: This never worked since <Return> would always exit the dialog.
    As a result, we had to implement eventFilter for TQLineEdit.
  */
  updateItemText();
  //fprintf( stdout, "Catching returnPressed() signal in line edit\n" ) ;
}

void TDEStringMatcher_UI::updateItemText()
{
  /*
     Propagate any changes made in the TQLineEdit widget back to
     corresponding TQListBox widget item and then focus on that.
  */

  if ( le_editPattern->text().isEmpty() ) return ;

  int currentIndex = lb_patternList->currentItem();
  if( le_editPattern->text() != "<new>" )
  {
    if ( currentIndex < 0 ) {
      // Add 1st entry to empty back-end list
      TDEStringMatcher::MatchSpec newPatternSpec = {
        TDEStringMatcher::PatternType::DEFAULT,
        TDEStringMatcher::ANCHandling::DEFAULT,
        true,
        TQString( le_editPattern->text() )
      };
      matchSpecs.push_back( newPatternSpec );
      // Add 1st entry to empty front-end list
      lb_patternList->insertItem( le_editPattern->text() );
      lb_patternList->setCurrentItem( 0 );
    }
    else {
      // Update existing back-end list entry
      matchSpecs[currentIndex].pattern = le_editPattern->text() ;
      // Update existing front-end list entry
      lb_patternList->changeItem( le_editPattern->text(), currentIndex );
      lb_patternList->setFocus();
    }

    // fprintf( stdout, "Modified item # %d ('%s')\n", lb_patternList->currentItem(), lb_patternList->currentText().latin1() ) ;
  }
  else {
    // User did not modify <new> list item, so throw it away
    removeItem();
  }
}

void TDEStringMatcher_UI::addItem()
{
  /*
    Add provisional new entry to the TQListBox widget and focus
    on the TQLineEdit widget for user modifications. If the entry
    is not immediately modified, it will will be removed.
  */

  if ( le_editPattern->text() == "<new>" ) {
    // We have not completed prior process of adding new item
    editItemText();
    return;
  }

  // Complete any pending changes to current item text;
  updateItemText();

  le_editPattern->setText( "<new>" );

  // Add proposed new entry to back-end list
  TDEStringMatcher::MatchSpec newPatternSpec = {
     TDEStringMatcher::PatternType::DEFAULT,
     TDEStringMatcher::ANCHandling::DEFAULT,
     true,
     le_editPattern->text()
  };
  matchSpecs.push_back( newPatternSpec );

  // Add proposed new entry to front-end list
  lb_patternList->insertItem( le_editPattern->text() );
  int newIndex = lb_patternList->count() - 1;
  lb_patternList->setCurrentItem( newIndex );

  editItemText();
  displayItemText();
  // fprintf( stdout, "Adding provisional item # %d ('%s')\n", lb_patternList->currentItem(), lb_patternList->currentText().latin1() ) ;
}

void TDEStringMatcher_UI::removeItem()
{
  if( lb_patternList->count() <= 0 ) return ;

  int currentIndex = lb_patternList->currentItem();
  //fprintf( stdout, "Removing item # %d ('%s')\n", currentIndex, lb_patternList->currentText().latin1() ) ;

  // Front-end
  lb_patternList->removeItem( lb_patternList->currentItem() );
  le_editPattern->setText("");

  // Back-end
  matchSpecs.erase( matchSpecs.begin() + currentIndex	 );
  displayItemText();
  lb_patternList->setFocus();
}

void TDEStringMatcher_UI::updatePatternType( int selectedNum )
{
  int currentIndex = lb_patternList->currentItem();
  matchSpecs[currentIndex].patternType = (TDEStringMatcher::PatternType) selectedNum ;
  //fprintf( stdout, "updatePatternType(): value changed to %d\n", selectedNum ) ;
}

void TDEStringMatcher_UI::updateANCHandling( int selectedNum )
{
  int currentIndex = lb_patternList->currentItem();
  matchSpecs[currentIndex].ancHandling = (TDEStringMatcher::ANCHandling) selectedNum ;
  //fprintf( stdout, "updateANCHandling(): value changed to %d\n", selectedNum ) ;
}

void TDEStringMatcher_UI::updateWantMatch( int index )
{
  int currentIndex = lb_patternList->currentItem();
  matchSpecs[currentIndex].expectMatch = (bool) index ;
}

void TDEStringMatcher_UI::setExitDisposition(int buttonNum )
{
  switch( buttonNum ) {
    case 0:
      dialogResult = UIresult::APPLY;
      break;
    case 1:
      dialogResult = UIresult::SAVE;
      break;
    case 2:
      dialogResult = UIresult::RELOAD;
      break;
  }
  //fprintf( stdout, "setExitDisposition(): value changed to %d\n", buttonNum ) ;
}

//=== Dialog exit and event handling functions ===//

bool TDEStringMatcher_UI::DialogEventHandler::eventFilter( TQObject *o, TQEvent *e )
{
  /*
    Configure <Return> to act like <Shift+Tab> in the TQLineEdit widget,
    thereby returning focus to corresponding item in the TQListBox widget.
  */
  if ( e->type() == TQEvent::KeyPress ) {
    // special processing for key press
    TQKeyEvent *k = (TQKeyEvent *)e;

    if ( k->key() == TQt::Key_Return || k->key() == TQt::Key_Enter ) {
      TQKeyEvent key(TQEvent::KeyPress, TQt::Key_Tab, TQt::SHIFT , 0, 0, true );
      TQApplication::sendEvent(o, &key);
      return true;
    }
  }
  // standard event processing
  return false;
}

bool TDEStringMatcher_UI::event( TQEvent * e ) {
  /*
    Configure <Ctrl+Return> to exit dialog normally regardless of current
    widget focus. User-selected dialogResult will be available to caller
    as the real dialog return code.
  */
  if ( e->type() != TQEvent::KeyPress ) {
    TQKeyEvent * ke = (TQKeyEvent*) e;
    if ( ke->key() == Key_Return && ke->state() == TQt::ControlButton ) {
      ke->accept();
      accept() ; //exitOK();
      return true;
    }
  /*
    <Escape> already terminates the dialog regardless of current widget
    focus. Therefore we don't need to do this:
    if ( ke->key() == Key_Escape ) {
      ke->accept();
      reject() ; //exitCancel();
      return true;
    }
  */
  }
  return TQWidget::event( e );
}

void TDEStringMatcher_UI::accept()
{
  /* Exit dialog "normally" with dialogResult as the actual return code*/
  done(1);
}


void TDEStringMatcher_UI::reject()
{
  /* Exit dialog prematurely */
  dialogResult = UIresult::NOCHANGE;
  done(0);
}

//=== Dialog data retrieval functions  ===//

UIresult TDEStringMatcher_UI::getDialogResult()
{
  return dialogResult ;
}

TDEStringMatcher::MatchSpecList& TDEStringMatcher_UI::getMatchSpecs()
{
  return matchSpecs ;
}

#include "update_tdestringmatcher_dialog.moc"
