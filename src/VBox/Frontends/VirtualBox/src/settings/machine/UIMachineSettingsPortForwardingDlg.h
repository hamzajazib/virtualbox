/* $Id: UIMachineSettingsPortForwardingDlg.h 113042 2026-02-16 14:07:21Z sergey.dubov@oracle.com $ */
/** @file
 * VBox Qt GUI - UIMachineSettingsPortForwardingDlg class declaration.
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

#ifndef FEQT_INCLUDED_SRC_settings_machine_UIMachineSettingsPortForwardingDlg_h
#define FEQT_INCLUDED_SRC_settings_machine_UIMachineSettingsPortForwardingDlg_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* GUI includes: */
#include "QIDialog.h"
#include "UIPortForwardingTable.h"

/* Forward declarations: */
class QIDialogButtonBox;
class UINotificationCenter;

/* Machine settings / Network page / NAT attachment / Port forwarding dialog: */
class SHARED_LIBRARY_STUFF UIMachineSettingsPortForwardingDlg : public QIDialog
{
    Q_OBJECT;

public:

    /** Constructs Port-forwarding dialog passing @a pParent to base-class.
      * @param  rules  Brings a list of current port-forwarding rules. */
    UIMachineSettingsPortForwardingDlg(QWidget *pParent, const UIPortForwardingDataList &rules);

    /** Returns a list of current port-forwarding rules. */
    UIPortForwardingDataList rules() const;

protected:

    /** Dismisses dialog, accepting result. */
    virtual void accept() RT_OVERRIDE;
    /** Dismisses dialog, rejecting result. */
    virtual void reject() RT_OVERRIDE;

private slots:

    /** Handles translation event. */
    void sltRetranslateUI();

private:

    /** Holds the table instance. */
    UIPortForwardingTable *m_pTable;
    /** Holds the button-box instance. */
    QIDialogButtonBox     *m_pButtonBox;

    /** Holds the local notification-center instance. */
    UINotificationCenter *m_pNotificationCenter;
};

#endif /* !FEQT_INCLUDED_SRC_settings_machine_UIMachineSettingsPortForwardingDlg_h */
