/*
 * This file is part of Licq, an instant messaging client for UNIX.
 * Copyright (C) 2003-2011 Licq developers
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

#include "userselectdlg.h"

#include "config.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

#include <licq/contactlist/owner.h>

#include "helpers/support.h"

using namespace LicqQtGui;
/* TRANSLATOR LicqQtGui::UserSelectDlg */

UserSelectDlg::UserSelectDlg(QWidget* parent)
  : QDialog(parent)
{
  Support::setWidgetProps(this, "UserSelectDialog");
  setWindowTitle(tr("Licq User Select"));
  setModal(true);
  setAttribute(Qt::WA_DeleteOnClose, true);

  QVBoxLayout* lay = new QVBoxLayout(this);

  QHBoxLayout* layUser = new QHBoxLayout();
  lay->addLayout(layUser);

  lblUser = new QLabel(tr("&User:"));
  cmbUser = new QComboBox();
  lblUser->setBuddy(cmbUser);
  layUser->addWidget(lblUser);
  layUser->addWidget(cmbUser);

  QHBoxLayout* layPassword = new QHBoxLayout();
  lay->addLayout(layPassword);

  lblPassword = new QLabel(tr("&Password:"));
  edtPassword = new QLineEdit();
  edtPassword->setEchoMode(QLineEdit::Password);
  edtPassword->setFocus();
  lblPassword->setBuddy(edtPassword);
  layPassword->addWidget(lblPassword);
  layPassword->addWidget(edtPassword);

  chkSavePassword = new QCheckBox(tr("&Save Password"));
  lay->addWidget(chkSavePassword);

  lay->addStretch();

  QDialogButtonBox* buttons = new QDialogButtonBox();
  lay->addWidget(buttons);

  btnOk = new QPushButton(tr("&Ok"));
  buttons->addButton(btnOk, QDialogButtonBox::AcceptRole);
  connect(btnOk, SIGNAL(clicked()), SLOT(slot_ok()));

  btnCancel = new QPushButton(tr("&Cancel"));
  buttons->addButton(btnCancel, QDialogButtonBox::RejectRole);
  connect(btnCancel, SIGNAL(clicked()), SLOT(close()));

  // Populate the combo box

  // For now, just have one owner
  {
    Licq::OwnerReadGuard o(LICQ_PPID);
    if (!o.isLocked())
    {
      close();
      return;
    }
    cmbUser->addItem(QString("%1 (%2)")
        .arg(QString::fromUtf8(o->getAlias().c_str()))
        .arg(o->accountId().c_str()));
    edtPassword->setText(o->password().c_str());
  }

  // Wait for dialog to finish before returning to caller
  exec();
}

UserSelectDlg::~UserSelectDlg()
{
}

void UserSelectDlg::slot_ok()
{
  Licq::OwnerWriteGuard o(LICQ_PPID);
  if (o.isLocked())
  {
    o->SetSavePassword(chkSavePassword->isChecked());
    o->setPassword(edtPassword->text().toLatin1().constData());
  }

  close();
}
