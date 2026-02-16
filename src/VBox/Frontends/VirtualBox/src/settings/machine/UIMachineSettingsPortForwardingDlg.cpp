/* $Id: UIMachineSettingsPortForwardingDlg.cpp 113042 2026-02-16 14:07:21Z sergey.dubov@oracle.com $ */
/** @file
 * VBox Qt GUI - UIMachineSettingsPortForwardingDlg class implementation.
 */

/*
 * Copyright (C) 2010-2026 Oracle and/or its affiliates.
 *
 * This file is part of VirtualBox base platform packages, as
 * available from https://www.virtualbox.org.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, in version 3 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <https://www.gnu.org/licenses>.
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */

/* Qt includes: */
#include <QPushButton>
#include <QVBoxLayout>

/* GUI includes: */
#include "QIDialogButtonBox.h"
#include "UIDesktopWidgetWatchdog.h"
#include "UIIconPool.h"
#include "UIMachineSettingsPortForwardingDlg.h"
#include "UIMessageCenter.h"
#include "UINotificationCenter.h"
#include "UITranslationEventListener.h"


UIMachineSettingsPortForwardingDlg::UIMachineSettingsPortForwardingDlg(QWidget *pParent,
                                                                       const UIPortForwardingDataList &rules)
    : QIDialog(pParent)
    , m_pTable(0)
    , m_pButtonBox(0)
    , m_pNotificationCenter(0)
{
#ifndef VBOX_WS_MAC
    /* Assign window icon: */
    setWindowIcon(UIIconPool::iconSetFull(":/nw_32px.png", ":/nw_16px.png"));
#endif

    /* Limit the minimum size to 33% of screen size: */
    setMinimumSize(gpDesktop->screenGeometry(this).size() / 3);

    /* Prepare local notification-center (parent to be assigned in the end): */
    m_pNotificationCenter = new UINotificationCenter(0);
    if (m_pNotificationCenter)
    {
        QPointer<UINotificationCenter> target = m_pNotificationCenter;
        setProperty("notification_center", QVariant::fromValue(target));
    }

    /* Create layout: */
    QVBoxLayout *pLayout = new QVBoxLayout(this);
    {
        /* Prepare table: */
        m_pTable = new UIPortForwardingTable(rules, false /* ip IPv6 protocol */, true /* allow empty guest IPs */);
        {
            m_pTable->layout()->setContentsMargins(0, 0, 0, 0);
            pLayout->addWidget(m_pTable);
        }

        /* Prepare button-box: */
        m_pButtonBox = new QIDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal);
        {
            connect(m_pButtonBox->button(QDialogButtonBox::Ok), &QPushButton::clicked,
                    this, &UIMachineSettingsPortForwardingDlg::accept);
            connect(m_pButtonBox->button(QDialogButtonBox::Cancel), &QPushButton::clicked,
                    this, &UIMachineSettingsPortForwardingDlg::reject);

            pLayout->addWidget(m_pButtonBox);
        }
    }

    /* Assign notification-center parent (after everything else is done): */
    m_pNotificationCenter->setParent(this);

    /* Apply language settings: */
    sltRetranslateUI();
    connect(&translationEventListener(), &UITranslationEventListener::sigRetranslateUI,
            this, &UIMachineSettingsPortForwardingDlg::sltRetranslateUI);
}

UIPortForwardingDataList UIMachineSettingsPortForwardingDlg::rules() const
{
    return m_pTable->rules();
}

void UIMachineSettingsPortForwardingDlg::accept()
{
    /* Make sure table has own data committed: */
    m_pTable->makeSureEditorDataCommitted();

    /* Validate table: */
    if (!m_pTable->validate())
        return;

    /* Call to base-class: */
    QIDialog::accept();
}

void UIMachineSettingsPortForwardingDlg::reject()
{
    /* Ask user to discard table changes if necessary: */
    if (   m_pTable->isChanged()
        && !msgCenter().confirmCancelingPortForwardingDialog(window()))
        return;

    /* Call to base-class: */
    QIDialog::reject();
}

void UIMachineSettingsPortForwardingDlg::sltRetranslateUI()
{
    /* Set window title: */
    setWindowTitle(tr("Port Forwarding Rules"));
}
