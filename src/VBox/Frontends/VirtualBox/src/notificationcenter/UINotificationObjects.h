/* $Id: UINotificationObjects.h 113213 2026-03-03 07:36:58Z sergey.dubov@oracle.com $ */
/** @file
 * VBox Qt GUI - Various UINotificationObjects declarations.
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

#ifndef FEQT_INCLUDED_SRC_notificationcenter_UINotificationObjects_h
#define FEQT_INCLUDED_SRC_notificationcenter_UINotificationObjects_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* Qt includes: */
#include <QUuid>

/* GUI includes: */
#include "UIDefs.h"
#include "UINotificationObject.h"

/* COM includes: */
#include "CAppliance.h"
#include "CCloudClient.h"
#include "CCloudMachine.h"
#include "CConsole.h"
#include "CDataStream.h"
#include "CExtPackFile.h"
#include "CExtPackManager.h"
#include "CForm.h"
#include "CFormValue.h"
#include "CGuest.h"
#include "CHost.h"
#include "CHostNetworkInterface.h"
#include "CMachine.h"
#include "CMedium.h"
#include "CSession.h"
#include "CSnapshot.h"
#include "CStringArray.h"
#ifdef VBOX_WITH_UPDATE_AGENT
# include "CUpdateAgent.h"
#endif
#include "CVFSExplorer.h"
#include "CVirtualSystemDescription.h"
#include "CVirtualSystemDescriptionForm.h"

/** UINotificationProgress extension for medium create functionality. */
class SHARED_LIBRARY_STUFF UINotificationProgressMediumCreate : public UINotificationProgress
{
    Q_OBJECT;

signals:

    /** Notifies listeners about @a comMedium was created. */
    void sigMediumCreated(const CMedium &comMedium);

public:

    /** Constructs medium create notification-progress.
      * @param  comTarget  Brings the medium being the target.
      * @param  uSize      Brings the target medium size.
      * @param  variants   Brings the target medium options. */
    UINotificationProgressMediumCreate(const CMedium &comTarget,
                                       qulonglong uSize,
                                       const QVector<KMediumVariant> &variants);

protected:

    /** Returns object name. */
    virtual QString name() const RT_OVERRIDE RT_FINAL;
    /** Returns object details. */
    virtual QString details() const RT_OVERRIDE RT_FINAL;
    /** Creates and returns started progress-wrapper. */
    virtual CProgress createProgress(COMResult &comResult) RT_OVERRIDE RT_FINAL;

private slots:

    /** Handles signal about progress being finished. */
    void sltHandleProgressFinished();

private:

    /** Holds the medium being the target. */
    CMedium                  m_comTarget;
    /** Holds the target location. */
    QString                  m_strLocation;
    /** Holds the target medium size. */
    qulonglong               m_uSize;
    /** Holds the target medium options. */
    QVector<KMediumVariant>  m_variants;
};

/** UINotificationProgress extension for medium copy functionality. */
class SHARED_LIBRARY_STUFF UINotificationProgressMediumCopy : public UINotificationProgress
{
    Q_OBJECT;

signals:

    /** Notifies listeners about @a comMedium was copied. */
    void sigMediumCopied(const CMedium &comMedium);

public:

    /** Constructs medium copy notification-progress.
      * @param  comSource    Brings the medium being copied.
      * @param  comTarget    Brings the medium being the target.
      * @param  variants     Brings the target medium options.
      * @param  uMediumSize  Brings the target medium size.
      */
    UINotificationProgressMediumCopy(const CMedium &comSource,
                                     const CMedium &comTarget,
                                     const QVector<KMediumVariant> &variants,
                                     qulonglong uMediumSize);

protected:

    /** Returns object name. */
    virtual QString name() const RT_OVERRIDE RT_FINAL;
    /** Returns object details. */
    virtual QString details() const RT_OVERRIDE RT_FINAL;
    /** Creates and returns started progress-wrapper. */
    virtual CProgress createProgress(COMResult &comResult) RT_OVERRIDE RT_FINAL;

private slots:

    /** Handles signal about progress being finished. */
    void sltHandleProgressFinished();

private:

    /** Holds the medium being copied. */
    CMedium                  m_comSource;
    /** Holds the medium being the target. */
    CMedium                  m_comTarget;
    /** Holds the source location. */
    QString                  m_strSourceLocation;
    /** Holds the target location. */
    QString                  m_strTargetLocation;
    /** Holds the target medium options. */
    QVector<KMediumVariant>  m_variants;
    /** Holds the target medium size. */
    qulonglong m_uMediumSize;
};

/** UINotificationProgress extension for medium move functionality. */
class SHARED_LIBRARY_STUFF UINotificationProgressMediumMove : public UINotificationProgress
{
    Q_OBJECT;

public:

    /** Constructs medium move notification-progress.
      * @param  comMedium    Brings the medium being moved.
      * @param  strLocation  Brings the desired location. */
    UINotificationProgressMediumMove(const CMedium &comMedium,
                                     const QString &strLocation);

protected:

    /** Returns object name. */
    virtual QString name() const RT_OVERRIDE RT_FINAL;
    /** Returns object details. */
    virtual QString details() const RT_OVERRIDE RT_FINAL;
    /** Creates and returns started progress-wrapper. */
    virtual CProgress createProgress(COMResult &comResult) RT_OVERRIDE RT_FINAL;

private:

    /** Holds the medium being moved. */
    CMedium  m_comMedium;
    /** Holds the initial location. */
    QString  m_strFrom;
    /** Holds the desired location. */
    QString  m_strTo;
};

/** UINotificationProgress extension for medium resize functionality. */
class SHARED_LIBRARY_STUFF UINotificationProgressMediumResize : public UINotificationProgress
{
    Q_OBJECT;

public:

    /** Constructs medium resize notification-progress.
      * @param  comMedium  Brings the medium being resized.
      * @param  uOldSize   Brings previous medium size.
      * @param  uNewSize   Brings desired medium size. */
    UINotificationProgressMediumResize(const CMedium &comMedium,
                                       qulonglong uOldSize,
                                       qulonglong uNewSize);

protected:

    /** Returns object name. */
    virtual QString name() const RT_OVERRIDE RT_FINAL;
    /** Returns object details. */
    virtual QString details() const RT_OVERRIDE RT_FINAL;
    /** Creates and returns started progress-wrapper. */
    virtual CProgress createProgress(COMResult &comResult) RT_OVERRIDE RT_FINAL;

private:

    /** Holds the medium being resized. */
    CMedium     m_comMedium;
    /** Holds the initial size. */
    qulonglong  m_uFrom;
    /** Holds the desired size. */
    qulonglong  m_uTo;
};

/** UINotificationProgress extension for deleting medium storage functionality. */
class SHARED_LIBRARY_STUFF UINotificationProgressMediumDeletingStorage : public UINotificationProgress
{
    Q_OBJECT;

signals:

    /** Notifies listeners about @a comMedium storage was deleted. */
    void sigMediumStorageDeleted(const CMedium &comMedium);

public:

    /** Constructs deleting medium storage notification-progress.
      * @param  comMedium  Brings the medium which storage being deleted. */
    UINotificationProgressMediumDeletingStorage(const CMedium &comMedium);

protected:

    /** Returns object name. */
    virtual QString name() const RT_OVERRIDE RT_FINAL;
    /** Returns object details. */
    virtual QString details() const RT_OVERRIDE RT_FINAL;
    /** Creates and returns started progress-wrapper. */
    virtual CProgress createProgress(COMResult &comResult) RT_OVERRIDE RT_FINAL;

private slots:

    /** Handles signal about progress being finished. */
    void sltHandleProgressFinished();

private:

    /** Holds the medium which storage being deleted. */
    CMedium  m_comMedium;
    /** Holds the medium location. */
    QString  m_strLocation;
};

/** UINotificationProgress extension for machine copy functionality. */
class SHARED_LIBRARY_STUFF UINotificationProgressMachineCopy : public UINotificationProgress
{
    Q_OBJECT;

public:

    /** Constructs machine copy notification-progress.
      * @param  comSource     Brings the machine being copied.
      * @param  comTarget     Brings the machine being the target.
      * @param  enmCloneMode  Brings the cloning mode.
      * @param  options       Brings the cloning options. */
    UINotificationProgressMachineCopy(const CMachine &comSource,
                                      const CMachine &comTarget,
                                      const KCloneMode &enmCloneMode,
                                      const QVector<KCloneOptions> &options);

protected:

    /** Returns object name. */
    virtual QString name() const RT_OVERRIDE RT_FINAL;
    /** Returns object details. */
    virtual QString details() const RT_OVERRIDE RT_FINAL;
    /** Creates and returns started progress-wrapper. */
    virtual CProgress createProgress(COMResult &comResult) RT_OVERRIDE RT_FINAL;

private slots:

    /** Handles signal about progress being finished. */
    void sltHandleProgressFinished();

private:

    /** Holds the machine being copied. */
    CMachine                m_comSource;
    /** Holds the machine being the target. */
    CMachine                m_comTarget;
    /** Holds the source name. */
    QString                 m_strSourceName;
    /** Holds the target name. */
    QString                 m_strTargetName;
    /** Holds the machine cloning mode. */
    KCloneMode              m_enmCloneMode;
    /** Holds the target machine options. */
    QVector<KCloneOptions>  m_options;
};

/** UINotificationProgress extension for machine move functionality. */
class SHARED_LIBRARY_STUFF UINotificationProgressMachineMove : public UINotificationProgress
{
    Q_OBJECT;

public:

    /** Constructs machine move notification-progress.
      * @param  uId             Brings the machine id.
      * @param  strDestination  Brings the move destination.
      * @param  strType         Brings the moving type. */
    UINotificationProgressMachineMove(const QUuid &uId,
                                      const QString &strDestination,
                                      const QString &strType);

protected:

    /** Returns object name. */
    virtual QString name() const RT_OVERRIDE RT_FINAL;
    /** Returns object details. */
    virtual QString details() const RT_OVERRIDE RT_FINAL;
    /** Creates and returns started progress-wrapper. */
    virtual CProgress createProgress(COMResult &comResult) RT_OVERRIDE RT_FINAL;

private slots:

    /** Handles signal about progress being finished. */
    void sltHandleProgressFinished();

private:

    /** Holds the machine id. */
    QUuid     m_uId;
    /** Holds the session being opened. */
    CSession  m_comSession;
    /** Holds the machine source. */
    QString   m_strSource;
    /** Holds the machine destination. */
    QString   m_strDestination;
    /** Holds the moving type. */
    QString   m_strType;
};

/** UINotificationProgress extension for machine power-up functionality. */
class SHARED_LIBRARY_STUFF UINotificationProgressMachinePowerUp : public UINotificationProgress
{
    Q_OBJECT;

public:

    /** Constructs machine power-up notification-progress.
      * @param  comMachine  Brings the machine being powered-up. */
    UINotificationProgressMachinePowerUp(const CMachine &comMachine, UILaunchMode enmLaunchMode);

protected:

    /** Returns object name. */
    virtual QString name() const RT_OVERRIDE RT_FINAL;
    /** Returns object details. */
    virtual QString details() const RT_OVERRIDE RT_FINAL;
    /** Creates and returns started progress-wrapper. */
    virtual CProgress createProgress(COMResult &comResult) RT_OVERRIDE RT_FINAL;

private slots:

    /** Handles signal about progress being finished. */
    void sltHandleProgressFinished();

private:

    /** Holds the machine being powered-up. */
    CMachine      m_comMachine;
    /** Holds the launch mode. */
    UILaunchMode  m_enmLaunchMode;
    /** Holds the session being opened. */
    CSession      m_comSession;
    /** Holds the machine name. */
    QString       m_strName;
};

/** UINotificationProgress extension for machine save-state functionality. */
class SHARED_LIBRARY_STUFF UINotificationProgressMachineSaveState : public UINotificationProgress
{
    Q_OBJECT;

signals:

    /** Notifies listeners about machine state saved.
      * @param  fSuccess  Brings whether state was saved successfully. */
    void sigMachineStateSaved(bool fSuccess);

public:

    /** Constructs machine save-state notification-progress.
      * @param  comMachine  Brings the machine being saved. */
    UINotificationProgressMachineSaveState(const CMachine &comMachine);

protected:

    /** Returns object name. */
    virtual QString name() const RT_OVERRIDE RT_FINAL;
    /** Returns object details. */
    virtual QString details() const RT_OVERRIDE RT_FINAL;
    /** Creates and returns started progress-wrapper. */
    virtual CProgress createProgress(COMResult &comResult) RT_OVERRIDE RT_FINAL;

private slots:

    /** Handles signal about progress being finished. */
    void sltHandleProgressFinished();

private:

    /** Holds the machine being saved. */
    CMachine  m_comMachine;
    /** Holds the session being opened. */
    CSession  m_comSession;
    /** Holds the machine name. */
    QString   m_strName;
};

/** UINotificationProgress extension for machine power-off functionality. */
class SHARED_LIBRARY_STUFF UINotificationProgressMachinePowerOff : public UINotificationProgress
{
    Q_OBJECT;

signals:

    /** Notifies listeners about machine powered off.
      * @param  fSuccess           Brings whether power off sequence successfully.
      * @param  fIncludingDiscard  Brings whether machine state should be discarded. */
    void sigMachinePoweredOff(bool fSuccess, bool fIncludingDiscard);

public:

    /** Constructs machine power-off notification-progress.
      * @param  comMachine         Brings the machine being powered off.
      * @param  comConsole         Brings the console of machine being powered off.
      * @param  fIncludingDiscard  Brings whether machine state should be discarded. */
    UINotificationProgressMachinePowerOff(const CMachine &comMachine,
                                          const CConsole &comConsole = CConsole(),
                                          bool fIncludingDiscard = false);

protected:

    /** Returns object name. */
    virtual QString name() const RT_OVERRIDE RT_FINAL;
    /** Returns object details. */
    virtual QString details() const RT_OVERRIDE RT_FINAL;
    /** Creates and returns started progress-wrapper. */
    virtual CProgress createProgress(COMResult &comResult) RT_OVERRIDE RT_FINAL;

private slots:

    /** Handles signal about progress being finished. */
    void sltHandleProgressFinished();

private:

    /** Holds the machine being powered off. */
    CMachine  m_comMachine;
    /** Holds the console of machine being powered off. */
    CConsole  m_comConsole;
    /** Holds whether machine state should be discarded. */
    bool      m_fIncludingDiscard;
    /** Holds the session being opened. */
    CSession  m_comSession;
    /** Holds the machine name. */
    QString   m_strName;
};

/** UINotificationProgress extension for machine media remove functionality. */
class SHARED_LIBRARY_STUFF UINotificationProgressMachineMediaRemove : public UINotificationProgress
{
    Q_OBJECT;

public:

    /** Constructs machine media remove notification-progress.
      * @param  comMachine  Brings the machine being removed.
      * @param  media       Brings the machine media being removed. */
    UINotificationProgressMachineMediaRemove(const CMachine &comMachine,
                                             const CMediumVector &media);

protected:

    /** Returns object name. */
    virtual QString name() const RT_OVERRIDE RT_FINAL;
    /** Returns object details. */
    virtual QString details() const RT_OVERRIDE RT_FINAL;
    /** Creates and returns started progress-wrapper. */
    virtual CProgress createProgress(COMResult &comResult) RT_OVERRIDE RT_FINAL;

private:

    /** Holds the machine being removed. */
    CMachine       m_comMachine;
    /** Holds the machine name. */
    QString        m_strName;
    /** Holds the machine media being removed. */
    CMediumVector  m_media;
};

/** UINotificationProgress extension for VFS explorer update functionality. */
class SHARED_LIBRARY_STUFF UINotificationProgressVFSExplorerUpdate : public UINotificationProgress
{
    Q_OBJECT;

public:

    /** Constructs VFS explorer update notification-progress.
      * @param  comExplorer  Brings the VFS explorer being updated. */
    UINotificationProgressVFSExplorerUpdate(const CVFSExplorer &comExplorer);

protected:

    /** Returns object name. */
    virtual QString name() const RT_OVERRIDE RT_FINAL;
    /** Returns object details. */
    virtual QString details() const RT_OVERRIDE RT_FINAL;
    /** Creates and returns started progress-wrapper. */
    virtual CProgress createProgress(COMResult &comResult) RT_OVERRIDE RT_FINAL;

private:

    /** Holds the VFS explorer being updated. */
    CVFSExplorer  m_comExplorer;
    /** Holds the VFS explorer path. */
    QString       m_strPath;
};

/** UINotificationProgress extension for VFS explorer files remove functionality. */
class SHARED_LIBRARY_STUFF UINotificationProgressVFSExplorerFilesRemove : public UINotificationProgress
{
    Q_OBJECT;

public:

    /** Constructs VFS explorer files remove notification-progress.
      * @param  comExplorer  Brings the VFS explorer removing the files.
      * @param  files        Brings a vector of files to be removed. */
    UINotificationProgressVFSExplorerFilesRemove(const CVFSExplorer &comExplorer,
                                                 const QVector<QString> &files);

protected:

    /** Returns object name. */
    virtual QString name() const RT_OVERRIDE RT_FINAL;
    /** Returns object details. */
    virtual QString details() const RT_OVERRIDE RT_FINAL;
    /** Creates and returns started progress-wrapper. */
    virtual CProgress createProgress(COMResult &comResult) RT_OVERRIDE RT_FINAL;

private:

    /** Holds the VFS explorer removing the files. */
    CVFSExplorer      m_comExplorer;
    /** Holds a vector of files to be removed. */
    QVector<QString>  m_files;
    /** Holds the VFS explorer path. */
    QString           m_strPath;
};

/** UINotificationProgress extension for subnet selection VSD form create functionality. */
class SHARED_LIBRARY_STUFF UINotificationProgressSubnetSelectionVSDFormCreate : public UINotificationProgress
{
    Q_OBJECT;

signals:

    /** Notifies listeners about VSD @a comForm created.
      * @param  comForm  Brings created VSD form. */
    void sigVSDFormCreated(const CVirtualSystemDescriptionForm &comForm);

public:

    /** Constructs subnet selection VSD form create notification-progress.
      * @param  comClient             Brings the cloud client being creating VSD form.
      * @param  comVsd                Brings the VSD, form being created for.
      * @param  strProviderShortName  Brings the short provider name.
      * @param  strProfileName        Brings the profile name. */
    UINotificationProgressSubnetSelectionVSDFormCreate(const CCloudClient &comClient,
                                                       const CVirtualSystemDescription &comVSD,
                                                       const QString &strProviderShortName,
                                                       const QString &strProfileName);

protected:

    /** Returns object name. */
    virtual QString name() const RT_OVERRIDE RT_FINAL;
    /** Returns object details. */
    virtual QString details() const RT_OVERRIDE RT_FINAL;
    /** Creates and returns started progress-wrapper. */
    virtual CProgress createProgress(COMResult &comResult) RT_OVERRIDE RT_FINAL;

private slots:

    /** Handles signal about progress being finished. */
    void sltHandleProgressFinished();

private:

    /** Holds the cloud client being creating VSD form. */
    CCloudClient                   m_comClient;
    /** Holds the VSD, form being created for. */
    CVirtualSystemDescription      m_comVSD;
    /** Holds the VSD form being created. */
    CVirtualSystemDescriptionForm  m_comVSDForm;
    /** Holds the short provider name. */
    QString                        m_strProviderShortName;
    /** Holds the profile name. */
    QString                        m_strProfileName;
};

/** UINotificationProgress extension for launch VSD form create functionality. */
class SHARED_LIBRARY_STUFF UINotificationProgressLaunchVSDFormCreate : public UINotificationProgress
{
    Q_OBJECT;

signals:

    /** Notifies listeners about VSD @a comForm created.
      * @param  comForm  Brings created VSD form. */
    void sigVSDFormCreated(const CVirtualSystemDescriptionForm &comForm);

public:

    /** Constructs launch VSD form create notification-progress.
      * @param  comClient             Brings the cloud client being creating VSD form.
      * @param  comVsd                Brings the VSD, form being created for.
      * @param  strProviderShortName  Brings the short provider name.
      * @param  strProfileName        Brings the profile name. */
    UINotificationProgressLaunchVSDFormCreate(const CCloudClient &comClient,
                                              const CVirtualSystemDescription &comVSD,
                                              const QString &strProviderShortName,
                                              const QString &strProfileName);

protected:

    /** Returns object name. */
    virtual QString name() const RT_OVERRIDE RT_FINAL;
    /** Returns object details. */
    virtual QString details() const RT_OVERRIDE RT_FINAL;
    /** Creates and returns started progress-wrapper. */
    virtual CProgress createProgress(COMResult &comResult) RT_OVERRIDE RT_FINAL;

private slots:

    /** Handles signal about progress being finished. */
    void sltHandleProgressFinished();

private:

    /** Holds the cloud client being creating VSD form. */
    CCloudClient                   m_comClient;
    /** Holds the VSD, form being created for. */
    CVirtualSystemDescription      m_comVSD;
    /** Holds the VSD form being created. */
    CVirtualSystemDescriptionForm  m_comVSDForm;
    /** Holds the short provider name. */
    QString                        m_strProviderShortName;
    /** Holds the profile name. */
    QString                        m_strProfileName;
};

/** UINotificationProgress extension for export VSD form create functionality. */
class SHARED_LIBRARY_STUFF UINotificationProgressExportVSDFormCreate : public UINotificationProgress
{
    Q_OBJECT;

signals:

    /** Notifies listeners about VSD @a comForm created.
      * @param  form  Brings created VSD form. */
    void sigVSDFormCreated(const QVariant &form);

public:

    /** Constructs export VSD form create notification-progress.
      * @param  comClient  Brings the cloud client being creating VSD form.
      * @param  comVsd     Brings the VSD, form being created for. */
    UINotificationProgressExportVSDFormCreate(const CCloudClient &comClient,
                                              const CVirtualSystemDescription &comVSD);

protected:

    /** Returns object name. */
    virtual QString name() const RT_OVERRIDE RT_FINAL;
    /** Returns object details. */
    virtual QString details() const RT_OVERRIDE RT_FINAL;
    /** Creates and returns started progress-wrapper. */
    virtual CProgress createProgress(COMResult &comResult) RT_OVERRIDE RT_FINAL;

private slots:

    /** Handles signal about progress being finished. */
    void sltHandleProgressFinished();

private:

    /** Holds the cloud client being creating VSD form. */
    CCloudClient                   m_comClient;
    /** Holds the VSD, form being created for. */
    CVirtualSystemDescription      m_comVSD;
    /** Holds the VSD form being created. */
    CVirtualSystemDescriptionForm  m_comVSDForm;
};

/** UINotificationProgress extension for import VSD form create functionality. */
class SHARED_LIBRARY_STUFF UINotificationProgressImportVSDFormCreate : public UINotificationProgress
{
    Q_OBJECT;

signals:

    /** Notifies listeners about VSD @a comForm created.
      * @param  form  Brings created VSD form. */
    void sigVSDFormCreated(const QVariant &form);

public:

    /** Constructs import VSD form create notification-progress.
      * @param  comClient  Brings the cloud client being creating VSD form.
      * @param  comVsd     Brings the VSD, form being created for. */
    UINotificationProgressImportVSDFormCreate(const CCloudClient &comClient,
                                              const CVirtualSystemDescription &comVSD);

protected:

    /** Returns object name. */
    virtual QString name() const RT_OVERRIDE RT_FINAL;
    /** Returns object details. */
    virtual QString details() const RT_OVERRIDE RT_FINAL;
    /** Creates and returns started progress-wrapper. */
    virtual CProgress createProgress(COMResult &comResult) RT_OVERRIDE RT_FINAL;

private slots:

    /** Handles signal about progress being finished. */
    void sltHandleProgressFinished();

private:

    /** Holds the cloud client being creating VSD form. */
    CCloudClient                   m_comClient;
    /** Holds the VSD, form being created for. */
    CVirtualSystemDescription      m_comVSD;
    /** Holds the VSD form being created. */
    CVirtualSystemDescriptionForm  m_comVSDForm;
};

/** UINotificationProgress extension for cloud image list functionality. */
class SHARED_LIBRARY_STUFF UINotificationProgressCloudImageList : public UINotificationProgress
{
    Q_OBJECT;

signals:

    /** Notifies listeners about image @a names received. */
    void sigImageNamesReceived(const QVariant &names);
    /** Notifies listeners about image @a ids received. */
    void sigImageIdsReceived(const QVariant &ids);

public:

    /** Constructs cloud images list notification-progress.
      * @param  comClient         Brings the cloud client being listing images.
      * @param  cloudImageStates  Brings the image states we are interested in. */
    UINotificationProgressCloudImageList(const CCloudClient &comClient,
                                         const QVector<KCloudImageState> &cloudImageStates);

protected:

    /** Returns object name. */
    virtual QString name() const RT_OVERRIDE RT_FINAL;
    /** Returns object details. */
    virtual QString details() const RT_OVERRIDE RT_FINAL;
    /** Creates and returns started progress-wrapper. */
    virtual CProgress createProgress(COMResult &comResult) RT_OVERRIDE RT_FINAL;

private slots:

    /** Handles signal about progress being finished. */
    void sltHandleProgressFinished();

private:

    /** Holds the cloud client being listing images. */
    CCloudClient               m_comClient;
    /** Holds the image states we are interested in. */
    QVector<KCloudImageState>  m_cloudImageStates;
    /** Holds the listed names. */
    CStringArray               m_comNames;
    /** Holds the listed ids. */
    CStringArray               m_comIds;
};

/** UINotificationProgress extension for cloud source boot volume list functionality. */
class SHARED_LIBRARY_STUFF UINotificationProgressCloudSourceBootVolumeList : public UINotificationProgress
{
    Q_OBJECT;

signals:

    /** Notifies listeners about source boot volume @a names received. */
    void sigImageNamesReceived(const QVariant &names);
    /** Notifies listeners about source boot volume @a ids received. */
    void sigImageIdsReceived(const QVariant &ids);

public:

    /** Constructs cloud source boot volumes list notification-progress.
      * @param  comClient  Brings the cloud client being listing source boot volumes. */
    UINotificationProgressCloudSourceBootVolumeList(const CCloudClient &comClient);

protected:

    /** Returns object name. */
    virtual QString name() const RT_OVERRIDE RT_FINAL;
    /** Returns object details. */
    virtual QString details() const RT_OVERRIDE RT_FINAL;
    /** Creates and returns started progress-wrapper. */
    virtual CProgress createProgress(COMResult &comResult) RT_OVERRIDE RT_FINAL;

private slots:

    /** Handles signal about progress being finished. */
    void sltHandleProgressFinished();

private:

    /** Holds the cloud client being listing source boot volumes. */
    CCloudClient  m_comClient;
    /** Holds the listed names. */
    CStringArray  m_comNames;
    /** Holds the listed ids. */
    CStringArray  m_comIds;
};

/** UINotificationProgress extension for cloud instance list functionality. */
class SHARED_LIBRARY_STUFF UINotificationProgressCloudInstanceList : public UINotificationProgress
{
    Q_OBJECT;

signals:

    /** Notifies listeners about instance @a names received. */
    void sigImageNamesReceived(const QVariant &names);
    /** Notifies listeners about instance @a ids received. */
    void sigImageIdsReceived(const QVariant &ids);

public:

    /** Constructs cloud instances list notification-progress.
      * @param  comClient  Brings the cloud client being listing instances. */
    UINotificationProgressCloudInstanceList(const CCloudClient &comClient);

protected:

    /** Returns object name. */
    virtual QString name() const RT_OVERRIDE RT_FINAL;
    /** Returns object details. */
    virtual QString details() const RT_OVERRIDE RT_FINAL;
    /** Creates and returns started progress-wrapper. */
    virtual CProgress createProgress(COMResult &comResult) RT_OVERRIDE RT_FINAL;

private slots:

    /** Handles signal about progress being finished. */
    void sltHandleProgressFinished();

private:

    /** Holds the cloud client being listing instances. */
    CCloudClient  m_comClient;
    /** Holds the listed names. */
    CStringArray  m_comNames;
    /** Holds the listed ids. */
    CStringArray  m_comIds;
};

/** UINotificationProgress extension for cloud source instance list functionality. */
class SHARED_LIBRARY_STUFF UINotificationProgressCloudSourceInstanceList : public UINotificationProgress
{
    Q_OBJECT;

signals:

    /** Notifies listeners about source instance @a names received. */
    void sigImageNamesReceived(const QVariant &names);
    /** Notifies listeners about source instance @a ids received. */
    void sigImageIdsReceived(const QVariant &ids);

public:

    /** Constructs cloud source instances list notification-progress.
      * @param  comClient  Brings the cloud client being listing source instances. */
    UINotificationProgressCloudSourceInstanceList(const CCloudClient &comClient);

protected:

    /** Returns object name. */
    virtual QString name() const RT_OVERRIDE RT_FINAL;
    /** Returns object details. */
    virtual QString details() const RT_OVERRIDE RT_FINAL;
    /** Creates and returns started progress-wrapper. */
    virtual CProgress createProgress(COMResult &comResult) RT_OVERRIDE RT_FINAL;

private slots:

    /** Handles signal about progress being finished. */
    void sltHandleProgressFinished();

private:

    /** Holds the cloud client being listing source instances. */
    CCloudClient  m_comClient;
    /** Holds the listed names. */
    CStringArray  m_comNames;
    /** Holds the listed ids. */
    CStringArray  m_comIds;
};

/** UINotificationProgress extension for cloud machine add functionality. */
class SHARED_LIBRARY_STUFF UINotificationProgressCloudMachineAdd : public UINotificationProgress
{
    Q_OBJECT;

signals:

    /** Notifies listeners about cloud @a comMachine was added.
      * @param  strProviderShortName  Brings the short provider name.
      * @param  strProfileName        Brings the profile name. */
    void sigCloudMachineAdded(const QString &strProviderShortName,
                              const QString &strProfileName,
                              const CCloudMachine &comMachine);

public:

    /** Constructs cloud machine add notification-progress.
      * @param  comClient             Brings the cloud client being adding machine.
      * @param  comMachine            Brings the cloud machine being added.
      * @param  strInstanceName       Brings the instance name.
      * @param  strProviderShortName  Brings the short provider name.
      * @param  strProfileName        Brings the profile name. */
    UINotificationProgressCloudMachineAdd(const CCloudClient &comClient,
                                          const CCloudMachine &comMachine,
                                          const QString &strInstanceName,
                                          const QString &strProviderShortName,
                                          const QString &strProfileName);

protected:

    /** Returns object name. */
    virtual QString name() const RT_OVERRIDE RT_FINAL;
    /** Returns object details. */
    virtual QString details() const RT_OVERRIDE RT_FINAL;
    /** Creates and returns started progress-wrapper. */
    virtual CProgress createProgress(COMResult &comResult) RT_OVERRIDE RT_FINAL;

private slots:

    /** Handles signal about progress being finished. */
    void sltHandleProgressFinished();

private:

    /** Holds the cloud client being adding machine. */
    CCloudClient   m_comClient;
    /** Holds the cloud machine being added. */
    CCloudMachine  m_comMachine;
    /** Holds the instance name. */
    QString        m_strInstanceName;
    /** Holds the short provider name. */
    QString        m_strProviderShortName;
    /** Holds the profile name. */
    QString        m_strProfileName;
};

/** UINotificationProgress extension for cloud machine create functionality. */
class SHARED_LIBRARY_STUFF UINotificationProgressCloudMachineCreate : public UINotificationProgress
{
    Q_OBJECT;

signals:

    /** Notifies listeners about cloud @a comMachine was created.
      * @param  strProviderShortName  Brings the short provider name.
      * @param  strProfileName        Brings the profile name. */
    void sigCloudMachineCreated(const QString &strProviderShortName,
                                const QString &strProfileName,
                                const CCloudMachine &comMachine);

public:

    /** Constructs cloud machine create notification-progress.
      * @param  comClient             Brings the cloud client being adding machine.
      * @param  comMachine            Brings the cloud machine being added.
      * @param  comVSD                Brings the virtual system description.
      * @param  strProviderShortName  Brings the short provider name.
      * @param  strProfileName        Brings the profile name. */
    UINotificationProgressCloudMachineCreate(const CCloudClient &comClient,
                                             const CCloudMachine &comMachine,
                                             const CVirtualSystemDescription &comVSD,
                                             const QString &strProviderShortName,
                                             const QString &strProfileName);

protected:

    /** Returns object name. */
    virtual QString name() const RT_OVERRIDE RT_FINAL;
    /** Returns object details. */
    virtual QString details() const RT_OVERRIDE RT_FINAL;
    /** Creates and returns started progress-wrapper. */
    virtual CProgress createProgress(COMResult &comResult) RT_OVERRIDE RT_FINAL;

private slots:

    /** Handles signal about progress being finished. */
    void sltHandleProgressFinished();

private:

    /** Holds the cloud client being adding machine. */
    CCloudClient               m_comClient;
    /** Holds the cloud machine being added. */
    CCloudMachine              m_comMachine;
    /** Holds the the virtual system description. */
    CVirtualSystemDescription  m_comVSD;
    /** Holds the cloud machine name. */
    QString                    m_strName;
    /** Holds the short provider name. */
    QString                    m_strProviderShortName;
    /** Holds the profile name. */
    QString                    m_strProfileName;
};

/** UINotificationProgress extension for cloud machine remove functionality. */
class SHARED_LIBRARY_STUFF UINotificationProgressCloudMachineRemove : public UINotificationProgress
{
    Q_OBJECT;

signals:

    /** Notifies listeners about cloud machine was removed.
      * @param  strProviderShortName  Brings the short provider name.
      * @param  strProfileName        Brings the profile name.
      * @param  strName               Brings the machine name. */
    void sigCloudMachineRemoved(const QString &strProviderShortName,
                                const QString &strProfileName,
                                const QString &strName);

public:

    /** Constructs cloud machine remove notification-progress.
      * @param  comMachine    Brings the cloud machine being removed.
      * @param  fFullRemoval  Brings whether cloud machine should be removed fully. */
    UINotificationProgressCloudMachineRemove(const CCloudMachine &comMachine,
                                             bool fFullRemoval,
                                             const QString &strProviderShortName,
                                             const QString &strProfileName);

protected:

    /** Returns object name. */
    virtual QString name() const RT_OVERRIDE RT_FINAL;
    /** Returns object details. */
    virtual QString details() const RT_OVERRIDE RT_FINAL;
    /** Creates and returns started progress-wrapper. */
    virtual CProgress createProgress(COMResult &comResult) RT_OVERRIDE RT_FINAL;

private slots:

    /** Handles signal about progress being finished. */
    void sltHandleProgressFinished();

private:

    /** Holds the cloud machine being removed. */
    CCloudMachine  m_comMachine;
    /** Holds the cloud machine name. */
    QString        m_strName;
    /** Holds whether cloud machine should be removed fully. */
    bool           m_fFullRemoval;
    /** Holds the short provider name. */
    QString        m_strProviderShortName;
    /** Holds the profile name. */
    QString        m_strProfileName;
};

/** UINotificationProgress extension for cloud machine power-up functionality. */
class SHARED_LIBRARY_STUFF UINotificationProgressCloudMachinePowerUp : public UINotificationProgress
{
    Q_OBJECT;

public:

    /** Constructs cloud machine power-up notification-progress.
      * @param  comMachine  Brings the machine being powered-up. */
    UINotificationProgressCloudMachinePowerUp(const CCloudMachine &comMachine);

protected:

    /** Returns object name. */
    virtual QString name() const RT_OVERRIDE RT_FINAL;
    /** Returns object details. */
    virtual QString details() const RT_OVERRIDE RT_FINAL;
    /** Creates and returns started progress-wrapper. */
    virtual CProgress createProgress(COMResult &comResult) RT_OVERRIDE RT_FINAL;

private:

    /** Holds the machine being powered-up. */
    CCloudMachine  m_comMachine;
    /** Holds the machine name. */
    QString        m_strName;
};

/** UINotificationProgress extension for cloud machine clone functionality. */
class SHARED_LIBRARY_STUFF UINotificationProgressCloudMachineClone : public UINotificationProgress
{
    Q_OBJECT;

public:

    /** Constructs cloud machine clone notification-progress.
      * @param  comClient     Brings the cloud client to clone cloud machine.
      * @param  comMachine    Brings the cloud machine to be cloned.
      * @param  strCloneName  Brings the clone name. */
    UINotificationProgressCloudMachineClone(const CCloudClient &comClient,
                                            const CCloudMachine &comMachine,
                                            const QString &strCloneName);

protected:

    /** Returns object name. */
    virtual QString name() const RT_OVERRIDE RT_FINAL;
    /** Returns object details. */
    virtual QString details() const RT_OVERRIDE RT_FINAL;
    /** Creates and returns started progress-wrapper. */
    virtual CProgress createProgress(COMResult &comResult) RT_OVERRIDE RT_FINAL;

private:

    /** Holds the client cloning machine. */
    CCloudClient   m_comClient;
    /** Holds the machine being reset. */
    CCloudMachine  m_comMachine;
    /** Holds the clone name. */
    QString        m_strCloneName;
    /** Holds the machine internal id. */
    QString        m_strId;
    /** Holds the machine name. */
    QString        m_strName;
};

/** UINotificationProgress extension for cloud machine reset functionality. */
class SHARED_LIBRARY_STUFF UINotificationProgressCloudMachineReset : public UINotificationProgress
{
    Q_OBJECT;

public:

    /** Constructs cloud machine reset notification-progress.
      * @param  comMachine  Brings the machine being reset. */
    UINotificationProgressCloudMachineReset(const CCloudMachine &comMachine);

protected:

    /** Returns object name. */
    virtual QString name() const RT_OVERRIDE RT_FINAL;
    /** Returns object details. */
    virtual QString details() const RT_OVERRIDE RT_FINAL;
    /** Creates and returns started progress-wrapper. */
    virtual CProgress createProgress(COMResult &comResult) RT_OVERRIDE RT_FINAL;

private:

    /** Holds the machine being reset. */
    CCloudMachine  m_comMachine;
    /** Holds the machine name. */
    QString        m_strName;
};

/** UINotificationProgress extension for cloud machine power-off functionality. */
class SHARED_LIBRARY_STUFF UINotificationProgressCloudMachinePowerOff : public UINotificationProgress
{
    Q_OBJECT;

public:

    /** Constructs cloud machine power-off notification-progress.
      * @param  comMachine  Brings the machine being powered-off. */
    UINotificationProgressCloudMachinePowerOff(const CCloudMachine &comMachine);

protected:

    /** Returns object name. */
    virtual QString name() const RT_OVERRIDE RT_FINAL;
    /** Returns object details. */
    virtual QString details() const RT_OVERRIDE RT_FINAL;
    /** Creates and returns started progress-wrapper. */
    virtual CProgress createProgress(COMResult &comResult) RT_OVERRIDE RT_FINAL;

private:

    /** Holds the machine being powered-off. */
    CCloudMachine  m_comMachine;
    /** Holds the machine name. */
    QString        m_strName;
};

/** UINotificationProgress extension for cloud machine shutdown functionality. */
class SHARED_LIBRARY_STUFF UINotificationProgressCloudMachineShutdown : public UINotificationProgress
{
    Q_OBJECT;

public:

    /** Constructs cloud machine shutdown notification-progress.
      * @param  comMachine  Brings the machine being shutdown. */
    UINotificationProgressCloudMachineShutdown(const CCloudMachine &comMachine);

protected:

    /** Returns object name. */
    virtual QString name() const RT_OVERRIDE RT_FINAL;
    /** Returns object details. */
    virtual QString details() const RT_OVERRIDE RT_FINAL;
    /** Creates and returns started progress-wrapper. */
    virtual CProgress createProgress(COMResult &comResult) RT_OVERRIDE RT_FINAL;

private:

    /** Holds the machine being shutdown. */
    CCloudMachine  m_comMachine;
    /** Holds the machine name. */
    QString        m_strName;
};

/** UINotificationProgress extension for cloud machine terminate functionality. */
class SHARED_LIBRARY_STUFF UINotificationProgressCloudMachineTerminate : public UINotificationProgress
{
    Q_OBJECT;

public:

    /** Constructs cloud machine terminate notification-progress.
      * @param  comMachine  Brings the machine being terminate. */
    UINotificationProgressCloudMachineTerminate(const CCloudMachine &comMachine);

protected:

    /** Returns object name. */
    virtual QString name() const RT_OVERRIDE RT_FINAL;
    /** Returns object details. */
    virtual QString details() const RT_OVERRIDE RT_FINAL;
    /** Creates and returns started progress-wrapper. */
    virtual CProgress createProgress(COMResult &comResult) RT_OVERRIDE RT_FINAL;

private:

    /** Holds the machine being terminated. */
    CCloudMachine  m_comMachine;
    /** Holds the machine name. */
    QString        m_strName;
};

/** UINotificationProgress extension for cloud machine settings form create functionality. */
class SHARED_LIBRARY_STUFF UINotificationProgressCloudMachineSettingsFormCreate : public UINotificationProgress
{
    Q_OBJECT;

signals:

    /** Notifies listeners about settings @a comForm created.
      * @param  form  Brings created VSD form. */
    void sigSettingsFormCreated(const QVariant &form);

public:

    /** Constructs cloud machine settings form create notification-progress.
      * @param  comMachine      Brings the machine form being created for.
      * @param  strMachineName  Brings the machine name. */
    UINotificationProgressCloudMachineSettingsFormCreate(const CCloudMachine &comMachine,
                                                         const QString &strMachineName);

protected:

    /** Returns object name. */
    virtual QString name() const RT_OVERRIDE RT_FINAL;
    /** Returns object details. */
    virtual QString details() const RT_OVERRIDE RT_FINAL;
    /** Creates and returns started progress-wrapper. */
    virtual CProgress createProgress(COMResult &comResult) RT_OVERRIDE RT_FINAL;

private slots:

    /** Handles signal about progress being finished. */
    void sltHandleProgressFinished();

private:

    /** Holds the machine form being created for. */
    CCloudMachine  m_comMachine;
    /** Holds the machine name. */
    QString        m_strMachineName;
    /** Holds the form being created. */
    CForm          m_comForm;
};

/** UINotificationProgress extension for cloud machine settings form apply functionality. */
class SHARED_LIBRARY_STUFF UINotificationProgressCloudMachineSettingsFormApply : public UINotificationProgress
{
    Q_OBJECT;

public:

    /** Constructs cloud machine settings form apply notification-progress.
      * @param  comForm         Brings the form being applied.
      * @param  strMachineName  Brings the machine name. */
    UINotificationProgressCloudMachineSettingsFormApply(const CForm &comForm,
                                                        const QString &strMachineName);

protected:

    /** Returns object name. */
    virtual QString name() const RT_OVERRIDE RT_FINAL;
    /** Returns object details. */
    virtual QString details() const RT_OVERRIDE RT_FINAL;
    /** Creates and returns started progress-wrapper. */
    virtual CProgress createProgress(COMResult &comResult) RT_OVERRIDE RT_FINAL;

private:

    /** Holds the machine form being created for. */
    CForm    m_comForm;
    /** Holds the machine name. */
    QString  m_strMachineName;
};

/** UINotificationProgress extension for cloud console connection create functionality. */
class SHARED_LIBRARY_STUFF UINotificationProgressCloudConsoleConnectionCreate : public UINotificationProgress
{
    Q_OBJECT;

public:

    /** Constructs cloud console connection create notification-progress.
      * @param  comMachine    Brings the cloud machine for which console connection being created.
      * @param  strPublicKey  Brings the public key used for console connection being created. */
    UINotificationProgressCloudConsoleConnectionCreate(const CCloudMachine &comMachine,
                                                       const QString &strPublicKey);

protected:

    /** Returns object name. */
    virtual QString name() const RT_OVERRIDE RT_FINAL;
    /** Returns object details. */
    virtual QString details() const RT_OVERRIDE RT_FINAL;
    /** Creates and returns started progress-wrapper. */
    virtual CProgress createProgress(COMResult &comResult) RT_OVERRIDE RT_FINAL;

private:

    /** Holds the cloud machine for which console connection being created. */
    CCloudMachine  m_comMachine;
    /** Holds the cloud machine name. */
    QString        m_strName;
    /** Holds the public key used for console connection being created. */
    QString        m_strPublicKey;
};

/** UINotificationProgress extension for cloud console connection delete functionality. */
class SHARED_LIBRARY_STUFF UINotificationProgressCloudConsoleConnectionDelete : public UINotificationProgress
{
    Q_OBJECT;

public:

    /** Constructs cloud console connection delete notification-progress.
      * @param  comMachine  Brings the cloud machine for which console connection being deleted. */
    UINotificationProgressCloudConsoleConnectionDelete(const CCloudMachine &comMachine);

protected:

    /** Returns object name. */
    virtual QString name() const RT_OVERRIDE RT_FINAL;
    /** Returns object details. */
    virtual QString details() const RT_OVERRIDE RT_FINAL;
    /** Creates and returns started progress-wrapper. */
    virtual CProgress createProgress(COMResult &comResult) RT_OVERRIDE RT_FINAL;

private:

    /** Holds the cloud machine for which console connection being deleted. */
    CCloudMachine  m_comMachine;
    /** Holds the cloud machine name. */
    QString        m_strName;
};

/** UINotificationProgress extension for cloud console log acquire functionality. */
class SHARED_LIBRARY_STUFF UINotificationProgressCloudConsoleLogAcquire : public UINotificationProgress
{
    Q_OBJECT;

signals:

    /** Notifies listeners about console @a strLog for cloud VM with @a strName read. */
    void sigLogRead(const QString &strName, const QString &strLog);

public:

    /** Constructs cloud console log acquire notification-progress.
      * @param  comMachine  Brings the cloud machine for which console log being acquired. */
    UINotificationProgressCloudConsoleLogAcquire(const CCloudMachine &comMachine);

protected:

    /** Returns object name. */
    virtual QString name() const RT_OVERRIDE RT_FINAL;
    /** Returns object details. */
    virtual QString details() const RT_OVERRIDE RT_FINAL;
    /** Creates and returns started progress-wrapper. */
    virtual CProgress createProgress(COMResult &comResult) RT_OVERRIDE RT_FINAL;

private slots:

    /** Handles signal about progress being finished. */
    void sltHandleProgressFinished();

private:

    /** Holds the cloud machine for which console log being acquired. */
    CCloudMachine  m_comMachine;
    /** Holds the cloud machine name. */
    QString        m_strName;
    /** Holds the stream log being read to. */
    CDataStream    m_comStream;
};

/** UINotificationProgress extension for snapshot take functionality. */
class SHARED_LIBRARY_STUFF UINotificationProgressSnapshotTake : public UINotificationProgress
{
    Q_OBJECT;

signals:

    /** Notifies listeners about snapshot with @a id taken. */
    void sigSnapshotTaken(const QVariant &id);

public:

    /** Constructs snapshot take notification-progress.
      * @param  comMachine              Brings the machine we are taking snapshot for.
      * @param  strSnapshotName         Brings the name of snapshot being taken.
      * @param  strSnapshotDescription  Brings the description of snapshot being taken. */
    UINotificationProgressSnapshotTake(const CMachine &comMachine,
                                       const QString &strSnapshotName,
                                       const QString &strSnapshotDescription);

protected:

    /** Returns object name. */
    virtual QString name() const RT_OVERRIDE RT_FINAL;
    /** Returns object details. */
    virtual QString details() const RT_OVERRIDE RT_FINAL;
    /** Creates and returns started progress-wrapper. */
    virtual CProgress createProgress(COMResult &comResult) RT_OVERRIDE RT_FINAL;

private slots:

    /** Handles signal about progress being finished. */
    void sltHandleProgressFinished();

private:

    /** Holds the machine we are taking snapshot for. */
    CMachine  m_comMachine;
    /** Holds the name of snapshot being taken. */
    QString   m_strSnapshotName;
    /** Holds the description of snapshot being taken. */
    QString   m_strSnapshotDescription;
    /** Holds the machine name. */
    QString   m_strMachineName;
    /** Holds the session being opened. */
    CSession  m_comSession;
    /** Holds the taken snapshot id. */
    QUuid     m_uSnapshotId;
};

/** UINotificationProgress extension for snapshot restore functionality. */
class SHARED_LIBRARY_STUFF UINotificationProgressSnapshotRestore : public UINotificationProgress
{
    Q_OBJECT;

signals:

    /** Notifies listeners about snapshot restored.
      * @param  fSuccess  Brings whether snapshot restored successfully. */
    void sigSnapshotRestored(bool fSuccess);

public:

    /** Constructs snapshot restore notification-progress.
      * @param  uMachineId   Brings the ID of machine we are restoring snapshot for.
      * @param  comSnapshot  Brings the snapshot being restored. */
    UINotificationProgressSnapshotRestore(const QUuid &uMachineId,
                                          const CSnapshot &comSnapshot = CSnapshot());
    /** Constructs snapshot restore notification-progress.
      * @param  comMachine   Brings the machine we are restoring snapshot for.
      * @param  comSnapshot  Brings the snapshot being restored. */
    UINotificationProgressSnapshotRestore(const CMachine &comMachine,
                                          const CSnapshot &comSnapshot = CSnapshot());

protected:

    /** Returns object name. */
    virtual QString name() const RT_OVERRIDE RT_FINAL;
    /** Returns object details. */
    virtual QString details() const RT_OVERRIDE RT_FINAL;
    /** Creates and returns started progress-wrapper. */
    virtual CProgress createProgress(COMResult &comResult) RT_OVERRIDE RT_FINAL;

private slots:

    /** Handles signal about progress being finished. */
    void sltHandleProgressFinished();

private:

    /** Holds the ID of machine we are restoring snapshot for. */
    QUuid      m_uMachineId;
    /** Holds the machine we are restoring snapshot for. */
    CMachine   m_comMachine;
    /** Holds the snapshot being restored. */
    CSnapshot  m_comSnapshot;
    /** Holds the machine name. */
    QString    m_strMachineName;
    /** Holds the snapshot name. */
    QString    m_strSnapshotName;
    /** Holds the session being opened. */
    CSession   m_comSession;
};

/** UINotificationProgress extension for snapshot delete functionality. */
class SHARED_LIBRARY_STUFF UINotificationProgressSnapshotDelete : public UINotificationProgress
{
    Q_OBJECT;

public:

    /** Constructs snapshot delete notification-progress.
      * @param  comMachine   Brings the machine we are deleting snapshot from.
      * @param  uSnapshotId  Brings the ID of snapshot being deleted. */
    UINotificationProgressSnapshotDelete(const CMachine &comMachine,
                                         const QUuid &uSnapshotId);

protected:

    /** Returns object name. */
    virtual QString name() const RT_OVERRIDE RT_FINAL;
    /** Returns object details. */
    virtual QString details() const RT_OVERRIDE RT_FINAL;
    /** Creates and returns started progress-wrapper. */
    virtual CProgress createProgress(COMResult &comResult) RT_OVERRIDE RT_FINAL;

private slots:

    /** Handles signal about progress being finished. */
    void sltHandleProgressFinished();

private:

    /** Holds the machine we are deleting snapshot from. */
    CMachine  m_comMachine;
    /** Holds the ID of snapshot being deleted. */
    QUuid     m_uSnapshotId;
    /** Holds the machine name. */
    QString   m_strMachineName;
    /** Holds the snapshot name. */
    QString   m_strSnapshotName;
    /** Holds the session being opened. */
    CSession  m_comSession;
};

/** UINotificationProgress extension for appliance write functionality. */
class SHARED_LIBRARY_STUFF UINotificationProgressApplianceWrite : public UINotificationProgress
{
    Q_OBJECT;

public:

    /** Constructs appliance write notification-progress.
      * @param  comAppliance  Brings the appliance being written.
      * @param  strFormat     Brings the appliance format.
      * @param  options       Brings the export options to be taken into account.
      * @param  strPath       Brings the appliance path. */
    UINotificationProgressApplianceWrite(const CAppliance &comAppliance,
                                         const QString &strFormat,
                                         const QVector<KExportOptions> &options,
                                         const QString &strPath);

protected:

    /** Returns object name. */
    virtual QString name() const RT_OVERRIDE RT_FINAL;
    /** Returns object details. */
    virtual QString details() const RT_OVERRIDE RT_FINAL;
    /** Creates and returns started progress-wrapper. */
    virtual CProgress createProgress(COMResult &comResult) RT_OVERRIDE RT_FINAL;

private:

    /** Holds the appliance being written. */
    CAppliance               m_comAppliance;
    /** Holds the appliance format. */
    QString                  m_strFormat;
    /** Holds the export options to be taken into account. */
    QVector<KExportOptions>  m_options;
    /** Holds the appliance path. */
    QString                  m_strPath;
};

/** UINotificationProgress extension for appliance read functionality. */
class SHARED_LIBRARY_STUFF UINotificationProgressApplianceRead : public UINotificationProgress
{
    Q_OBJECT;

public:

    /** Constructs appliance read notification-progress.
      * @param  comAppliance  Brings the appliance being read.
      * @param  strPath       Brings the appliance path. */
    UINotificationProgressApplianceRead(const CAppliance &comAppliance,
                                        const QString &strPath);

protected:

    /** Returns object name. */
    virtual QString name() const RT_OVERRIDE RT_FINAL;
    /** Returns object details. */
    virtual QString details() const RT_OVERRIDE RT_FINAL;
    /** Creates and returns started progress-wrapper. */
    virtual CProgress createProgress(COMResult &comResult) RT_OVERRIDE RT_FINAL;

private:

    /** Holds the appliance being read. */
    CAppliance  m_comAppliance;
    /** Holds the appliance path. */
    QString     m_strPath;
};

/** UINotificationProgress extension for import appliance functionality. */
class SHARED_LIBRARY_STUFF UINotificationProgressApplianceImport : public UINotificationProgress
{
    Q_OBJECT;

public:

    /** Constructs import appliance notification-progress.
      * @param  comAppliance  Brings the appliance being imported.
      * @param  options       Brings the import options to be taken into account. */
    UINotificationProgressApplianceImport(const CAppliance &comAppliance,
                                          const QVector<KImportOptions> &options);

protected:

    /** Returns object name. */
    virtual QString name() const RT_OVERRIDE RT_FINAL;
    /** Returns object details. */
    virtual QString details() const RT_OVERRIDE RT_FINAL;
    /** Creates and returns started progress-wrapper. */
    virtual CProgress createProgress(COMResult &comResult) RT_OVERRIDE RT_FINAL;

private:

    /** Holds the appliance being imported. */
    CAppliance               m_comAppliance;
    /** Holds the import options to be taken into account. */
    QVector<KImportOptions>  m_options;
};

/** UINotificationProgress extension for extension pack install functionality. */
class SHARED_LIBRARY_STUFF UINotificationProgressExtensionPackInstall : public UINotificationProgress
{
    Q_OBJECT;

public:

    /** Constructs extension pack install notification-progress.
      * @param  comExtPackFile        Brings the extension pack file to install.
      * @param  fReplace              Brings whether extension pack should be replaced.
      * @param  strExtensionPackName  Brings the extension pack name.
      * @param  strDisplayInfo        Brings the display info. */
    UINotificationProgressExtensionPackInstall(const CExtPackFile &comExtPackFile,
                                               bool fReplace,
                                               const QString &strExtensionPackName,
                                               const QString &strDisplayInfo);

protected:

    /** Returns object name. */
    virtual QString name() const RT_OVERRIDE RT_FINAL;
    /** Returns object details. */
    virtual QString details() const RT_OVERRIDE RT_FINAL;
    /** Creates and returns started progress-wrapper. */
    virtual CProgress createProgress(COMResult &comResult) RT_OVERRIDE RT_FINAL;

private:

    /** Holds the extension pack file to install. */
    CExtPackFile     m_comExtPackFile;
    /** Holds whether extension pack should be replaced. */
    bool             m_fReplace;
    /** Holds the extension pack name. */
    QString          m_strExtensionPackName;
    /** Holds the display info. */
    QString          m_strDisplayInfo;
};

/** UINotificationProgress extension for extension pack uninstall functionality. */
class SHARED_LIBRARY_STUFF UINotificationProgressExtensionPackUninstall : public UINotificationProgress
{
    Q_OBJECT;

public:

    /** Constructs extension pack uninstall notification-progress.
      * @param  comExtPackManager     Brings the extension pack manager.
      * @param  strExtensionPackName  Brings the extension pack name.
      * @param  strDisplayInfo        Brings the display info. */
    UINotificationProgressExtensionPackUninstall(const CExtPackManager &comExtPackManager,
                                                 const QString &strExtensionPackName,
                                                 const QString &strDisplayInfo);

protected:

    /** Returns object name. */
    virtual QString name() const RT_OVERRIDE RT_FINAL;
    /** Returns object details. */
    virtual QString details() const RT_OVERRIDE RT_FINAL;
    /** Creates and returns started progress-wrapper. */
    virtual CProgress createProgress(COMResult &comResult) RT_OVERRIDE RT_FINAL;

private:

    /** Holds the extension pack manager. */
    CExtPackManager  m_comExtPackManager;
    /** Holds the extension pack name. */
    QString          m_strExtensionPackName;
    /** Holds the display info. */
    QString          m_strDisplayInfo;
};

/** UINotificationProgress extension for guest additions install functionality. */
class SHARED_LIBRARY_STUFF UINotificationProgressGuestAdditionsInstall : public UINotificationProgress
{
    Q_OBJECT;

signals:

    /** Notifies listeners about guest additions installation failed.
      * @param  strSource  Brings the guest additions file path. */
    void sigGuestAdditionsInstallationFailed(const QString &strSource);

public:

    /** Constructs guest additions install notification-progress.
      * @param  comGuest   Brings the guest additions being installed to.
      * @param  strSource  Brings the guest additions file path. */
    UINotificationProgressGuestAdditionsInstall(const CGuest &comGuest,
                                                const QString &strSource);

protected:

    /** Returns object name. */
    virtual QString name() const RT_OVERRIDE RT_FINAL;
    /** Returns object details. */
    virtual QString details() const RT_OVERRIDE RT_FINAL;
    /** Creates and returns started progress-wrapper. */
    virtual CProgress createProgress(COMResult &comResult) RT_OVERRIDE RT_FINAL;

private slots:

    /** Handles signal about progress being finished. */
    void sltHandleProgressFinished();

private:

    /** Holds the guest additions being installed to. */
    CGuest   m_comGuest;
    /** Holds the guest additions file path. */
    QString  m_strSource;
};

/** UINotificationProgress extension for host-only network interface create functionality. */
class SHARED_LIBRARY_STUFF UINotificationProgressHostOnlyNetworkInterfaceCreate : public UINotificationProgress
{
    Q_OBJECT;

signals:

    /** Notifies listeners about host-only network interface created.
      * @param  comInterface  Brings network interface created. */
    void sigHostOnlyNetworkInterfaceCreated(const CHostNetworkInterface &comInterface);

public:

    /** Constructs host-only network interface create notification-progress.
      * @param  comHost       Brings the host network interface being created for.
      * @param  comInterface  Brings the network interface being created. */
    UINotificationProgressHostOnlyNetworkInterfaceCreate(const CHost &comHost,
                                                         const CHostNetworkInterface &comInterface);

protected:

    /** Returns object name. */
    virtual QString name() const RT_OVERRIDE RT_FINAL;
    /** Returns object details. */
    virtual QString details() const RT_OVERRIDE RT_FINAL;
    /** Creates and returns started progress-wrapper. */
    virtual CProgress createProgress(COMResult &comResult) RT_OVERRIDE RT_FINAL;

private slots:

    /** Handles signal about progress being finished. */
    void sltHandleProgressFinished();

private:

    /** Holds the host network interface being created for. */
    CHost                  m_comHost;
    /** Holds the network interface being created. */
    CHostNetworkInterface  m_comInterface;
};

/** UINotificationProgress extension for host-only network interface remove functionality. */
class SHARED_LIBRARY_STUFF UINotificationProgressHostOnlyNetworkInterfaceRemove : public UINotificationProgress
{
    Q_OBJECT;

signals:

    /** Notifies listeners about host-only network interface removed.
      * @param  strInterfaceName  Brings the name of network interface removed. */
    void sigHostOnlyNetworkInterfaceRemoved(const QString &strInterfaceName);

public:

    /** Constructs host-only network interface remove notification-progress.
      * @param  comHost       Brings the host network interface being removed for.
      * @param  uInterfaceId  Brings the ID of network interface being removed. */
    UINotificationProgressHostOnlyNetworkInterfaceRemove(const CHost &comHost,
                                                         const QUuid &uInterfaceId);

protected:

    /** Returns object name. */
    virtual QString name() const RT_OVERRIDE RT_FINAL;
    /** Returns object details. */
    virtual QString details() const RT_OVERRIDE RT_FINAL;
    /** Creates and returns started progress-wrapper. */
    virtual CProgress createProgress(COMResult &comResult) RT_OVERRIDE RT_FINAL;

private slots:

    /** Handles signal about progress being finished. */
    void sltHandleProgressFinished();

private:

    /** Holds the host network interface being removed for. */
    CHost    m_comHost;
    /** Holds the ID of network interface being removed. */
    QUuid    m_uInterfaceId;
    /** Holds the network interface name. */
    QString  m_strInterfaceName;
};

/** UINotificationProgress extension for virtual system description form value set functionality. */
class SHARED_LIBRARY_STUFF UINotificationProgressVsdFormValueSet : public UINotificationProgress
{
    Q_OBJECT;

public:

    /** Constructs virtual system description form value set notification-progress.
      * @param  comValue  Brings our value being set.
      * @param  fBool     Brings the value our value being set to. */
    UINotificationProgressVsdFormValueSet(const CBooleanFormValue &comValue, bool fBool);

    /** Constructs virtual system description form value set notification-progress.
      * @param  comValue   Brings our value being set.
      * @param  strString  Brings the value our value being set to. */
    UINotificationProgressVsdFormValueSet(const CStringFormValue &comValue, const QString &strString);

    /** Constructs virtual system description form value set notification-progress.
      * @param  comValue  Brings our value being set.
      * @param  iChoice   Brings the value our value being set to. */
    UINotificationProgressVsdFormValueSet(const CChoiceFormValue &comValue, int iChoice);

    /** Constructs virtual system description form value set notification-progress.
      * @param  comValue  Brings our value being set.
      * @param  iInteger  Brings the value our value being set to. */
    UINotificationProgressVsdFormValueSet(const CRangedIntegerFormValue &comValue, int iInteger);

    /** Constructs virtual system description form value set notification-progress.
      * @param  comValue    Brings our value being set.
      * @param  iInteger64  Brings the value our value being set to. */
    UINotificationProgressVsdFormValueSet(const CRangedInteger64FormValue &comValue, qlonglong iInteger64);

protected:

    /** Returns object name. */
    virtual QString name() const RT_OVERRIDE RT_FINAL;
    /** Returns object details. */
    virtual QString details() const RT_OVERRIDE RT_FINAL;
    /** Creates and returns started progress-wrapper. */
    virtual CProgress createProgress(COMResult &comResult) RT_OVERRIDE RT_FINAL;

private:

    /** Value type. */
    KFormValueType  m_enmType;

    /** Holds our value being set. */
    CFormValue  m_comValue;

    /** Holds the bool value. */
    bool       m_fBool;
    /** Holds the string value. */
    QString    m_strString;
    /** Holds the choice value. */
    int        m_iChoice;
    /** Holds the integer value. */
    int        m_iInteger;
    /** Holds the integer64 value. */
    qlonglong  m_iInteger64;
};

#ifdef VBOX_GUI_WITH_NETWORK_MANAGER
/** UINotificationDownloader extension for extension pack downloading functionality. */
class SHARED_LIBRARY_STUFF UINotificationDownloaderExtensionPack : public UINotificationDownloader
{
    Q_OBJECT;

signals:

    /** Notifies listeners about extension pack downloaded.
      * @param  strSource  Brings the EP source.
      * @param  strTarget  Brings the EP target.
      * @param  strDigest  Brings the EP digest. */
    void sigExtensionPackDownloaded(const QString &strSource,
                                    const QString &strTarget,
                                    const QString &strDigest);

public:

    /** Returns singleton instance, creates if necessary.
      * @param  strPackName  Brings the package name. */
    static UINotificationDownloaderExtensionPack *instance(const QString &strPackName);
    /** Returns whether singleton instance already created. */
    static bool exists();

    /** Destructs extension pack downloading notification-downloader.
      * @note  Notification-center can destroy us at any time. */
    virtual ~UINotificationDownloaderExtensionPack() RT_OVERRIDE RT_FINAL;

protected:

    /** Constructs extension pack downloading notification-downloader.
      * @param  strPackName  Brings the package name. */
    UINotificationDownloaderExtensionPack(const QString &strPackName);

    /** Returns object name. */
    virtual QString name() const RT_OVERRIDE RT_FINAL;
    /** Returns object details. */
    virtual QString details() const RT_OVERRIDE RT_FINAL;
    /** Creates and returns started downloader. */
    virtual UIDownloader *createDownloader() RT_OVERRIDE RT_FINAL;

private:

    /** Holds the singleton instance. */
    static UINotificationDownloaderExtensionPack *s_pInstance;

    /** Holds the name of pack being dowloaded. */
    QString  m_strPackName;
};

/** UINotificationDownloader extension for guest additions downloading functionality. */
class SHARED_LIBRARY_STUFF UINotificationDownloaderGuestAdditions : public UINotificationDownloader
{
    Q_OBJECT;

signals:

    /** Notifies listeners about guest additions downloaded.
      * @param  strLocation  Brings the UM location. */
    void sigGuestAdditionsDownloaded(const QString &strLocation);

public:

    /** Returns singleton instance, creates if necessary.
      * @param  strFileName  Brings the file name. */
    static UINotificationDownloaderGuestAdditions *instance(const QString &strFileName);
    /** Returns whether singleton instance already created. */
    static bool exists();

    /** Destructs guest additions downloading notification-downloader.
      * @note  Notification-center can destroy us at any time. */
    virtual ~UINotificationDownloaderGuestAdditions() RT_OVERRIDE RT_FINAL;

protected:

    /** Constructs guest additions downloading notification-downloader.
      * @param  strFileName  Brings the file name. */
    UINotificationDownloaderGuestAdditions(const QString &strFileName);

    /** Returns object name. */
    virtual QString name() const RT_OVERRIDE RT_FINAL;
    /** Returns object details. */
    virtual QString details() const RT_OVERRIDE RT_FINAL;
    /** Creates and returns started downloader. */
    virtual UIDownloader *createDownloader() RT_OVERRIDE RT_FINAL;

private:

    /** Holds the singleton instance. */
    static UINotificationDownloaderGuestAdditions *s_pInstance;

    /** Holds the name of file being dowloaded. */
    QString  m_strFileName;
};

/** UINotificationProgress extension for checking a new VirtualBox version. */
class SHARED_LIBRARY_STUFF UINotificationProgressNewVersionChecker : public UINotificationProgress
{
    Q_OBJECT;

public:

    /** Constructs new version check notification-progress.
      * @param  fForcedCall  Brings whether even negative result should be reflected. */
    UINotificationProgressNewVersionChecker(bool fForcedCall);

protected:

    /** Returns object name. */
    virtual QString name() const RT_OVERRIDE RT_FINAL;
    /** Returns object details. */
    virtual QString details() const RT_OVERRIDE RT_FINAL;
    /** Creates and returns started progress-wrapper. */
    virtual CProgress createProgress(COMResult &comResult) RT_OVERRIDE RT_FINAL;

private slots:

    /** Handles signal about progress being finished. */
    void sltHandleProgressFinished();

private:

    /** Holds whether this customer has forced privelegies. */
    bool          m_fForcedCall;
# ifdef VBOX_WITH_UPDATE_AGENT
    /** Holds the host update agent reference. */
    CUpdateAgent  m_comUpdateHost;
# endif
};
#endif /* VBOX_GUI_WITH_NETWORK_MANAGER */

#endif /* !FEQT_INCLUDED_SRC_notificationcenter_UINotificationObjects_h */
