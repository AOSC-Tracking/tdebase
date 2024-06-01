/*******************************************************************************
 syndaemon - daemon for the Synaptics touchpad driver which disables touchpad
             on keyboard input

 Copyright © 2004 Nadeem Hasan <nhasan@kde.org>
                  Stefan Kombrink <katakombi@web.de>
             2024 Mavridis Philippe <mavridisf@gmail.com>

 This program is free software: you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation, either version 3 of the License, or (at your option) any later
 version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

 You should have received a copy of the GNU General Public License along with
 this program. If not, see <https://www.gnu.org/licenses/>.

*******************************************************************************/

// TQt
#include <tqdatetime.h>
#include <tqtimer.h>

// TDE
#include <ksimpleconfig.h>
#include <tdecmdlineargs.h>
#include <kuniqueapplication.h>
#include <tdeaboutdata.h>
#include <tdelocale.h>
#include <kdebug.h>

// DCOP
#include <dcopclient.h>

// tdecm_touchpad
#include "touchpad_settings.h"

// SynDaemon
#include "syndaemon.h"
#include "syndaemon.moc"

const unsigned int SynDaemon::TIME_OUT = 150;
const unsigned int SynDaemon::POLL_INTERVAL = 100;
const unsigned int SynDaemon::KEYMAP_SIZE = 32;

unsigned char* SynDaemon::m_keyboard_mask;

SynDaemon::SynDaemon() : DCOPObject("syndaemon"), TQObject()
{
    m_typing = false;
    m_time = new TQTime();
    d_settings = new TouchpadSettings;

    m_keyboard_mask = new unsigned char[ KEYMAP_SIZE ];

    // open a connection to the X server
    m_display = XOpenDisplay(NULL);

    if (!m_display) kdError() << "Can't open display!" << endl;

    // setup keymap
    XModifierKeymap *modifiers;

    for (unsigned int i = 0; i < KEYMAP_SIZE; ++i)
        m_keyboard_mask[i] = 0xFF;

    modifiers = XGetModifierMapping(m_display);
    for (int i = 0; i < 8 * modifiers->max_keypermod; ++i)
    {
        KeyCode kc = modifiers->modifiermap[i];
        if (kc != 0) clearBit(m_keyboard_mask, kc);
    }

    XFreeModifiermap(modifiers);

    m_poll = new TQTimer(this);
    connect(m_poll, TQ_SIGNAL(timeout()), this, TQ_SLOT(poll()));
    m_poll->start(POLL_INTERVAL);
}

SynDaemon::~SynDaemon()
{
    setTouchpadOn(true);
    m_poll->stop();
    delete m_keyboard_mask;
}

void SynDaemon::stop()
{
    kapp->quit();
}

void SynDaemon::poll()
{
    // do nothing if the user has explicitly disabled the touchpad in the settings
    if (!touchpadEnabled()) return;

    if (hasKeyboardActivity())
    {
        m_time->start();

        if (!m_typing)
        {
            setTouchpadOn(false);
        }
    }

    else
    {
        if (m_typing && (m_time->elapsed() > TIME_OUT))
        {
            setTouchpadOn(true);
        }
    }
}

bool SynDaemon::touchpadEnabled()
{
    // We can't read from our own TouchpadSettings
    // as it contains the currently applied value
    // so we revert to this
    KSimpleConfig cfg("kcminputrc");
    cfg.setGroup("Touchpad");
    return cfg.readBoolEntry("Enabled", true);
}

void SynDaemon::setTouchpadOn(bool on)
{
    m_typing = !on;
    if (!d_settings->setTouchpadEnabled(on))
    {
        kdWarning() << "unable to turn off touchpad!" << endl;
    }
}

void SynDaemon::clearBit(unsigned char *ptr, int bit)
{
    int byteNum = bit / 8;
    int bitNum = bit % 8;
    ptr[byteNum] &= ~(1 << bitNum);
}

bool SynDaemon::hasKeyboardActivity()
{
    static unsigned char oldKeyState[KEYMAP_SIZE];
    unsigned char keyState[KEYMAP_SIZE];

    bool result = false;

    XQueryKeymap(m_display, (char*)keyState);

    // find pressed keys
    for (unsigned int i = 0; i < KEYMAP_SIZE; ++i)
    {
        if ((keyState[i] & ~oldKeyState[i]) & m_keyboard_mask[i])
        {
            result = true;
            break;
        }
    }

    // ignore any modifiers
    for (unsigned int i = 0; i < KEYMAP_SIZE; ++i)
    {
        if (keyState[i] & ~m_keyboard_mask[i])
        {
            result = false;
            break;
        }
    }

    // back up key states...
    for (unsigned int i = 0; i < KEYMAP_SIZE; ++i)
    {
        oldKeyState[i] = keyState[i];
    }

    return result;
}

extern "C" TDE_EXPORT int main(int argc, char *argv[])
{
    TDEAboutData aboutData( "syndaemon", I18N_NOOP("Synaptics helper daemon"),
        "0.1", I18N_NOOP("Synaptics helper daemon"), TDEAboutData::License_GPL_V2,
        "© 2024 Mavridis Philippe" );

    aboutData.addAuthor("Nadeem Hasan", I18N_NOOP("Author"), "nhasan@kde.org");
    aboutData.addAuthor("Mavridis Philippe", I18N_NOOP("Author"), "mavridisf@gmail.com");

    TDECmdLineArgs::init(argc, argv, &aboutData);

    TDEApplication app;
    app.disableSessionManagement();
    app.dcopClient()->registerAs("syndaemon", false);

    SynDaemon syndaemon;
    return app.exec();
}