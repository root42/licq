/*
 * This file is part of Licq, an instant messaging client for UNIX.
 * Copyright (C) 2007 Licq developers
 *
 * Licq is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Licq is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Licq; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "ownereditdlg.h"

#include "config.h"

#include <QComboBox>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>

#include <licq_icqd.h>

#include "core/messagebox.h"

#include "helpers/support.h"

#include "widgets/protocombobox.h"

using namespace LicqQtGui;
/* TRANSLATOR LicqQtGui::OwnerEditDlg */

OwnerEditDlg::OwnerEditDlg(unsigned long ppid, QWidget* parent)
  : QDialog(parent),
    myPpid(ppid)
{
  Support::setWidgetProps(this, "OwnerEdit");
  setAttribute(Qt::WA_DeleteOnClose, true);
  setWindowTitle(tr("Edit Account"));

  QGridLayout* lay = new QGridLayout(this);
  lay->setColumnStretch(2, 2);
  lay->setColumnMinimumWidth(1, 8);

  cmbProtocol = new ProtoComboBox(ppid == 0, this);

  edtId = new QLineEdit();
  connect(edtId, SIGNAL(returnPressed()), SLOT(slot_ok()));

  edtPassword = new QLineEdit();
  edtPassword->setEchoMode(QLineEdit::Password);
  connect(edtPassword, SIGNAL(returnPressed()), SLOT(slot_ok()));

  unsigned short i = 0;
  QLabel* lbl;

#define ADDWIDGET(name, widget) \
  lbl = new QLabel(name); \
  lbl->setBuddy(widget); \
  lay->addWidget(lbl, i, 0); \
  lay->addWidget(widget, i++, 2)

  ADDWIDGET(tr("Pro&tocol:"), cmbProtocol);
  ADDWIDGET(tr("&User ID:"), edtId);
  ADDWIDGET(tr("&Password:"), edtPassword);

#undef ADDWIDGET

  chkSave = new QCheckBox(tr("&Save Password"));
  lay->addWidget(chkSave, i++, 0, 1, 3);

  QDialogButtonBox* buttons = new QDialogButtonBox();
  buttons->addButton(QDialogButtonBox::Ok);
  buttons->addButton(QDialogButtonBox::Cancel);
  connect(buttons, SIGNAL(accepted()), SLOT(slot_ok()));
  connect(buttons, SIGNAL(rejected()), SLOT(close()));
  lay->addWidget(buttons, i++, 0, 1, 3);

  // Set the fields
  if (ppid != 0)
  {
    ICQOwner* o = gUserManager.FetchOwner(ppid, LOCK_R);
    if (o != NULL)
    {
      edtId->setText(o->IdString());
      edtId->setEnabled(false);
      edtPassword->setText(o->Password());
      chkSave->setChecked(o->SavePassword());
      gUserManager.DropOwner(ppid);
    }

    cmbProtocol->setCurrentPpid(ppid);
    cmbProtocol->setEnabled(false);
  }
  else
  {
    if (cmbProtocol->count() == 0)
    {
      InformUser(this, tr("Currently only one account per protocol is supported."));
      close();
      return;
    }
  }

  show();
}

void OwnerEditDlg::slot_ok()
{
  QString id = edtId->text();
  QString pwd = edtPassword->text();
  unsigned long ppid = myPpid == 0 ? cmbProtocol->currentPpid() : myPpid;

  if (id.isEmpty())
  {
    InformUser(this, tr("User ID field cannot be empty."));
    return;
  }

  if (myPpid == 0)
    gUserManager.AddOwner(id.toLocal8Bit(), ppid);

  ICQOwner* o = gUserManager.FetchOwner(ppid, LOCK_W);
  if (o == NULL)
    return;

  o->SetPassword(pwd.toLocal8Bit());
  o->SetSavePassword(chkSave->isChecked());

  gUserManager.DropOwner(ppid);
  gLicqDaemon->SaveConf();

  close();
}
