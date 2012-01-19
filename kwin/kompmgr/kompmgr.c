/*
 * $Id$
 *
 * Copyright © 2003 Keith Packard
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Keith Packard not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Keith Packard makes no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * KEITH PACKARD DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL KEITH PACKARD BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */


/* Modified by Matthew Hawn. I don't know what to say here so follow what it
   says above. Not that I can really do anything about it
*/

/* Modified by Dan Doel*/

/* Modified by Timothy Pearson
 *
 * CHANGELOG:
 * http://patchwork.freedesktop.org/patch/1049/	[Add default background color option]		08/11/2011
 * http://patchwork.freedesktop.org/patch/1052/ [Prevent flicker on root pixmap change]		08/11/2011
 * Added SIGUSR1 handler to change process UID	[Prevent flicker on login]			08/12/2011
 * Added ability to write PID of process to home directory					08/14/2011
 * Added SIGUSR2 handler to reload settings     [Prevent flicker on settings change]		08/14/2011
 * Added SIGTERM handler to clean up stale PID files on exit					08/14/2011
 * Added hack to work around ATI fglrx XDamage event generation bugs	[WORK_AROUND_FGLRX]	09/01/2011
 * Redraw root window automatically when X damage events are detected (this fixes xsetroot)	10/23/2011
 *
 * TODO:
 * http://patchwork.freedesktop.org/patch/1053/ [Fix window mapping with re-used window ids]
*/

/*
Version 2.x of xcompmgr, kompmgr changes by Thomas L�bking and Heiko Przybyl
check baghira.sf.net for more infos
*/
#define _VERSION_ 2.02
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <sys/poll.h>
#include <sys/time.h>
#include <sys/types.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <libgen.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/extensions/Xcomposite.h>
#include <X11/extensions/Xdamage.h>
#include <X11/extensions/Xrender.h>
#include <X11/extensions/shape.h>

#if COMPOSITE_MAJOR > 0 || COMPOSITE_MINOR >= 2
#define HAS_NAME_WINDOW_PIXMAP 1
#endif

#define CAN_DO_USABLE 1

#define WORK_AROUND_FGLRX 1

#define _TOPHEIGHT_(x) ((x >> 24) & 0xff)
#define _RIGHTWIDTH_(x) ((x >> 16) & 0xff)
#define _BOTTOMHEIGHT_(x) ((x >> 8) & 0xff)
#define _LEFTWIDTH_(x) (x & 0xff)

/* #define USE_ENV_HOME 1 */
#define WRITE_PID_FILE 1

#ifndef USE_ENV_HOME
#include <pwd.h>
#endif

typedef enum {
    WINTYPE_DESKTOP,
    WINTYPE_DOCK,
    WINTYPE_TOOLBAR,
    WINTYPE_MENU,
    WINTYPE_UTILITY,
    WINTYPE_SPLASH,
    WINTYPE_DIALOG,
    WINTYPE_NORMAL,
    WINTYPE_DROPDOWN_MENU,
    WINTYPE_POPUP_MENU,
    WINTYPE_TOOLTIP,
    WINTYPE_NOTIFY,
    WINTYPE_COMBO,
    WINTYPE_DND,
    NUM_WINTYPES
} wintype;

typedef struct _ignore {
    struct _ignore	*next;
    unsigned long	sequence;
} ignore;

typedef struct _win {
    struct _win		*next;
    Window		id;
#if HAS_NAME_WINDOW_PIXMAP
    Pixmap		pixmap;
#endif
    XWindowAttributes	a;
    XWindowAttributes	a_prev;
#if CAN_DO_USABLE
    Bool		usable;		    /* mapped and all damaged at one point */
    XRectangle		damage_bounds;	    /* bounds of damage */
#endif
    Bool                isInFade;
    int			mode;
    int			damaged;
    Damage		damage;
    Picture		picture;
    Picture		alphaPict;
    Picture		shadowPict;
    XserverRegion	borderSize;
    XserverRegion	decoRegion;
    XserverRegion	contentRegion;
    XserverRegion	extents;
    unsigned int	preShadeOpacity;
    Picture		shadow;
   /*Picture		alpha;*/
    int			shadow_dx;
    int			shadow_dy;
    int			shadow_width;
    int			shadow_height;
    unsigned int	opacity;
    unsigned int	shadowSize;
    wintype             windowType;
    unsigned long	damage_sequence;    /* sequence when damage was created */
    int                 destroyed;
    Bool		destruct_queued;
    Bool		destruct_requested;
    int			destruct_request_time;
    Bool                shapable; /* this will allow window managers to exclude windows if just the deco is shaped*/
    Bool		shaped;
    XRectangle		shape_bounds;
    XRectangle		shape_bounds_prev;
    unsigned int                 decoHash;
    Picture             dimPicture;

    /* for drawing translucent windows */
    XserverRegion	borderClip;
    struct _win		*prev_trans;

    /* setting whether a window will be transparent to the desktop or the windows below it */
    Bool		show_root_tile;
} win;

typedef struct _conv {
    int	    size;
    double  *data;
} conv;

typedef struct _fade {
    struct _fade	*next;
    win			*w;
    double		cur;
    double		finish;
    double		step;
    void		(*callback) (Display *dpy, win *w, Bool gone);
    Display		*dpy;
    unsigned int			decoHash;
    Bool		gone;
} fade;

struct sigaction usr_action;
sigset_t block_mask;

int		my_exit_code = 3;

win             *list;
fade		*fades;
Display		*dpy;
char	        *display = 0;
int		scr;
Window		root;
Picture		rootPicture;
Picture		rootBuffer;
Picture		blackPicture;
Picture		transBlackPicture;
Picture		rootTile;
XserverRegion	allDamage;
Bool		clipChanged;
#if HAS_NAME_WINDOW_PIXMAP
Bool		hasNamePixmap;
#endif
XRenderColor	fill_color;
int		root_height, root_width;
ignore		*ignore_head, **ignore_tail = &ignore_head;
int		xfixes_event, xfixes_error;
int		damage_event, damage_error;
int		composite_event, composite_error;
int		render_event, render_error;
int		xshape_event, xshape_error;
Bool		synchronize;
int		composite_opcode;
Bool		screen_damaged = False;
Bool            disable_argb = False;

int             shapeEvent;

/* find these once and be done with it */
Atom		opacityAtom;
Atom		shadowAtom;
Atom		shadeAtom;
Atom		shapableAtom;
Atom            decoHashAtom;
Atom            dimAtom;
Atom            deskChangeAtom;
Atom            winTypeAtom;
Atom            winTDETTDAtom;
Atom            winType[NUM_WINTYPES];
double          winTypeOpacity[NUM_WINTYPES];
Bool            winTypeShadow[NUM_WINTYPES];
Bool            winTypeFade[NUM_WINTYPES];

/* opacity property name; sometime soon I'll write up an EWMH spec for it */
#define OPACITY_PROP	"_KDE_WM_WINDOW_OPACITY"
#define SHADOW_PROP	"_KDE_WM_WINDOW_SHADOW"
#define SHADE_PROP	"_KDE_WM_WINDOW_SHADE"
#define SHAPABLE_PROP	"_KDE_WM_WINDOW_SHAPABLE"
#define DECOHASH_PROP	"_KDE_WM_WINDOW_DECOHASH"
#define DIM_PROP	"_KDE_WM_WINDOW_DIM"
#define DESKCHANGE_PROP "_KDE_WM_DESKTOP_CHANGE"

#define TRANSLUCENT	0xe0000000
#define OPAQUE		0xffffffff

conv            *gaussianMap;

#define WINDOW_SOLID	0
#define WINDOW_TRANS	1
#define WINDOW_ARGB	2

#define TRANS_OPACITY	0.75

#define NDEBUG 1
#define DEBUG_REPAINT 0
#define DEBUG_WINDOWS 0
#define DEBUG_EVENTS 0
#define MONITOR_REPAINT 0

#define SHADOWS		1
#define SHARP_SHADOW	0

typedef enum _compMode {
    CompSimple,		/* looks like a regular X server */
    CompServerShadows,	/* use window alpha for shadow; sharp, but precise */
    CompClientShadows	/* use window extents for shadow, blurred */
} CompMode;

static void
determine_mode(Display *dpy, win *w);

static double
get_opacity_percent(Display *dpy, win *w);

static XserverRegion
win_extents (Display *dpy, win *w);

CompMode    compMode = CompSimple;

int	    shadowRadius = 12;
int         shadowOffsetX = 0;
int         shadowOffsetY = 0;
double      shadowOpacity = .75;
XRenderColor shadowColor;

double  fade_in_step =  0.028;
double  fade_out_step = 0.03;
int	fade_delta =	10;
int	fade_time =	0;
Bool	fadeTrans = False;

Bool	autoRedirect = False;

#if WORK_AROUND_FGLRX
Bool	restartOnSigterm = True;
#endif

/* For shadow precomputation */
int            Gsize = -1;
unsigned char *shadowCorner = NULL;
unsigned char *shadowTop = NULL;

XRenderPictFormat* sXRenderFindVisualFormat(Display *dpy, _Xconst Visual *visual)
{
    XRenderPictFormat* format = XRenderFindVisualFormat(dpy,visual);
    if (format)
        return format;
    else
        return XRenderFindStandardFormat (dpy, PictStandardRGB24);
}

int
get_time_in_milliseconds ()
{
    struct timeval  tv;

    gettimeofday (&tv, NULL);
    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

void write_pid_file(pid_t pid)
{
#ifdef WRITE_PID_FILE
#ifdef USE_ENV_HOME
    const char *home = getenv("HOME");
#else
    const char *home;
    struct passwd *p;
    p = getpwuid(getuid());
    if (p)
        home = p->pw_dir;
    else
        home = getenv("HOME");
#endif
    const char *filename;
    const char *configfile = "/.kompmgr.pid";
    int n = strlen(home)+strlen(configfile)+1;
    filename = (char*)malloc(n*sizeof(char));
    memset(filename,0,n);
    strcat(filename, home);
    strcat(filename, configfile);

    printf("writing '%s' as pidfile\n\n", filename);

    /* now that we did all that by way of introduction...write the file! */
    FILE *pFile;
    char buffer[255];
    sprintf(buffer, "%d", pid);
    pFile = fopen(filename, "w");
    if (pFile) {
        fwrite(buffer,1,strlen(buffer), pFile);
        fclose(pFile);
    }

    free(filename);
    filename = NULL;
#endif
}

void delete_pid_file()
{
#ifdef WRITE_PID_FILE
#ifdef USE_ENV_HOME
    const char *home = getenv("HOME");
#else
    const char *home;
    struct passwd *p;
    p = getpwuid(getuid());
    if (p)
        home = p->pw_dir;
    else
        home = getenv("HOME");
#endif
    const char *filename;
    const char *configfile = "/.kompmgr.pid"; 
    int n = strlen(home)+strlen(configfile)+1;
    filename = (char*)malloc(n*sizeof(char));
    memset(filename,0,n);
    strcat(filename, home);
    strcat(filename, configfile);

    printf("deleting '%s' as pidfile\n\n", filename);

    /* now that we did all that by way of introduction...delete the file! */
    unlink(filename);

    free(filename);
    filename = NULL;
#endif

#if WORK_AROUND_FGLRX
    if ((my_exit_code == 3) && (restartOnSigterm)) {
        printf("kompmgr lost connection to X server, restarting...\n\r"); fflush(stdout);
        sleep(1);
        char me[2048];
        int chars = readlink("/proc/self/exe", me, sizeof(me));
        me[chars] = 0;
        me[2047] = 0;
	execl(me, basename(me), (char*)NULL);
    }
#endif
}

void handle_siguser (int sig)
{
    int uidnum;
    if (sig == SIGTERM) {
        // Trap this signal and keep running...
        return;
    }
    if (sig == SIGUSR1) {
        char newuid[1024];
#ifndef NDEBUG
        printf("Enter the new user ID:\n\r"); fflush(stdout);
#endif
        char *eof;
        newuid[0] = '\0';
        newuid[sizeof(newuid)-1] = '\0';
        eof = fgets(newuid, sizeof(newuid), stdin);
        uidnum = atoi(newuid);
#ifndef NDEBUG
        printf("Setting kompmgr process uid to %d...\n\r", uidnum); fflush(stdout);
#endif

        my_exit_code=4;
        delete_pid_file();
        my_exit_code=3;
        setuid(uidnum);
        write_pid_file(getpid());

    }
    else {
        uidnum = getuid();
    }
    if ((sig == SIGUSR1) || (sig == SIGUSR2)) {
#ifdef USE_ENV_HOME
        const char *home = getenv("HOME");
#else
        const char *home;
        struct passwd *p;
        p = getpwuid(uidnum);
        if (p)
            home = p->pw_dir;
        else
            home = getenv("HOME");
#endif
        const char *filename;
        const char *configfile = "/.xcompmgrrc";
        int n = strlen(home)+strlen(configfile)+1;
        filename = (char*)malloc(n*sizeof(char));
        memset(filename,0,n);
        strcat(filename, home);
        strcat(filename, configfile);

        loadConfig(filename); /* reload the configuration file */

        free(filename);
        filename = NULL;
    }
}

fade *
find_fade (win *w)
{
    fade    *f;

    for (f = fades; f; f = f->next)
    {
	if (f->w == w)
	    return f;
    }
    return 0;
}

void dequeue_fade (Display *dpy, fade *f)
{
   fade    **prev;
   f->w->isInFade = False;
   f->w->decoHash = f->decoHash;

   for (prev = &fades; *prev; prev = &(*prev)->next)
      if (*prev == f)
      {
         *prev = f->next;
         if (f->callback)
         {
            (*f->callback) (dpy, f->w, f->gone);
         }
         free (f);
         break;
      }
}

void
cleanup_fade (Display *dpy, win *w)
{
    fade *f = find_fade (w);
    if (f)
      dequeue_fade (dpy, f);
}

void
enqueue_fade (Display *dpy, fade *f)
{
    f->w->isInFade = True;
    if (!fades)
      fade_time = get_time_in_milliseconds () + fade_delta;
    f->next = fades;
    fades = f;
}

static void unmap_callback (Display *dpy, win *w, Bool gone);

static void
set_fade (Display *dpy, win *w, double start, double finish, double step,
		void (*callback) (Display *dpy, win *w, Bool gone),
		Bool gone, Bool exec_callback, Bool override, Bool wholeWin)
{
	fade    *f;

	f = find_fade (w);
	if (!f)
	{
		if (start == finish)
			return;
		f = malloc (sizeof (fade));
		f->next = 0;
		f->w = w;
		f->decoHash = w->decoHash;
		f->cur = start;
		enqueue_fade (dpy, f);
	}
	else if(!override)
		return;
	else
	{
		if (exec_callback && f->callback)
			(*f->callback)(dpy, f->w, f->gone);
	}
	if (finish < 0)
		finish = 0;
	if (finish > 1)
		finish = 1;
	f->finish = finish;
	if (f->cur < finish)
		f->step = step;
	else if (f->cur > finish)
		f->step = -step;
	f->gone = gone && (exec_callback || f->callback != unmap_callback);
	f->callback = callback;
	w->opacity = f->cur * OPAQUE;
	if (wholeWin)
		w->decoHash = 0;
#if 0
	printf ("set_fade start %g step %g\n", f->cur, f->step);
#endif
	determine_mode (dpy, w);
	if (w->shadow)
	{
		XRenderFreePicture (dpy, w->shadow);
		w->shadow = None;
                if( w->extents != None )
                    XFixesDestroyRegion( dpy, w->extents );
		w->extents = win_extents (dpy, w);
	}

	/* fading windows need to be drawn, mark them as damaged.
	   when a window maps, if it tries to fade in but it already at the right
	   opacity (map/unmap/map fast) then it will never get drawn without this
	   until it repaints */
	w->damaged = 1;
}

	int
fade_timeout (void)
{
	int now;
	int	delta;
	if (!fades)
		return -1;
	now = get_time_in_milliseconds();
	delta = fade_time - now;
	if (delta < 0)
		delta = 0;
	/*    printf ("timeout %d\n", delta); */
	return delta;
}

	void
run_fades (Display *dpy)
{
	int	    now = get_time_in_milliseconds();
	fade    *f, *next;
	int	    steps;
	Bool    need_dequeue;

#if 0
	printf ("run fades\n");
#endif
	if (fade_time - now > 0)
		return;
	steps = 1 + (now - fade_time) / fade_delta;
        for (next = fades; (f = next); )
	{
		win *w = f->w;
		next = f->next;
		f->cur += f->step * steps;
		if (f->cur >= 1)
			f->cur = 1;
		else if (f->cur < 0)
			f->cur = 0;
#if 0
		printf ("opacity now %g -> %g\n", f->cur, f->finish);
#endif
		w->opacity = f->cur * OPAQUE;
		need_dequeue = False;
		if (f->step > 0)
		{
			if (f->cur >= f->finish)
			{
				w->opacity = f->finish*OPAQUE;
				need_dequeue = True;
			}
		}
		else
		{
			if (f->cur <= f->finish)
			{
				w->opacity = f->finish*OPAQUE;
				need_dequeue = True;
			}
		}
		if (w->shadow)
		{
			XRenderFreePicture (dpy, w->shadow);
			w->shadow = None;
                        if( w->extents != None )
                            XFixesDestroyRegion( dpy, w->extents );
			w->extents = win_extents(dpy, w);
		}
		determine_mode (dpy, w);
		/* Must do this last as it might destroy f->w in callbacks */
		if (need_dequeue)
			dequeue_fade (dpy, f);
	}
	fade_time = now + fade_delta;
}

#define SHADOW_OFFSET_X	((-shadowRadius * 7 / 5) - shadowOffsetX * shadowRadius / 100) * w->shadowSize
#define SHADOW_OFFSET_Y	((-shadowRadius * 7 / 5) - shadowOffsetY * shadowRadius / 100) * w->shadowSize
/*#define SHADOW_OFFSET_X	(w->shadowSize * -shadowRadius * 7 / 500) - w->shadowSize * shadowOffsetX * shadowRadius / 10000
#define SHADOW_OFFSET_Y	(w->shadowSize * -shadowRadius * 7 / 500) - w->shadowSize * shadowOffsetY * shadowRadius / 10000*/

	static double
gaussian (double r, double x, double y)
{
	return ((1 / (sqrt (2 * M_PI * r))) *
			exp ((- (x * x + y * y)) / (2 * r * r)));
}


	static conv *
make_gaussian_map (Display *dpy, double r)
{
	conv	    *c;
	int		    size = ((int) ceil ((r * 3)) + 1) & ~1;
	int		    center = size / 2;
	int		    x, y;
	double	    t;
	double	    g;

	c = malloc (sizeof (conv) + size * size * sizeof (double));
	c->size = size;
	c->data = (double *) (c + 1);
	t = 0.0;
	for (y = 0; y < size; y++)
		for (x = 0; x < size; x++)
		{
			g = gaussian (r, (double) (x - center), (double) (y - center));
			t += g;
			c->data[y * size + x] = g;
		}
	/*    printf ("gaussian total %f\n", t); */
	for (y = 0; y < size; y++)
		for (x = 0; x < size; x++)
		{
			c->data[y*size + x] /= t;
		}
	return c;
}

/*
 * A picture will help
 *
 *	-center   0                width  width+center
 *  -center +-----+-------------------+-----+
 *	    |     |                   |     |
 *	    |     |                   |     |
 *        0 +-----+-------------------+-----+
 *	    |     |                   |     |
 *	    |     |                   |     |
 *	    |     |                   |     |
 *   height +-----+-------------------+-----+
 *	    |     |                   |     |
 * height+  |     |                   |     |
 *  center  +-----+-------------------+-----+
 */

	static unsigned char
sum_gaussian (conv *map, double opacity, int x, int y, int width, int height)
{
	int	    fx, fy;
	double  *g_data;
	double  *g_line = map->data;
	int	    g_size = map->size;
	int	    center = g_size / 2;
	int	    fx_start, fx_end;
	int	    fy_start, fy_end;
	double  v;

	/*
	 * Compute set of filter values which are "in range",
	 * that's the set with:
	 *	0 <= x + (fx-center) && x + (fx-center) < width &&
	 *  0 <= y + (fy-center) && y + (fy-center) < height
	 *
	 *  0 <= x + (fx - center)	x + fx - center < width
	 *  center - x <= fx	fx < width + center - x
	 */

	fx_start = center - x;
	if (fx_start < 0)
		fx_start = 0;
	fx_end = width + center - x;
	if (fx_end > g_size)
		fx_end = g_size;

	fy_start = center - y;
	if (fy_start < 0)
		fy_start = 0;
	fy_end = height + center - y;
	if (fy_end > g_size)
		fy_end = g_size;

	g_line = g_line + fy_start * g_size + fx_start;

	v = 0;
	for (fy = fy_start; fy < fy_end; fy++)
	{
		g_data = g_line;
		g_line += g_size;

		for (fx = fx_start; fx < fx_end; fx++)
			v += *g_data++;
	}
	if (v > 1)
		v = 1;

	return ((unsigned char) (v * opacity * 255.0));
}

/* precompute shadow corners and sides to save time for large windows */
	static void
presum_gaussian (conv *map)
{
	int center = map->size/2;
	int opacity, x, y;

	Gsize = map->size;

	if (shadowCorner)
		free ((void *)shadowCorner);
	if (shadowTop)
		free ((void *)shadowTop);

	shadowCorner = (unsigned char *)(malloc ((Gsize + 1) * (Gsize + 1) * 26));
	shadowTop = (unsigned char *)(malloc ((Gsize + 1) * 26));

	for (x = 0; x <= Gsize; x++)
	{
		shadowTop[25 * (Gsize + 1) + x] = sum_gaussian (map, 1, x - center, center, Gsize * 2, Gsize * 2);
		for(opacity = 0; opacity < 25; opacity++)
			shadowTop[opacity * (Gsize + 1) + x] = shadowTop[25 * (Gsize + 1) + x] * opacity / 25;
		for(y = 0; y <= x; y++)
		{
			shadowCorner[25 * (Gsize + 1) * (Gsize + 1) + y * (Gsize + 1) + x]
				= sum_gaussian (map, 1, x - center, y - center, Gsize * 2, Gsize * 2);
			shadowCorner[25 * (Gsize + 1) * (Gsize + 1) + x * (Gsize + 1) + y]
				= shadowCorner[25 * (Gsize + 1) * (Gsize + 1) + y * (Gsize + 1) + x];
			for(opacity = 0; opacity < 25; opacity++)
				shadowCorner[opacity * (Gsize + 1) * (Gsize + 1) + y * (Gsize + 1) + x]
					= shadowCorner[opacity * (Gsize + 1) * (Gsize + 1) + x * (Gsize + 1) + y]
					= shadowCorner[25 * (Gsize + 1) * (Gsize + 1) + y * (Gsize + 1) + x] * opacity / 25;
		}
	}
}

	static XImage *
make_shadow (Display *dpy, double opacity, int width, int height)
{
	XImage	    *ximage;
	unsigned char   *data;
	int		    gsize = gaussianMap->size;
	int		    ylimit, xlimit;
	int		    swidth = width + gsize;
	int		    sheight = height + gsize;
	int		    center = gsize / 2;
	int		    x, y;
	unsigned char   d;
	int		    x_diff;
	int             opacity_int = (int)(opacity * 25);
	data = malloc (swidth * sheight * sizeof (unsigned char));
	if (!data)
		return 0;
	ximage = XCreateImage (dpy,
			DefaultVisual(dpy, DefaultScreen(dpy)),
			8,
			ZPixmap,
			0,
			(char *) data,
			swidth, sheight, 8, swidth * sizeof (unsigned char));
	if (!ximage)
	{
		free (data);
		return 0;
	}
	/*
	 * Build the gaussian in sections
	 */

	/*
	 * center (fill the complete data array)
	 */
	if (Gsize > 0)
		d = shadowTop[opacity_int * (Gsize + 1) + Gsize];
	else
		d = sum_gaussian (gaussianMap, opacity, center, center, width, height);
	memset(data, d, sheight * swidth);

	/*
	 * corners
	 */
	ylimit = gsize;
	if (ylimit > sheight / 2)
		ylimit = (sheight + 1) / 2;
	xlimit = gsize;
	if (xlimit > swidth / 2)
		xlimit = (swidth + 1) / 2;

	for (y = 0; y < ylimit; y++)
		for (x = 0; x < xlimit; x++)
		{
			if (xlimit == Gsize && ylimit == Gsize)
				d = shadowCorner[opacity_int * (Gsize + 1) * (Gsize + 1) + y * (Gsize + 1) + x];
			else
				d = sum_gaussian (gaussianMap, opacity, x - center, y - center, width, height);
			data[y * swidth + x] = d;
			data[(sheight - y - 1) * swidth + x] = d;
			data[(sheight - y - 1) * swidth + (swidth - x - 1)] = d;
			data[y * swidth + (swidth - x - 1)] = d;
		}

	/*
	 * top/bottom
	 */
	x_diff = swidth - (gsize * 2);
	if (x_diff > 0 && ylimit > 0)
	{
		for (y = 0; y < ylimit; y++)
		{
			if (ylimit == Gsize)
				d = shadowTop[opacity_int * (Gsize + 1) + y];
			else
				d = sum_gaussian (gaussianMap, opacity, center, y - center, width, height);
			memset (&data[y * swidth + gsize], d, x_diff);
			memset (&data[(sheight - y - 1) * swidth + gsize], d, x_diff);
		}
	}

	/*
	 * sides
	 */

	for (x = 0; x < xlimit; x++)
	{
		if (xlimit == Gsize)
			d = shadowTop[opacity_int * (Gsize + 1) + x];
		else
			d = sum_gaussian (gaussianMap, opacity, x - center, center, width, height);
		for (y = gsize; y < sheight - gsize; y++)
		{
			data[y * swidth + x] = d;
			data[y * swidth + (swidth - x - 1)] = d;
		}
	}

	return ximage;
}

	static Picture
shadow_picture (Display *dpy, double opacity, Picture alpha_pict, int width, int height, int *wp, int *hp)
{
	XImage  *shadowImage;
	Pixmap  shadowPixmap;
	Pixmap  finalPixmap;
	Picture shadowPicture;
	Picture finalPicture;
	GC	    gc;

	shadowImage = make_shadow (dpy, opacity, width, height);
	if (!shadowImage)
		return None;
	shadowPixmap = XCreatePixmap (dpy, root, 
			shadowImage->width,
			shadowImage->height,
			8);
	if (!shadowPixmap)
	{
		XDestroyImage (shadowImage);
		return None;
	}

	shadowPicture = XRenderCreatePicture (dpy, shadowPixmap,
			XRenderFindStandardFormat (dpy, PictStandardA8),
			0, 0);
	if (!shadowPicture)
	{
		XDestroyImage (shadowImage);
		XFreePixmap (dpy, shadowPixmap);
		return None;
	}

	gc = XCreateGC (dpy, shadowPixmap, 0, 0);
	if (!gc)
	{
		XDestroyImage (shadowImage);
		XFreePixmap (dpy, shadowPixmap);
		XRenderFreePicture (dpy, shadowPicture);
		return None;
	}

	XPutImage (dpy, shadowPixmap, gc, shadowImage, 0, 0, 0, 0, 
			shadowImage->width,
			shadowImage->height);
	*wp = shadowImage->width;
	*hp = shadowImage->height;
	XFreeGC (dpy, gc);
	XDestroyImage (shadowImage);
	XFreePixmap (dpy, shadowPixmap);
	return shadowPicture;
}

	Picture
solid_picture (Display *dpy, Bool argb, double a, double r, double g, double b)
{
	Pixmap			pixmap;
	Picture			picture;
	XRenderPictureAttributes	pa;
	XRenderColor		c;

	pixmap = XCreatePixmap (dpy, root, 1, 1, argb ? 32 : 8);
	if (!pixmap)
		return None;

	pa.repeat = True;
	picture = XRenderCreatePicture (dpy, pixmap,
			XRenderFindStandardFormat (dpy, argb ? PictStandardARGB32 : PictStandardA8),
			CPRepeat,
			&pa);
	if (!picture)
	{
		XFreePixmap (dpy, pixmap);
		return None;
	}

	c.alpha = a * 0xffff;
	c.red = r * 0xffff;
	c.green = g * 0xffff;
	c.blue = b * 0xffff;
	XRenderFillRectangle (dpy, PictOpSrc, picture, &c, 0, 0, 1, 1);
	XFreePixmap (dpy, pixmap);
	return picture;
}

	void
discard_ignore (Display *dpy, unsigned long sequence)
{
	while (ignore_head)
	{
		if ((long) (sequence - ignore_head->sequence) > 0)
		{
			ignore  *next = ignore_head->next;
			free (ignore_head);
			ignore_head = next;
			if (!ignore_head)
				ignore_tail = &ignore_head;
		}
		else
			break;
	}
}

	void
set_ignore (Display *dpy, unsigned long sequence)
{
	ignore  *i = malloc (sizeof (ignore));
	if (!i)
		return;
	i->sequence = sequence;
	i->next = 0;
	*ignore_tail = i;
	ignore_tail = &i->next;
}

	int
should_ignore (Display *dpy, unsigned long sequence)
{
	discard_ignore (dpy, sequence);
	return ignore_head && ignore_head->sequence == sequence;
}

	static win *
find_win (Display *dpy, Window id)
{
	win	*w;

	for (w = list; w; w = w->next)
		if ((!w->destroyed) && (w->id == id))
			return w;
	return 0;
}

static char *backgroundProps[] = {
	"_XROOTPMAP_ID",
	"_XSETROOT_ID",
	0,
};

static Picture
root_tile (Display *dpy)
{
	Picture	    picture;
	Atom	    actual_type;
	Pixmap	    pixmap;
	int		    actual_format;
	unsigned long   nitems;
	unsigned long   bytes_after;
	unsigned char   *prop;
	Bool	    fill;
	XRenderPictureAttributes	pa;
	int		    p;

	pixmap = None;
	for (p = 0; backgroundProps[p]; p++)
	{
		if (XGetWindowProperty (dpy, root, XInternAtom (dpy, backgroundProps[p], False),
					0, 4, False, AnyPropertyType,
					&actual_type, &actual_format, &nitems, &bytes_after, &prop) == Success &&
				actual_type == XInternAtom (dpy, "PIXMAP", False) && actual_format == 32 && nitems == 1)
		{
			pixmap = *(long*)prop;
			XFree (prop);
			fill = False;
			break;
		}
	}
	if (!pixmap)
	{
		pixmap = XCreatePixmap (dpy, root, 1, 1, DefaultDepth (dpy, scr));
		fill = True;
	}
	pa.repeat = True;
	picture = XRenderCreatePicture (dpy, pixmap,
			sXRenderFindVisualFormat (dpy,
				DefaultVisual (dpy, scr)),
			CPRepeat, &pa);
	if (fill)
	{
		XRenderFillRectangle (dpy, PictOpSrc, picture, &fill_color,
				0, 0, 1, 1);
	}
	return picture;
}

static void
paint_root (Display *dpy)
{
	if (!rootTile)
		rootTile = root_tile (dpy);

	XRenderComposite (dpy, PictOpSrc,
			rootTile, None, rootBuffer,
			0, 0, 0, 0, 0, 0, root_width, root_height);
}

static XserverRegion
win_extents (Display *dpy, win *w)
{
	XRectangle r;

	r.x = w->a.x;
	r.y = w->a.y;
	r.width = w->a.width + w->a.border_width * 2;
	r.height = w->a.height + w->a.border_width * 2;
	if (winTypeShadow[w->windowType])
	{
		if (compMode == CompServerShadows || w->mode != WINDOW_ARGB)
		{
			XRectangle  sr;

			if (compMode == CompServerShadows)
			{
				w->shadow_dx = 2;
				w->shadow_dy = 7;
				w->shadow_width = w->a.width;
				w->shadow_height = w->a.height;
			}
			else
			{
				w->shadow_dx = SHADOW_OFFSET_X;
				w->shadow_dx = w->shadow_dx / 100;
				w->shadow_dy = SHADOW_OFFSET_Y;
				w->shadow_dy = w->shadow_dy / 100;
				if (!w->shadow)
				{
					double	opacity = shadowOpacity;
					if (w->shadowSize > 100)
						opacity = opacity/(w->shadowSize*0.015);
					if (w->mode == WINDOW_TRANS)
						opacity = opacity * ((double)w->opacity)/((double)OPAQUE);
					w->shadow = shadow_picture (dpy, opacity, w->alphaPict,
							w->a.width + w->a.border_width * 2 - 2*(shadowRadius - (w->shadowSize*shadowRadius/100)) ,
							w->a.height + w->a.border_width * 2 - 2*(shadowRadius - (w->shadowSize*shadowRadius/100)),
							&w->shadow_width, &w->shadow_height);
   				/*int kill;
   				w->alpha = shadow_picture (dpy, 0.9, w->alphaPict,
   				                            w->a.width + w->a.border_width * 2,
   				                            w->a.height + w->a.border_width * 2,
   				                            &kill, &kill);*/
				}
			}
			sr.x = w->a.x + w->shadow_dx;
			sr.y = w->a.y + w->shadow_dy;
			sr.width = w->shadow_width;
			sr.height = w->shadow_height;
			if (sr.x < r.x)
			{
				r.width = (r.x + r.width) - sr.x;
				r.x = sr.x;
			}
			if (sr.y < r.y)
			{
				r.height = (r.y + r.height) - sr.y;
				r.y = sr.y;
			}
			if (sr.x + sr.width > r.x + r.width)
				r.width = sr.x + sr.width - r.x;
			if (sr.y + sr.height > r.y + r.height)
				r.height = sr.y + sr.height - r.y;
		}
	}
	return XFixesCreateRegion (dpy, &r, 1);
}

	static XserverRegion
border_size (Display *dpy, win *w)
{
	XserverRegion   border;
	/*
	 * if window doesn't exist anymore,  this will generate an error
	 * as well as not generate a region.  Perhaps a better XFixes
	 * architecture would be to have a request that copies instead
	 * of creates, that way you'd just end up with an empty region
	 * instead of an invalid XID.
	 */
	set_ignore (dpy, NextRequest (dpy));
	border = XFixesCreateRegionFromWindow (dpy, w->id, WindowRegionBounding);
	/* translate this */
	set_ignore (dpy, NextRequest (dpy));
	XFixesTranslateRegion (dpy, border,
			w->a.x + w->a.border_width,
			w->a.y + w->a.border_width);
	return border;
}

	static XserverRegion
deco_region (Display *dpy, win *w)
{
	XserverRegion   title;
	XRectangle	    r; /*titlebounding rect*/
	/*
	 * if window doesn't exist anymore,  this will generate an error
	 * as well as not generate a region.  Perhaps a better XFixes
	 * architecture would be to have a request that copies instead
	 * of creates, that way you'd just end up with an empty region
	 * instead of an invalid XID.
	 */
   r.x = w->a.x - w->a.border_width + _LEFTWIDTH_(w->decoHash);
   r.y = w->a.y - w->a.border_width + _TOPHEIGHT_(w->decoHash);
   r.width = w->a.width + w->a.border_width * 2 - _LEFTWIDTH_(w->decoHash) - _RIGHTWIDTH_(w->decoHash);
   r.height = w->a.height + w->a.border_width - _TOPHEIGHT_(w->decoHash) - _BOTTOMHEIGHT_(w->decoHash);
	set_ignore (dpy, NextRequest (dpy));
	title = XFixesCreateRegion (dpy, &r, 1);
	if (!w->borderSize)
		w->borderSize = border_size (dpy, w);
	set_ignore (dpy, NextRequest (dpy));
   XFixesSubtractRegion(dpy, title, w->borderSize, title);
	return title;
}

static void finish_destroy_win (Display *dpy, Window id, Bool gone);

	static XserverRegion
content_region (Display *dpy, win *w)
{
	XserverRegion   content;
	XRectangle	    r; /*contentbounding rect*/
	/*
	 * if window doesn't exist anymore,  this will generate an error
	 * as well as not generate a region.  Perhaps a better XFixes
	 * architecture would be to have a request that copies instead
	 * of creates, that way you'd just end up with an empty region
	 * instead of an invalid XID.
	 */
   r.x = w->a.x - w->a.border_width + _LEFTWIDTH_(w->decoHash);
   r.y = w->a.y - w->a.border_width + _TOPHEIGHT_(w->decoHash);
   r.width = w->a.width + w->a.border_width * 2 - _LEFTWIDTH_(w->decoHash) - _RIGHTWIDTH_(w->decoHash);
   r.height = w->a.height + w->a.border_width - _TOPHEIGHT_(w->decoHash) - _BOTTOMHEIGHT_(w->decoHash);
	set_ignore (dpy, NextRequest (dpy));
	content = XFixesCreateRegion (dpy, &r, 1);
	if (!w->borderSize)
		w->borderSize = border_size (dpy, w);
	set_ignore (dpy, NextRequest (dpy));
	XFixesIntersectRegion(dpy, content, w->borderSize, content);
	return content;
}

	static void
paint_all (Display *dpy, XserverRegion region)
{
	win	*w;
	win	*t = 0;

#if DEBUG_WINDOWS
	int window_count = 0;
#endif

	if (!region)
	{
		XRectangle  r;
		r.x = 0;
		r.y = 0;
		r.width = root_width;
		r.height = root_height;
		region = XFixesCreateRegion (dpy, &r, 1);
	}
#if MONITOR_REPAINT
	rootBuffer = rootPicture;
#else
	if (!rootBuffer)
	{
		Pixmap	rootPixmap = XCreatePixmap (dpy, root, root_width, root_height,
				DefaultDepth (dpy, scr));
		rootBuffer = XRenderCreatePicture (dpy, rootPixmap,
				sXRenderFindVisualFormat (dpy,
					DefaultVisual (dpy, scr)),
				0, 0);
		XFreePixmap (dpy, rootPixmap);
	}
#endif
	XFixesSetPictureClipRegion (dpy, rootPicture, 0, 0, region);
#if MONITOR_REPAINT
	XRenderComposite (dpy, PictOpSrc, blackPicture, None, rootPicture,
			0, 0, 0, 0, 0, 0, root_width, root_height);
#endif
#if DEBUG_REPAINT
	printf ("paint:");
#endif

	// Time delayed garbage collect
	// It waits 10 seconds before destroying window data
	// This allows the fade out to perform smoothly under all conditions
	// Yes, this code is somewhat inefficient!
	// But it shouldn't matter unless someone has tens of thousands of windows open...
	// If the user can set a fade out that is longer than 10 seconds,
	// then the value must be increased.  I am assuming that 10 seconds
	// is far too long for any normal human being to wait... ;-)
	for (w = list; w; w = w->next)
	{
		if (w->destruct_requested) {
			int curtime = get_time_in_milliseconds();
			if ((curtime - w->destruct_request_time) > 10000) {
				finish_destroy_win (dpy, w->id, True);
				w = list;
			}
		}
	}

	for (w = list; w; w = w->next)
	{
#if DEBUG_WINDOWS
		window_count++;
#endif
#if CAN_DO_USABLE
		if (!w->usable)
			continue;
#endif

		/* never painted, ignore it */
		if ((!screen_damaged) && (!w->damaged))
			continue;

		/* skip invisible windows */
		if (w->a.x + w->a.width < 1 || w->a.y + w->a.height < 1 || w->a.x >= root_width || w->a.y >= root_height)
			continue;

		if (!w->picture)
		{
			XRenderPictureAttributes	pa;
			XRenderPictFormat		*format;
			Drawable			draw = w->id;

#if HAS_NAME_WINDOW_PIXMAP
			if (hasNamePixmap && !w->pixmap)
				w->pixmap = XCompositeNameWindowPixmap (dpy, w->id);
			if (w->pixmap)
				draw = w->pixmap;
#endif
			format = sXRenderFindVisualFormat (dpy, w->a.visual);
			pa.subwindow_mode = IncludeInferiors;
			w->picture = XRenderCreatePicture (dpy, draw,
					format,
					CPSubwindowMode,
					&pa);
		}
#if DEBUG_REPAINT
		printf (" 0x%x", w->id);
#endif
		if (clipChanged)
		{
			if (w->borderSize)
			{
				set_ignore (dpy, NextRequest (dpy));
				XFixesDestroyRegion (dpy, w->borderSize);
				w->borderSize = None;
			}
			if (w->decoRegion)
			{
				set_ignore (dpy, NextRequest (dpy));
				XFixesDestroyRegion (dpy, w->decoRegion);
				w->decoRegion = None;
			}
			if (w->contentRegion)
			{
				set_ignore (dpy, NextRequest (dpy));
				XFixesDestroyRegion (dpy, w->contentRegion);
				w->contentRegion = None;
			}
			if (w->extents)
			{
				XFixesDestroyRegion (dpy, w->extents);
				w->extents = None;
			}
			if (w->borderClip)
			{
				XFixesDestroyRegion (dpy, w->borderClip);
				w->borderClip = None;
			}
		}
		if (!w->borderSize)
			w->borderSize = border_size (dpy, w);
		if (!w->extents)
			w->extents = win_extents (dpy, w);
		if ((w->mode == WINDOW_SOLID) || ((w->mode == WINDOW_TRANS) && w->decoHash))
		{
			int	x, y, wid, hei;
#if HAS_NAME_WINDOW_PIXMAP
			x = w->a.x;
			y = w->a.y;
			wid = w->a.width + w->a.border_width * 2;
			hei = w->a.height + w->a.border_width * 2;
#else
			x = w->a.x + w->a.border_width;
			y = w->a.y + w->a.border_width;
			wid = w->a.width;
			hei = w->a.height;
#endif
			XFixesSetPictureClipRegion (dpy, rootBuffer, 0, 0, region);
			set_ignore (dpy, NextRequest (dpy));
			/*XFixesSubtractRegion (dpy, region, region, w->borderSize);
			  set_ignore (dpy, NextRequest (dpy));*/
			if (w->mode == WINDOW_SOLID)
			{
				XFixesSubtractRegion (dpy, region, region, w->borderSize);
				set_ignore (dpy, NextRequest (dpy));
				XRenderComposite (dpy, PictOpSrc, w->picture, None, rootBuffer,
						0, 0, 0, 0, x, y, wid, hei);
				if (w->dimPicture)
					XRenderComposite (dpy, PictOpOver, w->dimPicture, None, rootBuffer, 0, 0, 0, 0, x, y, wid, hei);
			}
			else
			{
				if (!w->contentRegion)
					w->contentRegion = content_region (dpy, w);
				XFixesSubtractRegion (dpy, region, region, w->contentRegion);
				set_ignore (dpy, NextRequest (dpy));
				/*solid part*/
				XRenderComposite (dpy, PictOpSrc, w->picture, None, rootBuffer,
						_LEFTWIDTH_(w->decoHash), _TOPHEIGHT_(w->decoHash), 0, 0,
						x + _LEFTWIDTH_(w->decoHash),
						y + _TOPHEIGHT_(w->decoHash),
						wid - _LEFTWIDTH_(w->decoHash) - _RIGHTWIDTH_(w->decoHash),
						hei - _TOPHEIGHT_(w->decoHash) - _BOTTOMHEIGHT_(w->decoHash));
			}
		}
		if (!w->borderClip)
		{
			w->borderClip = XFixesCreateRegion (dpy, 0, 0);
			XFixesCopyRegion (dpy, w->borderClip, region);
			XFixesIntersectRegion(dpy, w->borderClip, w->borderClip, w->borderSize);
		}
		w->prev_trans = t;
		t = w;
	}
#if DEBUG_WINDOWS
	printf("window count: %d\n\r", window_count);
#endif
#if DEBUG_REPAINT
	printf ("\n");
	fflush (stdout);
#endif
	XFixesSetPictureClipRegion (dpy, rootBuffer, 0, 0, region);
	paint_root (dpy);
	for (w = t; w; w = w->prev_trans)
	{
		if (w->shadowSize > 0){
		    if (winTypeShadow[w->windowType]) {
			switch (compMode) {
				case CompSimple:
					break;
				case CompServerShadows:
					XFixesSetPictureClipRegion (dpy, rootBuffer, 0, 0, w->borderClip);
					set_ignore (dpy, NextRequest (dpy));
					if (w->opacity != OPAQUE && !w->shadowPict)
						w->shadowPict = solid_picture (dpy, True,
								(double) w->opacity / OPAQUE * 0.3,
								0, 0, 0);
					XRenderComposite (dpy, PictOpOver, 
							w->shadowPict ? w->shadowPict : transBlackPicture,
							w->picture, rootBuffer,
							0, 0, 0, 0,
							w->a.x + w->shadow_dx,
							w->a.y + w->shadow_dy,
							w->shadow_width, w->shadow_height);
					break;
				case CompClientShadows:
					if (w->shadow)
					{
						XFixesSetPictureClipRegion (dpy, rootBuffer, 0, 0, w->borderClip);
						XRenderComposite (dpy, PictOpOver, blackPicture, w->shadow, rootBuffer,
								0, 0, 0, 0,
								w->a.x + w->shadow_dx,
								w->a.y + w->shadow_dy,
								w->shadow_width, w->shadow_height);
					}
					break;
			}
		    }
		}
		if (w->opacity != OPAQUE && !w->alphaPict)
			w->alphaPict = solid_picture (dpy, False, 
					(double) w->opacity / OPAQUE, shadowColor.red, shadowColor.green, shadowColor.blue);
		if (w->mode == WINDOW_TRANS)
		{
			int	x, y, wid, hei;
			XFixesIntersectRegion (dpy, w->borderClip, w->borderClip, w->borderSize);
			XFixesSetPictureClipRegion (dpy, rootBuffer, 0, 0, w->borderClip);
#if HAS_NAME_WINDOW_PIXMAP
			x = w->a.x;
			y = w->a.y;
			wid = w->a.width + w->a.border_width * 2;
			hei = w->a.height + w->a.border_width * 2;
#else
			x = w->a.x + w->a.border_width;
			y = w->a.y + w->a.border_width;
			wid = w->a.width;
			hei = w->a.height;
#endif
			set_ignore (dpy, NextRequest (dpy));
			if (!w->decoHash)
			{
				XRenderComposite (dpy, PictOpOver, w->picture, w->alphaPict, rootBuffer,
						0, 0, 0, 0, x, y, wid, hei);
			}
			else
			{
						/*trans part*/
                  /* PICTURE ;)
                  |-----------------------------|
                  |             top             |
                  |-----------------------------|
                  |l |                       | r|
                  |e |                       | i|
                  |f |                       | g|
                  |t |                       | h|
                  |--------------------------| t|
                  |           bottom         |  |
                  |--------------------------|--|*/
                  /*top*/
   				XRenderComposite (dpy, PictOpOver, w->picture, w->alphaPict, rootBuffer,
   				                  0, 0, 0, 0, x, y, wid, _TOPHEIGHT_(w->decoHash));
                  /*right*/
   				XRenderComposite (dpy, PictOpOver, w->picture, w->alphaPict, rootBuffer,
   				                  wid - _RIGHTWIDTH_(w->decoHash), _TOPHEIGHT_(w->decoHash),
   				                  0, 0,
   				                  x + wid - _RIGHTWIDTH_(w->decoHash),
   				                  y + _TOPHEIGHT_(w->decoHash), _RIGHTWIDTH_(w->decoHash),
   				                  hei - _TOPHEIGHT_(w->decoHash));
                  /*bottom*/
   				XRenderComposite (dpy, PictOpOver, w->picture, w->alphaPict, rootBuffer,
   				                  0, hei - _BOTTOMHEIGHT_(w->decoHash), 0, 0,
   				                  x, y + hei - _BOTTOMHEIGHT_(w->decoHash),
   				                  wid - _RIGHTWIDTH_(w->decoHash), _BOTTOMHEIGHT_(w->decoHash));
                  /*left*/
   				XRenderComposite (dpy, PictOpOver, w->picture, w->alphaPict, rootBuffer,
   				                  0, _TOPHEIGHT_(w->decoHash), 0, 0,
   				                  x, y + _TOPHEIGHT_(w->decoHash),
   				                  _LEFTWIDTH_(w->decoHash), hei - _TOPHEIGHT_(w->decoHash) - _BOTTOMHEIGHT_(w->decoHash));
		   }
		}
		else if (w->mode == WINDOW_ARGB)
		{
			int	x, y, wid, hei;
			XFixesIntersectRegion (dpy, w->borderClip, w->borderClip, w->borderSize);
			XFixesSetPictureClipRegion (dpy, rootBuffer, 0, 0, w->borderClip);
#if HAS_NAME_WINDOW_PIXMAP
			x = w->a.x;
			y = w->a.y;
			wid = w->a.width + w->a.border_width * 2;
			hei = w->a.height + w->a.border_width * 2;
#else
			x = w->a.x + w->a.border_width;
			y = w->a.y + w->a.border_width;
			wid = w->a.width;
			hei = w->a.height;
#endif
			set_ignore (dpy, NextRequest (dpy));
			/* Here we redraw the background of the transparent window if we want
			   to do anything special (i.e. anything other than showing the
			   windows and desktop prestacked behind of the window).
			   For example, if you want to blur the background or show another
			   background pixmap entirely here is the place to do it; simply
			   draw the new background onto rootBuffer before continuing! */
			if (w->isInFade == False) {
				if (w->show_root_tile == True) {
					XRenderComposite (dpy, PictOpSrc, rootTile, None, rootBuffer,
							x, y, x, y, 
							x, y, wid, hei);
				}
			}
			XRenderComposite (dpy, PictOpOver, w->picture, w->alphaPict, rootBuffer,
					0, 0, 0, 0, 
					x, y, wid, hei);
		}
		XFixesDestroyRegion (dpy, w->borderClip);
		w->borderClip = None;
	}
	XFixesDestroyRegion (dpy, region);
	if (rootBuffer != rootPicture)
	{
#if 0
		XTransform	t;
		t.matrix[0][0] = XDoubleToFixed (3.0 /*/ scale*/);
		t.matrix[0][1] = 0.0;
		t.matrix[0][2] = 0.0;

		t.matrix[1][0] = 0.0;
		t.matrix[1][1] = XDoubleToFixed (1.0 /*/ scale*/);
		t.matrix[1][2] = 0.0;

		t.matrix[2][0] = 0.0;
		t.matrix[2][1] = 0.0;
		t.matrix[2][2] = XDoubleToFixed (1.0);

		XRenderSetPictureTransform (dpy, rootBuffer, &t);
#endif
		XFixesSetPictureClipRegion (dpy, rootBuffer, 0, 0, None);
		XRenderComposite (dpy, PictOpSrc, rootBuffer, None, rootPicture,
				0, 0, 0, 0, 0, 0, root_width, root_height);
	}
	screen_damaged = False;
}

static void
add_damage (Display *dpy, XserverRegion damage)
{
	if (allDamage)
	{
		XFixesUnionRegion (dpy, allDamage, allDamage, damage);
		XFixesDestroyRegion (dpy, damage);
	}
	else
		allDamage = damage;
}

static void damage_win (Display *dpy, XDamageNotifyEvent *de);

static void
repair_win (Display *dpy, win *w)
{
	XserverRegion   parts;

	if (!w->damaged)
	{
		parts = win_extents (dpy, w);
		set_ignore (dpy, NextRequest (dpy));
		XDamageSubtract (dpy, w->damage, None, None);
	}
	else
	{
		XserverRegion	o;
		parts = XFixesCreateRegion (dpy, 0, 0);
		set_ignore (dpy, NextRequest (dpy));
		XDamageSubtract (dpy, w->damage, None, parts);
		XFixesTranslateRegion (dpy, parts,
				w->a.x + w->a.border_width,
				w->a.y + w->a.border_width);
		if (compMode == CompServerShadows)
		{
			o = XFixesCreateRegion (dpy, 0, 0);
			XFixesCopyRegion (dpy, o, parts);
			XFixesTranslateRegion (dpy, o, w->shadow_dx, w->shadow_dy);
			XFixesUnionRegion (dpy, parts, parts, o);
			XFixesDestroyRegion (dpy, o);
		}
	}
	add_damage (dpy, parts);
	w->damaged = 1;
}

static const char*
wintype_name(wintype type)
{
    const char *t;
    switch (type) {
    case WINTYPE_DESKTOP: t = "desktop"; break;
    case WINTYPE_DOCK:    t = "dock"; break;
    case WINTYPE_TOOLBAR: t = "toolbar"; break;
    case WINTYPE_MENU:    t = "menu"; break;
    case WINTYPE_UTILITY: t = "utility"; break;
    case WINTYPE_SPLASH:  t = "slash"; break;
    case WINTYPE_DIALOG:  t = "dialog"; break;
    case WINTYPE_NORMAL:  t = "normal"; break;
    case WINTYPE_DROPDOWN_MENU: t = "dropdown"; break;
    case WINTYPE_POPUP_MENU: t = "popup"; break;
    case WINTYPE_TOOLTIP: t = "tooltip"; break;
    case WINTYPE_NOTIFY:  t = "notification"; break;
    case WINTYPE_COMBO:   t = "combo"; break;
    case WINTYPE_DND:     t = "dnd"; break;
    default:              t = "unknown"; break;
    }
    return t;
}

void repaint_root_overlay_window ()
{
    XRectangle  r;
    r.x = 0;
    r.y = 0;
    r.width = root_width;
    r.height = root_height;
    XserverRegion  region = XFixesCreateRegion (dpy, &r, 1);
    add_damage (dpy, region);
}

static wintype
get_wintype_prop(Display * dpy, Window w)
{
    Atom actual;
    wintype ret;
    int format;
    unsigned long n, left, off;
    unsigned char *data;

    ret = (wintype)-1;
    off = 0;

    do {
        set_ignore (dpy, NextRequest (dpy));
        int result = XGetWindowProperty (dpy, w, winTypeAtom, off, 1L, False,
                                         XA_ATOM, &actual, &format,
                                         &n, &left, &data);

        if (result != Success)
            break;
        if (data != None)
        {
            int i;

            for (i = 0; i < NUM_WINTYPES; ++i) {
                Atom a;
                memcpy (&a, data, sizeof (Atom));
                if (a == winType[i]) {
                    /* known type */
                    ret = i;
                    break;
                }
            }

            XFree ( (void *) data);
        }

        ++off;
    } while (left >= 4 && ret == (wintype)-1);

    return ret;
}

static wintype
determine_wintype (Display *dpy, Window w, Window top)
{
    Window       root_return, parent_return;
    Window      *children = NULL;
    unsigned int nchildren, i;
    wintype      type;

    type = get_wintype_prop (dpy, w);
    if (type != (wintype)-1)
       return type;

    set_ignore (dpy, NextRequest (dpy));
    if (!XQueryTree (dpy, w, &root_return, &parent_return, &children,
                           &nchildren))
    {
       /* XQueryTree failed. */
       if (children)
           XFree ((void *)children);
       return (wintype)-1;
    }

    for (i = 0;i < nchildren;i++)
    {
       type = determine_wintype (dpy, children[i], top);
       if (type != (wintype)-1)
           return type;
    }

    if (children)
       XFree ((void *)children);

    if (w != top)
        return (wintype)-1;
    else
        return WINTYPE_NORMAL;
}

static unsigned int
get_opacity_prop(Display *dpy, win *w, unsigned int def);

static void
map_win (Display *dpy, Window id, unsigned long sequence, Bool fade)
{
	win		*w = find_win (dpy, id);
	Drawable	back;

	if (!w)
		return;

	w->a.map_state = IsViewable;

	/* This needs to be here or else we lose transparency messages */
	XSelectInput (dpy, id, PropertyChangeMask);

	/* This needs to be here since we don't get PropertyNotify when unmapped */
	w->opacity = get_opacity_prop (dpy, w, OPAQUE);
	determine_mode (dpy, w);

	w->windowType = determine_wintype (dpy, w->id, w->id);
	if ((w->windowType < 0) || (w->windowType > NUM_WINTYPES)) w->windowType = WINTYPE_NORMAL;
#if 0
	printf("window 0x%x type %s\n", w->id, wintype_name(w->windowType));
#endif

#if CAN_DO_USABLE
	w->damage_bounds.x = w->damage_bounds.y = 0;
	w->damage_bounds.width = w->damage_bounds.height = 0;
#endif
	w->damaged = 0;

#if WORK_AROUND_FGLRX
	if (w->a.x != 0) {
		XserverRegion extents = win_extents (dpy, w);
		XDamageNotifyEvent de;
		de.drawable = w->id;
		de.area.x = 0;
		de.area.y = 0;
		de.area.width = w->a.width + w->a.border_width * 2;
		de.area.height = w->a.height + w->a.border_width * 2;
		damage_win(dpy, &de);
		XFixesDestroyRegion (dpy, extents);
	}
#endif
	w->a_prev = w->a;

	if (fade && winTypeFade[w->windowType])
		set_fade (dpy, w, 0, get_opacity_prop(dpy, w, OPAQUE)*1.0/OPAQUE, fade_in_step, 0, False, True, True, True);
}

static void
finish_unmap_win (Display *dpy, win *w)
{
	w->damaged = 0;
#if CAN_DO_USABLE
	w->usable = False;
#endif
	if (w->extents != None)
	{
		add_damage (dpy, w->extents);    /* destroys region */
		w->extents = None;
	}

#if HAS_NAME_WINDOW_PIXMAP
	if (w->pixmap)
	{
		XFreePixmap (dpy, w->pixmap);
		w->pixmap = None;
	}
#endif

	if (w->picture)
	{
		set_ignore (dpy, NextRequest (dpy));
		XRenderFreePicture (dpy, w->picture);
		w->picture = None;
	}

	/* don't care about properties anymore */
	set_ignore (dpy, NextRequest (dpy));
	XSelectInput(dpy, w->id, 0);

	if (w->borderSize)
	{
		set_ignore (dpy, NextRequest (dpy));
		XFixesDestroyRegion (dpy, w->borderSize);
		w->borderSize = None;
	}
        
        if (w->decoRegion)
        {
            set_ignore (dpy, NextRequest (dpy));
            XFixesDestroyRegion (dpy, w->decoRegion);
            w->decoRegion = None;
        }
        
        if (w->contentRegion)
        {
            set_ignore (dpy, NextRequest (dpy));
            XFixesDestroyRegion (dpy, w->contentRegion);
            w->contentRegion = None;
        }
	
        if (w->shadow)
	{
		XRenderFreePicture (dpy, w->shadow);
		w->shadow = None;
	}
	if (w->borderClip)
	{
		XFixesDestroyRegion (dpy, w->borderClip);
		w->borderClip = None;
	}

	clipChanged = True;
}

#if HAS_NAME_WINDOW_PIXMAP
static void
unmap_callback (Display *dpy, win *w, Bool gone)
{
	finish_unmap_win (dpy, w);
}
#endif

static void
unmap_win (Display *dpy, Window id, Bool fade)
{
	win *w = find_win (dpy, id);
	if (!w)
		return;

	if (w->a.map_state != IsUnmapped) {
		w->a.map_state = IsUnmapped;
#if HAS_NAME_WINDOW_PIXMAP
		if (w->pixmap && fade && winTypeFade[w->windowType]) {
			set_fade (dpy, w, w->opacity*1.0/OPAQUE, 0.0, fade_out_step, unmap_callback, False, False, True, True);
		}
		else
#endif
			finish_unmap_win (dpy, w);
	}
}

/* Get the opacity prop from window
   not found: default
   otherwise the value
   */
	static unsigned int
get_opacity_prop(Display *dpy, win *w, unsigned int def)
{
	Atom actual;
	int format;
	unsigned long n, left;

	unsigned char *data;
	int result = XGetWindowProperty(dpy, w->id, opacityAtom, 0L, 1L, False, 
			XA_CARDINAL, &actual, &format, 
			&n, &left, &data);
	if (result == Success && data != NULL && format == 32 )
	{
		unsigned int i;
		i = *(long*)data;
		XFree( (void *) data);
		return i;
	}
	return def;
}

	static unsigned int
get_shadow_prop(Display *dpy, win *w)
{
	Atom actual;
	int format;
	unsigned long n, left;

	unsigned char *data = NULL;
	int result = XGetWindowProperty(dpy, w->id, shadowAtom, 0L, 1L, False, 
			XA_CARDINAL, &actual, &format, 
			&n, &left, &data);
	if (result == Success && data != NULL && format == 32 )
	{
		unsigned int i;
		i = *(long*)data;
		XFree( (void *) data);
		/*i added this for security reaons but limiting a value to 200% is somewhat indiscriminate
		  if (i > 200)
		  return 200;
		  else*/
		return i;
	}
	return 100;
}

	static unsigned int
get_shade_prop(Display *dpy, win *w)
{
	Atom actual;
	int format;
	unsigned long n, left;

	unsigned char *data = NULL;
	int result = XGetWindowProperty(dpy, w->id, shadeAtom, 0L, 1L, False, 
			XA_CARDINAL, &actual, &format, 
			&n, &left, &data);
	if (result == Success && data != NULL && format == 32 )
	{
		unsigned int i;
		i = *(long*)data;
		XFree( (void *) data);
		return i;
	}
	return 0;
}

	static Bool
get_shapable_prop(Display *dpy, win *w)
{
	Atom actual;
	int format;
	unsigned long n, left;

	unsigned char *data = NULL;
	int result = XGetWindowProperty(dpy, w->id, shapableAtom, 0L, 1L, False, 
			XA_CARDINAL, &actual, &format, 
			&n, &left, &data);
	if (result == Success && data != NULL && format == 32 )
	{
		unsigned int i;
		i = *(long*)data;
		XFree( (void *) data);
		return i==1;
	}
	return True; /*in general, the window should be shapable*/
}

static unsigned int
get_decoHash_prop(Display *dpy, win *w)
{
	Atom actual;
	int format;
	unsigned long n, left;

	unsigned char *data = NULL;
	int result = XGetWindowProperty(dpy, w->id, decoHashAtom, 0L, 1L, False, 
			XA_CARDINAL, &actual, &format, 
			&n, &left, &data);
	if (result == Success && data != NULL && format == 32 )
	{
		unsigned int i;
		i = *(long*)data;
		XFree( (void *) data);
		return i;
	}
	return 0; /*no titlebar*/
}

static unsigned int
get_dim_prop(Display *dpy, win *w)
{
    Atom actual;
    int format;
    unsigned long n, left;
    
    unsigned char *data = NULL;
    int result = XGetWindowProperty(dpy, w->id, dimAtom, 0L, 1L, False, 
                                    XA_CARDINAL, &actual, &format, 
                                    &n, &left, &data);
    if (result == Success && data != NULL)
    {
        unsigned int i;
        memcpy (&i, data, sizeof (unsigned int));
        XFree( (void *) data);
        if (i == 0) i = 1;
        return i;
    }
    return OPAQUE; /*in general, the window is not dimmed*/
}

static unsigned int
get_deskchange_prop(Display *dpy, Window id)
{
    Atom actual;
    int format;
    unsigned long n, left;
    
    unsigned char *data = NULL;
    int result = XGetWindowProperty(dpy, id, deskChangeAtom, 0L, 1L, False,
    XA_CARDINAL, &actual, &format,
    &n, &left, &data);
    if (result == Success && data != NULL)
    {
        unsigned int i;
        memcpy (&i, data, sizeof (unsigned int));
        XFree( (void *) data);
        if (i < 3)
            return i;
    }
    return 0; /*no valid change state*/
}

/* Get the opacity property from the window in a percent format
   not found: default
otherwise: the value
*/
	static double
get_opacity_percent(Display *dpy, win *w)
{
	if (w && w->isInFade)
	{
		fade *f = find_fade(w);
		return f->finish;
	}
	else
	{
		double def = winTypeOpacity[w->windowType];
		unsigned int opacity = get_opacity_prop (dpy, w, (unsigned int)(OPAQUE*def));
		return opacity*1.0/OPAQUE;
	}
}
#if 0
static void
damage_shape(Display *dpy, win *w, XRectangle *shape_damage)
{
    set_ignore (dpy, NextRequest (dpy));
    XserverRegion region = XFixesCreateRegion (dpy, shape_damage, 1);
    set_ignore (dpy, NextRequest (dpy));
    XserverRegion tmpRegion;
    add_damage(dpy, region);
    win *i;
    XRectangle *rect;
    int n;
    for (i = w; i; i = i->next)
    {
        XFixesIntersectRegion (dpy, tmpRegion, region, w->extents);
        rect = XFixesFetchRegion (dpy, region, &n);
        free(rect);
        printf("%d\n",n);
        if (n != 1)
        {
            w->damage = True;
            XFixesSubtractRegion (dpy, region, region, w->extents);
        }
        else
            break;
    }
    set_ignore (dpy, NextRequest (dpy));
    XFixesDestroyRegion (dpy, tmpRegion);
    set_ignore (dpy, NextRequest (dpy));
    XFixesDestroyRegion (dpy, region);
}
#endif

static Bool
get_window_transparent_to_desktop(Display * dpy, Window w)
{
	Atom actual;
	int format;
	unsigned long n, left;

	unsigned char *data;
	int result = XGetWindowProperty (dpy, w, winTDETTDAtom, 0L, 1L, False,
			XA_ATOM, &actual, &format,
			&n, &left, &data);

	if (result == Success && data != None && format == 32 )
	{
		Atom a;
		a = *(long*)data;
		XFree ( (void *) data);
		return True;
	}
	return False;
}

	static void
determine_mode(Display *dpy, win *w)
{
	int mode;
	XRenderPictFormat *format;
	unsigned int default_opacity;

	/* if trans prop == -1 fall back on  previous tests*/

	if (w->alphaPict)
	{
		XRenderFreePicture (dpy, w->alphaPict);
		w->alphaPict = None;
	}
	if (w->shadowPict)
	{
		XRenderFreePicture (dpy, w->shadowPict);
		w->shadowPict = None;
	}

	if (w->a.class == InputOnly)
	{
		format = 0;
	}
	else
	{
		format = XRenderFindVisualFormat (dpy, w->a.visual);
	}

        if (!disable_argb && format && format->type == PictTypeDirect && format->direct.alphaMask)
	{
		mode = WINDOW_ARGB;
	}
	else if (w->opacity != OPAQUE)
	{
		mode = WINDOW_TRANS;
	}
	else
	{
		mode = WINDOW_SOLID;
	}
	w->mode = mode;
	if (w->extents)
	{
		XserverRegion damage;
		damage = XFixesCreateRegion (dpy, 0, 0);
		XFixesCopyRegion (dpy, damage, w->extents);
		add_damage (dpy, damage);
	}
}

static Bool
determine_window_transparent_to_desktop (Display *dpy, Window w)
{
	Window       root_return, parent_return;
	Window      *children = NULL;
	unsigned int nchildren, i;
	Bool         type;

	type = get_window_transparent_to_desktop (dpy, w);
	if (type == True)
		return True;

	if (!XQueryTree (dpy, w, &root_return, &parent_return, &children,
				&nchildren))
	{
		/* XQueryTree failed. */
		if (children)
			XFree ((void *)children);
		return False;
	}

	for (i = 0;i < nchildren;i++)
	{
		type = determine_window_transparent_to_desktop (dpy, children[i]);
		if (type == True)
			return True;
	}

	if (children)
		XFree ((void *)children);

	return False;
}

	static void
add_win (Display *dpy, Window id, Window prev)
{
	win				*new = malloc (sizeof (win));
	win				**p;
        unsigned int			tmp;

	if (!new)
		return;
	if (prev)
	{
		for (p = &list; *p; p = &(*p)->next)
			if ((*p)->id == prev && !(*p)->destroyed)
				break;
	}
	else
		p = &list;
	new->id = id;
	set_ignore (dpy, NextRequest (dpy));
	if (!XGetWindowAttributes (dpy, id, &new->a))
	{
		free (new);
		return;
	}
	new->shaped = False;
	new->shape_bounds.x = new->a.x;
	new->shape_bounds.y = new->a.y;
	new->shape_bounds_prev = new->shape_bounds;
	new->shape_bounds.width = new->a.width;
	new->shape_bounds.height = new->a.height;
	new->a_prev = new->a;
	new->damaged = 0;
#if CAN_DO_USABLE
	new->usable = False;
#endif
#if HAS_NAME_WINDOW_PIXMAP
	new->pixmap = None;
#endif
	new->picture = None;
	if (new->a.class == InputOnly)
	{
		new->damage_sequence = 0;
		new->damage = None;
	}
	else
	{
		new->damage_sequence = NextRequest (dpy);
		new->damage = XDamageCreate (dpy, id, XDamageReportNonEmpty);
		XShapeSelectInput (dpy, id, ShapeNotifyMask);
	}
	new->isInFade = False;
	new->alphaPict = None;
	new->shadowPict = None;
	new->borderSize = None;
	new->decoRegion = None;
	new->contentRegion = None;
	new->extents = None;
	new->shadow = None;
	new->shadow_dx = 0;
	new->shadow_dy = 0;
	new->shadow_width = 0;
	new->shadow_height = 0;
	new->opacity = OPAQUE;
	new->destroyed = False;
	new->destruct_queued = False;
	new->destruct_requested = False;
	new->destruct_request_time = 0;
	new->shadowSize = 100;
	new->decoHash = 0;
	new->show_root_tile = determine_window_transparent_to_desktop(dpy, id);

	new->windowType = determine_wintype (dpy, new->id, new->id);
	if ((new->windowType < 0) || (new->windowType > NUM_WINTYPES)) new->windowType = WINTYPE_NORMAL;

	new->borderClip = None;
	new->prev_trans = 0;

	XShapeSelectInput( dpy, id, ShapeNotifyMask );

	new->shadowSize = get_shadow_prop (dpy, new);
	new->shapable = get_shapable_prop(dpy, new);
	new->decoHash = get_decoHash_prop(dpy, new);
	tmp = get_dim_prop(dpy, new);
        new->dimPicture = (tmp < OPAQUE) ? solid_picture (dpy, True, (double)tmp/OPAQUE, 0.1, 0.1, 0.1) : None;

	new->next = *p;
	*p = new;
	if (new->a.map_state == IsViewable)
		map_win (dpy, id, new->damage_sequence - 1, True);
}

	void
restack_win (Display *dpy, win *w, Window new_above)
{
	Window  old_above;

	if (w->next)
		old_above = w->next->id;
	else
		old_above = None;
	if (old_above != new_above)
	{
		win **prev;

		/* unhook */
		for (prev = &list; *prev; prev = &(*prev)->next)
			if ((*prev) == w)
				break;
		*prev = w->next;

		/* rehook */
		for (prev = &list; *prev; prev = &(*prev)->next)
		{
			if ((!(*prev)->destroyed) && ((*prev)->id == new_above))
				break;
		}
		w->next = *prev;
		*prev = w;
	}
}

	static void
configure_win (Display *dpy, XConfigureEvent *ce)
{
	win		    *w = find_win (dpy, ce->window);
	Window	    above;
	XserverRegion   damage = None;

	if (!w)
	{
		if (ce->window == root)
		{
			if (rootBuffer)
			{
				XRenderFreePicture (dpy, rootBuffer);
				rootBuffer = None;
			}
			root_width = ce->width;
			root_height = ce->height;
		}
		return;
	}

#if CAN_DO_USABLE
	if (w->usable)
#endif
	{
		damage = XFixesCreateRegion (dpy, 0, 0);
		if (w->extents != None)
                    XFixesCopyRegion (dpy, damage, w->extents);
	}
	w->shape_bounds.x -= w->a.x;
	w->shape_bounds.y -= w->a.y;
	w->a.x = ce->x;
	w->a.y = ce->y;
	/* Only destroy the pixmap if the window is mapped */
	if (w->a.map_state != IsUnmapped &&
		(w->a.width != ce->width || w->a.height != ce->height))
	{
#if HAS_NAME_WINDOW_PIXMAP
		if (w->pixmap)
		{
			XFreePixmap (dpy, w->pixmap);
			w->pixmap = None;
			if (w->picture)
			{
				XRenderFreePicture (dpy, w->picture);
				w->picture = None;
			}
		}
#endif
		if (w->shadow)
		{
			XRenderFreePicture (dpy, w->shadow);
			w->shadow = None;
		}
	}
	w->a.width = ce->width;
	w->a.height = ce->height;
	w->a.border_width = ce->border_width;
	w->a.override_redirect = ce->override_redirect;
        restack_win (dpy, w, ce->above);
	if (w->a.map_state != IsUnmapped && damage)
	{
		XserverRegion	extents = win_extents (dpy, w);
                XFixesUnionRegion (dpy, damage, damage, extents);
                XFixesDestroyRegion (dpy, extents);
		add_damage (dpy, damage);
	}
	w->shape_bounds.x += w->a.x;
	w->shape_bounds.y += w->a.y;
	if (!w->shaped)
	{
		w->shape_bounds.width = w->a.width;
		w->shape_bounds.height = w->a.height;
	}

	if (w->a.map_state != IsUnmapped)
		clipChanged = True;
}

	static void
circulate_win (Display *dpy, XCirculateEvent *ce)
{
	win	    *w = find_win (dpy, ce->window);
	Window  new_above;

	if (!w)
		return;

	if (ce->place == PlaceOnTop)
		new_above = list->id;
	else
		new_above = None;
	restack_win (dpy, w, new_above);
	clipChanged = True;
}

	static void
finish_destroy_win (Display *dpy, Window id, Bool gone)
{
	win	**prev, *w;

	for (prev = &list; (w = *prev); prev = &w->next)
		if (w->id == id && w->destroyed)
		{
			if (gone)
				finish_unmap_win (dpy, w);
			*prev = w->next;
			if (w->picture)
			{
				set_ignore (dpy, NextRequest (dpy));
				XRenderFreePicture (dpy, w->picture);
				w->picture = None;
			}
			if (w->alphaPict)
			{
				XRenderFreePicture (dpy, w->alphaPict);
				w->alphaPict = None;
			}
			if (w->shadowPict)
			{
				XRenderFreePicture (dpy, w->shadowPict);
				w->shadowPict = None;
			}
			if (w->shadow)
			{
				XRenderFreePicture (dpy, w->shadow);
				w->shadow = None;
			}
			if (w->damage != None)
			{
				set_ignore (dpy, NextRequest (dpy));
				XDamageDestroy (dpy, w->damage);
				w->damage = None;
			}
			cleanup_fade (dpy, w);
			free (w);
			break;
		}
}

#if HAS_NAME_WINDOW_PIXMAP
static void
destroy_callback (Display *dpy, win *w, Bool gone)
{
	finish_destroy_win (dpy, w->id, gone);
}
#endif

static void
destroy_win (Display *dpy, Window id, Bool gone, Bool fadeout)
{
	fade *f;
	win *w = find_win (dpy, id);

	if (w && w->destruct_queued == False) {
		f = find_fade (w);
		if (f) {
			w->destruct_queued = True;
			f->callback = destroy_callback;
			w->destroyed = True;
		}
		else {
			w->destroyed = True;
#if HAS_NAME_WINDOW_PIXMAP
			if (w->pixmap && fadeout && winTypeFade[w->windowType]) {
				set_fade (dpy, w, w->opacity*1.0/OPAQUE, 0.0, fade_out_step, destroy_callback, gone, False, True, True);
			}
			else
#endif
			{
				if (!gone) {
					finish_destroy_win (dpy, id, gone);
				}
				else {
					w->destruct_queued = True;
					w->destruct_requested = True;
					w->destruct_request_time = get_time_in_milliseconds();
				}
			}
		}
	}
}

/*
   static void
   dump_win (win *w)
   {
   printf ("\t%08lx: %d x %d + %d + %d (%d)\n", w->id,
   w->a.width, w->a.height, w->a.x, w->a.y, w->a.border_width);
   }


   static void
   dump_wins (void)
   {
   win	*w;

   printf ("windows:\n");
   for (w = list; w; w = w->next)
   dump_win (w);
   }
   */

	static void
damage_win (Display *dpy, XDamageNotifyEvent *de)
{
	win *w = find_win (dpy, de->drawable);

	if (!w)
		return;

#if WORK_AROUND_FGLRX
	if (w->a.map_state != IsViewable)
		return;
#endif

#if CAN_DO_USABLE
	if (!w->usable)
	{
		if (w->damage_bounds.width == 0 || w->damage_bounds.height == 0)
		{
			w->damage_bounds = de->area;
		}
		else
		{
			if (de->area.x < w->damage_bounds.x)
			{
				w->damage_bounds.width += (w->damage_bounds.x - de->area.x);
				w->damage_bounds.x = de->area.x;
			}
			if (de->area.y < w->damage_bounds.y)
			{
				w->damage_bounds.height += (w->damage_bounds.y - de->area.y);
				w->damage_bounds.y = de->area.y;
			}
			if (de->area.x + de->area.width > w->damage_bounds.x + w->damage_bounds.width)
				w->damage_bounds.width = de->area.x + de->area.width - w->damage_bounds.x;
			if (de->area.y + de->area.height > w->damage_bounds.y + w->damage_bounds.height)
				w->damage_bounds.height = de->area.y + de->area.height - w->damage_bounds.y;
		}
#if 0
		printf ("unusable damage [%d] %d, %d: %d x %d bounds %d, %d: %d x %d\n",
				de->drawable,
				de->area.x,
				de->area.y,
				de->area.width,
				de->area.height,
				w->damage_bounds.x,
				w->damage_bounds.y,
				w->damage_bounds.width,
				w->damage_bounds.height);
#endif
		if (w->damage_bounds.x <= 0 && 
				w->damage_bounds.y <= 0 &&
				w->a.width <= w->damage_bounds.x + w->damage_bounds.width &&
				w->a.height <= w->damage_bounds.y + w->damage_bounds.height)
		{
			clipChanged = True;
			if (winTypeFade[w->windowType]) {
				set_fade (dpy, w, 0, get_opacity_percent (dpy, w), fade_in_step, 0, False, True, True, False);
			}
			w->usable = True;
		}
	}
	if (w->usable)
#endif
		repair_win (dpy, w);
}

static const char *
shape_kind(int kind)
{
	static char	buf[128];
	
	switch(kind){
		case ShapeBounding: 
			return "ShapeBounding";
		case ShapeClip:
			return "ShapeClip";
		case ShapeInput:
			return "ShapeInput";
		default:
			sprintf (buf, "Shape %d", kind);
		return buf;
	}
}

static void
shape_win (Display *dpy, XShapeEvent *se)
{
	win	*w = find_win (dpy, se->window);
	
	if (!w)
		return;

	if (w->a.map_state == IsUnmapped)
		return;

	if (w->isInFade)
		return;
	
	if (se->kind == ShapeClip || se->kind == ShapeBounding)
	{
		XserverRegion region0;
		XserverRegion region1;

#if 0
      printf("win 0x%lx %s:%s %ux%u+%d+%d (@%d+%d)\n",
	     (unsigned long) se->window,
	     shape_kind(se->kind),
	     (se->shaped == True) ? "true" : "false",
	     se->width, se->height,
	     se->x, se->y,
	     w->a.x, w->a.y);
      printf("\told %s %d+%d (@%d+%d)\n",
	     (w->shaped == True) ? "true" : "false",
	     w->shape_bounds_prev.width, w->shape_bounds_prev.height,
	     w->shape_bounds_prev.x, w->shape_bounds_prev.y);
#endif

		clipChanged = True;
		
		region0 = XFixesCreateRegion (dpy, &w->shape_bounds_prev, 1);
		
		if (se->shaped == True)
		{
			w->shaped = True;
			w->shape_bounds.x = w->a.x + se->x;
			w->shape_bounds.y = w->a.y + se->y;
			w->shape_bounds.width = se->width;
			w->shape_bounds.height = se->height;
		}
		else
		{
			w->shaped = False;
			w->shape_bounds.x = w->a.x;
			w->shape_bounds.y = w->a.y;
			w->shape_bounds.width = w->a.width;
			w->shape_bounds.height = w->a.height;
		}
		
		region1 = XFixesCreateRegion (dpy, &w->shape_bounds, 1);
		XFixesUnionRegion (dpy, region0, region0, region1); 
		XFixesDestroyRegion (dpy, region1);
		
		/* ask for repaint of the old and new region */
		paint_all (dpy, region0);
	}

	w->shape_bounds_prev = w->shape_bounds;
}

static void
damage_screen (Display *dpy)
{
	XserverRegion region;
	XRectangle r;

	r.x = 0;
	r.y = 0;
	r.width = root_width;
	r.height = root_height;

	region = XFixesCreateRegion (dpy, &r, 1);
	add_damage (dpy, region);
	screen_damaged = True;
}

static int
error (Display *dpy, XErrorEvent *ev)
{
	int	    o;
	char    *name = 0;

	if (should_ignore (dpy, ev->serial))
		return 0;

	if (ev->request_code == composite_opcode &&
			ev->minor_code == X_CompositeRedirectSubwindows)
	{
		fprintf (stderr, "Another composite manager is already running\n");
		my_exit_code=2;
		exit (2);
	}

	o = ev->error_code - xfixes_error;
	switch (o) {
		case BadRegion: name = "BadRegion";	break;
		default: break;
	}
	o = ev->error_code - damage_error;
	switch (o) {
		case BadDamage: name = "BadDamage";	break;
		default: break;
	}
	o = ev->error_code - render_error;
	switch (o) {
		case BadPictFormat: name ="BadPictFormat"; break;
		case BadPicture: name ="BadPicture"; break;
		case BadPictOp: name ="BadPictOp"; break;
		case BadGlyphSet: name ="BadGlyphSet"; break;
		case BadGlyph: name ="BadGlyph"; break;
		default: break;
	}

#ifndef NDEBUG
	fprintf (stderr,"error %d request %d minor %d serial %d\n",
			ev->error_code, ev->request_code, ev->minor_code, ev->serial);
#endif

	/*    abort ();	    this is just annoying to most people */
	return 0;
}

	static void
expose_root (Display *dpy, Window root, XRectangle *rects, int nrects)
{
	XserverRegion  region = XFixesCreateRegion (dpy, rects, nrects);

	add_damage (dpy, region);
}


	static int
ev_serial (XEvent *ev)
{
	if ((ev->type & 0x7f) != KeymapNotify)
		return ev->xany.serial;
	return NextRequest (ev->xany.display);
}


	static char *
ev_name (XEvent *ev)
{
	static char	buf[128];
	switch (ev->type & 0x7f) {
		case Expose:
			return "Expose";
		case MapNotify:
			return "Map";
		case UnmapNotify:
			return "Unmap";
		case ReparentNotify:
			return "Reparent";
		case CirculateNotify:
			return "Circulate";
		default:
			if (ev->type == damage_event + XDamageNotify) {
				return "Damage";
			}
			else if (ev->type == xshape_event + ShapeNotify)
			{
				return "Shape";
			}
			sprintf (buf, "Event %d", ev->type);
			return buf;
	}
}

	static Window
ev_window (XEvent *ev)
{
	switch (ev->type) {
		case Expose:
			return ev->xexpose.window;
		case MapNotify:
			return ev->xmap.window;
		case UnmapNotify:
			return ev->xunmap.window;
		case ReparentNotify:
			return ev->xreparent.window;
		case CirculateNotify:
			return ev->xcirculate.window;
		default:
			if (ev->type == damage_event + XDamageNotify) {
// 				fprintf(stderr, "%d", ev->type);
				return ((XDamageNotifyEvent *) ev)->drawable;
			}
			else if (ev->type == xshape_event + ShapeNotify)
			{
// 				fprintf(stderr, "%d", ev->type);
				return ((XShapeEvent *) ev)->window;
			}
			return 0;
	}
}

void
setShadowColor(char *value){  /*format nach #xxxxxx (html) �ndern?*/
	unsigned int tmp;
	char **res = NULL;
	tmp = strtoul(value, res, 16);
	if( !value || strlen(value) < 6 || strlen(value) > 8 || (*(value+1) == 'x' && strlen(value) < 8) || res != NULL ){
		shadowColor.red = 0;
		shadowColor.green = 0;
		shadowColor.blue = 0;
		printf("wrong hexadecimal (use 0xXXXXXX or XXXXXX)! defaulting to black...\n");
		return;
	}
	shadowColor.blue = tmp&0xff;
	tmp >>= 8;
	shadowColor.green = tmp&0xff;
	tmp >>= 8;
	shadowColor.red = tmp&0xff;
}

typedef enum _option{
	Display_=0,
	Compmode,
	ExcludeDockShadows,
	FadeWindows,
	FadeTrans,
	AutoRedirect,
	Synchronize,
	ShadowColor,
	ShadowRadius,
	ShadowOpacity,
	ShadowOffsetX,
	ShadowOffsetY,
	FadeOutStep,
	FadeInStep,
	FadeDelta,
	DisableARGB,
	FadeMenuWindows,
	NUMBEROFOPTIONS
} Option;

const char *
options[NUMBEROFOPTIONS] = {
	"Display",              /*0*/
	"Compmode",             /*1*/
	"ExcludeDockShadows",   /*2*/
	"FadeWindows",          /*3*/
	"FadeTrans",            /*4*/
	"AutoRedirect",         /*5*/
	"Synchronize",          /*6*/
	"ShadowColor",          /*7*/
	"ShadowRadius",         /*8*/
	"ShadowOpacity",        /*9*/
	"ShadowOffsetX",        /*10*/
	"ShadowOffsetY",        /*11*/
	"FadeOutStep",          /*12*/
	"FadeInStep",           /*13*/
	"FadeDelta",            /*14*/
	"DisableARGB",          /*15*/
	"FadeMenuWindows",      /*16*/
	/*put your thingy in here...*/    
};

void
setValue(Option option, char *value ){
	switch(option){ /*please keep that upside-down, because this way adding a new option is easier (all in one view)*/

		case FadeDelta:
			fade_delta = atoi(value);
			if (fade_delta < 1)
				fade_delta = 10;
			break;
		case FadeInStep:
			fade_in_step = atof(value);
			if (fade_in_step <= 0)
				fade_in_step = 0.01;
			break;
		case FadeOutStep:
			fade_out_step = atof(value);
			if (fade_out_step <= 0)
				fade_out_step = 0.01;
			break;
		case ShadowOffsetY:
			shadowOffsetY = atoi(value);
			break;
		case ShadowOffsetX:
			shadowOffsetX = atoi(value);
			break;
		case ShadowOpacity:
			shadowOpacity = atof(value);
			break;
		case ShadowRadius:
			shadowRadius = atoi(value);
			break;                    
		case ShadowColor:
			setShadowColor(value);
			break;
		case Synchronize:
			synchronize = ( strcasecmp(value, "true") == 0 );
			break;
		case AutoRedirect:
			autoRedirect = ( strcasecmp(value, "true") == 0 );
			break;
		case FadeTrans:
			fadeTrans = ( strcasecmp(value, "true") == 0 );
			break;
		case FadeWindows:
			if ( strcasecmp(value, "true") == 0 ) {
				int i;
				for (i = 0; i < NUM_WINTYPES; ++i) {
					if (i != WINTYPE_POPUP_MENU)
						winTypeFade[i] = True;
				}
			}
			break;
		case FadeMenuWindows:
			if ( strcasecmp(value, "true") == 0 ) {
				winTypeFade[WINTYPE_POPUP_MENU] = True;
			}
			break;
		case ExcludeDockShadows:
			if ( strcasecmp(value, "true") == 0 ) {
				winTypeShadow[WINTYPE_DOCK] = False;
			}
			break;
		case Compmode:
			if( strcasecmp(value, "CompClientShadows") == 0 ){
				compMode = CompClientShadows;
			}
			else if( strcasecmp(value, "CompServerShadows") == 0 ){
				compMode = CompServerShadows;
			}
			else{
				compMode = CompSimple; /*default*/
			}
			break;
		case Display_:
                        break;
			display = strdup(value);
			break;
		case DisableARGB:
			disable_argb = ( strcasecmp(value, "true") == 0 );
			break;
		default:
			break;
	}
}

int
setParameter(char *line){
	char *name = strtok(line, "=");
	char *value = line+strlen(name)+1;
	Option i;
	for(i=Display_; i < NUMBEROFOPTIONS; i++){
		if( strcasecmp(name, *(options+i) ) == 0 ){
			setValue(i, value);
			name = value = NULL;
			return 1;
		}
	}
	printf("ignored unknown option: <%s>\n", name);
	name = value = NULL;
	return 0;
}

void
loadConfig(char *filename){
	FILE           *file = NULL;
	char            line[ 1024 ];
	size_t          length = 0;
	Bool            wasNull = False;
	Bool            section = False;

	if( filename == NULL ){
#ifdef USE_ENV_HOME
		const char *home = getenv("HOME");
#else
		const char *home;
		struct passwd *p;
		p = getpwuid(getuid());
		if (p)
			home = p->pw_dir;
		else
			home = getenv("HOME");
#endif
		const char *configfile = "/.xcompmgrrc"; 
		int n = strlen(home)+strlen(configfile)+1;
		filename = (char*)malloc(n*sizeof(char));
		memset(filename,0,n);
		wasNull = True;

		strcat(filename, home);
		strcat(filename, configfile);
	}

	printf("trying '%s' as configfile\n\n", filename);

	if( (file = fopen(filename, "r")) == NULL ){
		printf("failed to open config file. does it exist?\n");
		if( wasNull ){
			free(filename);
			filename = NULL;
		}
		return;
	}

	/*find section*/
	while( !section && fgets(line, 1023, file) != NULL ){
		if( strcmp(line, "[xcompmgr]\n") == 0 )
			section = True;
	}
	/*read and set values*/
	while( section && fgets(line, 1023, file) != NULL ){
		int ret = strlen( line );
		if( ret > 1 ){
			if( *line == '[' )/*found new section - maybe check for '\n'?*/
				break;
			*(line+ret-1) = '\0';
			setParameter(line);
		}
	}
	printf("\nfinished parsing the config file\n");
	fclose(file);
	if( wasNull ){
		free(filename);
		filename = NULL;
	}
}

	void
usage (char *program)
{
	fprintf (stderr, "%s v1.0\n", program);
	fprintf (stderr, "usage: %s [options]\n", program);
	fprintf (stderr, "Options\n");
	fprintf (stderr, "   -d display\n      Specifies which display should be managed.\n");
	fprintf (stderr, "   -r radius\n      Specifies the blur radius for client-side shadows. (default 12)\n");
	fprintf (stderr, "   -o opacity\n      Specifies the translucency for client-side shadows. (default .75)\n");
	fprintf (stderr, "   -l left-offset\n      Specifies the left offset for client-side shadows. (default -15)\n");
	fprintf (stderr, "   -t top-offset\n      Specifies the top offset for clinet-side shadows. (default -15)\n");
	fprintf (stderr, "   -b color\n      Specifies the background color to use if no root pixmap is set. (default is black)\n");
	fprintf (stderr, "   -I fade-in-step\n      Specifies the opacity change between steps while fading in. (default 0.028)\n");
	fprintf (stderr, "   -O fade-out-step\n      Specifies the opacity change between steps while fading out. (default 0.03)\n");
	fprintf (stderr, "   -D fade-delta-time\n      Specifies the time between steps in a fade in milliseconds. (default 10)\n");
	fprintf (stderr, "   -a\n      Use automatic server-side compositing. Faster, but no special effects.\n");
	fprintf (stderr, "   -c\n      Draw client-side shadows with fuzzy edges.\n");
	fprintf (stderr, "   -C\n      Avoid drawing shadows on dock/panel windows.\n");
	fprintf (stderr, "   -f\n      Fade windows in/out when opening/closing.\n");
	fprintf (stderr, "   -F\n      Fade windows during opacity changes.\n");
	fprintf (stderr, "   -n\n      Normal client-side compositing with transparency support\n");
	fprintf (stderr, "   -s\n      Draw server-side shadows with sharp edges.\n");
	fprintf (stderr, "   -S\n      Enable synchronous operation (for debugging).\n");
	fprintf (stderr, "   -x [0x]XXXXXX\n      Choose Custom Color in hex format\n");
	fprintf (stderr, "   -v\n      Print version Number and exit\n");
	fprintf (stderr, "   -h\n      Print this help\n");
	my_exit_code=2;
	exit (2);
}

static Bool
register_cm (void)
{
      Window w;
      Atom a;
      static char net_wm_cm[] = "_NET_WM_CM_Sxx";

      snprintf (net_wm_cm, sizeof (net_wm_cm), "_NET_WM_CM_S%d", scr);
      a = XInternAtom (dpy, net_wm_cm, False);

/*      w = XGetSelectionOwner (dpy, a);
      if (w != None)
      {
          XTextProperty tp;
          char **strs;
          int count;
          Atom winNameAtom = XInternAtom (dpy, "_NET_WM_NAME", False);

          if (!XGetTextProperty (dpy, w, &tp, winNameAtom) &&
          !XGetTextProperty (dpy, w, &tp, XA_WM_NAME))
          {
              fprintf (stderr,
              "Another composite manager is already running (0x%lx)\n",
              (unsigned long) w);
              return False;
          }
          if (XmbTextPropertyToTextList (dpy, &tp, &strs, &count) == Success)
          {
              fprintf (stderr,
               "Another composite manager is already running (%s)\n",
               strs[0]);

               XFreeStringList (strs);
          }

          XFree (tp.value);

          return False;
      }*/

      w = XCreateSimpleWindow (dpy, RootWindow (dpy, scr), 0, 0, 1, 1, 0, None,
		  None);
      Xutf8SetWMProperties(dpy, w, "kcompmgr", "kcompmgr", NULL, 0, NULL, NULL,
		  NULL);

      /* setting this causes kompmgr to abort on TDE login */
      /* XSetSelectionOwner (dpy, a, w, 0); */

      return True;
}

int
main (int argc, char **argv)
{
	XEvent	    ev;
	Window	    root_return, parent_return;
	Window	    *children;
	Pixmap	    transPixmap;
	Pixmap	    blackPixmap;
	unsigned int    nchildren;
	int		    i;
	XRenderPictureAttributes	pa;
	XRenderColor		c;
	XRectangle	    *expose_rects = 0;
	int		    size_expose = 0;
	int		    n_expose = 0;
	struct pollfd   ufd;
	int		    n;
	int		    last_update;
	int		    now;
	int		    p;
	int		    composite_major, composite_minor;
	Bool		    noDockShadow = False;

	for (i = 0; i < NUM_WINTYPES; ++i) {
		winTypeFade[i] = False;
		winTypeShadow[i] = False;
		winTypeOpacity[i] = 1.0;
	}

	int		    o;
	char		    *fill_color_name = NULL;
	char **res = NULL;

	shadowColor.red = 0;
	shadowColor.green = 0;
	shadowColor.blue = 0;

	// Initialize signal handlers
	sigfillset(&block_mask);
	usr_action.sa_handler = handle_siguser;
	usr_action.sa_mask = block_mask;
	usr_action.sa_flags = 0;
	sigaction(SIGUSR1, &usr_action, NULL);
        sigaction(SIGUSR2, &usr_action, NULL);
        sigaction(SIGTERM, &usr_action, NULL);

	loadConfig(NULL); /*we do that before cmdline-parsing, so config-values can be overridden*/
	/*used for shadow colors*/

	while ((o = getopt (argc, argv, "D:I:O:d:r:o:l:t:b:scnfFmCaSx:vhk")) != -1)
	{
		switch (o) {
			case 'd':
				display = optarg;
				break;
			case 'D':
				fade_delta = atoi (optarg);
				if (fade_delta < 1)
					fade_delta = 10;
				break;
			case 'I':
				fade_in_step = atof (optarg);
				if (fade_in_step <= 0)
					fade_in_step = 0.01;
				break;
			case 'O':
				fade_out_step = atof (optarg);
				if (fade_out_step <= 0)
					fade_out_step = 0.01;
				break;
			case 's':
				compMode = CompServerShadows;
				for (i = 0; i < NUM_WINTYPES; ++i)
					winTypeShadow[i] = True;
				break;
			case 'c':
				compMode = CompClientShadows;
				for (i = 0; i < NUM_WINTYPES; ++i)
					winTypeShadow[i] = True;
				break;
			case 'C':
				winTypeShadow[WINTYPE_DOCK] = False;
				break;
			case 'n':
				compMode = CompSimple;
				for (i = 0; i < NUM_WINTYPES; ++i)
					winTypeShadow[i] = False;
				break;
			case 'f':
				for (i = 0; i < NUM_WINTYPES; ++i) {
					if (i != WINTYPE_POPUP_MENU)
						winTypeFade[i] = True;
				}
				break;
			case 'm':
				winTypeFade[WINTYPE_POPUP_MENU] = True;
				break;
			case 'F':
				fadeTrans = True;
				break;
			case 'a':
				autoRedirect = True;
				break;
			case 'S':
				synchronize = True;
				break;
			case 'r':
				shadowRadius = atoi (optarg);
				break;
			case 'o':
				shadowOpacity = atof (optarg);
				break;
			case 'l':
				shadowOffsetX = atoi (optarg);
				break;
			case 't':
				shadowOffsetY = atoi (optarg);
				break;
			case 'b':
				fill_color_name = optarg;
				break;
			case 'x':
				if( compMode != CompClientShadows ){
					fprintf(stderr, "sorry, but we need ClientShadows (-c) for coloring to work properly!\ndefaulting to black...\n");
					break;
				}
				setShadowColor(optarg);
				break;
			case 'v': fprintf (stderr, "%s v%-3.2f\n", argv[0], _VERSION_); my_exit_code=0; exit (0);
			case 'k':
				restartOnSigterm = False;
				break;
			case 'h':
			default:
				  usage (argv[0]);
				  break;
		}
	}

	/* don't bother to do anything for the desktop */
	winTypeOpacity[WINTYPE_DESKTOP] = 1.0;
	winTypeShadow[WINTYPE_DESKTOP] = False;
	winTypeFade[WINTYPE_DESKTOP] = False;

	dpy = XOpenDisplay (display);
	if (!dpy)
	{
		fprintf (stderr, "Can't open display\n");
		my_exit_code=2;
		exit (2);
	}
	XSetErrorHandler (error);
	if (synchronize)
		XSynchronize (dpy, 1);
	scr = DefaultScreen (dpy);
	root = RootWindow (dpy, scr);

	if (!XRenderQueryExtension (dpy, &render_event, &render_error))
	{
		fprintf (stderr, "No render extension\n");
		my_exit_code=2;
		exit (2);
	}
	if (!XQueryExtension (dpy, COMPOSITE_NAME, &composite_opcode,
				&composite_event, &composite_error))
	{
		fprintf (stderr, "No composite extension\n");
		my_exit_code=2;
		exit (2);
	}
	XCompositeQueryVersion (dpy, &composite_major, &composite_minor);
#if HAS_NAME_WINDOW_PIXMAP
	if (composite_major > 0 || composite_minor >= 2)
		hasNamePixmap = True;
#endif

	if (!XDamageQueryExtension (dpy, &damage_event, &damage_error))
	{
		fprintf (stderr, "No damage extension\n");
		my_exit_code=2;
		exit (2);
	}
	if (!XFixesQueryExtension (dpy, &xfixes_event, &xfixes_error))
	{
		fprintf (stderr, "No XFixes extension\n");
		my_exit_code=2;
		exit (2);
	}
	if (!XShapeQueryExtension (dpy, &xshape_event, &xshape_error))
	{
		fprintf (stderr, "No XShape extension\n");
		my_exit_code=2;
		exit (2);
	}

	fprintf(stderr, "Started\n");

	if (!register_cm())
	{
		my_exit_code=2;
		exit (2);
	}

	/* get atoms */
	shadowAtom = XInternAtom (dpy, SHADOW_PROP, False);
	opacityAtom = XInternAtom (dpy, OPACITY_PROP, False);
	shadeAtom = XInternAtom (dpy, SHADE_PROP, False);
	shapableAtom = XInternAtom (dpy, SHAPABLE_PROP, False);
	decoHashAtom = XInternAtom (dpy, DECOHASH_PROP, False);
        dimAtom = XInternAtom (dpy, DIM_PROP, False);
        deskChangeAtom = XInternAtom (dpy, DESKCHANGE_PROP, False);
	winTypeAtom = XInternAtom (dpy, "_NET_WM_WINDOW_TYPE", False);
	winTDETTDAtom = XInternAtom (dpy, "_KDE_TRANSPARENT_TO_DESKTOP", False);
	winType[WINTYPE_DESKTOP] = XInternAtom (dpy, "_NET_WM_WINDOW_TYPE_DESKTOP", False);
	winType[WINTYPE_DOCK] = XInternAtom (dpy, "_NET_WM_WINDOW_TYPE_DOCK", False);
	winType[WINTYPE_TOOLBAR] = XInternAtom (dpy, "_NET_WM_WINDOW_TYPE_TOOLBAR", False);
	winType[WINTYPE_MENU] = XInternAtom (dpy, "_NET_WM_WINDOW_TYPE_MENU", False);
	winType[WINTYPE_UTILITY] = XInternAtom (dpy, "_NET_WM_WINDOW_TYPE_UTILITY", False);
	winType[WINTYPE_SPLASH] = XInternAtom (dpy, "_NET_WM_WINDOW_TYPE_SPLASH", False);
	winType[WINTYPE_DIALOG] = XInternAtom (dpy, "_NET_WM_WINDOW_TYPE_DIALOG", False);
	winType[WINTYPE_NORMAL] = XInternAtom (dpy, "_NET_WM_WINDOW_TYPE_NORMAL", False);
	winType[WINTYPE_DROPDOWN_MENU] = XInternAtom (dpy, "_NET_WM_WINDOW_TYPE_DROPDOWN_MENU", False);
	winType[WINTYPE_POPUP_MENU] = XInternAtom (dpy, "_NET_WM_WINDOW_TYPE_POPUP_MENU", False);
	winType[WINTYPE_TOOLTIP] = XInternAtom (dpy, "_NET_WM_WINDOW_TYPE_TOOLTIP", False);
	winType[WINTYPE_NOTIFY] = XInternAtom (dpy, "_NET_WM_WINDOW_TYPE_NOTIFICATION", False);
	winType[WINTYPE_COMBO] = XInternAtom (dpy, "_NET_WM_WINDOW_TYPE_COMBO", False);
	winType[WINTYPE_DND] = XInternAtom (dpy, "_NET_WM_WINDOW_TYPE_DND", False);

	pa.subwindow_mode = IncludeInferiors;

	if (compMode == CompClientShadows)
	{
		gaussianMap = make_gaussian_map(dpy, shadowRadius);
		presum_gaussian (gaussianMap);
	}

	if (fill_color_name)
	{
		XColor c;
		if (! XParseColor (dpy, DefaultColormap (dpy, scr),
			   fill_color_name, &c))
		{
			fprintf (stderr, "Could not parse fill color.\n");
			my_exit_code=2;
			exit (2);
		}
		if (! XAllocColor (dpy, DefaultColormap (dpy, scr), &c))
		{
			fprintf (stderr, "Could not allocate color.\n");
			my_exit_code=2;
			exit (2);
		}

		fill_color.red = c.red;
		fill_color.green = c.green;
		fill_color.blue = c.blue;
	}
	else
	{
		fill_color.red = fill_color.green = fill_color.blue = 0x0;
	}
	fill_color.alpha = 0xffff;

	root_width = DisplayWidth (dpy, scr);
	root_height = DisplayHeight (dpy, scr);

	rootPicture = XRenderCreatePicture (dpy, root,
			sXRenderFindVisualFormat (dpy,
				DefaultVisual (dpy, scr)),
			CPSubwindowMode,
			&pa);
	blackPicture = solid_picture (dpy, True, 1, (double)(shadowColor.red)/0xff, (double)(shadowColor.green)/0xff, (double)(shadowColor.blue)/0xff);
	if (compMode == CompServerShadows)
		transBlackPicture = solid_picture (dpy, True, 0.3, 0, 0, 0);
	allDamage = None;
	clipChanged = True;
	XGrabServer (dpy);
	if (autoRedirect)
		XCompositeRedirectSubwindows (dpy, root, CompositeRedirectAutomatic);
	else
	{
        int dummy;
		XCompositeRedirectSubwindows (dpy, root, CompositeRedirectManual);
		XSelectInput (dpy, root,
				SubstructureNotifyMask|
				ExposureMask|
				StructureNotifyMask|
				PropertyChangeMask |
				VisibilityChangeMask);

		/*shaping stuff*/
		XShapeQueryExtension(dpy, &shapeEvent, &dummy);

		XShapeSelectInput (dpy, root, ShapeNotifyMask);
		XQueryTree (dpy, root, &root_return, &parent_return, &children, &nchildren);
		for (i = 0; i < nchildren; i++)
			add_win (dpy, children[i], i ? children[i-1] : None);
		XFree (children);
	}
	XUngrabServer (dpy);
	ufd.fd = ConnectionNumber (dpy);
	ufd.events = POLLIN;
	if (!autoRedirect)
		paint_all (dpy, None);

        /* Under no circumstances should these two lines EVER be moved earlier in main() than this point */
        atexit(delete_pid_file);
        write_pid_file(getpid());

	for (;;)
	{
		/*	dump_wins (); */
		do {
			if (autoRedirect)
				XFlush (dpy);
			if (!QLength (dpy))
			{
				if (poll (&ufd, 1, fade_timeout()) == 0)
				{
					run_fades (dpy);
					break;
				}
			}

			XNextEvent (dpy, &ev);
			if ((ev.type & 0x7f) != KeymapNotify)
				discard_ignore (dpy, ev.xany.serial);
#if DEBUG_EVENTS
			printf ("event %10.10s serial 0x%08x window 0x%08x\n",
					ev_name(&ev), ev_serial (&ev), ev_window (&ev));
#endif
			if (!autoRedirect) switch (ev.type) {
				case CreateNotify:
					add_win (dpy, ev.xcreatewindow.window, 0);
					break;
				case ConfigureNotify:
					configure_win (dpy, &ev.xconfigure);
					break;
				case DestroyNotify:
					destroy_win (dpy, ev.xdestroywindow.window, True, True);
					break;
				case MapNotify:
					map_win (dpy, ev.xmap.window, ev.xmap.serial, True);
					break;
				case UnmapNotify:
					unmap_win (dpy, ev.xunmap.window, True);
					break;
				case ReparentNotify:
					if (ev.xreparent.parent == root)
						add_win (dpy, ev.xreparent.window, 0);
					else
						destroy_win (dpy, ev.xreparent.window, False, True);
					break;
				case CirculateNotify:
					circulate_win (dpy, &ev.xcirculate);
					break;
				case Expose:
					if (ev.xexpose.window == root)
					{
						int more = ev.xexpose.count + 1;
						if (n_expose == size_expose)
						{
							if (expose_rects)
							{
								expose_rects = realloc (expose_rects, 
										(size_expose + more) * 
										sizeof (XRectangle));
								size_expose += more;
							}
							else
							{
								expose_rects = malloc (more * sizeof (XRectangle));
								size_expose = more;
							}
						}
						expose_rects[n_expose].x = ev.xexpose.x;
						expose_rects[n_expose].y = ev.xexpose.y;
						expose_rects[n_expose].width = ev.xexpose.width;
						expose_rects[n_expose].height = ev.xexpose.height;
						n_expose++;
						if (ev.xexpose.count == 0)
						{
							expose_root (dpy, root, expose_rects, n_expose);
							n_expose = 0;
						}
					}
					break;
				case PropertyNotify:
					for (p = 0; backgroundProps[p]; p++)
					{
						if (ev.xproperty.atom == XInternAtom (dpy, backgroundProps[p], False))
						{
							if (rootTile)
							{
								XRenderFreePicture (dpy, rootTile);
								rootTile = None;
								damage_screen (dpy);
								break;
							}
						}
					}
					/* Window set shade? */
					if (ev.xproperty.atom == shadeAtom)
					{
						win * w = find_win(dpy, ev.xproperty.window);
						if (w){
							unsigned int tmp = get_shade_prop(dpy, w);
							if (tmp)
							{
								if (tmp == 1)
								{
									w->preShadeOpacity = w->opacity;
									w->opacity = w->opacity-1; /*assuming that no human being will ever be able to shade an invisable window ;) */
									determine_mode(dpy, w);
								}
								else if (tmp == 2)
								{
									w->opacity = w->preShadeOpacity;
									determine_mode(dpy, w);
								}
							}
							break;
						}
					}
					else if (ev.xproperty.atom == shapableAtom)
					{
						win * w = find_win(dpy, ev.xproperty.window);
						if (w)
						{
							w->shapable = get_shapable_prop(dpy, w);
							/*			printf("%u is %s shapable\n",w->id,w->shapable?"":"not");*/
						}
						else
							printf("arrrg, window not found\n");
					}
					else if (ev.xproperty.atom == decoHashAtom)
					{
						win * w = find_win(dpy, ev.xproperty.window);
						if (w)
						{
							w->decoHash = get_decoHash_prop(dpy, w);
						}
						else
							printf("arrrg, window not found\n");
					}
					else if (ev.xproperty.atom == dimAtom)
					{
						win * w = find_win(dpy, ev.xproperty.window);
						if (w)
						{
							unsigned int tmp = get_dim_prop(dpy, w);
							if (w->dimPicture)
							{
							XRenderFreePicture (dpy, w->dimPicture);
							w->dimPicture = None;
							}
							if (tmp < OPAQUE)
							w->dimPicture = solid_picture (dpy, True, (double)tmp/OPAQUE, 0.1, 0.1, 0.1);
						}
						else
						printf("arrrg, window not found\n");
					}
					/* check if Trans or Shadow property was changed */    
					else if (ev.xproperty.atom == opacityAtom || ev.xproperty.atom == shadowAtom)
					{
						/* reset mode and redraw window */
						win * w = find_win(dpy, ev.xproperty.window);
						if (w)
						{
                                                    unsigned int tmp;
                                                    unsigned int oldShadowSize = w->shadowSize;
							if (ev.xproperty.atom == opacityAtom)
							{
								tmp = get_opacity_prop(dpy, w, OPAQUE);
   								/*This will most probably happen if window is in fade - resulting in that the fade process isn't updated or broken -> we may have a wrong opacity in the future*/
   								/*if (tmp == w->opacity)
									break;*/ /*skip if opacity does not change*/
								if (fadeTrans)
								{
									static double start, finish, step;
									start = w->opacity*1.0/OPAQUE;
									finish = (tmp*1.0)/OPAQUE;
									
									if ( start > finish )
										step = fade_out_step;
									else
										step = fade_in_step;
									
									set_fade (dpy, w, start, finish, step, 0, False, True, True, False);
								break;
								}
								else {
									w->opacity = tmp;
								}
							}
							else
							{
								tmp = get_shadow_prop(dpy, w);
								if (tmp == w->shadowSize)
									break; /*skip if shadow does not change*/
								w->shadowSize = tmp;
								/* if (w->isInFade)
									break; */
							}
							if (w->shadow)
							{
								XRenderFreePicture (dpy, w->shadow);
								w->shadow = None;
							}
							if (oldShadowSize < w->shadowSize) /* this is important to catch size changes on cleanup with determine_mode*/
							{
								if( w->extents != None )
									XFixesDestroyRegion( dpy, w->extents );
								w->extents = win_extents (dpy, w);
								determine_mode(dpy, w);
							}
							else
							{
								determine_mode(dpy, w);
								if( w->extents != None )
									XFixesDestroyRegion( dpy, w->extents );
								w->extents = win_extents (dpy, w);
							}
						}
		}
                else if (ev.xproperty.atom == deskChangeAtom)
                {
                    /*just set global variable*/
                    unsigned int tmp = get_deskchange_prop(dpy, ev.xproperty.window);
                    printf("desk change, state:%d\n",tmp);
                }
                break;
	    default:
		if (ev.type == damage_event + XDamageNotify)
                {
                    /*                     printf("damaging win: %u\n",ev.xany.window);*/
		    damage_win (dpy, (XDamageNotifyEvent *) &ev);
		    repaint_root_overlay_window();
                }
		if (ev.type == xshape_event + ShapeNotify)
		{
		    shape_win (dpy, (XShapeEvent *) &ev);
		}
                if (ev.type == shapeEvent)
                {
                    win * w = find_win(dpy, ev.xany.window);
#if 1
                    if (w && w->shapable)
#endif
#if 0
                    if (w)
#endif
                    {
#if 0
                        XRectangle rect;
                        rect.x = ((XShapeEvent*)&ev)->x;
                        rect.y = ((XShapeEvent*)&ev)->y;
                        rect.width = ((XShapeEvent*)&ev)->width;
                        rect.height = ((XShapeEvent*)&ev)->height;
                        damage_shape(dpy, w, &rect);
#endif
#if 0
			if (w->shadowSize != 0)
			{
			    w->shadowSize = 0;
			    XRenderFreePicture (dpy, w->shadow);
                            w->shadow = None;
			    determine_mode(dpy, w);
                            if( w->extents != None )
                                XFixesDestroyRegion( dpy, w->extents );
                            w->extents = win_extents (dpy, w);
			}
#endif
                        /*this is hardly efficient, but a current workaraound
                        shaping support isn't that good so far (e.g. we lack shaped shadows)
                        IDEA: use XRender to scale/shift a copy of the window and then blur it*/
#if 1
                        if (w->picture)
                        {
                        clipChanged = True;
                        repair_win (dpy, w);
                        }
#endif
                    }
                }
		break;
	    }
	} while (QLength (dpy));
	if (allDamage && !autoRedirect)
	{
	    paint_all (dpy, allDamage);
            XSync (dpy, False);
	    allDamage = None;
	    clipChanged = False;
	}
    }

    XClearArea (dpy, root, 0, 0, 0, 0, True);
    XSync (dpy, False);
}
