/* $Id: UINotificationDefs.h 113510 2026-03-23 14:57:39Z sergey.dubov@oracle.com $ */
/** @file
 * VBox Qt GUI - UINotificationCenter related definitions.
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

#ifndef FEQT_INCLUDED_SRC_notificationcenter_UINotificationDefs_h
#define FEQT_INCLUDED_SRC_notificationcenter_UINotificationDefs_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/** Possible notification types. */
enum NotificationType
{
    NotificationType_Unknown = 0,
    NotificationType_Info,
    NotificationType_Question,
    NotificationType_Warning,
    NotificationType_Critical,
    NotificationType_GuruMeditation
};

#endif /* !FEQT_INCLUDED_SRC_notificationcenter_UINotificationDefs_h */
