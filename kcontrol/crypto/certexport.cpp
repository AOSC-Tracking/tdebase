/**
 * certexport.cpp
 *
 * Copyright (c) 2001 George Staikos <staikos@kde.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published 
 *  by the Free Software Foundation; either version 2.1 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "certexport.h"
#include <tqpushbutton.h>
#include <klineedit.h>
#include <tdefiledialog.h>
#include <tqradiobutton.h>
#include <tqvbuttongroup.h>
#include <tqlayout.h>
#include <tqlabel.h>
#include <tdelocale.h>
#include <kdebug.h>
#include <tdemessagebox.h>
#include <ksslall.h>


KCertExport::KCertExport(TQWidget *parent, const char *name) 
                             : KDialog(parent, name, true) {
TQGridLayout *grid = new TQGridLayout(this, 9, 6, marginHint(), spacingHint());

   setCaption(i18n("X509 Certificate Export"));

   TQVButtonGroup *bg = new TQVButtonGroup(i18n("Format"), this);
   _pem = new TQRadioButton(i18n("&PEM"), bg);
   _netscape = new TQRadioButton(i18n("&Netscape"), bg);
   _der = new TQRadioButton(i18n("&DER/ASN1"), bg);
   _text = new TQRadioButton(i18n("&Text"), bg);
   grid->addMultiCellWidget(bg, 0, 4, 0, 3);
   _pem->setChecked(true);

   grid->addMultiCellWidget(new TQLabel(i18n("Filename:"), this), 5, 5, 0, 3);

   _filename = new KLineEdit(this);
   grid->addMultiCellWidget(_filename, 6, 6, 0, 4);
   connect(_filename, TQ_SIGNAL(textChanged(const TQString &)), this, TQ_SLOT(slotTextChanged(const TQString &)));
   connect(_filename, TQ_SIGNAL(returnPressed()), this, TQ_SLOT(slotExport()));

   _choose = new TQPushButton("...", this);
   grid->addWidget(_choose, 6, 5);
   connect(_choose, TQ_SIGNAL(clicked()), this, TQ_SLOT(slotChoose()));
   
   _export = new TQPushButton(i18n("&Export"), this);
   grid->addWidget(_export, 8, 4);
   connect(_export, TQ_SIGNAL(clicked()), this, TQ_SLOT(slotExport()));
   _export->setEnabled(false);

   _cancel = new TQPushButton(i18n("&Cancel"), this);
   grid->addWidget(_cancel, 8, 5);
   connect(_cancel, TQ_SIGNAL(clicked()), this, TQ_SLOT(reject()));
}


KCertExport::~KCertExport() {

}


void KCertExport::setCertificate(KSSLCertificate *c) {
   _c = c;
}


void KCertExport::slotExport() {
TQByteArray cert;
TQString certt;

   if (_filename->text().isEmpty()) return;

   if (!_c) {
      KMessageBox::sorry(this, i18n("Internal error. Please report to kfm-devel@kde.org."), i18n("SSL"));
      return;
   }

   if (_der->isChecked()) {
      cert = _c->toDer();
   } else if (_pem->isChecked()) {
      cert = _c->toPem();
   } else if (_text->isChecked()) {
      certt = _c->toText();
   } else {  // netscape
      cert = _c->toNetscape();
   }

      if ((!_text->isChecked() && cert.size() <= 0) && certt.isEmpty()) {
         KMessageBox::error(this, i18n("Error converting the certificate into the requested format."), i18n("SSL"));
         reject();
         return;
      }

      TQFile outFile(_filename->text());

      if (!outFile.open(IO_WriteOnly)) {
         KMessageBox::error(this, i18n("Error opening file for output."), i18n("SSL"));
         reject();
         return;
      }

      if (_text->isChecked())
        outFile.writeBlock(certt.local8Bit(), certt.length());
      else outFile.writeBlock(cert);

      outFile.close();

accept();
}


void KCertExport::slotChoose() {
  //TQString newFile = KFileDialog::getSaveFileName("::x509save", i18n("*.pem|Privacy Enhanced Mail Format\n*.der|DER/ASN1 Format"));
  TQString newFile = KFileDialog::getSaveFileName(TQString::null, "application/x-x509-ca-cert");

   //  Dunno about this one yet
   // \n*.ncert|Netscape certificate files");

  if (!newFile.isEmpty()) _filename->setText(newFile);
}


void KCertExport::slotTextChanged(const TQString& x) {
  _export->setEnabled(!x.isEmpty());
}
  

#include "certexport.moc"

