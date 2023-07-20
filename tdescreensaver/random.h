/*
    Copyright (c) 2003 Chris Howells <howells@kde.org>

    *************************************************************************
    *                                                                       *
    * This program is free software; you can redistribute it and/or         *
    * modify it under the terms of the GNU Lesser General Public            *
    * License as published by the Free Software Foundation; version         *
    * 2 of the License.                                                     *
    *                                                                       *
    *************************************************************************
*/

#ifndef RANDOM_H
#define RANDOM_H

class TQWidget;
class TQCheckBox;

class KRandomSetup : public KDialogBase
{
	TQ_OBJECT
	public:
		KRandomSetup( TQWidget *parent = NULL, const char *name = NULL );

	private:

		TQWidget *preview;
		TQCheckBox *openGL;
		TQCheckBox *manipulateScreen;

		private slots:

		void slotOk();

};

#endif
