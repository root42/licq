/*
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <qvalidator.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qcheckbox.h>
#include <qpushbutton.h>

#include "adduserdlg.h"

#include "icqd.h"

AddUserDlg::AddUserDlg(CICQDaemon *s, QWidget *parent, const char *name)
   : QDialog(parent, name)
{
   server = s;
   resize(240, 120);
   lblUin = new QLabel(tr("New User UIN:"), this);
   lblUin->setGeometry(10, 15, 80, 20);
   edtUin = new QLineEdit(this);
   edtUin->setGeometry(100, 15, 120, 20);
   edtUin->setValidator(new QIntValidator(0, 2147483647, edtUin));
   chkAlert = new QCheckBox(tr("&Alert User"), this);
   chkAlert->setGeometry(10, 50, 180, 20);
   btnOk = new QPushButton("&Ok", this);
   btnOk->setGeometry(30, 80, 80, 30);
   btnCancel = new QPushButton(tr("&Cancel"), this);
   btnCancel->setGeometry(130, 80, 80, 30);
   connect (btnOk, SIGNAL(clicked()), SLOT(ok()) );
   connect (edtUin, SIGNAL(returnPressed()), SLOT(ok()) );
   connect (btnCancel, SIGNAL(clicked()), SLOT(reject()) );
}


void AddUserDlg::show()
{
   edtUin->setText("");
   edtUin->setFocus();
   chkAlert->setChecked(false);
   QDialog::show();
}

void AddUserDlg::hide()
{
   QDialog::hide();
   delete this;
}


void AddUserDlg::ok()
{
   unsigned long nUin = atol((const char *)edtUin->text());
   if (nUin != 0)
   {
     server->AddUserToList(nUin);
     if (chkAlert->isChecked()) // alert the user they were added
       server->icqAlertUser(nUin);
   }
   accept();
}

#include "moc/moc_adduserdlg.h"
