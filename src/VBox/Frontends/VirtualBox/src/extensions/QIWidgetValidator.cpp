/* $Id: QIWidgetValidator.cpp 113052 2026-02-17 09:56:11Z sergey.dubov@oracle.com $ */
/** @file
 * VBox Qt GUI - Qt extensions: QIWidgetValidator class implementation.
 */

/*
 * Copyright (C) 2006-2026 Oracle and/or its affiliates.
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

/* GUI includes: */
#include "QIWidgetValidator.h"

/* Other VBox includes: */
#include "iprt/assert.h"


/*********************************************************************************************************************************
*   Class QIObjectValidator implementation.                                                                                      *
*********************************************************************************************************************************/

QIObjectValidator::QIObjectValidator(QValidator *pValidator, QObject *pParent /* = 0 */)
    : QObject(pParent)
    , m_pValidator(pValidator)
    , m_enmState(QValidator::Invalid)
{
    prepare();
}

void QIObjectValidator::sltValidate(QString strInput /* = QString() */)
{
    /* Make sure validator assigned: */
    AssertPtrReturnVoid(m_pValidator);

    /* Validate: */
    int iPosition = 0;
    const QValidator::State enmState = m_pValidator->validate(strInput, iPosition);

    /* If validity state changed: */
    if (m_enmState != enmState)
    {
        /* Update last validity state: */
        m_enmState = enmState;

        /* Notifies listener(s) about validity change: */
        emit sigValidityChange(m_enmState);
    }
}

void QIObjectValidator::prepare()
{
    /* Make sure validator assigned: */
    AssertPtrReturnVoid(m_pValidator);

    /* Register validator as child: */
    m_pValidator->setParent(this);

    /* Validate: */
    sltValidate();
}


/*********************************************************************************************************************************
*   Class QIObjectValidatorGroup implementation.                                                                                 *
*********************************************************************************************************************************/

QIObjectValidatorGroup::QIObjectValidatorGroup(QObject *pParent)
    : QObject(pParent)
    , m_fResult(false)
{
}

void QIObjectValidatorGroup::addObjectValidator(QIObjectValidator *pObjectValidator)
{
    /* Make sure object-validator passed: */
    AssertPtrReturnVoid(pObjectValidator);

    /* Register object-validator as child: */
    pObjectValidator->setParent(this);

    /* Insert object-validator to internal map: */
    m_group.insert(pObjectValidator, toResult(pObjectValidator->state()));

    /* Attach object-validator to group: */
    connect(pObjectValidator, &QIObjectValidator::sigValidityChange,
            this, &QIObjectValidatorGroup::sltValidate);
}

void QIObjectValidatorGroup::sltValidate(QValidator::State enmState)
{
    /* Determine sender object-validator: */
    QIObjectValidator *pObjectValidatorSender = qobject_cast<QIObjectValidator*>(sender());
    /* Make sure that is one of our senders: */
    AssertReturnVoid(pObjectValidatorSender && m_group.contains(pObjectValidatorSender));

    /* Update internal map: */
    m_group[pObjectValidatorSender] = toResult(enmState);

    /* Enumerate all the registered object-validators: */
    bool fResult = true;
    foreach (QIObjectValidator *pObjectValidator, m_group.keys())
        if (!toResult(pObjectValidator->state()))
        {
            fResult = false;
            break;
        }

    /* If validity state changed: */
    if (m_fResult != fResult)
    {
        /* Update last validity state: */
        m_fResult = fResult;

        /* Notifies listener(s) about validity change: */
        emit sigValidityChange(m_fResult);
    }
}

/* static */
bool QIObjectValidatorGroup::toResult(QValidator::State enmState)
{
    return enmState == QValidator::Acceptable;
}


/*********************************************************************************************************************************
*   Class QIULongValidator implementation.                                                                                       *
*********************************************************************************************************************************/

QValidator::State QIULongValidator::validate(QString &strInput, int &iPosition) const
{
    Q_UNUSED(iPosition);

    /* Get the stripped string: */
    QString strStripped = strInput.trimmed();

    /* 'Intermediate' for empty string or started from '0x': */
    if (strStripped.isEmpty() ||
        strStripped.toUpper() == QString("0x").toUpper())
        return Intermediate;

    /* Convert to ulong: */
    bool fOk;
    ulong uEntered = strInput.toULong(&fOk, 0);

    /* 'Invalid' if failed to convert: */
    if (!fOk)
        return Invalid;

    /* 'Acceptable' if fits the bounds: */
    if (uEntered >= m_uBottom && uEntered <= m_uTop)
        return Acceptable;

    /* 'Invalid' if more than top, 'Intermediate' if less than bottom: */
    return uEntered > m_uTop ? Invalid : Intermediate;
}
