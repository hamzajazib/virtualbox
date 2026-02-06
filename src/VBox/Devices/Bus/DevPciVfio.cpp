/* $Id: DevPciVfio.cpp 112861 2026-02-06 21:17:47Z alexander.eichner@oracle.com $ */
/** @file
 * PCI passthrough device emulation using VFIO/IOMMUFD.
 */

/*
 * Copyright (C) 2026 Oracle and/or its affiliates.
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


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#define LOG_GROUP LOG_GROUP_DEV_PCI_RAW
#define PDMPCIDEV_INCLUDE_PRIVATE  /* Hack to get pdmpcidevint.h included at the right point. */
#include <VBox/pci.h>
#include <VBox/log.h>
#include <VBox/msi.h>
#include <VBox/vmm/pdmdev.h>
#include <VBox/vmm/stam.h>
#include <VBox/vmm/pdmpci.h>
#include <VBox/vmm/pdmpcidev.h>
#include <iprt/assert.h>
#include <iprt/string.h>
#include <iprt/errcore.h>

#include <linux/vfio.h>
#include <linux/iommufd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>

#include "VBoxDD.h"


/*********************************************************************************************************************************
*   Defined Constants And Macros                                                                                                 *
*********************************************************************************************************************************/

/** eventfd2() syscall for the interrupt handling. */
#define LNX_SYSCALL_EVENTFD2          290


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/

typedef struct VFIOPCIBAR
{
    /** Region type, 0 - disabled, 1 - PIO, 2 - MMIO. */
    uint8_t             bType;
    /** Type dependent data. */
    union
    {
        /** Start offset of the PIO region. */
        uint64_t        offPio;
        /** Start of the MMIO mapping. */
        volatile void   *pvMmio;
    } u;
    /** Type dependent handle. */
    union
    {
        /** I/O port region. */
        IOMIOPORTHANDLE hIoPort;
        /** MMIO region. */
        IOMMMIOHANDLE   hMmio;
    } hnd;
} VFIOPCIBAR;
typedef VFIOPCIBAR *PVFIOPCIBAR;
typedef const VFIOPCIBAR *PCVFIOPCIBAR;

typedef struct VFIOPCI
{
    /** Pointer to the device instance. */
    PPDMDEVINSR3         pDevIns;
    /** The device instance. */
    int                  iInstance;

    /** The IOMMU file descriptor. */
    int                  iFdIommu;
    /** The vfio cdev file descriptor .*/
    int                  iFdVfio;

    /** The IOMMU page table object id. */
    uint32_t             idIommuHwpt;

    /** The start offset of the PCI config space. */
    uint64_t             offPciCfg;
    /** Size of the PCI config space. */
    size_t               cbPciCfg;
    /** The PCI BAR information. */
    VFIOPCIBAR           aBars[VBOX_PCI_NUM_REGIONS];

    /** The poll structure for the interrupts. */
    struct pollfd        aIrqFds[2];

    PPDMTHREAD           pThrdIrq;
} VFIOPCI;
/** Pointer to the raw PCI instance data. */
typedef VFIOPCI *PVFIOPCI;


#ifndef VBOX_DEVICE_STRUCT_TESTCASE

/**
 * eventfd2() syscall wrapper.
 *
 * @returns IPRT status code.
 * @param   uValInit            The initial value of the maintained counter.
 * @param   fFlags              Flags controlling the eventfd behavior.
 * @param   piFdEvt             Where to store the file descriptor of the eventfd object on success.
 */
DECLINLINE(int) pciVfioLnxEventfd2(uint32_t uValInit, uint32_t fFlags, int *piFdEvt)
{
    int rcLnx = syscall(LNX_SYSCALL_EVENTFD2, uValInit, fFlags);
    if (RT_UNLIKELY(rcLnx == -1))
        return RTErrConvertFromErrno(errno);

    *piFdEvt = rcLnx;
    return VINF_SUCCESS;
}


DECLINLINE(int) pciVfioCfgSpaceReadU8(PVFIOPCI pThis, uint32_t offReg, uint8_t *pb)
{
    ssize_t cb = pread(pThis->iFdVfio, pb, 1, pThis->offPciCfg + offReg);
    if (cb != 1)
        return RTErrConvertFromErrno(errno);

    return VINF_SUCCESS;
}


DECLINLINE(int) pciVfioCfgSpaceReadU16(PVFIOPCI pThis, uint32_t offReg, uint16_t *pu16)
{
    ssize_t cb = pread(pThis->iFdVfio, pu16, 2, pThis->offPciCfg + offReg);
    if (cb != 2)
        return RTErrConvertFromErrno(errno);

    return VINF_SUCCESS;
}


DECLINLINE(int) pciVfioCfgSpaceReadU32(PVFIOPCI pThis, uint32_t offReg, uint32_t *pu32)
{
    ssize_t cb = pread(pThis->iFdVfio, pu32, 4, pThis->offPciCfg + offReg);
    if (cb != 4)
        return RTErrConvertFromErrno(errno);

    return VINF_SUCCESS;
}


DECLINLINE(int) pciVfioCfgSpaceReadU64(PVFIOPCI pThis, uint32_t offReg, uint64_t *pu64)
{
    ssize_t cb = pread(pThis->iFdVfio, pu64, 8, pThis->offPciCfg + offReg);
    if (cb != 8)
        return RTErrConvertFromErrno(errno);

    return VINF_SUCCESS;
}


DECLINLINE(int) pciVfioCfgSpaceWriteU8(PVFIOPCI pThis, uint32_t offReg, uint8_t u8)
{
    ssize_t cb = pwrite(pThis->iFdVfio, &u8, 1, pThis->offPciCfg + offReg);
    if (cb != 1)
        return RTErrConvertFromErrno(errno);

    return VINF_SUCCESS;
}


DECLINLINE(int) pciVfioCfgSpaceWriteU16(PVFIOPCI pThis, uint32_t offReg, uint16_t u16)
{
    ssize_t cb = pwrite(pThis->iFdVfio, &u16, 2, pThis->offPciCfg + offReg);
    if (cb != 2)
        return RTErrConvertFromErrno(errno);

    return VINF_SUCCESS;
}

DECLINLINE(int) pciVfioCfgSpaceWriteU32(PVFIOPCI pThis, uint32_t offReg, uint32_t u32)
{
    ssize_t cb = pwrite(pThis->iFdVfio, &u32, 4, pThis->offPciCfg + offReg);
    if (cb != 4)
        return RTErrConvertFromErrno(errno);

    return VINF_SUCCESS;
}


DECLINLINE(int) pciVfioCfgSpaceWriteU64(PVFIOPCI pThis, uint32_t offReg, uint64_t u64)
{
    ssize_t cb = pwrite(pThis->iFdVfio, &u64, 8, pThis->offPciCfg + offReg);
    if (cb != 8)
        return RTErrConvertFromErrno(errno);

    return VINF_SUCCESS;
}


/**
 * @callback_method_impl{FNIOMIOPORTNEWOUT}
 */
static DECLCALLBACK(VBOXSTRICTRC) pciVfioPioWrite(PPDMDEVINS pDevIns, void *pvUser, RTIOPORT offPort, uint32_t u32, unsigned cb)
{
    RT_NOREF(pDevIns, pvUser, offPort, u32, cb);
    AssertMsgFailed(("Should not happen\n"));
    return VINF_SUCCESS;
}


/**
 * @callback_method_impl{FNIOMIOPORTNEWIN}
 */
static DECLCALLBACK(VBOXSTRICTRC) pciVfioPioRead(PPDMDEVINS pDevIns, void *pvUser, RTIOPORT offPort, uint32_t *pu32, unsigned cb)
{
    *pu32 = 0;
    RT_NOREF(pDevIns, pvUser, offPort, cb);
    AssertMsgFailed(("Should not happen\n"));
    return VERR_IOM_IOPORT_UNUSED;
}


/**
 * @callback_method_impl{FNIOMMMIONEWREAD}
 */
static DECLCALLBACK(VBOXSTRICTRC) pciVfioMmioRead(PPDMDEVINS pDevIns, void *pvUser, RTGCPHYS off, void *pv, unsigned cb)
{
    RT_NOREF(pDevIns);
    PCVFIOPCIBAR pBar = (PCVFIOPCIBAR)pvUser;

    Assert(pBar->bType == 2);
    volatile uint8_t *pb = (volatile uint8_t *)pBar->u.pvMmio + off;
    switch (cb)
    {
        case 1: *(uint8_t *)pv = *pb; break;
        case 2: *(uint16_t *)pv = *(volatile uint16_t *)pb; break;
        case 4: *(uint32_t *)pv = *(volatile uint32_t *)pb; break;
        case 8: *(uint64_t *)pv = *(volatile uint64_t *)pb; break;
        default:
            memcpy(pv, (const void *)pb, cb); /** @todo Not correct as memcpy and volatile doesn't mix well */
            break;
    }

    return VINF_SUCCESS;
}


/**
 * @callback_method_impl{FNIOMMMIONEWWRITE}
 */
static DECLCALLBACK(VBOXSTRICTRC) pciVfioMmioWrite(PPDMDEVINS pDevIns, void *pvUser, RTGCPHYS off, void const *pv, unsigned cb)
{
    RT_NOREF(pDevIns);
    PCVFIOPCIBAR pBar = (PCVFIOPCIBAR)pvUser;

    Assert(pBar->bType == 2);
    volatile uint8_t *pb = (volatile uint8_t *)pBar->u.pvMmio + off;
    switch (cb)
    {
        case 1: *(volatile uint8_t *)pb  = *(uint8_t const *)pv;  break;
        case 2: *(volatile uint16_t *)pb = *(uint16_t const *)pv; break;
        case 4: *(volatile uint32_t *)pb = *(uint32_t const *)pv; break;
        case 8: *(volatile uint64_t *)pb = *(uint64_t const *)pv; break;
        default:
            memcpy((void *)pb, pv, cb); /** @todo Not correct as memcpy and volatile doesn't mix well */
            break;
    }

    return VINF_SUCCESS;
}


DECLINLINE(int) pciVfioQueryRegionInfo(PVFIOPCI pThis, uint32_t uRegion, struct vfio_region_info *pRegionInfo)
{
    RT_ZERO(*pRegionInfo);
    pRegionInfo->argsz = sizeof(*pRegionInfo);
    pRegionInfo->index = uRegion;

    int rcLnx = ioctl(pThis->iFdVfio, VFIO_DEVICE_GET_REGION_INFO, pRegionInfo);
    if (rcLnx == -1)
        return PDMDevHlpVMSetError(pThis->pDevIns, RTErrConvertFromErrno(errno), RT_SRC_POS,
                                N_("Getting information for region %u of opened VFIO device failed with %d"), uRegion, errno);

    const int iInstance = pThis->iInstance;
    LogRel(("VFIO#%d: Region %u:\n"
            "VFIO#%d:     flags:       %#RX32\n"
            "VFIO#%d:     index:       %#RU32\n"
            "VFIO#%d:     cap_offset:  %#RX32\n"
            "VFIO#%d:     size:        %#RX64\n"
            "VFIO#%d:     offset:      %#RX64\n",
            iInstance, uRegion,
            iInstance, pRegionInfo->flags,
            iInstance, pRegionInfo->index,
            iInstance, pRegionInfo->cap_offset,
            iInstance, pRegionInfo->size,
            iInstance, pRegionInfo->offset));

    return VINF_SUCCESS;
}


static int pciVfioSetupBar(PVFIOPCI pThis, PPDMDEVINS pDevIns, PPDMPCIDEV pPciDev, uint32_t uRegion, uint32_t uVfioRegion)
{
    struct vfio_region_info RegionInfo; RT_ZERO(RegionInfo);
    int rc = pciVfioQueryRegionInfo(pThis, uVfioRegion, &RegionInfo);
    if (RT_FAILURE(rc))
        return rc;

    if (RegionInfo.flags)
    {
        if (RegionInfo.flags & VFIO_REGION_INFO_FLAG_MMAP)
        {
            int fProt =   ((RegionInfo.flags & VFIO_REGION_INFO_FLAG_READ)  ? PROT_READ  : 0)
                        | ((RegionInfo.flags & VFIO_REGION_INFO_FLAG_WRITE) ? PROT_WRITE : 0);
            pThis->aBars[uRegion].bType    = 2;
            pThis->aBars[uRegion].u.pvMmio = mmap(NULL, RegionInfo.size, fProt, MAP_FILE | MAP_SHARED, pThis->iFdVfio, RegionInfo.offset);
            if (pThis->aBars[uRegion].u.pvMmio == MAP_FAILED)
                return PDMDevHlpVMSetError(pDevIns, RTErrConvertFromErrno(errno), RT_SRC_POS,
                                           N_("Mapping BAR%u at offset %#RX64 with size %RX64 failed with %d"),
                                           uRegion, RegionInfo.offset, RegionInfo.size, errno);

            rc = PDMDevHlpMmioCreate(pDevIns, RegionInfo.size, pPciDev, uRegion /*iPciRegion*/,
                                     pciVfioMmioWrite, pciVfioMmioRead, &pThis->aBars[uRegion],
                                     IOMMMIO_FLAGS_READ_PASSTHRU | IOMMMIO_FLAGS_WRITE_PASSTHRU, "MMIO",
                                     &pThis->aBars[uRegion].hnd.hMmio);
            AssertLogRelRCReturn(rc, rc);

            /** @todo properly detect 64-bit and prefetchable BARs. */
            rc = PDMDevHlpPCIIORegionRegisterMmioEx(pDevIns, pPciDev, uRegion, RegionInfo.size,
                                                    (PCIADDRESSSPACE)(PCI_ADDRESS_SPACE_MEM | PCI_ADDRESS_SPACE_BAR64),
                                                    pThis->aBars[uRegion].hnd.hMmio, NULL);
            AssertLogRelRCReturn(rc, rc);
        }
        else
        {
            /* This assumes PIO. */
            pThis->aBars[uRegion].bType    = 1;
            pThis->aBars[uRegion].u.offPio = RegionInfo.offset;

            rc = PDMDevHlpPCIIORegionCreateIo(pDevIns, uRegion, RegionInfo.size,
                                              pciVfioPioWrite, pciVfioPioRead, NULL /*pvUser*/,
                                              "PIO", NULL /*paExtDescs*/, &pThis->aBars[uRegion].hnd.hIoPort);
            AssertRCReturn(rc, PDMDEV_SET_ERROR(pDevIns, rc, N_("Cannot register PCI I/O region")));
        }
    }
    else /* Not available. */
        Assert(RegionInfo.size == 0);

    return VINF_SUCCESS;
}


DECLINLINE(int) pciVfioQueryIrqInfo(PVFIOPCI pThis, uint32_t uIrq, struct vfio_irq_info *pIrqInfo)
{
    RT_ZERO(*pIrqInfo);
    pIrqInfo->argsz = sizeof(*pIrqInfo);
    pIrqInfo->index = uIrq;

    int rcLnx = ioctl(pThis->iFdVfio, VFIO_DEVICE_GET_IRQ_INFO, pIrqInfo);
    if (rcLnx == -1)
        return PDMDevHlpVMSetError(pThis->pDevIns, RTErrConvertFromErrno(errno), RT_SRC_POS,
                                N_("Getting information for irq %u of opened VFIO device failed with %d"), uIrq, errno);

    const int iInstance = pThis->iInstance;
    LogRel(("VFIO#%d: Irq %u:\n"
            "VFIO#%d:     flags:       %#RX32\n"
            "VFIO#%d:     index:       %#RU32\n"
            "VFIO#%d:     count:       %#RU32\n",
            iInstance, uIrq,
            iInstance, pIrqInfo->flags,
            iInstance, pIrqInfo->index,
            iInstance, pIrqInfo->count));

    return VINF_SUCCESS;
}


static int pciVfioSetupIrq(PVFIOPCI pThis, PPDMDEVINS pDevIns, PPDMPCIDEV pPciDev, uint32_t uVfioIrq)
{
    RT_NOREF(pDevIns, pPciDev);
    struct vfio_irq_info IrqInfo; RT_ZERO(IrqInfo);
    int rc = pciVfioQueryIrqInfo(pThis, uVfioIrq, &IrqInfo);
    if (RT_FAILURE(rc))
        return rc;

    if (IrqInfo.count)
    {
        if (uVfioIrq == VFIO_PCI_INTX_IRQ_INDEX)
        {
            Assert(   IrqInfo.index == 0
                   && IrqInfo.count == 1
                   && (IrqInfo.flags & VFIO_IRQ_INFO_EVENTFD));

            /* Create an assign an eventfd. */
            int iFdEvt = 0;
            rc = pciVfioLnxEventfd2(0 /*uValInit*/, 0 /*fFlags*/, &iFdEvt);
            if (RT_FAILURE(rc))
                return rc;

            union
            {
                struct vfio_irq_set IrqSet;
                uint32_t            au32[sizeof(struct vfio_irq_set) / sizeof(uint32_t) + 1];
            } uBuf;

            uBuf.IrqSet.argsz = sizeof(uBuf);
            uBuf.IrqSet.flags = VFIO_IRQ_SET_DATA_EVENTFD | VFIO_IRQ_SET_ACTION_TRIGGER;
            uBuf.IrqSet.index = 0;
            uBuf.IrqSet.start = 0;
            uBuf.IrqSet.count = 1;
            uBuf.au32[sizeof(struct vfio_irq_set) / sizeof(uint32_t)] = iFdEvt;

            int rcLnx = ioctl(pThis->iFdVfio, VFIO_DEVICE_SET_IRQS, &uBuf);
            if (rcLnx == -1)
                return PDMDevHlpVMSetError(pThis->pDevIns, RTErrConvertFromErrno(errno), RT_SRC_POS,
                                        N_("Assigning one INTX interrupt failed with %d (%u)"), errno, sizeof(uBuf));

            pThis->aIrqFds[0].fd     = iFdEvt;
            pThis->aIrqFds[0].events = POLLIN | POLLRDNORM | POLLRDBAND | POLLPRI | POLLERR;
        }
        else if (uVfioIrq == VFIO_PCI_MSI_IRQ_INDEX)
        {
            Assert(   IrqInfo.index == 1
                   && IrqInfo.count == 1
                   && (IrqInfo.flags & VFIO_IRQ_INFO_EVENTFD));

            /* Create an assign an eventfd. */
            int iFdEvt = 0;
            rc = pciVfioLnxEventfd2(0 /*uValInit*/, 0 /*fFlags*/, &iFdEvt);
            if (RT_FAILURE(rc))
                return rc;

            union
            {
                struct vfio_irq_set IrqSet;
                uint32_t            au32[sizeof(struct vfio_irq_set) / sizeof(uint32_t) + 1];
            } uBuf;

            uBuf.IrqSet.argsz = sizeof(uBuf);
            uBuf.IrqSet.flags = VFIO_IRQ_SET_DATA_EVENTFD | VFIO_IRQ_SET_ACTION_TRIGGER;
            uBuf.IrqSet.index = 1;
            uBuf.IrqSet.start = 0;
            uBuf.IrqSet.count = 1;
            uBuf.au32[sizeof(struct vfio_irq_set) / sizeof(uint32_t)] = iFdEvt;

            int rcLnx = ioctl(pThis->iFdVfio, VFIO_DEVICE_SET_IRQS, &uBuf);
            if (rcLnx == -1)
                return PDMDevHlpVMSetError(pThis->pDevIns, RTErrConvertFromErrno(errno), RT_SRC_POS,
                                        N_("Assigning one INTX interrupt failed with %d (%u)"), errno, sizeof(uBuf));

            pThis->aIrqFds[1].fd     = iFdEvt;
            pThis->aIrqFds[1].events = POLLIN | POLLRDNORM | POLLRDBAND | POLLPRI | POLLERR;

            /* Add capability. */
            PDMMSIREG MsiReg;
            RT_ZERO(MsiReg);
            MsiReg.cMsiVectors     = 1;
            MsiReg.iMsiCapOffset   = 0x50;
            MsiReg.iMsiNextOffset  = 0;
            MsiReg.fMsi64bit       = true;
            rc = PDMDevHlpPCIRegisterMsi(pDevIns, &MsiReg);
            if (RT_FAILURE(rc))
            {
                AssertFailed();
                PDMPciDevSetCapabilityList(pPciDev, 0x0);
            }
        }
        /** @todo MSI-X */
    }
    else /* Not available. */
        Assert(IrqInfo.count == 0);

    return VINF_SUCCESS;
}


static int pciVfioCfgSpaceSetup(PVFIOPCI pThis, PPDMPCIDEV pPciDev)
{
    /* Copy over the standard 256 PCI config space header. */
    uint16_t u16;
    int rc = pciVfioCfgSpaceReadU16(pThis, VBOX_PCI_VENDOR_ID, &u16);
    if (RT_FAILURE(rc)) return rc;
    PDMPciDevSetVendorId(pPciDev, u16);

    rc = pciVfioCfgSpaceReadU16(pThis, VBOX_PCI_DEVICE_ID, &u16);
    if (RT_FAILURE(rc)) return rc;
    PDMPciDevSetDeviceId(pPciDev, u16);

    rc = pciVfioCfgSpaceReadU16(pThis, VBOX_PCI_COMMAND, &u16);
    if (RT_FAILURE(rc)) return rc;
    PDMPciDevSetCommand(pPciDev, u16);

    rc = pciVfioCfgSpaceReadU16(pThis, VBOX_PCI_STATUS, &u16);
    if (RT_FAILURE(rc)) return rc;
    PDMPciDevSetStatus(pPciDev, u16);

    uint8_t u8;
    rc = pciVfioCfgSpaceReadU8(pThis, VBOX_PCI_REVISION_ID, &u8);
    if (RT_FAILURE(rc)) return rc;
    PDMPciDevSetRevisionId(pPciDev, u16);

    rc = pciVfioCfgSpaceReadU8(pThis, VBOX_PCI_CLASS_PROG, &u8);
    if (RT_FAILURE(rc)) return rc;
    PDMPciDevSetClassProg(pPciDev, u8);

    rc = pciVfioCfgSpaceReadU8(pThis, VBOX_PCI_CLASS_SUB, &u8);
    if (RT_FAILURE(rc)) return rc;
    PDMPciDevSetClassSub(pPciDev, u8);

    rc = pciVfioCfgSpaceReadU8(pThis, VBOX_PCI_CLASS_BASE, &u8);
    if (RT_FAILURE(rc)) return rc;
    PDMPciDevSetClassBase(pPciDev, u8);

    /** @todo VBOX_PCI_CACHE_LINE_SIZE */
    /** @todo VBOX_PCI_LATENCY_TIMER   */

    rc = pciVfioCfgSpaceReadU8(pThis, VBOX_PCI_HEADER_TYPE, &u8);
    if (RT_FAILURE(rc)) return rc;
    PDMPciDevSetHeaderType(pPciDev, u8); /* Should be 0 */

    rc = pciVfioCfgSpaceReadU8(pThis, VBOX_PCI_BIST, &u8);
    if (RT_FAILURE(rc)) return rc;
    PDMPciDevSetBIST(pPciDev, u8);

    rc = pciVfioCfgSpaceReadU8(pThis, VBOX_PCI_INTERRUPT_LINE, &u8);
    if (RT_FAILURE(rc)) return rc;
    PDMPciDevSetInterruptLine(pPciDev, u8);

    rc = pciVfioCfgSpaceReadU8(pThis, VBOX_PCI_INTERRUPT_PIN, &u8);
    if (RT_FAILURE(rc)) return rc;
    PDMPciDevSetInterruptPin(pPciDev, u8);

    PDMPciDevSetCapabilityList(pPciDev, 0x50);

    /** @todo Parse the capability list */

    return VINF_SUCCESS;
}


/**
 * @callback_method_impl{FNPCICONFIGREAD}
 */
static DECLCALLBACK(VBOXSTRICTRC) pciVfioConfigRead(PPDMDEVINS pDevIns, PPDMPCIDEV pPciDev,
                                                    uint32_t uAddress, unsigned cb, uint32_t *pu32Value)
{
    PVFIOPCI pThis = PDMDEVINS_2_DATA(pDevIns, PVFIOPCI);
    RT_NOREF(pPciDev, cb, pu32Value, pThis);

    if (uAddress >= VBOX_PCI_BASE_ADDRESS_0 && uAddress < VBOX_PCI_CARDBUS_CIS)
        return VINF_PDM_PCI_DO_DEFAULT;

#if 0
    switch (cb)
    {
        case 1:
        {
            uint8_t u8;
            int rc = pciVfioCfgSpaceReadU8(pThis, uAddress, &u8);
            if (RT_FAILURE(rc)) return rc;
            *pu32Value = u8;
            break;
        }
        case 2:
        {
            uint16_t u16;
            int rc = pciVfioCfgSpaceReadU16(pThis, uAddress, &u16);
            if (RT_FAILURE(rc)) return rc;
            *pu32Value = u16;
            break;
        }
        case 4:
        {
            uint32_t u32;
            int rc = pciVfioCfgSpaceReadU32(pThis, uAddress, &u32);
            if (RT_FAILURE(rc)) return rc;
            *pu32Value = u32;
            break;
        }
        default:
            AssertFailedReturn(VERR_INVALID_PARAMETER);
    }
#endif

    return VINF_PDM_PCI_DO_DEFAULT;
}


/**
 * @callback_method_impl{FNPCICONFIGWRITE}
 */
static DECLCALLBACK(VBOXSTRICTRC) pciVfioConfigWrite(PPDMDEVINS pDevIns, PPDMPCIDEV pPciDev,
                                                     uint32_t uAddress, unsigned cb, uint32_t u32Value)
{
    PVFIOPCI pThis = PDMDEVINS_2_DATA(pDevIns, PVFIOPCI);

    RT_NOREF(pPciDev, pDevIns, uAddress ,cb, u32Value);

    if (uAddress >= VBOX_PCI_BASE_ADDRESS_0 && uAddress < VBOX_PCI_CARDBUS_CIS)
        return VINF_PDM_PCI_DO_DEFAULT;

#if 1
    switch (cb)
    {
        case 1:
        {
            int rc = pciVfioCfgSpaceWriteU8(pThis, uAddress, (uint8_t)u32Value);
            if (RT_FAILURE(rc)) return rc;
            break;
        }
        case 2:
        {
            int rc = pciVfioCfgSpaceWriteU16(pThis, uAddress, (uint16_t)u32Value);
            if (RT_FAILURE(rc)) return rc;
            break;
        }
        case 4:
        {
            int rc = pciVfioCfgSpaceWriteU32(pThis, uAddress, (uint32_t)u32Value);
            if (RT_FAILURE(rc)) return rc;
            break;
        }
        default:
            AssertFailedReturn(VERR_INVALID_PARAMETER);
    }
#endif

    return VINF_PDM_PCI_DO_DEFAULT;
}


/**
 * The IRQ poller thread.
 *
 * @returns VBox status code.
 * @param   pDevIns     The device instance.
 * @param   pThread     The command thread.
 */
static DECLCALLBACK(int) pciVfioIrqPoller(PPDMDEVINS pDevIns, PPDMTHREAD pThread)
{
    PVFIOPCI pThis = PDMDEVINS_2_DATA(pDevIns, PVFIOPCI);

    if (pThread->enmState == PDMTHREADSTATE_INITIALIZING)
        return VINF_SUCCESS;

    while (pThread->enmState == PDMTHREADSTATE_RUNNING)
    {
        int rcPsx = poll(&pThis->aIrqFds[1], 1 /*RT_ELEMENTS(pThis->aIrqFds)*/, -1);
        if (rcPsx == 1)
        {
            for (uint32_t i = 1; i < RT_ELEMENTS(pThis->aIrqFds); i++)
            {
                if (pThis->aIrqFds[i].revents)
                {
                    pThis->aIrqFds[i].revents = 0;
                    uint64_t u64;
                    ssize_t cb = read(pThis->aIrqFds[i].fd, &u64, sizeof(u64));
                    Assert(cb == sizeof(u64));

                    LogRel(("VFIO: interrupt for i=%u\n", i));
                    PDMDevHlpPCISetIrq(pDevIns, 0, 1);

                    /** @todo The interrupt seems to be masked and we would need a mechanism
                     * to get notified when the interrupt is de-asserted in the interrupt controller
                     * (through an EOI for example) so we can unmask them. With KVM this is supported
                     * when KVM_CAP_IRQFD_RESAMPLE is available.
                     */
#if 0
                    union
                    {
                        struct vfio_irq_set IrqSet;
                        uint32_t            au32[sizeof(struct vfio_irq_set) / sizeof(uint32_t) + 1];
                    } uBuf;

                    uBuf.IrqSet.argsz = sizeof(uBuf);
                    uBuf.IrqSet.flags = VFIO_IRQ_SET_DATA_EVENTFD | VFIO_IRQ_SET_ACTION_UNMASK;
                    uBuf.IrqSet.index = i;
                    uBuf.IrqSet.start = 0;
                    uBuf.IrqSet.count = 1;
                    uBuf.au32[sizeof(struct vfio_irq_set) / sizeof(uint32_t)] = pThis->aIrqFds[i].fd;

                    int rcLnx = ioctl(pThis->iFdVfio, VFIO_DEVICE_SET_IRQS, &uBuf);
                    if (rcLnx == -1)
                        LogRel(("Unmasking one INTX interrupt failed with %d", errno));
#endif
                }
            }
        }
        else
            AssertFailed();
    }

    LogFlowFunc(("Poller thread terminating\n"));
    return VINF_SUCCESS;
}


/**
 * Wakes up the IRQ poller to respond to state changes.
 *
 * @returns VBox status code.
 * @param   pDevIns     The device instance.
 * @param   pThread     The command thread.
 */
static DECLCALLBACK(int) pciVfioIrqPollerWakeup(PPDMDEVINS pDevIns, PPDMTHREAD pThread)
{
    RT_NOREF(pThread);
    Log4Func(("\n"));
    PVFIOPCI pThis = PDMDEVINS_2_DATA(pDevIns, PVFIOPCI);
    RT_NOREF(pThis);
    return VINF_SUCCESS;
}


static int pciVfioMapRegion(PVFIOPCI pThis, RTGCPHYS GCPhysStart, uintptr_t uPtrMapping, size_t cbMapping)
{
    struct iommu_ioas_map Map;
    Map.size       = sizeof(Map);
    Map.flags      = IOMMU_IOAS_MAP_FIXED_IOVA | IOMMU_IOAS_MAP_WRITEABLE | IOMMU_IOAS_MAP_READABLE;
    Map.ioas_id    = pThis->idIommuHwpt;
    Map.__reserved = 0;
    Map.user_va    = uPtrMapping;
    Map.length     = cbMapping;
    Map.iova       = GCPhysStart;

    LogRel(("Mapping GCPhysStart=%RGp uPtrMapping=%p cbMapping=%#zx\n", GCPhysStart, uPtrMapping, cbMapping));

    int rcLnx = ioctl(pThis->iFdIommu, IOMMU_IOAS_MAP, &Map);
    if (rcLnx == -1)
    {
        LogRel(("errno=%d\n", errno));
        return RTErrConvertFromErrno(errno);
    }

    return VINF_SUCCESS;
}


/**
 * @interface_method_impl{PDMDEVREG,pfnPowerOn}
 */
static DECLCALLBACK(void) pciVfioPowerOn(PPDMDEVINS pDevIns)
{
    PVFIOPCI pThis = PDMDEVINS_2_DATA(pDevIns, PVFIOPCI);

    /** @todo This is a really gross hack because we currently lack
     * a dedicated interface to get knowledge about guest RAM mappings.
     * This might also return mappings for stuff not being guest RAM,
     * and it only works in simple NEM mode currently, where everything is
     * pre-allocated and guest pages don't change their mapping.
     */
    RTGCPHYS  GCPhysStart = 0;
    uintptr_t uPtrMapping = 0;
    size_t    cbMapping   = 0;
    for (RTGCPHYS GCPhys = 0; GCPhys < 10 * _1G64; GCPhys += _4K)
    {
        void *pv = NULL;
        PGMPAGEMAPLOCK Lock;
        int rc = PDMDevHlpPhysGCPhys2CCPtr(pDevIns, GCPhys, 0 /*fFlags*/, &pv, &Lock);
        if (RT_SUCCESS(rc))
        {
            if (cbMapping)
            {
                if (uPtrMapping + cbMapping == (uintptr_t)pv)
                    cbMapping += _4K;
                else
                {
                    rc = pciVfioMapRegion(pThis, GCPhysStart, uPtrMapping, cbMapping);
                    if (RT_FAILURE(rc))
                        LogRel(("Mapping %RGp/%zu failed with %Rrc\n", GCPhysStart, cbMapping, rc));

                    GCPhysStart = GCPhys;
                    uPtrMapping = (uintptr_t)pv;
                    cbMapping = _4K;
                }
            }
            else
            {
                Assert(!uPtrMapping);
                GCPhysStart = GCPhys;
                uPtrMapping = (uintptr_t)pv;
                cbMapping = _4K;
            }
            PDMDevHlpPhysReleasePageMappingLock(pDevIns, &Lock);
        }
        else if (cbMapping)
        {
            LogRel(("PDMDevHlpPhysGCPhys2CCPtr(,%RGp) -> %Rrc\n", GCPhys, rc));
            /* Map what we currently have. */
            Assert(uPtrMapping);
            rc = pciVfioMapRegion(pThis, GCPhysStart, uPtrMapping, cbMapping);
            if (RT_FAILURE(rc))
                LogRel(("Mapping %RGp/%zu failed with %Rrc\n", GCPhysStart, cbMapping, rc));
            cbMapping   = 0;
            uPtrMapping = 0;
            GCPhysStart = GCPhys;
        }
    }

    if (cbMapping)
    {
        /* Map what we currently have. */
        Assert(uPtrMapping);
        int rc = pciVfioMapRegion(pThis, GCPhysStart, uPtrMapping, cbMapping);
        if (RT_FAILURE(rc))
            LogRel(("Mapping %RGp/%zu failed with %Rrc\n", GCPhysStart, cbMapping, rc));
        cbMapping   = 0;
        uPtrMapping = 0;
    }
}


/**
 * @interface_method_impl{PDMDEVREG,pfnDestruct}
 */
static DECLCALLBACK(int) pciVfioDestruct(PPDMDEVINS pDevIns)
{
    PVFIOPCI pThis = PDMDEVINS_2_DATA(pDevIns, PVFIOPCI);

    /* Detach address space. */
    if (pThis->iFdIommu != -1 && pThis->iFdVfio != -1)
    {
        struct vfio_device_detach_iommufd_pt VfioDetach;
        VfioDetach.argsz   = sizeof(VfioDetach);
        VfioDetach.flags   = 0;
        int rcLnx = ioctl(pThis->iFdVfio, VFIO_DEVICE_DETACH_IOMMUFD_PT, &VfioDetach);
        AssertLogRel(("VFIO#%d: Failed to detach IOMMU page table with %d\n", pThis->iInstance, errno));

        struct iommu_destroy HwptDestroy;
        HwptDestroy.size = sizeof(HwptDestroy);
        HwptDestroy.id   = pThis->idIommuHwpt;
        rcLnx = ioctl(pThis->iFdIommu, IOMMU_DESTROY, &HwptDestroy);
        AssertLogRel(("VFIO#%d: Failed to destroy I/O address space with %d\n", pThis->iInstance, errno));
    }

    if (pThis->iFdIommu != -1)
        close(pThis->iFdIommu);
    if (pThis->iFdVfio != -1)
        close(pThis->iFdIommu);

    return VINF_SUCCESS;
}


/**
 * @interface_method_impl{PDMDEVREG,pfnConstruct}
 */
static DECLCALLBACK(int) pciVfioConstruct(PPDMDEVINS pDevIns, int iInstance, PCFGMNODE pCfg)
{
    PDMDEV_CHECK_VERSIONS_RETURN(pDevIns);

    PVFIOPCI      pThis = PDMDEVINS_2_DATA(pDevIns, PVFIOPCI);
    PCPDMDEVHLPR3 pHlp  = pDevIns->pHlpR3;

    PPDMPCIDEV pPciDev = pDevIns->apPciDevs[0];
    PDMPCIDEV_ASSERT_VALID(pDevIns, pPciDev);

    pThis->pDevIns   = pDevIns;
    pThis->iInstance = iInstance;
    pThis->iFdIommu  = -1;
    pThis->iFdVfio   = -1;

    /*
     * Validate configuration.
     */
    if (!pHlp->pfnCFGMAreValuesValid(pCfg,
                                     "VfioPath\0"
                                     "IommuPath\0"
                                    ))
        return VERR_PDM_DEVINS_UNKNOWN_CFG_VALUES;

    /* Query configuration. */
    char szPath[RTPATH_MAX];
    int rc = pHlp->pfnCFGMQueryString(pCfg, "VfioPath", &szPath[0], sizeof(szPath));
    if (RT_FAILURE(rc))
        return PDMDevHlpVMSetError(pDevIns, rc, RT_SRC_POS,
                                   N_("Configuration error: Querying \"VfioPath\" failed"));

    pThis->iFdVfio = open(szPath, O_RDWR);
    if (pThis->iFdVfio == -1)
        return PDMDevHlpVMSetError(pDevIns, RTErrConvertFromErrno(errno), RT_SRC_POS,
                                   N_("Opening VFIO device \"%s\" failed with %d"), szPath, errno);

    rc = pHlp->pfnCFGMQueryString(pCfg, "IommuPath", &szPath[0], sizeof(szPath));
    if (RT_FAILURE(rc))
        return PDMDevHlpVMSetError(pDevIns, rc, RT_SRC_POS,
                                   N_("Configuration error: Querying \"IommuPath\" failed"));

    pThis->iFdIommu = open(szPath, O_RDWR);
    if (pThis->iFdIommu == -1)
        return PDMDevHlpVMSetError(pDevIns, RTErrConvertFromErrno(errno), RT_SRC_POS,
                                   N_("Opening IOMMU path \"%s\" failed with %d"), szPath, errno);

    /* Bind the IOMMU to the device. */
    struct vfio_device_bind_iommufd VfioBind; RT_ZERO(VfioBind);
    VfioBind.argsz   = sizeof(VfioBind);
    VfioBind.flags   = 0;
    VfioBind.iommufd = pThis->iFdIommu;
    int rcLnx = ioctl(pThis->iFdVfio, VFIO_DEVICE_BIND_IOMMUFD, &VfioBind);
    if (rcLnx == -1)
        return PDMDevHlpVMSetError(pDevIns, RTErrConvertFromErrno(errno), RT_SRC_POS,
                                   N_("Binding IOMMU device to opened VFIO device failed with %d"), errno);

    /* Allocate a new I/O address space on the IOMMU. */
    struct iommu_ioas_alloc IoasAlloc;
    IoasAlloc.size  = sizeof(IoasAlloc);
    IoasAlloc.flags = 0;
    rcLnx = ioctl(pThis->iFdIommu, IOMMU_IOAS_ALLOC, &IoasAlloc);
    if (rcLnx == -1)
        return PDMDevHlpVMSetError(pDevIns, RTErrConvertFromErrno(errno), RT_SRC_POS,
                                   N_("Allocating I/O address space for VFIO device failed with %d"), errno);

    /* And bind it to the device. */
    struct vfio_device_attach_iommufd_pt VfioAttachIommu;
    VfioAttachIommu.argsz = sizeof(VfioAttachIommu);
    VfioAttachIommu.flags = 0;
    VfioAttachIommu.pt_id = IoasAlloc.out_ioas_id;
    pThis->idIommuHwpt = IoasAlloc.out_ioas_id;
    rcLnx = ioctl(pThis->iFdVfio, VFIO_DEVICE_ATTACH_IOMMUFD_PT, &VfioAttachIommu);
    if (rcLnx == -1)
        return PDMDevHlpVMSetError(pDevIns, RTErrConvertFromErrno(errno), RT_SRC_POS,
                                   N_("Attaching IO address space to opened VFIO device failed with %d"), errno);

    struct vfio_device_info DevInfo;
    DevInfo.argsz = sizeof(DevInfo);
    rcLnx = ioctl(pThis->iFdVfio, VFIO_DEVICE_GET_INFO, &DevInfo);
    if (rcLnx == -1)
        return PDMDevHlpVMSetError(pDevIns, RTErrConvertFromErrno(errno), RT_SRC_POS,
                                   N_("Getting device information of opened VFIO device failed with %d"), errno);

    LogRel(("VFIO#%d: Info flags:       %#RX32\n"
            "VFIO#%d: Info num_regions: %#RU32\n"
            "VFIO#%d: Info num_irqs:    %#RU32\n"
            "VFIO#%d: Info cap_offset:  %#RX32\n",
            iInstance, DevInfo.flags,
            iInstance, DevInfo.num_regions,
            iInstance, DevInfo.num_irqs,
            iInstance, DevInfo.cap_offset));

    /** @todo Only support PCI devices. */

    /* Setup access to the PCI config space first. */
    struct vfio_region_info RegionInfo;
    rc = pciVfioQueryRegionInfo(pThis, VFIO_PCI_CONFIG_REGION_INDEX, &RegionInfo);
    if (RT_FAILURE(rc))
        return rc;

    if (   (RegionInfo.flags & (VFIO_REGION_INFO_FLAG_READ | VFIO_REGION_INFO_FLAG_WRITE))
        != (VFIO_REGION_INFO_FLAG_READ | VFIO_REGION_INFO_FLAG_WRITE))
        return PDMDevHlpVMSetError(pDevIns, VERR_INVALID_STATE, RT_SRC_POS,
                                   N_("The PCI config region is not marked as read/write as expected"));

    pThis->offPciCfg = RegionInfo.offset;
    pThis->cbPciCfg  = RegionInfo.size;

    /* Setup the PCI config space. */
    rc = pciVfioCfgSpaceSetup(pThis, pPciDev);
    if (RT_FAILURE(rc))
        return rc;

    LogRel(("VFIO#%d: Attached PCI device %04x:%04x\n", iInstance,
            PDMPciDevGetVendorId(pPciDev), PDMPciDevGetDeviceId(pPciDev)));

    /* Attach the device. */
    rc = PDMDevHlpPCIRegister(pDevIns, pPciDev);
    if (RT_FAILURE(rc))
        return rc;

    rc = PDMDevHlpPCIInterceptConfigAccesses(pDevIns, pPciDev, pciVfioConfigRead, pciVfioConfigWrite);
    if (RT_FAILURE(rc))
        return rc;

    /* Set up BAR0 through BAR5. */
    static uint32_t s_aVfioPciRegions[] =
    {
        VFIO_PCI_BAR0_REGION_INDEX,
        VFIO_PCI_BAR1_REGION_INDEX,
        VFIO_PCI_BAR2_REGION_INDEX,
        VFIO_PCI_BAR3_REGION_INDEX,
        VFIO_PCI_BAR4_REGION_INDEX,
        VFIO_PCI_BAR5_REGION_INDEX
    };

    for (uint32_t i = 0; i < RT_ELEMENTS(s_aVfioPciRegions); i++)
    {
        rc = pciVfioSetupBar(pThis, pDevIns, pPciDev, i, s_aVfioPciRegions[i]);
        if (RT_FAILURE(rc))
            return rc;
    }

    /** @todo ROM BAR and VGA region index. */

    /* Set up interrupts. */
    static uint32_t s_aVfioIrqs[] =
    {
        /*VFIO_PCI_INTX_IRQ_INDEX,*/
        VFIO_PCI_MSI_IRQ_INDEX,
        /* VFIO_PCI_MSIX_IRQ_INDEX */
        /* VFIO_PCI_ERR_IRQ_INDEX */
        /* VFIO_PCI_REQ_IRQ_INDEX */
    };

    for (uint32_t i = 0; i < RT_ELEMENTS(s_aVfioIrqs); i++)
    {
        rc = pciVfioSetupIrq(pThis, pDevIns, pPciDev, s_aVfioIrqs[i]);
        if (RT_FAILURE(rc))
            return rc;
    }

    /* Spin up the interrupt poller. */
    char szDev[64];
    RT_ZERO(szDev);
    RTStrPrintf(szDev, sizeof(szDev), "VFIO-%u", iInstance);
    rc = PDMDevHlpThreadCreate(pDevIns, &pThis->pThrdIrq, pThis, pciVfioIrqPoller, pciVfioIrqPollerWakeup,
                               0 /* cbStack */, RTTHREADTYPE_IO, szDev);
    AssertLogRelRCReturn(rc, rc);

    return rc;
}


/**
 * The device registration structure.
 */
const PDMDEVREG g_DevicePciVfio =
{
    /* .u32Version = */             PDM_DEVREG_VERSION,
    /* .uReserved0 = */             0,
    /* .szName = */                 "pci-vfio",
    /* .fFlags = */                 PDM_DEVREG_FLAGS_DEFAULT_BITS | PDM_DEVREG_FLAGS_NEW_STYLE,
    /* .fClass = */                 PDM_DEVREG_CLASS_HOST_DEV,
    /* .cMaxInstances = */          ~0U,
    /* .uSharedVersion = */         42,
    /* .cbInstanceShared = */       sizeof(VFIOPCI),
    /* .cbInstanceCC = */           0,
    /* .cbInstanceRC = */           0,
    /* .cMaxPciDevices = */         1,
    /* .cMaxMsixVectors = */        VBOX_MSIX_MAX_ENTRIES,
    /* .pszDescription = */         "PCI passthrough through VFIO",
#if defined(IN_RING3)
    /* .pszRCMod = */               "",
    /* .pszR0Mod = */               "",
    /* .pfnConstruct = */           pciVfioConstruct,
    /* .pfnDestruct = */            pciVfioDestruct,
    /* .pfnRelocate = */            NULL,
    /* .pfnMemSetup = */            NULL,
    /* .pfnPowerOn = */             pciVfioPowerOn,
    /* .pfnReset = */               NULL, //pciVfioReset,
    /* .pfnSuspend = */             NULL,
    /* .pfnResume = */              NULL,
    /* .pfnAttach = */              NULL,
    /* .pfnDetach = */              NULL,
    /* .pfnQueryInterface = */      NULL,
    /* .pfnInitComplete = */        NULL,
    /* .pfnPowerOff = */            NULL,
    /* .pfnSoftReset = */           NULL,
    /* .pfnReserved0 = */           NULL,
    /* .pfnReserved1 = */           NULL,
    /* .pfnReserved2 = */           NULL,
    /* .pfnReserved3 = */           NULL,
    /* .pfnReserved4 = */           NULL,
    /* .pfnReserved5 = */           NULL,
    /* .pfnReserved6 = */           NULL,
    /* .pfnReserved7 = */           NULL,
#elif defined(IN_RING0)
    /* .pfnEarlyConstruct = */      NULL,
    /* .pfnConstruct = */           NULL,
    /* .pfnDestruct = */            NULL,
    /* .pfnFinalDestruct = */       NULL,
    /* .pfnRequest = */             NULL,
    /* .pfnReserved0 = */           NULL,
    /* .pfnReserved1 = */           NULL,
    /* .pfnReserved2 = */           NULL,
    /* .pfnReserved3 = */           NULL,
    /* .pfnReserved4 = */           NULL,
    /* .pfnReserved5 = */           NULL,
    /* .pfnReserved6 = */           NULL,
    /* .pfnReserved7 = */           NULL,
#elif defined(IN_RC)
    /* .pfnConstruct = */           NULL,
    /* .pfnReserved0 = */           NULL,
    /* .pfnReserved1 = */           NULL,
    /* .pfnReserved2 = */           NULL,
    /* .pfnReserved3 = */           NULL,
    /* .pfnReserved4 = */           NULL,
    /* .pfnReserved5 = */           NULL,
    /* .pfnReserved6 = */           NULL,
    /* .pfnReserved7 = */           NULL,
#else
# error "Not in IN_RING3, IN_RING0 or IN_RC!"
#endif
    /* .u32VersionEnd = */          PDM_DEVREG_VERSION
};

#endif /* !VBOX_DEVICE_STRUCT_TESTCASE */

