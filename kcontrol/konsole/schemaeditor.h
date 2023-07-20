/***************************************************************************
                          schemaeditor.h  -  description
                             -------------------
    begin                : mar apr 17 16:44:59 CEST 2001
    copyright            : (C) 2001 by Andrea Rizzi
    email                : rizzi@kde.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef SCHEMAEDITOR_H
#define SCHEMAEDITOR_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <tdeapplication.h>
#include <tqwidget.h>
class TQPixmap;
class TDESharedPixmap;

#include "schemadialog.h"

/** SchemaEditor is the base class of the porject */
class SchemaEditor : public SchemaDialog
{
  TQ_OBJECT 
  public:
    /** constructor */
    SchemaEditor(TQWidget* parent=0, const char *name=0);
    /** destructor */
    ~SchemaEditor();

    TQString schema();
    void setSchema(TQString);
    bool isModified() const { return schMod; }
    void querySave();

  signals:
  	void changed();
	void schemaListChanged(const TQStringList &titles, const TQStringList &filenames);

  public slots:
  	void slotColorChanged(int);
  	void imageSelect();  	
	void slotTypeChanged(int);
	void readSchema(int);
	void saveCurrent();
	void removeCurrent();
	void previewLoaded(bool l);		
	void getList();
  private slots:
	void show();
	void schemaModified();
	void loadAllSchema(TQString currentFile="");
	void updatePreview();
  private:
	bool schMod;
  	TQMemArray<TQColor> color;
	TQMemArray<int> type; // 0= custom, 1= sysfg, 2=sysbg, 3=rcolor
	TQMemArray<bool> transparent;
	TQMemArray<bool> bold;
	TQPixmap pix;
	TDESharedPixmap *spix;
	TQString defaultSchema;	
	bool loaded;
	bool schemaLoaded;
	bool change;
	int oldSchema;
	int oldSlot;
	TQString readSchemaTitle(const TQString& filename);
	void schemaListChanged();

};

#endif
