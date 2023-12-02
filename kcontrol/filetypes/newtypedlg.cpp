
#include <tqlayout.h>
#include <tqlabel.h>
#include <tqwhatsthis.h>
#include <tqcombobox.h>

#include <tdelocale.h>
#include <klineedit.h>

#include "newtypedlg.h"

NewTypeDialog::NewTypeDialog(TQStringList groups,
			     TQWidget *parent, const char *name)
  : KDialogBase(parent, name, true, i18n( "Create New File Type" ), 
    Ok|Cancel, Ok, true)
{
  TQFrame *main = makeMainWidget();
  TQVBoxLayout *topl = new TQVBoxLayout(main, 0, spacingHint());

  TQGridLayout *grid = new TQGridLayout(2, 2);
  grid->setColStretch(1, 1);
  topl->addLayout(grid);

  TQLabel *l = new TQLabel(i18n("Group:"), main);
  grid->addWidget(l, 0, 0);

  groupCombo = new TQComboBox(main);
  //groupCombo->setEditable( true ); M.O.: Currently, the code in filetypesview isn't capable of handling
  //new top level types; so better not let them be added than crash.
  groupCombo->insertStringList(groups);
  grid->addWidget(groupCombo, 0, 1);

  TQWhatsThis::add( groupCombo, i18n("Select the category under which"
    " the new file type should be added.") );

  l = new TQLabel(i18n("Type name:"), main);
  grid->addWidget(l, 1, 0);

  typeEd = new KLineEdit(main);
  grid->addWidget(typeEd, 1, 1);

  typeEd->setFocus();

  // Set a minimum size so that caption is not half-hidden
  setMinimumSize( 300, 50 );
}

TQString NewTypeDialog::group() const 
{ 
  return groupCombo->currentText(); 
}


TQString NewTypeDialog::text() const 
{ 
  return typeEd->text(); 
}
