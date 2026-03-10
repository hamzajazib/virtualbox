/* $Id: UINotificationObject.h 113302 2026-03-10 11:24:07Z sergey.dubov@oracle.com $ */
/** @file
 * VBox Qt GUI - UINotificationObject class declaration.
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

#ifndef FEQT_INCLUDED_SRC_notificationcenter_UINotificationObject_h
#define FEQT_INCLUDED_SRC_notificationcenter_UINotificationObject_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* Qt includes: */
#include <QObject>

/* GUI includes: */
#include "UILibraryDefs.h"

/* COM includes: */
#include "CProgress.h"

/* Forward declarations: */
class UINotificationProgressTask;
#ifdef VBOX_GUI_WITH_NETWORK_MANAGER
class UIDownloader;
class UINewVersionChecker;
#endif

/** QObject-based notification-object. */
class SHARED_LIBRARY_STUFF UINotificationObject : public QObject
{
    Q_OBJECT;

signals:

    /** Notifies model about closing.
      * @param  fDismiss  Brings whether message closed as dismissed. */
    void sigAboutToClose(bool fDismiss);

public:

    /** Constructs notification-object. */
    UINotificationObject();

    /** Returns object name. */
    virtual QString name() const = 0;
    /** Returns object details. */
    virtual QString details() const = 0;
    /** Returns object internal name. */
    virtual QString internalName() const = 0;
    /** Returns object help keyword. */
    virtual QString helpKeyword() const = 0;
    /** Returns whether object is critical. */
    virtual bool isCritical() const = 0;
    /** Returns whether object is done. */
    virtual bool isDone() const = 0;
    /** Handles notification-object being added. */
    virtual void handle() = 0;

    /** Defines whether we should show message again. */
    void setDoNotShowAgain(bool fDismiss) { m_fDismiss = fDismiss; }
    /** Returns whether we should show message again. */
    bool isDoNotShowAgain() const { return m_fDismiss; }

public slots:

    /** Notifies model about dismissing. */
    virtual void dismiss();
    /** Notifies model about closing. */
    virtual void close();

private:

    /** Holds whether we should show message again. */
    bool  m_fDismiss;
};

/** UINotificationObject extension for notification-simple. */
class SHARED_LIBRARY_STUFF UINotificationSimple : public UINotificationObject
{
    Q_OBJECT;

public:

    /** Returns object name. */
    virtual QString name() const RT_OVERRIDE RT_FINAL { return m_strName; }
    /** Returns object details. */
    virtual QString details() const RT_OVERRIDE RT_FINAL { return m_strDetails; }
    /** Returns object internal name. */
    virtual QString internalName() const RT_OVERRIDE RT_FINAL { return m_strInternalName; }
    /** Returns object help keyword. */
    virtual QString helpKeyword() const RT_OVERRIDE RT_FINAL { return m_strHelpKeyword; }
    /** Returns whether object is critical. */
    virtual bool isCritical() const RT_OVERRIDE RT_FINAL { return m_fCritical; }
    /** Handles notification-object being added. */
    virtual void handle() RT_OVERRIDE RT_FINAL {}

    /** Defines whether object is @a fCritical. */
    void setCritical(bool fCritical) { m_fCritical = fCritical; }

protected:

    /** Constructs notification-simple.
      * @param  strName          Brings the message name.
      * @param  strDetails       Brings the message details.
      * @param  strInternalName  Brings the message internal name.
      * @param  strHelpKeyword   Brings the message help keyword. */
    UINotificationSimple(const QString &strName,
                         const QString &strDetails,
                         const QString &strInternalName,
                         const QString &strHelpKeyword);

    /** Returns whether message with passed @a strInternalName is suppressed. */
    static bool isSuppressed(const QString &strInternalName);

    /** Holds the message name. */
    QString  m_strName;
    /** Holds the message details. */
    QString  m_strDetails;
    /** Holds the message internal name. */
    QString  m_strInternalName;
    /** Holds the message help keyword. */
    QString  m_strHelpKeyword;
    /** Holds whether message is critical. */
    bool     m_fCritical;
};

/** UINotificationObject extension for notification-progress. */
class SHARED_LIBRARY_STUFF UINotificationProgress : public UINotificationObject
{
    Q_OBJECT;

signals:

    /** Notifies listeners about progress started. */
    void sigProgressStarted();
    /** Notifies listeners about progress changed.
      * @param  uPercent  Brings new progress percentage value. */
    void sigProgressChange(ulong uPercent);
    /** Notifies listeners about progress finished. */
    void sigProgressFinished();

public:

    /** Constructs notification-progress. */
    UINotificationProgress();
    /** Destructs notification-progress. */
    virtual ~UINotificationProgress() RT_OVERRIDE;

    /** Creates and returns started progress-wrapper. */
    virtual CProgress createProgress(COMResult &comResult) = 0;

    /** Returns current progress percentage value. */
    ulong percent() const;
    /** Returns whether progress is cancelable. */
    bool isCancelable() const;
    /** Returns error-message if any. */
    QString error() const;

    /** Returns object internal name. */
    virtual QString internalName() const RT_OVERRIDE RT_FINAL { return QString(); }
    /** Returns object help keyword. */
    virtual QString helpKeyword() const RT_OVERRIDE RT_FINAL { return QString(); }
    /** Returns whether object is critical. */
    virtual bool isCritical() const RT_OVERRIDE RT_FINAL { return true; }
    /** Returns whether object is done. */
    virtual bool isDone() const RT_OVERRIDE RT_FINAL { return m_fDone; }
    /** Handles notification-object being added. */
    virtual void handle() RT_OVERRIDE RT_FINAL;

public slots:

    /** Stops the progress and notifies model about closing. */
    virtual void close() RT_OVERRIDE RT_FINAL;

private slots:

    /** Handles signal about progress changed.
      * @param  uPercent  Brings new progress percentage value. */
    void sltHandleProgressChange(ulong uPercent);
    /** Handles signal about progress finished. */
    void sltHandleProgressFinished();

private:

    /** Holds the instance of progress-task being wrapped by this notification-progress. */
    UINotificationProgressTask *m_pTask;

    /** Holds the last cached progress percentage value. */
    ulong  m_uPercent;
    /** Holds whether current progress is done. */
    bool   m_fDone;
};

#ifdef VBOX_GUI_WITH_NETWORK_MANAGER
/** UINotificationObject extension for notification-downloader. */
class SHARED_LIBRARY_STUFF UINotificationDownloader : public UINotificationObject
{
    Q_OBJECT;

signals:

    /** Notifies listeners about progress started. */
    void sigProgressStarted();
    /** Notifies listeners about progress changed.
      * @param  uPercent  Brings new progress percentage value. */
    void sigProgressChange(ulong uPercent);
    /** Notifies listeners about progress failed. */
    void sigProgressFailed();
    /** Notifies listeners about progress canceled. */
    void sigProgressCanceled();
    /** Notifies listeners about progress finished. */
    void sigProgressFinished();

public:

    /** Constructs notification-downloader. */
    UINotificationDownloader();
    /** Destructs notification-downloader. */
    virtual ~UINotificationDownloader() RT_OVERRIDE;

    /** Creates and returns started downloader-wrapper. */
    virtual UIDownloader *createDownloader() = 0;

    /** Returns current progress percentage value. */
    ulong percent() const;
    /** Returns error-message if any. */
    QString error() const;

    /** Returns object internal name. */
    virtual QString internalName() const RT_OVERRIDE RT_FINAL { return QString(); }
    /** Returns object help keyword. */
    virtual QString helpKeyword() const RT_OVERRIDE RT_FINAL { return QString(); }
    /** Returns whether object is critical. */
    virtual bool isCritical() const RT_OVERRIDE RT_FINAL { return true; }
    /** Returns whether object is done. */
    virtual bool isDone() const RT_OVERRIDE RT_FINAL { return m_fDone; }
    /** Handles notification-object being added. */
    virtual void handle() RT_OVERRIDE RT_FINAL;

public slots:

    /** Stops the downloader and notifies model about closing. */
    virtual void close() RT_OVERRIDE RT_FINAL;

private slots:

    /** Handles signal about progress changed.
      * @param  uPercent  Brings new progress percentage value. */
    void sltHandleProgressChange(ulong uPercent);
    /** Handles signal about progress failed.
      * @param  strError  Brings error message if any. */
    void sltHandleProgressFailed(const QString &strError);
    /** Handles signal about progress canceled. */
    void sltHandleProgressCanceled();
    /** Handles signal about progress finished. */
    void sltHandleProgressFinished();

private:

    /** Holds the instance of downloader being wrapped by this notification-downloader. */
    UIDownloader *m_pDownloader;

    /** Holds the last cached downloader percentage value. */
    ulong    m_uPercent;
    /** Holds the error message is any. */
    QString  m_strError;
    /** Holds whether current downloader is done. */
    bool     m_fDone;
};
#endif /* VBOX_GUI_WITH_NETWORK_MANAGER */

#endif /* !FEQT_INCLUDED_SRC_notificationcenter_UINotificationObject_h */
