#ifndef _MEMORY_H_KDEINFO_INCLUDED_
#define _MEMORY_H_KDEINFO_INCLUDED_

#include <tqwidget.h>
#include <tqframe.h>
#include <tqlabel.h>
#include <tqtabdialog.h>
#include <tqpushbutton.h>
#include <tqtimer.h>

#include <tdecmodule.h>
#include <tdeaboutdata.h>
#include <config.h>

#ifdef HAVE_LONG_LONG
/* better to use long-long, because some 32bit-machines have more total 
   memory (with swap) than just the 4GB which fits into a 32bit-long */
typedef unsigned long long t_memsize;
#else
typedef unsigned long t_memsize;
#endif

#define COLOR_USED_SWAP   TQColor(255,0,0)
#define COLOR_USED_DATA   TQColor(255,180,88)
#define COLOR_USED_BUFFER TQColor(184,200,0)
#define COLOR_USED_CACHE  TQColor(156,192,0)
#define COLOR_USED_RAM    TQColor(220,200,88)
#define COLOR_FREE_MEMORY TQColor(127,255,212)

class KMemoryWidget:public TDECModule {
  TQ_OBJECT 
	  
  public:
    KMemoryWidget(TQWidget * parent, const char *name = 0);
    ~KMemoryWidget();

    TQString quickHelp() const;

  private:
    TQString Not_Available_Text;
    TQTimer *timer;

    bool ram_colors_initialized,
	swap_colors_initialized, 
	all_colors_initialized;

    TQColor ram_colors[4];
    TQString ram_text[4];
    
    TQColor swap_colors[2];
    TQString swap_text[2];
    
    TQColor all_colors[3];
    TQString all_text[3];

    void update();

    bool Display_Graph(int widgetindex,
		      int count,
		      t_memsize total,
		      t_memsize *used, 
		      TQColor *color,
		      TQString *text);
    public slots:
    void update_Values();
};


#endif // _MEMORY_H_KDEINFO_INCLUDED_

