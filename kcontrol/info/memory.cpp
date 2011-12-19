/*
 *  memory.cpp
 *
 *  prints memory-information and shows a graphical display.
 *
 *  Copyright (c) 1999-2002 Helge Deller <deller@gmx.de>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 *
 */

#include <sys/param.h>		/* for BSD */

#include <tqlayout.h>
#include <tqpainter.h>
#include <tqdrawutil.h>
#include <tqtooltip.h>

#include <kglobal.h>
#include <kdialog.h>
#include <kseparator.h>

#include "memory.h"

enum {				/* entries for Memory_Info[] */
    TOTAL_MEM = 0,		/* total physical memory (without swaps) */
    FREE_MEM,			/* total free physical memory (without swaps) */
#if !defined(__svr4__) || !defined(sun)
#if !defined(__NetBSD__) && !defined(__OpenBSD__)
    SHARED_MEM,			/* shared memory size */
    BUFFER_MEM,			/* buffered memory size */
#else
    ACTIVE_MEM,
    INACTIVE_MEM,
#endif
#endif
    CACHED_MEM,			/* cache memory size (located in ram) */
    SWAP_MEM,			/* total size of all swap-partitions */
    FREESWAP_MEM,		/* free memory in swap-partitions */
    MEM_LAST_ENTRY
};

/*
   all update()-functions should put either 
   their results _OR_ the value NO_MEMORY_INFO into Memory_Info[]
*/
static t_memsize Memory_Info[MEM_LAST_ENTRY];

#define MEMORY(x)	((t_memsize) (x))	/* it's easier... */
#define NO_MEMORY_INFO	MEMORY(-1)		/* DO NOT CHANGE */
#define ZERO_IF_NO_INFO(value) ((value) != NO_MEMORY_INFO ? (value) : 0)


/******************/
/* Implementation */
/******************/

static TQLabel *MemSizeLabel[MEM_LAST_ENTRY][2];

enum { MEM_RAM_AND_HDD, MEM_RAM, MEM_HDD, MEM_LAST };
static TQWidget *Graph[MEM_LAST];
static TQLabel *GraphLabel[MEM_LAST];

#define SPACING 16

static TQString formatted_unit(t_memsize value)
{
    if (value > (1024 * 1024))
        if (value > (1024 * 1024 * 1024))
            return i18n("%1 GB").arg(KGlobal::locale()->formatNumber(value / (1024 * 1024 * 1024.0), 2));
        else
            return i18n("%1 MB").arg(KGlobal::locale()->formatNumber(value / (1024 * 1024.0), 2));
    else
        return i18n("%1 KB").arg(KGlobal::locale()->formatNumber(value / 1024.0, 2));
}

KMemoryWidget::KMemoryWidget(TQWidget * parent, const char *name)
:  KCModule(parent, name)
{

    KAboutData *about =
    new KAboutData(I18N_NOOP("kcminfo"),
		I18N_NOOP("KDE Panel Memory Information Control Module"),
		0, 0, KAboutData::License_GPL,
		I18N_NOOP("(c) 1998 - 2002 Helge Deller"));

    about->addAuthor("Helge Deller", 0, "deller@gmx.de");
    setAboutData( about );

    TQString title, initial_str;
    TQLabel *Widget = 0;
    int i, j;

    ram_colors_initialized =
    swap_colors_initialized = 
    all_colors_initialized = false;

    setButtons(Help);

    /* default string for no Information... */
    Not_Available_Text = i18n("Not available.");

    TQVBoxLayout *top = new TQVBoxLayout(this, 0, KDialog::spacingHint());

    TQHBoxLayout *hbox = new TQHBoxLayout();
    top->addLayout(hbox);

    /* stretch the left side */
    hbox->addStretch();

    /* first create the Informationtext-Widget */
    TQVBoxLayout *vbox = new TQVBoxLayout(hbox, 0);
    for (i = TOTAL_MEM; i < MEM_LAST_ENTRY; ++i) {
	switch (i) {
	case TOTAL_MEM:
	    title = i18n("Total physical memory:");
	    break;
	case FREE_MEM:
	    title = i18n("Free physical memory:");
	    break;
#if !defined(__svr4__) || !defined(sun)
#if !defined(__NetBSD__) && !defined(__OpenBSD__)
	case SHARED_MEM:
	    title = i18n("Shared memory:");
	    break;
	case BUFFER_MEM:
	    title = i18n("Disk buffers:");
	    break;
#else
	case ACTIVE_MEM:
	    title = i18n("Active memory:");
	    break;
	case INACTIVE_MEM:
	    title = i18n("Inactive memory:");
	    break;
#endif
#endif
	case CACHED_MEM:
	    title = i18n("Disk cache:");
	    break;
	case SWAP_MEM:
	    vbox->addSpacing(SPACING);
	    title = i18n("Total swap space:");
	    break;
	case FREESWAP_MEM:
	    title = i18n("Free swap space:");
	    break;
	default:
	    title = "";
	    break;
	};
	Widget = new TQLabel(title, this);
	Widget->setAlignment(AlignLeft);
	vbox->addWidget(Widget, 1);
    }

    /* then the memory-content-widgets */
    for (j = 0; j < 2; j++) {
	vbox = new TQVBoxLayout(hbox, 0);
	for (i = TOTAL_MEM; i < MEM_LAST_ENTRY; ++i) {
	    if (i == SWAP_MEM)
		vbox->addSpacing(SPACING);
	    Widget = new TQLabel(this);
	    Widget->setAlignment(AlignRight);
	    MemSizeLabel[i][j] = Widget;
	    vbox->addWidget(Widget, 1);
	}
    }

    /* stretch the right side */
    hbox->addStretch();

    KSeparator *line = new KSeparator(KSeparator::HLine, this);
    top->addWidget(line);

    /* now the Graphics */
    TQString hint;
    hbox = new TQHBoxLayout(top, 1);
    for (i = MEM_RAM_AND_HDD; i < MEM_LAST; i++) {
	hbox->addSpacing(SPACING);
	vbox = new TQVBoxLayout(hbox);

	switch (i) {
	case MEM_RAM_AND_HDD:
	    title = i18n("Total Memory");
	    hint = i18n("This graph gives you an overview of the "
			"usage of <b>all available memory</b> (the sum of "
			"physical memory and swap space) in your system.");
	    break;
	case MEM_RAM:
	    title = i18n("Physical Memory");
	    hint = i18n("This graph gives you an overview of "
			"the usage of <b>physical memory</b> in your system."
			"<p>Most operating systems (including Linux) "
			"will use as much of the available physical "
			"memory as possible for a disk cache, "
			"to speed up the reading and writing of files. "
			"<p>This means that if you are seeing a small amount "
			"of <b>Free Physical Memory</b> and a large amount of "
			"<b>Disk Cache</b>, your system is well configured.");
	    break;
	case MEM_HDD:
	    title = i18n("Swap Space");
	    hint = i18n("Swap space is the <b>virtual memory</b> "
			"available to the system. "
			"<p>It will be used when needed, and is provided "
			"through one or more swap partitions and/or swap files.");
	    break;
	default:
	    hint = title = TQString::null; 
	    break;
	};

	if (hint.length())
	  hint = "<qt>" + hint + "</qt>";

	Widget = new TQLabel("<b>" + title + "</b>", this);
	Widget->setAlignment(AlignCenter);
	TQToolTip::add(Widget, hint);
	vbox->addWidget(Widget);
	vbox->addSpacing(SPACING / 2);

	TQWidget *g = new TQWidget(this);
	g->setMinimumWidth(2 * SPACING);
	g->setMinimumHeight(3 * SPACING);
	g->setBackgroundMode(NoBackground);
	TQToolTip::add(g, hint); // add the tooltip
	Graph[i] = g;
	vbox->addWidget(g, 2);
	vbox->addSpacing(SPACING / 2);

	Widget = new TQLabel(this);	/* xx MB used. */
	Widget->setAlignment(AlignCenter);
	TQToolTip::add(Widget, hint);
	GraphLabel[i] = Widget;
	vbox->addWidget(Widget);
    }
    hbox->addSpacing(SPACING);

    timer = new TQTimer(this);
    timer->start(100);
    TQObject::connect(timer, TQT_SIGNAL(timeout()), this,
		     TQT_SLOT(update_Values()));

    update();
}

KMemoryWidget::~KMemoryWidget()
{
    /* stop the timer */
    timer->stop();
}


TQString KMemoryWidget::quickHelp() const
{
  return i18n("<h1>Memory Information</h1>"
	" This display shows you the current memory usage of your system."
	" The values are updated on a regular basis and give you an"
	" overview of the physical and virtual used memory." );
}


/* Graphical Memory Display */
bool KMemoryWidget::Display_Graph(int widgetindex,
				int count,
			      	t_memsize total,
			      	t_memsize * used, 
			      	TQColor * color,
				TQString *text)
{
    TQWidget *graph = Graph[widgetindex];
    int width = graph->width();
    int height = graph->height();
    TQPixmap pm(width, height);
    TQPainter paint;

    paint.tqbegin(&pm, this);

    TQPen pen(TQColor(0, 0, 0));

    if (! ZERO_IF_NO_INFO(total)) {
	paint.fillRect(1, 1, width - 2, height - 2,
		       TQBrush(TQColor(128, 128, 128)));
	paint.setPen(pen);
	paint.drawRect(graph->rect());
	GraphLabel[widgetindex]->setText(Not_Available_Text);
	paint.end();
	bitBlt(graph, 0, 0, &pm);
	return false;
    }

    int startline = height-2;
    int	percent, localheight;
    t_memsize last_used = 0;
    
    while (count--) {
	last_used = *used;
	
#ifdef HAVE_LONG_LONG
	percent = (((long long)last_used) * 1000 + 5) / (total * 10);
#else
	/* prevent integer overflow with usage of double type */
	percent = (int) ((((double)last_used) * 1000 + 5) / (total * 10));
#endif

    	if (count)
		localheight = ((height-2) * percent) / 100;
	else
		localheight = startline;

	if (localheight>0) {
		paint.fillRect(1, startline, width-2, -localheight, *color);

    		if (localheight >= SPACING) {
			paint.drawText(0, startline-localheight, width, localheight,
				AlignCenter | WordBreak, 
				TQString("%1 %2%").arg(*text).arg(percent));
		}
    	}
	
	startline -= localheight;
	
	++used;
	++color;
	++text;
    }
	
    /* draw surrounding box */
    paint.setPen(pen);
    TQRect r = graph->rect();
    qDrawShadePanel(&paint, r.x(), r.y(), r.width(), r.height(), tqpalette().active(), true, 1);
    paint.end();
    bitBlt(graph, 0, 0, &pm);

    GraphLabel[widgetindex]->setText(i18n("%1 free").arg(formatted_unit(last_used)));
 
    return true;
}

/* update_Values() is the main-loop for updating the Memory-Information */
void KMemoryWidget::update_Values()
{
    int i;
    bool ok1;
    TQLabel *label;
    t_memsize used[5];

    update();			/* get the Information from memory_linux, memory_fbsd */

    /* update the byte-strings */
    for (i = TOTAL_MEM; i < MEM_LAST_ENTRY; i++) {
	label = MemSizeLabel[i][0];
	if (Memory_Info[i] == NO_MEMORY_INFO)
	    label->clear();
	else
	    label->setText(i18n("%1 bytes =").
			   arg(KGlobal::locale()->
			       formatNumber(Memory_Info[i], 0)));
    }

    /* update the MB-strings */
    for (i = TOTAL_MEM; i < MEM_LAST_ENTRY; i++) {
	label = MemSizeLabel[i][1];
	label->setText((Memory_Info[i] != NO_MEMORY_INFO)
		       ? formatted_unit(Memory_Info[i])
		       : Not_Available_Text);
    }

    /* display graphical output (ram, hdd, at last: HDD+RAM) */
    /* be careful ! Maybe we have not all info available ! */
    
    /* RAM usage: */
    /* don't rely on the SHARED_MEM value since it may refer to 
     * the size of the System V sharedmem in 2.4.x. Calculate instead! */

    used[1] = 0;
#if !defined(__svr4__) || !defined(sun)
#if !defined(__NetBSD__) && !defined(__OpenBSD__)
    used[1] = ZERO_IF_NO_INFO(Memory_Info[BUFFER_MEM]);
#endif
#endif
    used[2] = ZERO_IF_NO_INFO(Memory_Info[CACHED_MEM]);
    used[3] = ZERO_IF_NO_INFO(Memory_Info[FREE_MEM]);
    used[0] = ZERO_IF_NO_INFO(Memory_Info[TOTAL_MEM]) - used[1] - used[2] - used[3];
    if (!ram_colors_initialized) {
		ram_colors_initialized = true;
		ram_text[0] = i18n("Application Data");
		ram_colors[0] = COLOR_USED_DATA; /* used+shared */
		ram_text[1] = i18n("Disk Buffers");
		ram_colors[1] = COLOR_USED_BUFFER; /* buffers */
		ram_text[2] = i18n("Disk Cache");
		ram_colors[2] = COLOR_USED_CACHE; /* cached */
		ram_text[3] = i18n("Free Physical Memory");
		ram_colors[3] = COLOR_FREE_MEMORY; /* free */
    }
    ok1 = Display_Graph(MEM_RAM, 4, Memory_Info[TOTAL_MEM],
		      used, ram_colors, ram_text);

    /* SWAP usage: */
    used[1] = ZERO_IF_NO_INFO(Memory_Info[FREESWAP_MEM]);
    used[0] = ZERO_IF_NO_INFO(Memory_Info[SWAP_MEM]) - used[1];
    if (!swap_colors_initialized) {
		swap_colors_initialized = true;
		swap_text[0] = i18n("Used Swap");
		swap_colors[0] = COLOR_USED_SWAP; /* used */
		swap_text[1] = i18n("Free Swap");
		swap_colors[1] = COLOR_FREE_MEMORY; /* free */
    }
    Display_Graph(MEM_HDD, 2, Memory_Info[SWAP_MEM],
		      used, swap_colors, swap_text);
    
    /* RAM + SWAP usage: */
    /* used[0] already contains the amount of used swap */
    used[2] = Memory_Info[FREE_MEM] + ZERO_IF_NO_INFO(Memory_Info[FREESWAP_MEM]);
    used[1] = Memory_Info[TOTAL_MEM] - Memory_Info[FREE_MEM];
    if (!all_colors_initialized) {
		all_colors_initialized = true;
		all_text[0] = i18n("Used Memory (swap part)");
		all_colors[0] = COLOR_USED_SWAP; /* used swap */
		all_text[1] = i18n("Used Memory (physical part)");
		all_colors[1] = COLOR_USED_RAM; /* used ram */
		all_text[2] = i18n("Free Memory (total)");
		all_colors[2] = COLOR_FREE_MEMORY; /* free ram+swap*/
    }
    Display_Graph(MEM_RAM_AND_HDD, 3,
		  ok1 ? Memory_Info[TOTAL_MEM] + ZERO_IF_NO_INFO(Memory_Info[SWAP_MEM])
		  	: NO_MEMORY_INFO,
		  used, all_colors, all_text);
}


/* Include system-specific code */

#ifdef __linux__
#include "memory_linux.cpp"
#elif defined(__APPLE__)
#include "memory_osx.cpp"
#elif defined(sgi) && sgi
#include "memory_sgi.cpp"
#elif defined(__svr4__) && defined(sun)
#include "memory_solaris.cpp"
#elif defined(__FreeBSD__) || defined(__DragonFly__)
#include "memory_fbsd.cpp"
#elif defined(__hpux)
#include "memory_hpux.cpp"
#elif defined(__NetBSD__) || defined(__OpenBSD__)
#include "memory_netbsd.cpp"
#elif __osf__
#include "memory_tru64.cpp"
#else

/* Default for unsupported systems */
void KMemoryWidget::update()
{
    int i;
    for (i = TOTAL_MEM; i < MEM_LAST_ENTRY; ++i)
	Memory_Info[i] = NO_MEMORY_INFO;
}

#endif
#include "memory.moc"
