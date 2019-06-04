 /*
 *  This file is part of the Trinity Desktop Environment
 *
 *  Original file taken from the OpenSUSE tdebase builds
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
 */

// ---

int TQTDEIntegration::information( TQWidget* parent, const TQString& caption,
    const TQString& text, int button0, int button1, int button2 )
    {
    return tqttde_messageBox1(
        TQMessageBox::Information, parentToWinId( parent ), caption, text, button0, button1, button2 );
    }

int TQTDEIntegration::question( TQWidget* parent, const TQString& caption,
    const TQString& text, int button0, int button1, int button2 )
    {
    return tqttde_messageBox1(
        TQMessageBox::Question, parentToWinId( parent ), caption, text, button0, button1, button2 );
    }

int TQTDEIntegration::warning( TQWidget* parent, const TQString& caption,
    const TQString& text, int button0, int button1, int button2 )
    {
    return tqttde_messageBox1(
        TQMessageBox::Warning, parentToWinId( parent ), caption, text, button0, button1, button2 );
    }

int TQTDEIntegration::critical( TQWidget* parent, const TQString& caption,
    const TQString& text, int button0, int button1, int button2 )
    {
    return tqttde_messageBox1(
        TQMessageBox::Critical, parentToWinId( parent ), caption, text, button0, button1, button2 );
    }

int TQTDEIntegration::information( TQWidget* parent, const TQString& caption,
    const TQString& text, const TQString& button0Text, const TQString& button1Text, const TQString& button2Text,
    int defaultButton, int escapeButton )
    {
    return tqttde_messageBox2(
        TQMessageBox::Information, parentToWinId( parent ), caption, text, button0Text, button1Text, button2Text, defaultButton, escapeButton );
    }

int TQTDEIntegration::question( TQWidget* parent, const TQString& caption,
    const TQString& text, const TQString& button0Text, const TQString& button1Text, const TQString& button2Text,
    int defaultButton, int escapeButton )
    {
    return tqttde_messageBox2(
        TQMessageBox::Question, parentToWinId( parent ), caption, text, button0Text, button1Text, button2Text, defaultButton, escapeButton );
    }

int TQTDEIntegration::warning( TQWidget* parent, const TQString& caption,
    const TQString& text, const TQString& button0Text, const TQString& button1Text, const TQString& button2Text,
    int defaultButton, int escapeButton )
    {
    return tqttde_messageBox2(
        TQMessageBox::Warning, parentToWinId( parent ), caption, text, button0Text, button1Text, button2Text, defaultButton, escapeButton );
    }

int TQTDEIntegration::critical( TQWidget* parent, const TQString& caption,
    const TQString& text, const TQString& button0Text, const TQString& button1Text, const TQString& button2Text,
    int defaultButton, int escapeButton )
    {
    return tqttde_messageBox2(
        TQMessageBox::Critical, parentToWinId( parent ), caption, text, button0Text, button1Text, button2Text, defaultButton, escapeButton );
    }
