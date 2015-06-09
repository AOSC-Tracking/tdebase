#ifndef __KCM_LAYOUT_H__
#define __KCM_LAYOUT_H__


#include <tdecmodule.h>

#include <tqstring.h>
#include <tqlistview.h>

#include "kxkbconfig.h"


class OptionListItem;
class LayoutConfigWidget;
class XkbRules;

class LayoutConfig : public TDECModule
{
  Q_OBJECT

public:
  LayoutConfig(TQWidget *parent = 0L, const char *name = 0L);
  virtual ~LayoutConfig();

  void load();
  void save();
  void defaults();
  void initUI();
  virtual TQString handbookDocPath() const;
  
protected:
  TQString createOptionString();
  void updateIndicator(TQListViewItem* selLayout);

protected slots:
  void moveUp();
  void moveDown();
  void variantChanged();
  void displayNameChanged(const TQString& name);
  void latinChanged();
  void layoutSelChanged(TQListViewItem *);
  void loadRules();
  void updateLayoutCommand();
  void updateOptionsCommand();
  void add();
  void remove();

  void changed();

private:
  LayoutConfigWidget* widget;

  XkbRules *m_rules;
  KxkbConfig m_kxkbConfig;
  TQDict<OptionListItem> m_optionGroups;

  TQWidget* makeOptionsTab();
  void updateStickyLimit();
  static LayoutUnit getLayoutUnitKey(TQListViewItem *sel);
};


#endif
