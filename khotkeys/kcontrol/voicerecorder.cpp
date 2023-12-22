/****************************************************************************

 KHotKeys
 
 Copyright (C) 2005 Olivier Goffgart <ogoffart @ kde.org>

 Distributed under the terms of the GNU General Public License version 2.
 
****************************************************************************/

#include <tqcolor.h>
#include <tqevent.h>

#include "voicerecorder.h"
#include "soundrecorder.h"
#include "voicesignature.h"
#include "voices.h"
#include "khotkeysglobal.h"
#include <kpushbutton.h>
#include <klineedit.h>
#include <tdelocale.h>
#include <tdetempfile.h>
#include <tqlabel.h>
#include <tqpainter.h>
#include <tdemessagebox.h>
#include <klibloader.h>
#include <kstandarddirs.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

namespace KHotKeys
{

VoiceRecorder::arts_play_fun VoiceRecorder::arts_play = NULL;

bool VoiceRecorder::init( KLibrary* lib )
{
#ifdef HAVE_ARTS
    if( arts_play == NULL && lib != NULL )
        arts_play = (arts_play_fun) lib->symbol( "khotkeys_voicerecorder_arts_play" );
#endif
//    kdDebug( 1217 ) << "voicerecorder:" << arts_play << ":" << lib << endl;
    return arts_play != NULL;
}

VoiceRecorder::VoiceRecorder(const Sound& sound_P, const TQString &voiceId, TQWidget *parent, const char *name)
	: Voice_input_widget_ui(parent, name) , _recorder( SoundRecorder::create(this)) , _state(sNotModified), _tempFile(0L) ,  _voiceId(voiceId)
{
	_sound=sound_P;
	buttonPlay->setEnabled(sound_P.size() > 50);
	buttonStop->setEnabled(false);

	connect (_recorder , TQT_SIGNAL(recorded(const Sound& )) , this , TQT_SLOT(slotSoundRecorded(const Sound& ) ));

	//if(voiceid_P.isEmpty())
	emit recorded(false);
	
	drawSound();
}

VoiceRecorder::~VoiceRecorder()
{
	delete _tempFile;
}

void VoiceRecorder::slotRecordPressed()
{
       buttonRecord->setEnabled(false);
       buttonPlay->setEnabled(false);
       buttonStop->setEnabled(true);
      _recorder->start();
       label->setText(i18n("Recording..."));
}

void VoiceRecorder::slotStopPressed()
{
       buttonRecord->setEnabled(true);
       buttonPlay->setEnabled(false);
       buttonStop->setEnabled(false);
       _recorder->stop();
}

void VoiceRecorder::slotPlayPressed()
{
#ifdef HAVE_ARTS
        if( !haveArts() || arts_play == NULL )
            return;
	/*if(!_modified)
	{
		TQString fileName = locateLocal( "appdata", _original_voiuceid +  ".wav"  );
                arts_play( fileName );
	}
	else
	{*/
	if(!_tempFile)
	{
		_tempFile=new KTempFile(TQString::null,".wav");
		_tempFile->setAutoDelete(true);
	}
	_sound.save(_tempFile->name());
        arts_play( _tempFile->name());
#endif
}

Sound VoiceRecorder::sound() const
{
		return _sound;
}


void VoiceRecorder::slotSoundRecorded(const Sound &sound)
{
	buttonPlay->setEnabled(true);
	_sound=sound;

	bool correct=drawSound()  &&  sound.size()>50;
	if(correct)
	{
		TQString vm=voice_handler->isNewSoundFarEnough( VoiceSignature(sound), _voiceId);
		if(!vm.isNull())
		{
			KMessageBox::sorry (this, i18n("The word you recorded is too close to the existing reference '%1'. Please record another word.").arg(vm) );
			//TODO: messagebox saying there are too much noise	
			correct=false;
		}
	}
	else
	{
		KMessageBox::sorry (this, i18n("Unable to extract voice information from noise.\nIf this error occurs repeatedly, it suggests that there is either too much background noise, or the quality of your microphone is too poor.") );
	}
		
	_state=correct ? sModified : sIncorrect;
	emit recorded(correct);
}


/*VoiceSignature VoiceRecorder::voiceSig() const
{
	if(voiceId().isEmpty())
		return VoiceSignature();
	TQString fileName = locateLocal( "appdata", voiceId() +  ".wav"  );
	_sound.save( fileName );
	return VoiceSignature(_sound);
}*/

bool VoiceRecorder::drawSound()
{
	label->setText(TQString::null);
	uint length=_sound.size();

	if(length < 2)
		return false;

	int width=label->width();
	int height=label->height();
	TQPixmap pix(width,height);
	pix.fill(TQColor(255,255,255));
	TQPainter p;
	p.begin(&pix);

	p.setPen(TQPen(TQColor("green"),1));
	p.drawLine(0,height/2,width,height/2);
	
	p.setPen(TQPen(TQColor("red"),1));
	
	uint lx=0;
	uint ly=height/2;

	/***     DRAW THE TQT_SIGNAL     ******/
	for(uint f=1; f<length; f++)
	{
		uint nx=f*width/length;
		uint ny=(uint)(height/2* (1.0 - _sound.at(f)));
		p.drawLine(lx,ly,nx,ny);
		lx=nx; ly=ny;
	}

	unsigned int start=0 , stop=0;
	bool res=KHotKeys::VoiceSignature::window(_sound,&start,&stop);
	p.setPen(TQPen(TQColor("blue"),1));
	if(res)
	{
		p.drawLine(start*width/length ,0,start*width/length  ,height);
		p.drawLine(stop*width/length ,0,stop*width/length  ,height);
	}
	else
	{
		p.drawLine(0 ,0,  width  ,height);
		p.drawLine(width ,0,  0  ,height);
	}

	p.end();

	label->setPixmap(pix);
	return res;
}


} // namespace KHotKeys

#include "voicerecorder.moc"
