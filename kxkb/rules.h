#ifndef __RULES_H__
#define __RULES_H__

#include <tqstring.h>
#include <tqdict.h>
#include <tqmap.h>


class XkbRules
{
public:

  XkbRules(bool layoutsOnly=false);

  const TQDict<char> &models() const { return m_models; };
  const TQDict<char> &layouts() const { return m_layouts; };
  const TQDict<char> &options() const { return m_options; };

  TQStringList getAvailableVariants(const TQString& layout);

protected:
  void loadRules(TQString filename, bool layoutsOnly=false);

private:

  TQDict<char> m_models;
  TQDict<char> m_layouts;
  TQDict<char> m_options;
  TQDict<TQStringList> m_varLists;

  TQString X11_DIR;	// pseudo-constant

//  void fixLayouts();
};


#endif
