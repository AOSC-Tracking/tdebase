/*
Copyright 2011-2013 Timothy Pearson <kb9vqf@pearsoncomputing.net>

This file is part of tdekbdledsync, the TDE Keyboard LED Synchronization Daemon

tdekbdledsync is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3
of the License, or (at your option) any later version.

tdekbdledsync is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public
License along with tdekbdledsync. If not, see http://www.gnu.org/licenses/.

*/

// The idea here is to periodically read the Xorg core keyboard state, and then forcibly set the physical LED states on all attached keyboards to match (via the event interface)
// Once every half second should work well enough on most systems

#include <stdio.h>
#include <stdlib.h>
#include <exception>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <dirent.h>
#include <linux/vt.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <sys/time.h>
#include <termios.h>
#include <signal.h>
#include <stdint.h>
extern "C" {
#include <libudev.h>
#include "getfd.h"
}
#include <libgen.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/XKBlib.h>
#include <X11/extensions/XTest.h>
#include <X11/keysym.h>

using namespace std;

// WARNING
// MAX_KEYBOARDS must be greater than or equal to MAX_INPUT_NODE
#define MAX_KEYBOARDS 128
#define MAX_INPUT_NODE 128

#define TestBit(bit, array) (array[(bit) / 8] & (1 << ((bit) % 8)))

typedef unsigned char byte;

char filename[32];
char key_bitmask[(KEY_MAX + 7) / 8];

int keyboard_fd_num;
int keyboard_fds[MAX_KEYBOARDS];

Display* display = NULL;

int lockfd = -1;
char lockFileName[256];

// --------------------------------------------------------------------------------------
// Useful function from Stack Overflow
// http://stackoverflow.com/questions/874134/find-if-string-endswith-another-string-in-c
// --------------------------------------------------------------------------------------
/*  returns 1 iff str ends with suffix  */
int str_ends_with(const char * str, const char * suffix) {

  if( str == NULL || suffix == NULL )
    return 0;

  size_t str_len = strlen(str);
  size_t suffix_len = strlen(suffix);

  if(suffix_len > str_len)
    return 0;

  return 0 == strncmp( str + str_len - suffix_len, suffix, suffix_len );
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// Useful function from Stack Overflow
// http://stackoverflow.com/questions/1599459/optimal-lock-file-method
// --------------------------------------------------------------------------------------
int tryGetLock(char const *lockName) {
	mode_t m = umask( 0 );
	int fd = open( lockName, O_RDWR|O_CREAT, 0666 );
	umask( m );
	if( fd >= 0 && flock( fd, LOCK_EX | LOCK_NB ) < 0 ) {
		close( fd );
		fd = -1;
	}
	return fd;
}

// --------------------------------------------------------------------------------------
// Useful function from Stack Overflow
// http://stackoverflow.com/questions/1599459/optimal-lock-file-method
// --------------------------------------------------------------------------------------
void releaseLock(int fd, char const *lockName) {
	if( fd < 0 ) {
		return;
	}
	remove( lockName );
	close( fd );
}


// --------------------------------------------------------------------------------------
// Get the VT number X is running on
// (code taken from GDM, daemon/getvt.c, GPLv2+)
// --------------------------------------------------------------------------------------
int get_x_vtnum(Display *dpy)
{
	Atom prop;
	Atom actualtype;
	int actualformat;
	unsigned long nitems;
	unsigned long bytes_after;
	unsigned char *buf;
	int num;
	
	prop = XInternAtom (dpy, "XFree86_VT", False);
	if (prop == None)
	return -1;
	
	if (XGetWindowProperty (dpy, DefaultRootWindow (dpy), prop, 0, 1,
				False, AnyPropertyType, &actualtype, &actualformat,
				&nitems, &bytes_after, &buf)) {
		return -1;
	}
	
	if (nitems != 1) {
		XFree (buf);
		return -1;
	}
	
	switch (actualtype) {
		case XA_CARDINAL:
		case XA_INTEGER:
		case XA_WINDOW:
		switch (actualformat) {
			case 8:
				num = (*(uint8_t  *)(void *)buf);
			break;
			case 16:
				num = (*(uint16_t *)(void *)buf);
			break;
			case 32:
				num = (*(uint32_t *)(void *)buf);
			break;
			default:
				XFree (buf);
				return -1;
			}
		break;
		default:
		XFree (buf);
		return -1;
	}
	
	XFree (buf);
	
	return num;
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// Get the specified xkb mask modifier
// (code taken from numlockx)
// --------------------------------------------------------------------------------------
unsigned int xkb_mask_modifier(XkbDescPtr xkb, const char *name) {
	int i;
	if( !xkb || !xkb->names ) {
		return 0;
	}
	for( i = 0; i < XkbNumVirtualMods; i++ ) {
		char* modStr = XGetAtomName( xkb->dpy, xkb->names->vmods[i] );
		if( modStr != NULL && strcmp(name, modStr) == 0 ) {
			unsigned int mask;
			XkbVirtualModsToReal( xkb, 1 << i, &mask );
			return mask;
		}
	}
	return 0;
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// Get the capslock xkb mask modifier
// --------------------------------------------------------------------------------------
unsigned int xkb_capslock_mask() {
	return LockMask;
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// Get the numlock xkb mask modifier
// (code taken from numlockx)
// --------------------------------------------------------------------------------------
unsigned int xkb_numlock_mask() {
	XkbDescPtr xkb;
	if(( xkb = XkbGetKeyboard(display, XkbAllComponentsMask, XkbUseCoreKbd )) != NULL ) {
		unsigned int mask = xkb_mask_modifier( xkb, "NumLock" );
		XkbFreeKeyboard( xkb, 0, True );
		return mask;
	}
	return 0;
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// Get the scroll lock xkb mask modifier
// (code taken from numlockx and modified)
// --------------------------------------------------------------------------------------
unsigned int xkb_scrolllock_mask() {
	XkbDescPtr xkb;
	if(( xkb = XkbGetKeyboard(display, XkbAllComponentsMask, XkbUseCoreKbd )) != NULL ) {
		unsigned int mask = xkb_mask_modifier( xkb, "ScrollLock" );
		XkbFreeKeyboard( xkb, 0, True );
		return mask;
	}
	return 0;
}
// --------------------------------------------------------------------------------------


int find_keyboards() {
	int i, j;
	int fd;
	char name[256] = "Unknown";

	keyboard_fd_num = 0;
	for (i=0; i<MAX_KEYBOARDS; i++) {
		keyboard_fds[i] = 0;
	}

	for (i=0; i<MAX_INPUT_NODE; i++) {
		snprintf(filename, sizeof(filename), "/dev/input/event%d", i);

		fd = open(filename, O_RDWR|O_SYNC);
		if (fd >= 0) {
			ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(key_bitmask)), key_bitmask);

			// Ensure that we do not detect tsak faked keyboards
			ioctl (fd, EVIOCGNAME(sizeof(name)), name);
			if (str_ends_with(name, "+tsak") == 0) {
				struct input_id input_info;
				ioctl (fd, EVIOCGID, &input_info);
				if ((input_info.vendor != 0) && (input_info.product != 0)) {
					/* We assume that anything that has an alphabetic key in the
						QWERTYUIOP range in it is the main keyboard. */
					for (j = KEY_Q; j <= KEY_P; j++) {
						if (TestBit(j, key_bitmask)) {
							keyboard_fds[keyboard_fd_num] = fd;
						}
					}
				}
			}
	
			if (keyboard_fds[keyboard_fd_num] == 0) {
				close(fd);
			}
			else {
				keyboard_fd_num++;
			}
		}
	}
	return 0;
}

void handle_sigterm(int signum) {
	if (lockfd >= 0) {
		releaseLock(lockfd, lockFileName);
	}
	exit(0);
}

int main() {
	int current_keyboard;
	char name[256] = "Unknown";
	unsigned int states;
	struct input_event ev;
	struct vt_stat vtstat;
	int vt_fd;
	int x11_vt_num = -1;
//	XEvent xev;
	XkbStateRec state;

	bool num_lock_set = false;
	bool caps_lock_set = false;
	bool scroll_lock_set = false;

	int num_lock_mask;
	int caps_lock_mask;
	int scroll_lock_mask;

	int evBase;
	int errBase;

	// Register cleanup handlers
	struct sigaction action;
	memset(&action, 0, sizeof(struct sigaction));
	action.sa_handler = handle_sigterm;
	sigaction(SIGTERM, &action, NULL);

	// Open X11 display
	display = XOpenDisplay(NULL);
	if (!display) {
		printf ("[tdekbdledsync] Unable to open X11 display!\n");
		return -1;
	}

	// Ensure only one process is running on a given display
	sprintf(lockFileName, "/var/lock/tdekbdledsync-%s.lock", XDisplayString(display));
	lockfd = tryGetLock(lockFileName);
	if (lockfd < 0) {
		printf ("[tdekbdledsync] Another instance of this program is already running on this X11 display!\n[tdekbdledsync] Lockfile detected at '%s'\n", lockFileName);
		return -2;
	}

	// Set up Xkb extension
	int i1, mn, mj;
	mj = XkbMajorVersion;
	mn = XkbMinorVersion;
	if (!XkbQueryExtension(display, &i1, &evBase, &errBase, &mj, &mn)) {
		printf("[tdekbdledsync] Server doesn't support a compatible XKB\n");
		releaseLock(lockfd, lockFileName);
		return -3;
	}
	XkbSelectEvents(display, XkbUseCoreKbd, XkbStateNotifyMask, XkbStateNotifyMask);

	// Get X server VT number
	x11_vt_num = get_x_vtnum(display);

	// Open console socket
	vt_fd = getfd(NULL);

	// Monitor for hotplugged keyboards
	struct udev *udev;
	struct udev_device *dev;
	struct udev_monitor *mon;
	struct timeval tv;

	// Create the udev object
	udev = udev_new();
	if (!udev) {
		printf("[tdekbdledsync] Cannot connect to udev interface\n");
		releaseLock(lockfd, lockFileName);
		return -4;
	}

	// Set up a udev monitor to monitor input devices
	mon = udev_monitor_new_from_netlink(udev, "udev");
	udev_monitor_filter_add_match_subsystem_devtype(mon, "input", NULL);
	udev_monitor_enable_receiving(mon);

	while (1) {
		// Get masks
		num_lock_mask = xkb_numlock_mask();
		caps_lock_mask = xkb_capslock_mask();
		scroll_lock_mask = xkb_scrolllock_mask();

		// Find keyboards
		find_keyboards();
		if (keyboard_fd_num == 0) {
			printf ("[tdekbdledsync] Could not find any usable keyboard(s)!\n");
			releaseLock(lockfd, lockFileName);
			return -5;
		}
		else {
			fprintf(stderr, "[tdekbdledsync] Found %d keyboard(s)\n", keyboard_fd_num);

			for (current_keyboard=0;current_keyboard<keyboard_fd_num;current_keyboard++) {
				// Print device name
				ioctl(keyboard_fds[current_keyboard], EVIOCGNAME (sizeof (name)), name);
				fprintf(stderr, "[tdekbdledsync] Syncing keyboard: (%s)\n", name);
			}

			while (1) {
				// Get current active VT
				if (ioctl(vt_fd, VT_GETSTATE, &vtstat)) {
					fprintf(stderr, "[tdekbdledsync] Unable to get current VT!\n");
					releaseLock(lockfd, lockFileName);
					return -6;
				}

				if (x11_vt_num == vtstat.v_active) {
					// Get Virtual Core keyboard status
					if (XkbGetIndicatorState(display, XkbUseCoreKbd, &states) != Success) {
						fprintf(stderr, "[tdekbdledsync] Unable to query X11 Virtual Core keyboard!\n");
						releaseLock(lockfd, lockFileName);
						return -7;
					}

					XkbGetState(display, XkbUseCoreKbd, &state);

					caps_lock_set = (state.mods & caps_lock_mask);
					num_lock_set = (state.mods & num_lock_mask);
					scroll_lock_set = (state.mods & scroll_lock_mask);

					for (current_keyboard=0;current_keyboard<keyboard_fd_num;current_keyboard++) {
						// Set LEDs
						ev.type = EV_LED;
						ev.code = LED_CAPSL;
						ev.value = caps_lock_set;
						if (write(keyboard_fds[current_keyboard], &ev, sizeof(ev)) < 0) {
							fprintf(stderr, "[tdekbdledsync] Unable to set LED state\n");
						}

						ev.type = EV_LED;
						ev.code = LED_NUML;
						ev.value = num_lock_set;
						if (write(keyboard_fds[current_keyboard], &ev, sizeof(ev)) < 0) {
							fprintf(stderr, "[tdekbdledsync] Unable to set LED state\n");
						}

						ev.type = EV_LED;
						ev.code = LED_SCROLLL;
						ev.value = scroll_lock_set;
						if (write(keyboard_fds[current_keyboard], &ev, sizeof(ev)) < 0) {
							fprintf(stderr, "[tdekbdledsync] Unable to set LED state\n");
						}
					}
				}
				else {
					// Ensure the X server is still alive
					// If the X server has terminated, this will fail and the program will terminate
					XSync(display, False);
				}

				// Check the hotplug monitoring process to see if any keyboards were added or removed
				fd_set readfds;
				FD_ZERO(&readfds);
				FD_SET(udev_monitor_get_fd(mon), &readfds);
				tv.tv_sec = 0;
				tv.tv_usec = 0;
				int fdcount = select(udev_monitor_get_fd(mon)+1, &readfds, NULL, NULL, &tv);
				if (fdcount < 0) {
					if (errno == EINTR) {
						fprintf(stderr, "[tdekbdledsync] Signal caught in hotplug monitoring process; ignoring\n");
					}
					else {
						fprintf(stderr, "[tdekbdledsync] Select failed on udev file descriptor in hotplug monitoring process\n");
					}
				}
				else {
					dev = udev_monitor_receive_device(mon);
					if (dev) {
						if (strcmp(udev_device_get_action(dev), "add") == 0) {
							// Reload keyboards
							break;
						}
						if (strcmp(udev_device_get_action(dev), "remove") == 0) {
							// Reload keyboards
							break;
						}
					}
				}

				// Poll
				usleep(250*1000);

// 				// Wait for an Xkb event
// 				// FIXME
// 				// This prevents the udev hotplug monitor from working, as XNextEvent does not stop blocking when a keyboard hotplug occurs
// 				while (1) {
// 					XNextEvent(display, &xev);
// 					if (xev.type == evBase + XkbEventCode) {
// 						XkbEvent *xkb_event = reinterpret_cast<XkbEvent*>(&xev);
// 						if (xkb_event->any.xkb_type & XkbStateNotify) {
// 							if ((xkb_event->state.changed & XkbModifierStateMask) || (xkb_event->state.changed & XkbModifierBaseMask)) {
// 								// Modifier state has changed
// 								// Synchronize keyboard indicators
// 								break;
// 							}
// 						}
// 					}
// 				}
			}
		}

		// Close all keyboard file descriptors
		for (int current_keyboard=0;current_keyboard<keyboard_fd_num;current_keyboard++) {
			close(keyboard_fds[current_keyboard]);
		}
	}

	releaseLock(lockfd, lockFileName);
	return 0;
}
