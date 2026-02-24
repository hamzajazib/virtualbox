/* $Id: UINotificationQuestion.cpp 113144 2026-02-24 12:38:24Z sergey.dubov@oracle.com $ */
/** @file
 * VBox Qt GUI - Various UINotificationQuestion implementations.
 */

/*
 * Copyright (C) 2021-2026 Oracle and/or its affiliates.
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
#include <QApplication>

/* GUI includes: */
#include "UINotificationCenter.h"
#include "UINotificationQuestion.h"


/* static */
QMap<QString, QUuid> UINotificationQuestion::m_questions = QMap<QString, QUuid>();

/* static */
bool UINotificationQuestion::confirmCreatingPath(const QString &strPath)
{
    return createBlockingQuestion(
        QApplication::translate("UIMessageCenter", "Create machine path?"),
        QApplication::translate("UIMessageCenter", "<p>Selected path doesn't exist:<br>%1</p>"
                                "<p>Would you like to create it?</p>").arg(strPath));
}

/* static */
bool UINotificationQuestion::confirmResetMachine(const QString &strNames)
{
    return createBlockingQuestion(
        QApplication::translate("UIMessageCenter", "Reset following machines?"),
        QApplication::translate("UIMessageCenter", "<p>Do you really want to reset the following virtual machines?</p>"
                                                   "<p><b>%1</b></p><p>This will cause any unsaved data in applications running "
                                                   "inside it to be lost.</p>").arg(strNames),
        QStringList() << QString() /* cancel button text */
                      << QApplication::translate("UIMessageCenter", "Reset", "machine") /* ok button text */,
        true /* Ok by default? */,
        "confirmResetMachine" /* internal name */);
}

/* static */
bool UINotificationQuestion::confirmSnapshotRemoval(const QString &strName)
{
    return createBlockingQuestion(
        QApplication::translate("UIMessageCenter", "Deleting the snapshot?"),
        QApplication::translate("UIMessageCenter", "<p>Deleting the snapshot will cause the state information saved in it to be "
                                                   "lost, and storage data spread over several image files that VirtualBox has "
                                                   "created together with the snapshot will be merged into one file. This can be "
                                                   "a lengthy process, and the information in the snapshot cannot be "
                                                   "recovered.</p></p>Are you sure you want to delete the selected snapshot "
                                                   "<b>%1</b>?</p>").arg(strName),
        QStringList() << QString() /* cancel button text */
                      << QApplication::translate("UIMessageCenter", "Delete") /* ok button text */);
}

UINotificationQuestion::UINotificationQuestion(const QString &strName,
                                               const QString &strDetails,
                                               const QStringList &buttonNames,
                                               bool fOkByDefault,
                                               const QString &strInternalName,
                                               const QString &strHelpKeyword)
    : UINotificationSimple(strName,
                           strDetails,
                           strInternalName,
                           strHelpKeyword)
    , m_buttonNames(buttonNames)
    , m_fOkByDefault(fOkByDefault)
    , m_enmResult(Question::Result_Cancel)
    , m_fDone(false)
{
}

UINotificationQuestion::~UINotificationQuestion()
{
    /* Remove questions from known: */
    m_questions.remove(internalName());
}

/* static */
void UINotificationQuestion::createQuestionInt(UINotificationCenter *pParent,
                                               const QString &strName,
                                               const QString &strDetails,
                                               const QStringList &buttonNames,
                                               bool fOkByDefault,
                                               const QString &strInternalName,
                                               const QString &strHelpKeyword)
{
    /* Make sure parent is set: */
    AssertPtr(pParent);
    UINotificationCenter *pEffectiveParent = pParent ? pParent : gpNotificationCenter;

    /* Check if question suppressed: */
    if (isSuppressed(strInternalName))
        return;

    /* Check if question already exists: */
    if (   !strInternalName.isEmpty()
        && m_questions.contains(strInternalName))
        return;

    /* Create question finally: */
    const QUuid uId = pEffectiveParent->append(new UINotificationQuestion(strName,
                                                                          strDetails,
                                                                          buttonNames,
                                                                          fOkByDefault,
                                                                          strInternalName,
                                                                          strHelpKeyword));
    if (!strInternalName.isEmpty())
        m_questions[strInternalName] = uId;
}

/* static */
int UINotificationQuestion::createBlockingQuestionInt(UINotificationCenter *pParent,
                                                      const QString &strName,
                                                      const QString &strDetails,
                                                      const QStringList &buttonNames,
                                                      bool fOkByDefault,
                                                      const QString &strInternalName,
                                                      const QString &strHelpKeyword)
{
    /* Make sure parent is set: */
    AssertPtr(pParent);
    UINotificationCenter *pEffectiveParent = pParent ? pParent : gpNotificationCenter;

    /* Check if question suppressed: */
    if (isSuppressed(strInternalName))
        return Question::Result_Accept;

    /* Create question finally: */
    QPointer<UINotificationQuestion> pQuestion = new UINotificationQuestion(strName,
                                                                            strDetails,
                                                                            buttonNames,
                                                                            fOkByDefault,
                                                                            strInternalName,
                                                                            strHelpKeyword);
    const int iResult = pEffectiveParent->showBlocking(pQuestion);
    delete pQuestion;
    return iResult;
}

/* static */
void UINotificationQuestion::createQuestion(const QString &strName,
                                            const QString &strDetails,
                                            const QStringList &buttonNames /* = QStringList() */,
                                            bool fOkByDefault /* = true */,
                                            const QString &strInternalName /* = QString() */,
                                            const QString &strHelpKeyword /* = QString() */,
                                            QWidget *pParent /* = 0 */)
{
    /* Acquire notification-center, make sure it's present: */
    UINotificationCenter *pCenter = UINotificationCenter::acquire(pParent);
    AssertPtrReturnVoid(pCenter);

    /* Redirect to wrapper above: */
    return createQuestionInt(pCenter, strName, strDetails, buttonNames, fOkByDefault, strInternalName, strHelpKeyword);
}

/* static */
int UINotificationQuestion::createBlockingQuestion(const QString &strName,
                                                   const QString &strDetails,
                                                   const QStringList &buttonNames /* = QStringList() */,
                                                   bool fOkByDefault /* = true */,
                                                   const QString &strInternalName /* = QString() */,
                                                   const QString &strHelpKeyword /* = QString() */,
                                                   QWidget *pParent /* = 0 */)
{
    /* Acquire notification-center, make sure it's present: */
    UINotificationCenter *pCenter = UINotificationCenter::acquire(pParent);
    AssertPtrReturn(pCenter, 0);

    /* Redirect to wrapper above: */
    return createBlockingQuestionInt(pCenter, strName, strDetails, buttonNames, fOkByDefault, strInternalName, strHelpKeyword);
}

/* static */
void UINotificationQuestion::destroyQuestion(const QString &strInternalName,
                                             UINotificationCenter *pParent /* = 0 */)
{
    /* Check if question really exists: */
    if (!m_questions.contains(strInternalName))
        return;

    /* Choose effective parent: */
    UINotificationCenter *pEffectiveParent = pParent ? pParent : gpNotificationCenter;

    /* Destroy question finally: */
    pEffectiveParent->revoke(m_questions.value(strInternalName));
    m_questions.remove(strInternalName);
}
