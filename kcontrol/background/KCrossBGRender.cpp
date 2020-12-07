/*
 * Copyright (C) 2008 Danilo Cesar Lemes de Paula <danilo@mandriva.com>
 * Copyright (C) 2008 Gustavo Boiko <boiko@mandriva.com>
 * Mandriva Conectiva
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License version 2 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <tqdom.h>
#include <tqfile.h>

#include <kdebug.h>

#include "KCrossBGRender.h"
#include <tqapplication.h>
#include <kimageeffect.h>


KCrossBGRender::KCrossBGRender(int desk, int screen, bool drawBackgroundPerScreen, TDEConfig *config): KBackgroundRenderer(desk,screen,drawBackgroundPerScreen,config)
{
	useCrossEfect = false;
	if ( wallpaperList()[0].endsWith("xml",false) ) {
		initCrossFade(wallpaperList()[0]);
	}
}
		

void KCrossBGRender::initCrossFade(TQString xmlFile)
{	
	useCrossEfect = true;
	if (xmlFile.isEmpty()){
		useCrossEfect = false;
		return;
	}
	secs = 0;
	timeList.empty();

	// read the XMLfile
	TQDomDocument xmldoc = TQDomDocument(xmlFile);
	TQFile file( xmlFile );
	if ( !file.open( IO_ReadOnly ) ) {
		useCrossEfect = false;
		return;
	}
	if ( !xmldoc.setContent( &file ) ) {
		useCrossEfect = false;
		file.close();
		return;
	}
	file.close();

	TQDomElement docElem = xmldoc.documentElement();
	TQDomNode n = docElem.firstChild();
	while( !n.isNull() ) {
		TQDomElement e = n.toElement(); // try to convert the node to an element.
		if( !e.isNull() ) {
			if (e.tagName() == "starttime") {
				createStartTime(e);	
			} else if (e.tagName() == "transition") {
				createTransition(e);
			} else if (e.tagName() == "static") {
				createStatic(e);
			}
		}
		n = n.nextSibling();
	}

	// Setting "now" state
	setCurrentEvent(true);
	pix = getCurrentPixmap();
	
	useCrossEfect = true;
}


KCrossBGRender::~KCrossBGRender(){
}

TQPixmap KCrossBGRender::pixmap() {
	fixEnabled();	
	if (!useCrossEfect){
		TQPixmap p = KBackgroundRenderer::pixmap();
		kdDebug() << "Inherited " << p.size() << endl;
		if (p.width() == 0 && p.height() == 0){
			p.convertFromImage(image());
		}
		return p;
	}
	
	return pix;
}

bool KCrossBGRender::needWallpaperChange(){
	if (!useCrossEfect) {
		return KBackgroundRenderer::needWallpaperChange();
	}

	bool forceChange = setCurrentEvent();    // If we change the current state
	if (forceChange){			 // do not matter what hapens
		actualPhase = 0;                 // we need to change background
		return true;
	}

	// Return false if it's not a transition
	if (!current.transition) {
		return false;
	}

	double timeLeft, timeTotal;
	TQTime now = TQTime::currentTime();

	timeLeft = now.secsTo(current.etime);
	if (timeLeft < 0) {
		timeLeft += 86400; // before midnight
	}
	timeTotal = current.stime.secsTo(current.etime);
	if (timeTotal < 0) {
		timeTotal += 86400;
	}

	double passed  = timeTotal - timeLeft;
	double timeCell   =  timeTotal/60; //Time cell size

	//kdDebug() << "\ntimeleft:" << timeLeft << " timeTotal:" << timeTotal 
	//          << "\npassed:" << passed << " timeCell:" << timeCell
	//	  << "\nactualPhase: " << actualPhase << endl;	

	int aux = passed/timeCell;	
	if(actualPhase != aux){
		//kdDebug() << "needWallpaperChange() => returned true" << endl;
		actualPhase = passed/timeCell;
		return true;
	}

	//kdDebug() << "needWallpaperChange() => returned false" << endl;
	return false;
}

/* 
 * This method change the enabledEffect flag to TRUE of FALSE, according 
 * with multiWallpaperMode and FileName (it needs to be a XML)
 */
void KCrossBGRender::fixEnabled(){

	
	TQString w = wallpaperList()[0];
	useCrossEfect = false;	
	if(multiWallpaperMode() == Random || multiWallpaperMode() == InOrder){
		
		if ( w != xmlFileName ){
			// New XML File
			xmlFileName = w;
			if (w.endsWith("xml",false)){
				initCrossFade(wallpaperList()[0]);
				//useCrossEfect = true;	
			}else{
				// Not, it's not a xml file
				useCrossEfect = false;
			}
		}else if (w.endsWith("xml",false)){
			//xmlFile doesn't change
			//but it's there
			useCrossEfect = true;
		}else{
			// it's not a XML file
			useCrossEfect = false;
		}
	}
}

void KCrossBGRender::changeWallpaper(bool init){



	fixEnabled();

	if (!useCrossEfect){
		KBackgroundRenderer::changeWallpaper(init);
		return;
	}

	pix = getCurrentPixmap();


}


bool KCrossBGRender::setCurrentEvent(bool init){
	TQTime now = TQTime::currentTime();
	

	//Verify if is need to change
	if (!(init || now <= current.stime || now >= current.etime )) {
		return false;
	}

	TQValueList<KBGCrossEvent>::iterator it;
	for ( it = timeList.begin(); it != timeList.end(); ++it ){

		// Look for time
		if ( ((*it).stime <= now && now <= (*it).etime) ||   //normal situation
		     ((*it).etime <= (*it).stime && (now >= (*it).stime ||
		     				     now <= (*it).etime) ) )	
		{
			current = *it;
			actualPhase = 0;

			//kdDebug() << "Cur: " << current.stime << "< now <" << current.etime << endl;
			return true;
		}
	}

	return false;
}

TQPixmap KCrossBGRender::getCurrentPixmap()
{
	float alpha;
	TQPixmap ret;
	TQImage tmp;
	TQImage p1;
	if (!tmp.load(current.pix1))
		return TQPixmap();

	// scale the pixmap to fit in the screen
	//p1 = TQPixmap(QApplication::desktop()->screenGeometry().size());
	//TQPainter p(&p1);
	//p.drawPixmap(p1.rect(), tmp);
	//
	p1 = tmp.smoothScale(TQApplication::desktop()->screenGeometry().size());

	if (current.transition){
		TQTime now = TQTime::currentTime();
		double timeLeft,timeTotal;

		TQImage p2;

		if (!tmp.load(current.pix2) )
			return NULL;

		p2 = tmp.smoothScale(TQApplication::desktop()->screenGeometry().size());
		//TQPainter p(&p2);
		//p.drawPixmap(p2.rect(), tmp);

		timeLeft = now.secsTo(current.etime);
		if (timeLeft < 0)
			timeLeft += 86400;
		timeTotal = current.stime.secsTo(current.etime);
		if (timeTotal < 0)
			timeTotal += 86400;
		
		alpha = (timeTotal - timeLeft)/timeTotal;

		//ret = crossFade(p2,p1,alpha);
		tmp = KImageEffect::blend(p2,p1,alpha);
		ret.convertFromImage(tmp);
		return ret;
	}else{
		ret.convertFromImage(p1);
		return ret;
	}	
	

}

void KCrossBGRender::createStartTime(TQDomElement docElem)
{	
	int hour;
	int minutes;

	TQDomNode n = docElem.firstChild();
	while( !n.isNull() ) {
		TQDomElement e = n.toElement();
		if( !e.isNull() ) {
			if (e.tagName() == "hour"){
				hour =  e.text().toInt();
			}else if ( e.tagName() == "minute" ){
				minutes = e.text().toInt();
			}
			
		}
		
		n = n.nextSibling();
	}
	secs = hour*60*60 + minutes*60;
}
void KCrossBGRender::createTransition(TQDomElement docElem)
{
	int duration;
	TQString from;
	TQString to;

	TQDomNode n = docElem.firstChild();
	while( !n.isNull() ) {
		TQDomElement e = n.toElement();
		if( !e.isNull() ) {
			if (e.tagName() == "duration"){
				duration = e.text().toFloat();
			}else if ( e.tagName() == "from" ){
				from = e.text();
			}
			else if ( e.tagName() == "to" ){
				to = e.text();
			}
			
		}
		n = n.nextSibling();
	}
	TQTime startTime(0,0,0);
	startTime = startTime.addSecs(secs);
	TQTime endTime(0,0,0);
	endTime = endTime.addSecs(secs+duration);

	secs += duration;
	
	KBGCrossEvent l = {true, from, to, startTime,endTime};

	timeList.append(l);

}
void KCrossBGRender::createStatic(TQDomElement docElem)
{	
	int duration;
	TQString file;
	
	TQDomNode n = docElem.firstChild();
	while( !n.isNull() ) {
		TQDomElement e = n.toElement();
		if( !e.isNull() ) {
			if (e.tagName() == "duration"){
				duration = e.text().toFloat();
			}else if ( e.tagName() == "file" ){
				file = e.text();
			}
			
		}
		n = n.nextSibling();
	}
	
	TQTime startTime(0,0,0);
	startTime = startTime.addSecs(secs);
	TQTime endTime(0,0,0);
	endTime = endTime.addSecs(secs+duration);
	
	secs += duration;

	KBGCrossEvent l = {false, file, NULL, startTime,endTime};
	timeList.append(l);
}

#include "KCrossBGRender.moc"
