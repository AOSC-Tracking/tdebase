/****************************************************************************

 KHotKeys
 
 Copyright (C) 2005 Olivier Goffart  <ogoffart @ kde.org>

 Distributed under the terms of the GNU General Public License version 2.
 
****************************************************************************/

#ifndef VOICES_H_
#define VOICES_H_

#include <tqwidget.h>
#include <tdeshortcut.h>

class Sound;
class TQTimer;
class TDEGlobalAccel;

namespace KHotKeys
{

class Voice;
class SoundRecorder;

class Voice_trigger;
class VoiceSignature;


class TDE_EXPORT Voice  : public TQObject
    {
    TQ_OBJECT
    public:
        Voice( bool enabled_P, TQObject* parent_P );
        virtual ~Voice();
        void enable( bool enable_P );

		void register_handler( Voice_trigger* );
		void unregister_handler( Voice_trigger* );
//		bool x11Event( XEvent* e );
		
		void set_shortcut( const TDEShortcut &k);
		
		/**
		 * return TQString::null is a new signature is far enough from others signature
		 * otherwise, return the stringn which match.
		 */
		TQString isNewSoundFarEnough(const VoiceSignature& s, const TQString& currentTrigger);
		
		bool doesVoiceCodeExists(const TQString &s);

    public slots:
         void record_start();
         void record_stop();

    private slots:
		void slot_sound_recorded( const Sound & );
		void slot_key_pressed();
		void slot_timeout();

    signals:
        void handle_voice( const TQString &voice );
    private:

        bool _enabled;
        bool _recording;

		TQValueList<Voice_trigger *> _references;
		SoundRecorder *_recorder;
		
		TDEShortcut _shortcut;
		TDEGlobalAccel *_kga;
		
		TQTimer *_timer;
    };

	
TDE_EXPORT extern Voice* voice_handler;

} // namespace KHotKeys

#endif
