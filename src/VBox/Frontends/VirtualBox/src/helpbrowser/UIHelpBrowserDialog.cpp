/* $Id: UIHelpBrowserDialog.cpp 113213 2026-03-03 07:36:58Z sergey.dubov@oracle.com $ */
/** @file
 * VBox Qt GUI - UIHelpBrowserDialog class implementation.
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
#include <QFileInfo>
#include <QLabel>
#include <QMenuBar>
#include <QStatusBar>

/* GUI includes: */
#include "UICommon.h"
#include "UIDesktopWidgetWatchdog.h"
#include "UIExtraDataManager.h"
#include "UIHelpBrowserDialog.h"
#include "UIHelpBrowserWidget.h"
#include "UINotificationMessage.h"
#include "UITranslationEventListener.h"
#ifndef VBOX_WS_MAC
# include "UIIconPool.h"
#endif

/* Other VBox includes: */
#include <iprt/assert.h>
#include <VBox/version.h> /* VBOX_PRODUCT */


/*********************************************************************************************************************************
*   Class UIHelpBrowserDialog implementation.                                                                                    *
*********************************************************************************************************************************/

QPointer<UIHelpBrowserDialog> UIHelpBrowserDialog::m_pInstance;

UIHelpBrowserDialog::UIHelpBrowserDialog(QWidget *pParent, QWidget *pCenterWidget, const QString &strHelpFilePath)
    : QIMainWindow(pParent)
    , m_strHelpFilePath(strHelpFilePath)
    , m_pWidget(0)
    , m_pCenterWidget(pCenterWidget)
    , m_iGeometrySaveTimerId(-1)
    , m_pZoomLabel(0)
{
#ifndef VBOX_WS_MAC
    /* Assign window icon: */
    setWindowIcon(UIIconPool::iconSetFull(":/log_viewer_find_32px.png", ":/log_viewer_find_16px.png"));
#endif

    setAttribute(Qt::WA_DeleteOnClose);
    statusBar()->show();
    m_pZoomLabel = new QLabel;
    statusBar()->addPermanentWidget(m_pZoomLabel);

    prepareCentralWidget();
    loadSettings();
    sltRetranslateUI();
    connect(&translationEventListener(), &UITranslationEventListener::sigRetranslateUI,
        this, &UIHelpBrowserDialog::sltRetranslateUI);
}

void UIHelpBrowserDialog::showHelpForKeyword(const QString &strKeyword)
{
    if (m_pWidget)
        m_pWidget->showHelpForKeyword(strKeyword);
}

void UIHelpBrowserDialog::sltRetranslateUI()
{
    setWindowTitle(UIHelpBrowserWidget::tr("%1 User Guide", "[Product Name] User Guide").arg(VBOX_PRODUCT));
}

bool UIHelpBrowserDialog::event(QEvent *pEvent)
{
    switch (pEvent->type())
    {
        case QEvent::Resize:
        case QEvent::Move:
        {
            if (m_iGeometrySaveTimerId != -1)
                killTimer(m_iGeometrySaveTimerId);
            m_iGeometrySaveTimerId = startTimer(300);
            break;
        }
        case QEvent::Timer:
        {
            QTimerEvent *pTimerEvent = static_cast<QTimerEvent*>(pEvent);
            if (pTimerEvent->timerId() == m_iGeometrySaveTimerId)
            {
                killTimer(m_iGeometrySaveTimerId);
                m_iGeometrySaveTimerId = -1;
                saveDialogGeometry();
            }
            break;
        }
        default:
            break;
    }
    return QIMainWindow::event(pEvent);
}

void UIHelpBrowserDialog::prepareCentralWidget()
{
    m_pWidget = new UIHelpBrowserWidget(EmbedTo_Dialog, m_strHelpFilePath);
    AssertPtrReturnVoid(m_pWidget);
    setCentralWidget((m_pWidget));
    sltZoomPercentageChanged(m_pWidget->zoomPercentage());
    connect(m_pWidget, &UIHelpBrowserWidget::sigCloseDialog,
            this, &UIHelpBrowserDialog::close);
    connect(m_pWidget, &UIHelpBrowserWidget::sigStatusBarMessage,
            this, &UIHelpBrowserDialog::sltStatusBarMessage);
    connect(m_pWidget, &UIHelpBrowserWidget::sigStatusBarVisible,
            this, &UIHelpBrowserDialog::sltStatusBarVisibilityChange);
    connect(m_pWidget, &UIHelpBrowserWidget::sigZoomPercentageChanged,
            this, &UIHelpBrowserDialog::sltZoomPercentageChanged);

    const QList<QMenu*> menuList = m_pWidget->menus();
    foreach (QMenu *pMenu, menuList)
        menuBar()->addMenu(pMenu);
}

void UIHelpBrowserDialog::loadSettings()
{
    const QRect availableGeo = gpDesktop->availableGeometry(this);
    int iDefaultWidth = availableGeo.width() / 2;
    int iDefaultHeight = availableGeo.height() * 3 / 4;
    QRect defaultGeo(0, 0, iDefaultWidth, iDefaultHeight);

    const QRect geo = gEDataManager->helpBrowserDialogGeometry(this, m_pCenterWidget, defaultGeo);
    restoreGeometry(geo);
}

void UIHelpBrowserDialog::saveDialogGeometry()
{
    const QRect geo = currentGeometry();
    gEDataManager->setHelpBrowserDialogGeometry(geo, isCurrentlyMaximized());
}

bool UIHelpBrowserDialog::shouldBeMaximized() const
{
    return gEDataManager->helpBrowserDialogShouldBeMaximized();
}

void UIHelpBrowserDialog::sltStatusBarMessage(const QString& strLink, int iTimeOut)
{
    statusBar()->showMessage(strLink, iTimeOut);
}

void UIHelpBrowserDialog::sltStatusBarVisibilityChange(bool fVisible)
{
    statusBar()->setVisible(fVisible);
}

void UIHelpBrowserDialog::sltZoomPercentageChanged(int iPercentage)
{
    if (m_pZoomLabel)
        m_pZoomLabel->setText(QString("%1%").arg(QString::number(iPercentage)));
}

/* static */
void UIHelpBrowserDialog::findManualFileAndShow(const QString &strKeyword /* = QString() */)
{
    showUserManual(uiCommon().helpFile(), strKeyword);
}

/* static */
void UIHelpBrowserDialog::showUserManual(const QString &strHelpFilePath, const QString &strKeyword)
{
    if (!QFileInfo(strHelpFilePath).exists())
    {
        UINotificationMessage::cannotFindHelpFile(strHelpFilePath);
        return;
    }
    if (!m_pInstance)
    {
        m_pInstance = new UIHelpBrowserDialog(0 /* parent */, 0 /* Center Widget */, strHelpFilePath);
        AssertReturnVoid(m_pInstance);
    }

    m_pInstance->show();
    m_pInstance->setWindowState(m_pInstance->windowState() & ~Qt::WindowMinimized);
    m_pInstance->activateWindow();
    if (!strKeyword.isEmpty())
        m_pInstance->showHelpForKeyword(strKeyword);

}
