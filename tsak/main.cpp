/*
Copyright 2010 Adam Marchetti
Copyright 2011-2012 Timothy Pearson <kb9vqf@pearsoncomputing.net>

This file is part of tsak, the TDE Secure Attention Key daemon

tsak is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3
of the License, or (at your option) any later version.

tsak is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public
License along with tsak. If not, see http://www.gnu.org/licenses/.

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <dirent.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <sys/time.h>
#include <termios.h>
#include <signal.h>
#include <libudev.h>
#include <libgen.h>

#define FIFO_DIR "/tmp/ksocket-global"
#define FIFO_FILE_OUT "/tmp/ksocket-global/tsak"
#define FIFO_LOCKFILE_OUT "/tmp/ksocket-global/tsak.lock"

#define MAX_KEYBOARDS 64
#define MAX_INPUT_NODE 128

#define TestBit(bit, array) (array[(bit) / 8] & (1 << ((bit) % 8)))

typedef unsigned char byte;

bool        mPipeOpen_out = false;
int         mPipe_fd_out = -1;

int         mPipe_lockfd_out = -1;

char filename[32];
char key_bitmask[(KEY_MAX + 7) / 8];

struct sigaction usr_action;
sigset_t block_mask;

int keyboard_fd_num;
int keyboard_fds[MAX_KEYBOARDS];
int child_pids[MAX_KEYBOARDS];

const char *keycode[256] =
{
	"", "<esc>", "1", "2", "3", "4", "5", "6", "7", "8",
	"9", "0", "−", "=", "<backspace>", "<tab>", "q", "w", "e", "r",
	"t", "y", "u", "i", "o", "p", "[", "]", "\n", "<control>",
	"a", "s", "d", "f", "g", "h", "j", "k", "l", ";",
	"'", "", "<shift>", "\\", "z", "x", "c", "v", "b", "n",
	"m", ",", ".", "/", "<shift>", "", "<alt>", " ", "<capslock>",
	"<f1>", "<f2>", "<f3>", "<f4>", "<f5>", "<f6>", "<f7>", "<f8>", "<f9>", "<f10>",
	"<numlock>", "<scrolllock>", "", "", "", "", "", "", "", "",
	"", "", "\\", "f11", "f12", "", "", "", "", "",
	"", "", "", "<control>", "", "<sysrq>", "", "", "<control>", "", "",
	"<alt>", "", "", "", "", "", "", "", "", "",
	"", "<del>", "", "", "", "", "", "", "", "",
	"", "", "", "", "", "", "", "", "", "",
	"", "", "", "", "", "", "", "", "", "",
	"", "", "", "", "", "", "", "", "", "",
	"", "", "", "", "", "", "", "", "", "",
	"", "", "", "", "", "", "", "", "", "",
	"", "", "", "", "", "", "", "", "", "",
	"", "", "", "", "", "", "", "", "", "",
	"", "", "", "", "", "", "", "", "", ""
};

/* returns 1 if bit number i is set, otherwise returns 0 */
int bit_set(size_t i, const byte* a)
{
  return a[i/CHAR_BIT] & (1 << i%CHAR_BIT);
}

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

/* Assign features (supported axes and keys) of the physical input device (devin)
 * to the virtual input device (devout) */
static void copy_features(int devin, int devout)
{
	byte evtypes[EV_MAX/CHAR_BIT + 1] = {0};
	byte codes[KEY_MAX/CHAR_BIT + 1];
	unsigned i,code;
	int op;
	if (ioctl(devin, EVIOCGBIT(0, sizeof(evtypes)), evtypes) < 0) return;
	for(i=0;i<EV_MAX;++i) {
		if (bit_set(i, evtypes)) {
			switch(i) {
				case EV_KEY: op = UI_SET_KEYBIT; break;
				case EV_REL: op = UI_SET_RELBIT; break;
				case EV_ABS: op = UI_SET_ABSBIT; break;
				case EV_MSC: op = UI_SET_MSCBIT; break;
				case EV_LED: op = UI_SET_LEDBIT; break;
				case EV_SND: op = UI_SET_SNDBIT; break;
				case EV_SW: op = UI_SET_SWBIT; break;
				default: op = -1;
			}
		}
		if (op == -1) continue;
		ioctl(devout, UI_SET_EVBIT, i);
		memset(codes,0,sizeof(codes));
		if (ioctl(devin, EVIOCGBIT(i, sizeof(codes)), codes) >= 0) {
			for(code=0;code<KEY_MAX;code++) {
				if (bit_set(code, codes)) ioctl(devout, op, code);
			}
		}
	}
}

int find_keyboards() {
	int i, j;
	int fd;
	char name[256] = "Unknown";

	keyboard_fd_num = 0;
	for (i=0; i<MAX_KEYBOARDS; i++) {
		keyboard_fds[i] = 0;
	}
	
	for (i=0; i<MAX_INPUT_NODE; i++) {
		snprintf(filename,sizeof(filename), "/dev/input/event%d", i);

		fd = open(filename, O_RDWR|O_SYNC);
		ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(key_bitmask)), key_bitmask);

		// Ensure that we do not detect our own tsak faked keyboards
		ioctl (fd, EVIOCGNAME (sizeof (name)), name);
		if (str_ends_with(name, "+tsak") == 0) {
			/* We assume that anything that has an alphabetic key in the
				QWERTYUIOP range in it is the main keyboard. */
			for (j = KEY_Q; j <= KEY_P; j++) {
				if (TestBit(j, key_bitmask)) {
					keyboard_fds[keyboard_fd_num] = fd;
				}
			}
		}

		if (keyboard_fds[keyboard_fd_num] == 0) {
			close (fd);
		}
		else {
			keyboard_fd_num++;
		}
	}
	return 0;
}

void tearDownPipe()
{
	if (mPipeOpen_out == true) {
		mPipeOpen_out = false;
		close(mPipe_fd_out);
		unlink(FIFO_FILE_OUT);
	}
}

void tearDownLockingPipe()
{
	close(mPipe_lockfd_out);
	unlink(FIFO_LOCKFILE_OUT);
}

bool setFileLock(int fd, bool close_on_failure)
{
	struct flock fl;
	
	fl.l_type = F_WRLCK;
	fl.l_whence = SEEK_SET;
	fl.l_start = 0;
	fl.l_len = 1;

	// Set the exclusive file lock
	if (fcntl(fd, F_SETLK, &fl) == -1) {
		close(fd);
		return false;
	}

	return true;
}

bool checkFileLock()
{
	struct flock fl;

	fl.l_type    = F_WRLCK;   /* Test for any lock on any part of file. */
	fl.l_start   = 0;
	fl.l_whence  = SEEK_SET;
	fl.l_len     = 0;

	int fd = open(FIFO_LOCKFILE_OUT, O_RDWR | O_NONBLOCK);
	fcntl(fd, F_GETLK, &fl);  /* Overwrites lock structure with preventors. */

	if (fd > -1) {
		if (fl.l_type == F_WRLCK) {
			return false;
		}
		return true;
	}

	return true;
}

bool setupPipe()
{
	/* Create the FIFOs if they do not exist */
	umask(0);
	mkdir(FIFO_DIR,0644);
	
	mknod(FIFO_FILE_OUT, S_IFIFO|0600, 0);
	chmod(FIFO_FILE_OUT, 0600);
	
	mPipe_fd_out = open(FIFO_FILE_OUT, O_RDWR | O_NONBLOCK);
	if (mPipe_fd_out > -1) {
		mPipeOpen_out = true;
	}

	// Set the exclusive file lock
	return setFileLock(mPipe_fd_out, true);
}

bool setupLockingPipe()
{
	/* Create the FIFOs if they do not exist */
	umask(0);
	mkdir(FIFO_DIR,0644);
	
	mknod(FIFO_LOCKFILE_OUT, S_IFIFO|0600, 0);
	chmod(FIFO_LOCKFILE_OUT, 0600);
	
	mPipe_lockfd_out = open(FIFO_LOCKFILE_OUT, O_RDWR | O_NONBLOCK);
	if (mPipe_lockfd_out > -1) {
		// Set the exclusive file lock
		return setFileLock(mPipe_lockfd_out, true);
	}

	return false;
}

void broadcast_sak()
{
	// Let anyone listening to our interface know that an SAK keypress was received
	// I highly doubt there are more than 255 VTs active at once...
	int i;
	for (i=0;i<255;i++) {
		write(mPipe_fd_out, "SAK\n\r", 6);
	}
}

void restart_tsak()
{
	int i;

	fprintf(stderr, "Forcibly terminating...\n");

	// Close down all child processes
	for (i=0; i<MAX_KEYBOARDS; i++) {
		if (child_pids[i] != 0) {
			kill(child_pids[i], SIGKILL);
		}
	}

	// Wait for process termination
	sleep(1);

	// Release all exclusive keyboard locks
	for (int current_keyboard=0;current_keyboard<keyboard_fd_num;current_keyboard++) {
		if(ioctl(keyboard_fds[current_keyboard], EVIOCGRAB, 0) < 0) {
			fprintf(stderr, "Failed to release exclusive input device lock");
		}
		close(keyboard_fds[current_keyboard]);
	}

#if 1
	// Restart now
	// Note that the execl function never returns
	char me[2048];
	int chars = readlink("/proc/self/exe", me, sizeof(me));
	me[chars] = 0;
	me[2047] = 0;
	execl(me, basename(me), (char*)NULL);
#else
	_exit(0);	
#endif
}

class PipeHandler
{
public:
	PipeHandler();
	~PipeHandler();
};

PipeHandler::PipeHandler()
{
}

PipeHandler::~PipeHandler()
{
	tearDownLockingPipe();
}

int main (int argc, char *argv[])
{
	struct input_event ev[64];
	struct input_event event;
	struct input_event revev;
	struct uinput_user_dev devinfo={0};
	int devout[MAX_KEYBOARDS], rd, i, value, size = sizeof (struct input_event);
	char name[256] = "Unknown";
	bool ctrl_down = false;
	bool alt_down = false;
	bool hide_event = false;
	bool established = false;
	bool testrun = false;
	int current_keyboard;
	bool can_proceed;

	for (i=0; i<MAX_KEYBOARDS; i++) {
		child_pids[i] = 0;
	}

	if (argc == 2) {
		if (strcmp(argv[1], "checkactive") == 0) {
			testrun = true;
		}
	}

	// Check for existing file locks
	if (!checkFileLock()) {
		fprintf(stderr, "Another instance of this program is already running [1]\n");
		return 8;
	}
	if (!setupLockingPipe()) {
		fprintf(stderr, "Another instance of this program is already running [2]\n");
		return 8;
	}

	// Create the output pipe
	PipeHandler controlpipe;
	if (!setupPipe()) {
		fprintf(stderr, "Another instance of this program is already running\n");
		return 8;
	}

	while (1) {
		if ((getuid ()) != 0) {
			printf ("You are not root! This WILL NOT WORK!\nDO NOT attempt to bypass security restrictions, e.g. by changing keyboard permissions or owner, if you want the SAK system to remain secure...\n");
			return 5;
		}
	
		// Find keyboards
		find_keyboards();
		if (keyboard_fd_num == 0) {
			printf ("Could not find any usable keyboard(s)!\n");
			// Make sure everyone knows we physically can't detect a SAK 
			// Before we do this we broadcast one so that active dialogs are updated appropriately
			// Also, we keep watching for a keyboard to be added via a forked child process...
			broadcast_sak();
			if (established)
				sleep(1);
			else {
				int i=fork();
				if (i<0) return 12; // fork failed
				if (i>0) {
					return 4;
				}
				sleep(1);
				restart_tsak();
			}
		}
		else {
			fprintf(stderr, "Found %d keyboard(s)\n", keyboard_fd_num);

			can_proceed = true;
			for (current_keyboard=0;current_keyboard<keyboard_fd_num;current_keyboard++) {
				// Print Device Name
				ioctl (keyboard_fds[current_keyboard], EVIOCGNAME (sizeof (name)), name);
				fprintf(stderr, "Reading from keyboard: (%s)\n", name);
			
				// Create filtered virtual output device
				devout[current_keyboard]=open("/dev/misc/uinput",O_RDWR|O_NONBLOCK);
				if (devout[current_keyboard]<0) {
					devout[current_keyboard]=open("/dev/uinput",O_RDWR|O_NONBLOCK);
					if (devout[current_keyboard]<0) {
						perror("open(\"/dev/misc/uinput\")");
					}
				}
				if (devout[current_keyboard]<0) {
					can_proceed = false;
					fprintf(stderr, "Unable to open /dev/uinput or /dev/misc/uinput (char device 10:223).\nPossible causes:\n 1) Device node does not exist\n 2) Kernel not compiled with evdev [INPUT_EVDEV] and uinput [INPUT_UINPUT] user level driver support\n 3) Permission denied.\n");
					perror("open(\"/dev/uinput\")");
					if (established)
						sleep(1);
					else
						return 3;
				}
			}

			if (can_proceed == true) {
				for (current_keyboard=0;current_keyboard<keyboard_fd_num;current_keyboard++) {
					if(ioctl(keyboard_fds[current_keyboard], EVIOCGRAB, 2) < 0) {
						close(keyboard_fds[current_keyboard]);
						fprintf(stderr, "Failed to grab exclusive input device lock");
						if (established)
							sleep(1);
						else
							return 1;
					}
					else {
						ioctl(keyboard_fds[current_keyboard], EVIOCGNAME(UINPUT_MAX_NAME_SIZE), devinfo.name);
						strncat(devinfo.name, "+tsak", UINPUT_MAX_NAME_SIZE-1);
						fprintf(stderr, "%s\n", devinfo.name);
						ioctl(keyboard_fds[current_keyboard], EVIOCGID, &devinfo.id);
						
						copy_features(keyboard_fds[current_keyboard], devout[current_keyboard]);
						write(devout[current_keyboard],&devinfo,sizeof(devinfo));
						if (ioctl(devout[current_keyboard],UI_DEV_CREATE)<0) {
							fprintf(stderr, "Unable to create input device with UI_DEV_CREATE\n");
							if (established)
								sleep(1);
							else
								return 2;
						}
						else {
							fprintf(stderr, "Device created.\n");
	
							if (established == false) {
								int i=fork();
								if (i<0) return 9; // fork failed
								if (i>0) {
									child_pids[current_keyboard] = i;
									continue;
								}
								setupLockingPipe();
							}
	
							established = true;
	
							if (testrun == true) {
								return 0;
							}
					
							while (1) {
								if ((rd = read (keyboard_fds[current_keyboard], ev, size * 2)) < size) {
									fprintf(stderr, "Read failed.\n");
									break;
								}

								// Replicate LED events from the virtual keyboard to the physical keyboard
								int rrd = read(devout[current_keyboard], &revev, size);
								if (rrd >= size) {
									if (revev.type == EV_LED) {
										write(keyboard_fds[current_keyboard], &revev, sizeof(revev));
									}
								}
								
								value = ev[0].value;
								
								if (value != ' ' && ev[1].value == 0 && ev[1].type == 1){ // Read the key release event
									if (keycode[(ev[1].code)]) {
										if (strcmp(keycode[(ev[1].code)], "<control>") == 0) ctrl_down = false;
										if (strcmp(keycode[(ev[1].code)], "<alt>") == 0) alt_down = false;
									}
								}
								if (value != ' ' && ev[1].value == 1 && ev[1].type == 1){ // Read the key press event
									if (keycode[(ev[1].code)]) {
										if (strcmp(keycode[(ev[1].code)], "<control>") == 0) ctrl_down = true;
										if (strcmp(keycode[(ev[1].code)], "<alt>") == 0) alt_down = true;
									}
								}
						
								hide_event = false;
								if (keycode[(ev[1].code)]) {
									if (alt_down && ctrl_down && (strcmp(keycode[(ev[1].code)], "<del>") == 0)) {
										hide_event = true;
									}
								}
						
								if ((hide_event == false) && (ev[0].type != EV_LED) && (ev[1].type != EV_LED)) {
									// Pass the event on...
									event = ev[0];
									write(devout[current_keyboard], &event, sizeof event);
									event = ev[1];
									write(devout[current_keyboard], &event, sizeof event);
								}
								if (hide_event == true) {
									// Let anyone listening to our interface know that an SAK keypress was received
									broadcast_sak();
								}
							}
						}
					}
				}

				// fork udev monitor process
				int i=fork();
				if (i<0) return 10; // fork failed
				if (i>0) {
					// Terminate parent
					return 0;
				}

				// Prevent multiple process instances from starting
				setupLockingPipe();

				// Wait a little bit so that udev hotplug can stabilize before we start monitoring
				sleep(1);

				fprintf(stderr, "Hotplug monitoring process started\n");

				// Monitor for hotplugged keyboards
				int j;
				int hotplug_fd;
				bool is_new_keyboard;
				struct udev *udev;
				struct udev_device *dev;
				struct udev_monitor *mon;

				// Create the udev object
				udev = udev_new();
				if (!udev) {
					fprintf(stderr, "Cannot connect to udev interface\n");
					return 11;
				}

				// Set up a udev monitor to monitor input devices
				mon = udev_monitor_new_from_netlink(udev, "udev");
				udev_monitor_filter_add_match_subsystem_devtype(mon, "input", NULL);
				udev_monitor_enable_receiving(mon);
	
				while (1) {
					// Watch for input from the monitoring process
					dev = udev_monitor_receive_device(mon);
					if (dev) {
						// If a keyboard was removed we need to restart...
						if (strcmp(udev_device_get_action(dev), "remove") == 0) {
							udev_device_unref(dev);
							udev_unref(udev);
							restart_tsak();
						}

						is_new_keyboard = false;
						snprintf(filename,sizeof(filename), "%s", udev_device_get_devnode(dev));
						udev_device_unref(dev);

						// Print name of keyboard
						hotplug_fd = open(filename, O_RDWR|O_SYNC);
						ioctl(hotplug_fd, EVIOCGBIT(EV_KEY, sizeof(key_bitmask)), key_bitmask);
					
						/* We assume that anything that has an alphabetic key in the
							QWERTYUIOP range in it is the main keyboard. */
						for (j = KEY_Q; j <= KEY_P; j++) {
							if (TestBit(j, key_bitmask)) {
								is_new_keyboard = true;
							}
						}
						ioctl (hotplug_fd, EVIOCGNAME (sizeof (name)), name);
						close(hotplug_fd);

						// Ensure that we do not detect our own tsak faked keyboards
						if (str_ends_with(name, "+tsak") == 1) {
							is_new_keyboard = false;
						}

						// If a keyboard was added we need to restart...
						if (is_new_keyboard == true) {
							fprintf(stderr, "Hotplugged new keyboard: (%s)\n", name);
							udev_unref(udev);
							restart_tsak();
						}
					}
					else {
						fprintf(stderr, "No Device from receive_device(). An error occured.\n");
					}
				}

				udev_unref(udev);

				fprintf(stderr, "Hotplug monitoring process terminated\n");
			}
		}
	}

	return 6;
}
