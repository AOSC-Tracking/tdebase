#ifndef _FILETYPEDETAILS_H
#define _FILETYPEDETAILS_H

#include <tqtabwidget.h>
class TypesListItem;
class TDEIconButton;
class TQLineEdit;
class TQListBox;
class TQButtonGroup;
class TQCheckBox;
class TQRadioButton;
class TQPushButton;
class KServiceListWidget;

/**
 * This widget contains the right part of the file type configuration
 * dialog, that shows the details for a file type.
 * It is implemented as a separate class so that it can be used by
 * the keditfiletype program to show the details of a single mimetype.
 */
class FileTypeDetails : public TQTabWidget
{
  TQ_OBJECT
public:
  FileTypeDetails(TQWidget *parent = 0, const char *name = 0);

  void setTypeItem( TypesListItem * item );

protected:
  void updateRemoveButton();
  void updateAskSave();

signals:
  void embedMajor(const TQString &major, bool &embed); // To adjust whether major type is being embedded
  void changed(bool);

protected slots:
  void updateIcon(TQString icon);
  void updateDescription(const TQString &desc);
  void addExtension();
  void removeExtension();
  void enableExtButtons(int index);
  void slotAutoEmbedClicked(int button);
  void slotAskSaveToggled(bool);

private:
  TypesListItem * m_item;

  // First tab - General
  TDEIconButton *iconButton;
  TQListBox *extensionLB;
  TQPushButton *addExtButton, *removeExtButton;
  TQLineEdit *description;
  KServiceListWidget *serviceListWidget;

  // Second tab - Embedding
  TQButtonGroup *m_autoEmbed;
  KServiceListWidget *embedServiceListWidget;
  TQRadioButton *m_rbOpenSeparate;
  TQCheckBox *m_chkAskSave;
  TQRadioButton *m_rbGroupSettings;
};

#endif
