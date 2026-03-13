/* $Id: RecordingInternals.cpp 113380 2026-03-13 10:01:45Z andreas.loeffler@oracle.com $ */
/** @file
 * Recording internals code.
 */

/*
 * Copyright (C) 2012-2026 Oracle and/or its affiliates.
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

#include "RecordingInternals.h"
#include "RecordingUtils.h"

#include <iprt/assert.h>
#include <iprt/critsect.h>
#include <iprt/mem.h>

#ifdef DEBUG
# include <math.h>
# include <iprt/file.h>
# include <iprt/formats/bmp.h>
#endif

#include <VBox/log.h>

#ifdef LOG_GROUP
# undef LOG_GROUP
#endif
#define LOG_GROUP LOG_GROUP_RECORDING


/*********************************************************************************************************************************
*   Prototypes                                                                                                                   *
*********************************************************************************************************************************/
DECLINLINE(int) recordingVideoFrameInit(PRECORDINGVIDEOFRAME pFrame, uint32_t fFlags, uint32_t uWidth, uint32_t uHeight, uint32_t uPosX, uint32_t uPosY,
                                        uint8_t uBPP, RECORDINGPIXELFMT enmFmt);


/**
 * Allocates an empty video frame, inline version.
 *
 * @returns Allocated video frame on success, or NULL on failure.
 */
DECLINLINE(PRECORDINGVIDEOFRAME) recordingVideoFrameAlloc(void)
{
    return (PRECORDINGVIDEOFRAME)RTMemAlloc(sizeof(RECORDINGVIDEOFRAME));
}

/**
 * Allocates an empty video frame.
 *
 * @returns Allocated video frame on success, or NULL on failure.
 */
PRECORDINGVIDEOFRAME RecordingVideoFrameAlloc(void)
{
    PRECORDINGVIDEOFRAME pFrame = recordingVideoFrameAlloc();
    AssertPtrReturn(pFrame, NULL);
    RT_BZERO(pFrame, sizeof(RECORDINGVIDEOFRAME));
    return pFrame;
}

/**
 * Returns an allocated video frame from given image data.
 *
 * @returns Allocated video frame on success, or NULL on failure.
 * @param   pvData              Pointer to image data to use.
 * @param   x                   X location hint (in pixel) to use for allocated frame.
 *                              This is *not* the offset within \a pvData!
 * @param   y                   X location hint (in pixel) to use for allocated frame.
 *                              This is *not* the offset within \a pvData!
 * @param   w                   Width (in pixel) of \a pvData image data.
 * @param   h                   Height (in pixel) of \a pvData image data.
 * @param   uBPP                Bits per pixel) of \a pvData image data.
 * @param   enmFmt              Pixel format of \a pvData image data.
 */
PRECORDINGVIDEOFRAME RecordingVideoFrameAllocEx(const void *pvData, uint32_t x, uint32_t y, uint32_t w, uint32_t h,
                                                uint8_t uBPP, RECORDINGPIXELFMT enmFmt)
{
    PRECORDINGVIDEOFRAME pFrame = recordingVideoFrameAlloc();
    AssertPtrReturn(pFrame, NULL);
    int vrc = recordingVideoFrameInit(pFrame, RECORDINGVIDEOFRAME_F_VISIBLE, w, h, x, y, uBPP, enmFmt);
    AssertRCReturn(vrc, NULL);
    memcpy(pFrame->pau8Buf, pvData, pFrame->cbBuf);

    return VINF_SUCCESS;
}

/**
 * Frees a recording video frame.
 *
 * @param   pFrame              Pointer to video frame to free. The pointer will be invalid after return.
 */
void RecordingVideoFrameFree(PRECORDINGVIDEOFRAME pFrame)
{
    if (!pFrame)
        return;

    RecordingVideoFrameDestroy(pFrame);

    RTMemFree(pFrame);
}

/**
 * Initializes a recording frame, inline version.
 *
 * @returns VBox status code.
 * @param   pFrame              Pointer to video frame to initialize.
 * @param   fFlags              Flags of type RECORDINGVIDEOFRAME_F_XXX.
 * @param   uWidth              Width (in pixel) of video frame.
 * @param   uHeight             Height (in pixel) of video frame.
 * @param   uPosX               X positioning hint.
 * @param   uPosY               Y positioning hint.
 * @param   uBPP                Bits per pixel (BPP).
 * @param   enmFmt              Pixel format to use.
 */
DECLINLINE(int) recordingVideoFrameInit(PRECORDINGVIDEOFRAME pFrame, uint32_t fFlags, uint32_t uWidth, uint32_t uHeight,
                                        uint32_t uPosX, uint32_t uPosY, uint8_t uBPP, RECORDINGPIXELFMT enmFmt)
{
    AssertPtrReturn(pFrame, VERR_INVALID_POINTER);
    AssertReturn(uWidth, VERR_INVALID_PARAMETER);
    AssertReturn(uHeight, VERR_INVALID_PARAMETER);
    AssertReturn(uBPP && uBPP % 8 == 0, VERR_INVALID_PARAMETER);

    /* Calculate bytes per pixel and set pixel format. */
    const unsigned uBytesPerPixel = uBPP / 8;

    /* Calculate bytes per pixel and set pixel format. */
    const size_t cbRGBBuf = uWidth * uHeight * uBytesPerPixel;
    AssertReturn(cbRGBBuf, VERR_INVALID_PARAMETER);

    pFrame->pau8Buf = (uint8_t *)RTMemAlloc(cbRGBBuf);
    AssertPtrReturn(pFrame->pau8Buf, VERR_NO_MEMORY);
    pFrame->cbBuf  = cbRGBBuf;

    pFrame->fFlags             = fFlags;
    pFrame->Info.uWidth        = uWidth;
    pFrame->Info.uHeight       = uHeight;
    pFrame->Info.uBPP          = uBPP;
    pFrame->Info.enmPixelFmt   = enmFmt;
    pFrame->Info.uBytesPerLine = uWidth * uBytesPerPixel;
    pFrame->Pos.x              = uPosX;
    pFrame->Pos.y              = uPosY;

    return VINF_SUCCESS;
}

/**
 * Initializes a recording frame.
 *
 * @param   pFrame              Pointer to video frame to initialize.
 * @param   fFlags              Flags of type RECORDINGVIDEOFRAME_F_XXX.
 * @param   uWidth              Width (in pixel) of video frame.
 * @param   uHeight             Height (in pixel) of video frame.
 * @param   uPosX               X positioning hint.
 * @param   uPosY               Y positioning hint.
 * @param   uBPP                Bits per pixel (BPP).
 * @param   enmFmt              Pixel format to use.
 */
int RecordingVideoFrameInit(PRECORDINGVIDEOFRAME pFrame, uint32_t fFlags, uint32_t uWidth, uint32_t uHeight, uint32_t uPosX, uint32_t uPosY,
                            uint8_t uBPP, RECORDINGPIXELFMT enmFmt)
{
    return recordingVideoFrameInit(pFrame, fFlags, uWidth, uHeight, uPosX, uPosY, uBPP, enmFmt);
}

/**
 * Destroys a recording video frame.
 *
 * @param   pFrame              Pointer to video frame to destroy.
 */
void RecordingVideoFrameDestroy(PRECORDINGVIDEOFRAME pFrame)
{
    if (!pFrame)
        return;

    if (pFrame->pau8Buf)
    {
        Assert(pFrame->cbBuf);
        RTMemFree(pFrame->pau8Buf);
        pFrame->pau8Buf = NULL;
        pFrame->cbBuf  = 0;
    }
}

/**
 * Duplicates a video frame.
 *
 * @returns Pointer to duplicated frame on success, or NULL on failure.
 * @param   pFrame              Video frame to duplicate.
 */
PRECORDINGVIDEOFRAME RecordingVideoFrameDup(PRECORDINGVIDEOFRAME pFrame)
{
    PRECORDINGVIDEOFRAME pFrameDup = (PRECORDINGVIDEOFRAME)RTMemDup(pFrame, sizeof(RECORDINGVIDEOFRAME));
    AssertPtrReturn(pFrameDup, NULL);
    pFrameDup->pau8Buf = (uint8_t *)RTMemDup(pFrame->pau8Buf, pFrame->cbBuf);
    AssertPtrReturnStmt(pFrameDup, RTMemFree(pFrameDup), NULL);

    return pFrameDup;
}

/**
 * Clears the content of a video recording frame, inlined version.
 *
 * @param   pFrame              Video recording frame to clear content for.
 */
DECLINLINE(void) recordingVideoFrameClear(PRECORDINGVIDEOFRAME pFrame)
{
    RT_BZERO(pFrame->pau8Buf, pFrame->cbBuf);
}

/**
 * Clears the content of a video recording frame.
 *
 * @param   pFrame              Video recording frame to clear content for.
 */
void RecordingVideoFrameClear(PRECORDINGVIDEOFRAME pFrame)
{
    recordingVideoFrameClear(pFrame);
}

/**
 * Simple blitting function for raw image data, inlined version.
 *
 * @returns VBox status code.
 * @param   pu8Dst              Destination buffer.
 * @param   cbDst               Size (in bytes) of \a pu8Dst.
 * @param   uDstX               X destination (in pixel) within destination frame.
 * @param   uDstY               Y destination (in pixel) within destination frame.
 * @param   uDstBytesPerLine    Bytes per line in destination buffer.
 * @param   uDstBPP             BPP of destination buffer.
 * @param   enmDstFmt           Pixel format of source data. Must match \a pFrame.
 * @param   pu8Src              Source data to blit. Must be in the same pixel format as \a pFrame.
 * @param   cbSrc               Size (in bytes) of \a pu8Src.
 * @param   uSrcX               X start (in pixel) within source data.
 * @param   uSrcY               Y start (in pixel) within source data.
 * @param   uSrcWidth           Width (in pixel) to blit from source data.
 * @param   uSrcHeight          Height (in pixel) to blit from data.
 * @param   uSrcBytesPerLine    Bytes per line in source data.
 * @param   uSrcBPP             BPP of source data. Must match \a pFrame.
 * @param   enmSrcFmt           Pixel format of source data. Must match \a pFrame.
 */
DECLINLINE(int) recordingVideoBlitRaw(uint8_t *pu8Dst, size_t cbDst, uint32_t uDstX, uint32_t uDstY,
                                      uint32_t uDstBytesPerLine, uint8_t uDstBPP, RECORDINGPIXELFMT enmDstFmt,
                                      const uint8_t *pu8Src, size_t cbSrc, uint32_t uSrcX, uint32_t uSrcY, uint32_t uSrcWidth, uint32_t uSrcHeight,
                                      uint32_t uSrcBytesPerLine, uint8_t uSrcBPP, RECORDINGPIXELFMT enmSrcFmt)
{
    RT_NOREF(enmDstFmt, enmSrcFmt);

    uint8_t const uDstBytesPerPixel = uDstBPP / 8;
    uint8_t const uSrcBytesPerPixel = uSrcBPP / 8;

    size_t offSrc = RT_MIN(uSrcY * uSrcBytesPerLine + uSrcX * uSrcBytesPerPixel, cbSrc);
    size_t offDst = RT_MIN(uDstY * uDstBytesPerLine + uDstX * uDstBytesPerPixel, cbDst);

    for (uint32_t y = 0; y < uSrcHeight; y++)
    {
        size_t const cbToCopy = RT_MIN(cbDst - offDst,
                                       RT_MIN(uSrcWidth * uSrcBytesPerPixel, cbSrc - offSrc));
        if (!cbToCopy)
            break;
        memcpy(pu8Dst + offDst, (const uint8_t *)pu8Src + offSrc, cbToCopy);
        offDst = RT_MIN(offDst + uDstBytesPerLine, cbDst);
        Assert(offDst <= cbDst);
        offSrc = RT_MIN(offSrc + uSrcBytesPerLine, cbSrc);
        Assert(offSrc <= cbSrc);
    }

    return VINF_SUCCESS;
}

/**
 * Simple blitting function for raw image data.
 *
 * @returns VBox status code.
 * @param   pu8Dst              Destination buffer.
 * @param   cbDst               Size (in bytes) of \a pu8Dst.
 * @param   uDstX               X destination (in pixel) within destination frame.
 * @param   uDstY               Y destination (in pixel) within destination frame.
 * @param   uDstBytesPerLine    Bytes per line in destination buffer.
 * @param   uDstBPP             BPP of destination buffer.
 * @param   enmDstFmt           Pixel format of source data. Must match \a pFrame.
 * @param   pu8Src              Source data to blit. Must be in the same pixel format as \a pFrame.
 * @param   cbSrc               Size (in bytes) of \a pu8Src.
 * @param   uSrcX               X start (in pixel) within source data.
 * @param   uSrcY               Y start (in pixel) within source data.
 * @param   uSrcWidth           Width (in pixel) to blit from source data.
 * @param   uSrcHeight          Height (in pixel) to blit from data.
 * @param   uSrcBytesPerLine    Bytes per line in source data.
 * @param   uSrcBPP             BPP of source data. Must match \a pFrame.
 * @param   enmSrcFmt           Pixel format of source data. Must match \a pFrame.
 */
int RecordingVideoBlitRaw(uint8_t *pu8Dst, size_t cbDst, uint32_t uDstX, uint32_t uDstY,
                          uint32_t uDstBytesPerLine, uint8_t uDstBPP, RECORDINGPIXELFMT enmDstFmt,
                          const uint8_t *pu8Src, size_t cbSrc, uint32_t uSrcX, uint32_t uSrcY, uint32_t uSrcWidth, uint32_t uSrcHeight,
                          uint32_t uSrcBytesPerLine, uint8_t uSrcBPP, RECORDINGPIXELFMT enmSrcFmt)
{
    return recordingVideoBlitRaw(pu8Dst, cbDst, uDstX, uDstY, uDstBytesPerLine,uDstBPP, enmDstFmt,
                                 pu8Src, cbSrc, uSrcX, uSrcY, uSrcWidth, uSrcHeight,uSrcBytesPerLine, uSrcBPP, enmSrcFmt);
}

/**
 * Simple blitting function for raw image data with alpha channel, inlined version.
 *
 * @param   pFrame              Destination frame.
 * @param   uDstX               X destination (in pixel) within destination frame.
 * @param   uDstY               Y destination (in pixel) within destination frame.
 * @param   pu8Src              Source data to blit. Must be in the same pixel format as \a pFrame.
 * @param   cbSrc               Size (in bytes) of \a pu8Src.
 * @param   uSrcX               X start (in pixel) within source data.
 * @param   uSrcY               Y start (in pixel) within source data.
 * @param   uSrcWidth           Width (in pixel) to blit from source data.
 * @param   uSrcHeight          Height (in pixel) to blit from data.
 * @param   uSrcBytesPerLine    Bytes per line in source data.
 * @param   uSrcBPP             BPP of source data. Must match \a pFrame.
 * @param   enmFmt              Pixel format of source data. Must match \a pFrame.
 *                              Only supports RECORDINGPIXELFMT_BRGA32 for now.
 */
DECLINLINE(void) recordingVideoFrameBlitRawAlpha(PRECORDINGVIDEOFRAME pFrame, uint32_t uDstX, uint32_t uDstY,
                                                 const uint8_t *pu8Src, size_t cbSrc, uint32_t uSrcX, uint32_t uSrcY, uint32_t uSrcWidth, uint32_t uSrcHeight,
                                                 uint32_t uSrcBytesPerLine, uint8_t uSrcBPP, RECORDINGPIXELFMT enmFmt)
{
    Assert(enmFmt == RECORDINGPIXELFMT_BRGA32);
    Assert(pFrame->Info.enmPixelFmt == enmFmt);
    Assert(pFrame->Info.uBPP % 8 == 0);

    if (   !pFrame->pau8Buf
        || !pu8Src
        || !uSrcWidth
        || !uSrcHeight)
        return;

    if (   uDstX >= pFrame->Info.uWidth
        || uDstY >= pFrame->Info.uHeight)
        return;

    uint32_t const cbBytesPerPixel = 4;
    uint32_t const cRows = RT_MIN(uSrcHeight, pFrame->Info.uHeight - uDstY);
    uint32_t const cCols = RT_MIN(uSrcWidth,  pFrame->Info.uWidth  - uDstX);

    if (uSrcBPP != 32)
        return;

    for (uint32_t y = 0; y < cRows; y++)
    {
        size_t const offSrcRow = (size_t)(uSrcY + y) * uSrcBytesPerLine + (size_t)uSrcX * cbBytesPerPixel;
        size_t const offDstRow = (size_t)(uDstY + y) * pFrame->Info.uBytesPerLine + (size_t)uDstX * cbBytesPerPixel;
        if (   offSrcRow + (size_t)cCols * cbBytesPerPixel > cbSrc
            || offDstRow + (size_t)cCols * cbBytesPerPixel > pFrame->cbBuf)
            break;

        const uint8_t *puSrcRow = pu8Src + offSrcRow;
              uint8_t *puDstRow = pFrame->pau8Buf + offDstRow;

        for (uint32_t x = 0; x < cCols; x++)
        {
            uint8_t const *pu8SrcBlend = puSrcRow + cbBytesPerPixel * x;
            uint8_t       *pu8DstBlend = puDstRow + cbBytesPerPixel * x;

            if (pu8SrcBlend[3])
            {
                uint8_t const u8SrcAlpha    = pu8SrcBlend[3];
                uint8_t const u8SrcAlphaInv = 255 - u8SrcAlpha;
                pu8DstBlend[2] = (unsigned char)((u8SrcAlpha * pu8SrcBlend[2] + u8SrcAlphaInv * pu8DstBlend[2]) >> 8); /* R */
                pu8DstBlend[1] = (unsigned char)((u8SrcAlpha * pu8SrcBlend[1] + u8SrcAlphaInv * pu8DstBlend[1]) >> 8); /* G */
                pu8DstBlend[0] = (unsigned char)((u8SrcAlpha * pu8SrcBlend[0] + u8SrcAlphaInv * pu8DstBlend[0]) >> 8); /* B */
                pu8DstBlend[3] = 0xff;                                                                                 /* A */
            }
        }
    }

#if 0
    RecordingUtilsDbgDumpImageData(pu8Src, cbSrc, NULL /* pszPath */, "cursor-src", uSrcX, uSrcY, uSrcWidth, uSrcHeight, uSrcBytesPerLine, 32);
    RecordingUtilsDbgDumpVideoFrameEx(pFrame, NULL /* pszPath */, "cursor-dst");
#endif

    return;
}

/**
 * Simple blitting function for raw image data.
 *
 * @returns VBox status code.
 * @param   pDstFrame           Destination frame.
 * @param   uDstX               X destination (in pixel) within destination frame.
 * @param   uDstY               Y destination (in pixel) within destination frame.
 * @param   pu8Src              Source data to blit. Must be in the same pixel format as \a pFrame.
 * @param   cbSrc               Size (in bytes) of \a pu8Src.
 * @param   uSrcX               X start (in pixel) within source data.
 * @param   uSrcY               Y start (in pixel) within source data.
 * @param   uSrcWidth           Width (in pixel) to blit from source data.
 * @param   uSrcHeight          Height (in pixel) to blit from data.
 * @param   uSrcBytesPerLine    Bytes per line in source data.
 * @param   uSrcBPP             BPP of source data. Must match \a pFrame.
 * @param   enmFmt              Pixel format of source data. Must match \a pFrame.
 */
int RecordingVideoFrameBlitRaw(PRECORDINGVIDEOFRAME pDstFrame, uint32_t uDstX, uint32_t uDstY,
                               const uint8_t *pu8Src, size_t cbSrc, uint32_t uSrcX, uint32_t uSrcY, uint32_t uSrcWidth, uint32_t uSrcHeight,
                               uint32_t uSrcBytesPerLine, uint8_t uSrcBPP, RECORDINGPIXELFMT enmFmt)
{
    return recordingVideoBlitRaw(/* Destination */
                                pDstFrame->pau8Buf, pDstFrame->cbBuf, uDstX, uDstY,
                                pDstFrame->Info.uBytesPerLine, pDstFrame->Info.uBPP, pDstFrame->Info.enmPixelFmt,
                                /* Source */
                                pu8Src, cbSrc, uSrcX, uSrcY, uSrcWidth, uSrcHeight, uSrcBytesPerLine, uSrcBPP, enmFmt);
}

/**
 * Simple blitting function for raw image data with alpha channel.
 *
 * @param   pDstFrame           Destination frame.
 * @param   uDstX               X destination (in pixel) within destination frame.
 * @param   uDstY               Y destination (in pixel) within destination frame.
 * @param   pu8Src              Source data to blit. Must be in the same pixel format as \a pFrame.
 * @param   cbSrc               Size (in bytes) of \a pu8Src.
 * @param   uSrcX               X start (in pixel) within source data.
 * @param   uSrcY               Y start (in pixel) within source data.
 * @param   uSrcWidth           Width (in pixel) to blit from source data.
 * @param   uSrcHeight          Height (in pixel) to blit from data.
 * @param   uSrcBytesPerLine    Bytes per line in source data.
 * @param   uSrcBPP             BPP of source data. Must match \a pFrame.
 * @param   enmFmt              Pixel format of source data. Must match \a pFrame.
 */
void RecordingVideoFrameBlitRawAlpha(PRECORDINGVIDEOFRAME pDstFrame, uint32_t uDstX, uint32_t uDstY,
                                     const uint8_t *pu8Src, size_t cbSrc, uint32_t uSrcX, uint32_t uSrcY, uint32_t uSrcWidth, uint32_t uSrcHeight,
                                     uint32_t uSrcBytesPerLine, uint8_t uSrcBPP, RECORDINGPIXELFMT enmFmt)
{
    recordingVideoFrameBlitRawAlpha(pDstFrame, uDstX, uDstY,
                                    pu8Src, cbSrc, uSrcX, uSrcY, uSrcWidth, uSrcHeight, uSrcBytesPerLine, uSrcBPP, enmFmt);
}

/**
 * Simple blitting function for video frames.
 *
 * @returns VBox status code.
 * @param   pDstFrame           Destination frame.
 * @param   uDstX               X destination (in pixel) within destination frame.
 * @param   uDstY               Y destination (in pixel) within destination frame.
 * @param   pSrcFrame           Source frame.
 * @param   uSrcX               X start (in pixel) within source frame.
 * @param   uSrcY               Y start (in pixel) within source frame.
 * @param   uSrcWidth           Width (in pixel) to blit from source frame.
 * @param   uSrcHeight          Height (in pixel) to blit from frame.
 *
 * @note    Does NOT check for limits, so use with care!
 */
int RecordingVideoFrameBlitFrame(PRECORDINGVIDEOFRAME pDstFrame, uint32_t uDstX, uint32_t uDstY,
                                 PRECORDINGVIDEOFRAME pSrcFrame, uint32_t uSrcX, uint32_t uSrcY, uint32_t uSrcWidth, uint32_t uSrcHeight)
{
    return recordingVideoBlitRaw(/* Dest */
                                 pDstFrame->pau8Buf, pDstFrame->cbBuf, uDstX, uDstY,
                                 pDstFrame->Info.uBytesPerLine, pDstFrame->Info.uBPP, pDstFrame->Info.enmPixelFmt,
                                 /* Source */
                                 pSrcFrame->pau8Buf, pSrcFrame->cbBuf, uSrcX, uSrcY, uSrcWidth, uSrcHeight,
                                 pSrcFrame->Info.uBytesPerLine, pSrcFrame->Info.uBPP, pSrcFrame->Info.enmPixelFmt);
}

#ifdef VBOX_WITH_AUDIO_RECORDING
/**
 * Destroys a recording audio frame.
 *
 * @param   pFrame              Pointer to audio frame to destroy.
 */
DECLINLINE(void) recordingAudioFrameDestroy(PRECORDINGAUDIOFRAME pFrame)
{
    if (!pFrame)
        return;

    if (pFrame->pvBuf)
    {
        Assert(pFrame->cbBuf);
        RTMemFree(pFrame->pvBuf);
        pFrame->pvBuf = NULL;
        pFrame->cbBuf = 0;
    }
}

/**
 * Frees a previously allocated recording audio frame.
 *
 * @param   pFrame              Audio frame to free. The pointer will be invalid after return.
 */
void RecordingAudioFrameFree(PRECORDINGAUDIOFRAME pFrame)
{
    if (!pFrame)
        return;

    recordingAudioFrameDestroy(pFrame);

    RTMemFree(pFrame);
    pFrame = NULL;
}
#endif /* VBOX_WITH_AUDIO_RECORDING */

/**
 * Destroys a recording frame.
 *
 * @param   pFrame              Pointer to recording frame to destroy.
 */
void RecordingFrameDestroy(PRECORDINGFRAME pFrame)
{
    if (!pFrame)
        return;

    AssertMsgReturnVoid(pFrame->cRefs == 0, ("Recording frame still holds references (%RU64)\n", pFrame->cRefs));

    switch (pFrame->enmType)
    {
#ifdef VBOX_WITH_AUDIO_RECORDING
        case RECORDINGFRAME_TYPE_AUDIO:
            recordingAudioFrameDestroy(&pFrame->u.Audio);
            break;
#endif
        case RECORDINGFRAME_TYPE_VIDEO:
            RecordingVideoFrameDestroy(&pFrame->u.Video);
            break;

        case RECORDINGFRAME_TYPE_CURSOR_SHAPE:
            RecordingVideoFrameDestroy(&pFrame->u.CursorShape);
            break;

        default:
            /* Nothing to do here. */
            break;
    }

    RTMemFree(pFrame);
    pFrame = NULL;
}

/**
 * Frees a recording frame.
 *
 * @param   pFrame              Pointer to recording frame to free.
 *                              The pointer will be invalid after return.
 */
void RecordingFrameFree(PRECORDINGFRAME pFrame)
{
    if (!pFrame)
        return;

    RecordingFrameDestroy(pFrame);

    RTMemFree(pFrame);
    pFrame = NULL;
}

/**
 * Returns the current reference count.
 *
 * @returns  Number of current references.
 */
uint64_t RecordingFrameRefs(PRECORDINGFRAME pFrame)
{
    Assert(pFrame->cRefs <= 16); /* Helps finding refcounting bugs. Value chosen at random. */
    return ASMAtomicReadU64(&pFrame->cRefs);
}

/**
 * Adds a reference to a recording block.
 *
 * @returns  Number of new references.
 */
uint64_t RecordingFrameAcquire(PRECORDINGFRAME pFrame)
{
    Assert(pFrame->cRefs <= 16); /* Helps finding refcounting bugs. Value chosen at random. */
    return ASMAtomicIncU64(&pFrame->cRefs);
}

/**
 * Releases a reference to a recording block.
 *
 * @returns  Number of new references after release.
 */
uint64_t RecordingFrameRelease(PRECORDINGFRAME pFrame)
{
    Assert(pFrame->cRefs);
    return ASMAtomicDecU64(&pFrame->cRefs);
}


/*********************************************************************************************************************************
 * Recording Circular Buffer                                                                                                     *
 ********************************************************************************************************************************/
/**
 * Creates a recording buffer.
 *
 * @returns VBox status code.
 * @param   pBuf            Pointer to recording buffer to create.
 * @param   cbSize          Size (in bytes) the recording buffer can store.
 */
int RecordingCircBufCreate(PRECORDINGCIRCBUF pBuf, size_t cbSize)
{
    AssertPtrReturn(pBuf, VERR_INVALID_POINTER);
    AssertReturn(cbSize, VERR_INVALID_PARAMETER);

    int rc = RTCircBufCreate(&pBuf->pCircBuf, cbSize);
    if (RT_FAILURE(rc))
        return rc;

    pBuf->uBasePos  = 0;
    pBuf->uWritePos = 0;
    pBuf->cRdr      = 0;
    RT_ZERO(pBuf->aRdr);

    return VINF_SUCCESS;
}

/**
 * Destroys a recording buffer.
 *
 * @param   pBuf            The recording buffer to destroy.
 */
void RecordingCircBufDestroy(PRECORDINGCIRCBUF pBuf)
{
    if (!pBuf)
        return;

    RTCircBufDestroy(pBuf->pCircBuf);
}

/**
 * Adds a reader.
 *
 * @returns VBox status code.
 * @param   pBuf            The recording buffer.
 * @param   pIdRdr          ID of the added reader. Optional and can be NULL.
 *
 * @note:   New readers start at current write position (won't see old data).
 */
int RecordingCircBufAddReader(PRECORDINGCIRCBUF pBuf, uint32_t *pIdRdr)
{
    uint32_t const id = ASMAtomicReadU32(&pBuf->cRdr);
    uint64_t const uW = ASMAtomicReadU64(&pBuf->uWritePos);
    ASMAtomicWriteU64(&pBuf->aRdr[id].uReadPos, uW);
    if (pIdRdr)
        *pIdRdr = id;
    ASMAtomicIncU32(&pBuf->cRdr);

    return VINF_SUCCESS;
}

/**
 * Removes a reader.
 *
 * @returns VBox status code.
 * @param   pBuf            The recording buffer.
 * @param   idRdr           ID of reader to remove.
 */
int RecordingCircBufRemoveReader(PRECORDINGCIRCBUF pBuf, uint32_t idRdr)
{
    ASMAtomicWriteU64(&pBuf->aRdr[idRdr].uReadPos, 0);
    ASMAtomicDecU32(&pBuf->cRdr);
    return VINF_SUCCESS;
}

/**
 * Computes the minimum consumer read position.
 *
 * @returns Minimum read position among active consumers. If none are active, returns uWriteSnap.
 * @param   pBuf        The recording buffer.
 * @param   uWriteSnap  Snapshot of uWritePos to use.
 *
 * @note Writer-thread only.
 */
DECLINLINE(uint64_t) recCircBufGetMinReadPos(PRECORDINGCIRCBUF pBuf, uint64_t uWriteSnap)
{
    uint64_t       uMin = uWriteSnap;
    uint32_t const cRdr = ASMAtomicReadU32(&pBuf->cRdr);

    for (size_t i = 0; i < cRdr; i++)
        uMin = RT_MIN(uMin, ASMAtomicReadU64(&pBuf->aRdr[i].uReadPos));

    return uMin;
}

/**
 * Advances the underlying RTCircBuf read pointer by @a cb bytes, reclaiming space.
 *
 * @returns VBox status code.
 * @param   pBuf    The recording buffer.
 * @param   cb      Bytes to reclaim.
 *
 * @note Writer-thread only.
 */
DECLINLINE(int) recCircBufReclaim(RECORDINGCIRCBUF *pBuf, size_t cb)
{
    while (cb)
    {
        void  *pv = NULL;
        size_t cbBlk = 0;
        RTCircBufAcquireReadBlock(pBuf->pCircBuf, cb, &pv, &cbBlk);
        RTCircBufReleaseReadBlock(pBuf->pCircBuf, cbBlk);
        ASMAtomicAddU64(&pBuf->uBasePos, cbBlk);
        cb -= cbBlk;
    }
    return VINF_SUCCESS;
}

/**
 * Reclaims underlying RTCircBuf space up to the slowest consumer.
 *
 * @returns VBox status code.
 * @param   pBuf    The recording buffer.
 *
 * @note Writer-thread only.
 */
DECLINLINE(int) recCircBufSyncReclaimWriter(RECORDINGCIRCBUF *pBuf)
{
    uint64_t const uWrite = ASMAtomicReadU64(&pBuf->uWritePos);
    uint64_t const uMin   = recCircBufGetMinReadPos(pBuf, uWrite);
    uint64_t const uBase  = ASMAtomicReadU64(&pBuf->uBasePos);

    if (uMin < uBase)
        AssertFailedReturn(VERR_INTERNAL_ERROR);

    uint64_t const cbAdv64 = uMin - uBase;
    if (!cbAdv64)
        return VINF_SUCCESS;

    return recCircBufReclaim(pBuf, (size_t)cbAdv64);
}

/**
 * Acquires a contiguous write block in the recording buffer.
 *
 * This is non-blocking. If there is no space (because the slowest consumer is
 * holding the buffer full), this returns VERR_TRY_AGAIN.
 *
 * @returns VBox status code.
 * @retval  VINF_SUCCESS on success.
 * @retval  VERR_TRY_AGAIN if no space is currently available.
 *
 * @param   pBuf            The recording buffer.
 * @param   cbReq           Requested bytes.
 * @param   ppv             Where to return pointer to writeable memory.
 * @param   pcbAcquired     Where to return the size of the contiguous block.
 *
 * @note Writer-thread only.
 */
int RecordingCircBufAcquireWrite(PRECORDINGCIRCBUF pBuf, size_t cbReq, void **ppv, size_t *pcbAcquired)
{
    *ppv = NULL;
    *pcbAcquired = 0;

    int rc = recCircBufSyncReclaimWriter(pBuf);
    if (RT_FAILURE(rc))
        return rc;

    void  *pvDst = NULL;
    size_t cbBlk = 0;
    RTCircBufAcquireWriteBlock(pBuf->pCircBuf, cbReq, &pvDst, &cbBlk);
    if (!cbBlk)
        return VERR_TRY_AGAIN;

    *ppv = pvDst;
    *pcbAcquired = cbBlk;
    return VINF_SUCCESS;
}

/**
 * Releases a previously acquired write block, committing @a cbToCommit bytes.
 *
 * @returns VBox status code.
 * @param   pBuf        The recording buffer.
 * @param   cbToCommit  Bytes to commit.
 *
 * @note Writer-thread only.
 */
int RecordingCircBufReleaseWrite(PRECORDINGCIRCBUF pBuf, size_t cbToCommit)
{
    RTCircBufReleaseWriteBlock(pBuf->pCircBuf, cbToCommit);
    ASMAtomicAddU64(&pBuf->uWritePos, cbToCommit);
    return VINF_SUCCESS;
}

/**
 * Acquires a contiguous readable block for a consumer (no copying).
 *
 * This is non-blocking. If no data is available for this consumer, returns VERR_TRY_AGAIN.
 * The returned pointer remains valid until the writer reclaims space past it, which is
 * prevented until all consumers have advanced beyond that data.
 *
 * @returns VBox status code.
 * @retval  VINF_SUCCESS on success.
 * @retval  VERR_TRY_AGAIN if no data is available for this consumer.
 * @retval  VERR_NOT_FOUND if @a id does not exist.
 *
 * @param   pBuf            The recording buffer.
 * @param   idRdr           The reader ID.
 * @param   cbReq           Requested bytes.
 * @param   ppv             Where to return pointer to readable memory.
 * @param   pcbAcquired     Where to return the size of the contiguous block.
 */
int RecordingCircBufAcquireRead(PRECORDINGCIRCBUF pBuf, uint32_t idRdr, size_t cbReq, const void **ppv, size_t *pcbAcquired)
{
    *ppv = NULL;
    *pcbAcquired = 0;

    uint64_t const uW    = ASMAtomicReadU64(&pBuf->uWritePos);
    uint64_t const uR    = ASMAtomicReadU64(&pBuf->aRdr[idRdr].uReadPos);
    uint64_t const uBase = ASMAtomicReadU64(&pBuf->uBasePos);

    if (uR < uBase || uR > uW)
        return VERR_INTERNAL_ERROR;

    size_t const cbAvail = uW - uR;
    if (!cbAvail)
        return VERR_TRY_AGAIN;

    size_t cbWant = cbReq < cbAvail ? cbReq : cbAvail;

    size_t const offPeek = uR - uBase;

    const void *pv = NULL;
    size_t cbBlk;
    RTCircBufPeek(pBuf->pCircBuf, offPeek, cbWant, &pv, &cbBlk);
    if (!cbBlk)
        return VERR_TRY_AGAIN;

    *ppv = pv;
    *pcbAcquired = cbBlk;
    return VINF_SUCCESS;
}

/**
 * Releases a previously acquired read block, consuming @a cbToConsume bytes.
 *
 * @returns VBox status code.
 * @param   pBuf        The recording buffer.
 * @param   idRdr       The reader ID.
 * @param   cbToConsume Bytes to consume.
 */
int RecordingCircBufReleaseRead(PRECORDINGCIRCBUF pBuf, uint32_t idRdr, size_t cbToConsume)
{
    ASMAtomicAddU64(&pBuf->aRdr[idRdr].uReadPos, cbToConsume);
    return VINF_SUCCESS;
}

/**
 * Returns readable bytes for a given reader.
 *
 * @returns Readable bytes for @a idRdr.
 * @param   pBuf        The recording buffer.
 * @param   idRdr       The reader ID.
 */
size_t RecordingCircBufReadable(const PRECORDINGCIRCBUF pBuf, uint32_t idRdr)
{
    uint64_t const uWritePos = ASMAtomicReadU64((volatile uint64_t *)&pBuf->uWritePos);
    uint64_t const uReadPos  = ASMAtomicReadU64((volatile uint64_t *)&pBuf->aRdr[idRdr].uReadPos);
    uint64_t const uBasePos  = ASMAtomicReadU64((volatile uint64_t *)&pBuf->uBasePos);

    if (uReadPos < uBasePos)
        return (size_t)(uWritePos - uBasePos);
    if (uReadPos > uWritePos)
        return 0;
    return (size_t)(uWritePos - uReadPos);
}

/**
 * Returns the currently used size (in bytes).
 *
 * @returns Currently used size (in bytes).
 * @param   pBuf        The recording buffer.
 */
size_t RecordingCircBufUsed(const PRECORDINGCIRCBUF pBuf)
{
    uint64_t const uWritePos = ASMAtomicReadU64((volatile uint64_t *)&pBuf->uWritePos);
    uint64_t const uBasePos  = ASMAtomicReadU64((volatile uint64_t *)&pBuf->uBasePos);

    Assert(uWritePos >= uBasePos);
    return uWritePos - uBasePos;
}

/**
 * Returns the total size (in bytes).
 *
 * @returns Total size (in bytes).
 * @param   pBuf        The recording buffer.
 */
size_t RecordingCircBufSize(PRECORDINGCIRCBUF pBuf)
{
    return RTCircBufSize(pBuf->pCircBuf);
}


/*********************************************************************************************************************************
 * Recording Frame Pool                                                                                                          *
 ********************************************************************************************************************************/

/**
 * Initializes a recording frame pool.
 *
 * @returns VBox status code.
 * @param   pPool               Frame pool to initialize.
 * @param   enmType             Frame type the pool stores.
 * @param   cbFrame             Size (in bytes) of a single frame entry.
 * @param   cCapacity           Number of frames the pool can hold.
 */

int RecordingFramePoolInit(PRECORDINGFRAMEPOOL pPool, RECORDINGFRAME_TYPE enmType, size_t cbFrame, size_t cCapacity)
{
    AssertReturn(enmType != RECORDINGFRAME_TYPE_INVALID, VERR_INVALID_PARAMETER);
    AssertReturn(cbFrame >= sizeof(RECORDINGFRAME), VERR_INVALID_PARAMETER);

    int vrc = RecordingCircBufCreate(&pPool->CircBuf, cbFrame * cCapacity);
    AssertRCReturn(vrc, vrc);

    pPool->uId     = enmType;
    pPool->enmType = enmType;
    pPool->cbFrame = cbFrame;
    pPool->cFrames = cCapacity;

    return vrc;
}

/**
 * Destroys a recording frame pool.
 *
 * @param   pPool               Frame pool to destroy.
 */
void RecordingFramePoolDestroy(PRECORDINGFRAMEPOOL pPool)
{
    if (pPool->enmType != RECORDINGFRAME_TYPE_INVALID)
    {
        RecordingCircBufDestroy(&pPool->CircBuf);

        pPool->enmType = RECORDINGFRAME_TYPE_INVALID;
        pPool->cbFrame = 0;
        pPool->cFrames = 0;
    }
}

/**
 * Adds a frame pool reader.
 *
 * @returns VBox status code.
 * @param   pPool               Frame pool.
 * @param   pIdRdr              Where to return reader ID. Optional.
 */
int  RecordingFramePoolAddReader(PRECORDINGFRAMEPOOL pPool, uint32_t *pIdRdr)
{
    return RecordingCircBufAddReader(&pPool->CircBuf, pIdRdr);
}

/**
 * Removes a frame pool reader.
 *
 * @returns VBox status code.
 * @param   pPool               Frame pool.
 * @param   idRdr               Reader ID to remove.
 */
int  RecordingFramePoolRemoveReader(PRECORDINGFRAMEPOOL pPool, uint32_t idRdr)
{
    return RecordingCircBufRemoveReader(&pPool->CircBuf, idRdr);
}

/**
 * Acquires a readable frame from the pool.
 *
 * @returns Pointer to frame on success, or NULL if none is available.
 * @param   pPool               Frame pool.
 * @param   idRdr               Reader ID.
 */
PRECORDINGFRAME RecordingFramePoolAcquireRead(PRECORDINGFRAMEPOOL pPool, uint32_t idRdr)
{
    const void *pv;
    size_t      cbAcq;
    /* ignore rc */ RecordingCircBufAcquireRead(&pPool->CircBuf, idRdr, pPool->cbFrame, &pv, &cbAcq);
    Assert(!cbAcq || cbAcq == pPool->cbFrame);
    return (PRECORDINGFRAME)pv;
}

/**
 * Releases a previously acquired readable frame.
 *
 * @param   pPool               Frame pool.
 * @param   idRdr               Reader ID.
 */
void RecordingFramePoolReleaseRead(PRECORDINGFRAMEPOOL pPool, uint32_t idRdr)
{
    RecordingCircBufReleaseRead(&pPool->CircBuf, idRdr, pPool->cbFrame);
}

/**
 * Acquires a writable frame slot from the pool.
 *
 * @returns Pointer to writable frame on success, or NULL if full.
 * @param   pPool               Frame pool.
 */
PRECORDINGFRAME RecordingFramePoolAcquireWrite(PRECORDINGFRAMEPOOL pPool)
{
    void   *pv;
    size_t  cbAcq;
    /* ignore rc */ RecordingCircBufAcquireWrite(&pPool->CircBuf, pPool->cbFrame, &pv, &cbAcq);
    Assert(!cbAcq || cbAcq == pPool->cbFrame);
    if (!cbAcq)
        LogRelMax(256, ("Recording: Warning: Frame pool %s full, skipping\n", RecordingUtilsRecordingFrameTypeToStr(pPool->enmType)));
    return (PRECORDINGFRAME)pv;
}

/**
 * Releases a writable frame slot after writing.
 *
 * @param   pPool               Frame pool.
 */
void RecordingFramePoolReleaseWrite(PRECORDINGFRAMEPOOL pPool)
{
    RecordingCircBufReleaseWrite(&pPool->CircBuf, pPool->cbFrame);
}

/**
 * Reclaims freeable slots in a frame pool.
 *
 * @returns VBox status code.
 * @param   pPool               Frame pool.
 *
 * @note Writer-thread only.
 */
int RecordingFramePoolReclaim(PRECORDINGFRAMEPOOL pPool)
{
    return recCircBufSyncReclaimWriter(&pPool->CircBuf);
}

/**
 * Returns whether a frame pool is initialized.
 *
 * @returns @c true if initialized, @c false if not.
 * @param   pPool               Frame pool to check.
 */
bool RecordingFramePoolIsInitialized(PRECORDINGFRAMEPOOL pPool)
{
    return (pPool->enmType != RECORDINGFRAME_TYPE_INVALID);
}

/**
 * Returns the configured frame size of a pool.
 *
 * @returns Frame size (in bytes).
 * @param   pPool               Frame pool.
 */
size_t RecordingFramePoolFrameSize(const PRECORDINGFRAMEPOOL pPool)
{
    return pPool->cbFrame;
}

/**
 * Returns the free frames of a frame pool.
 *
 * @returns Free frames of a frame pool.
 * @param   pPool               Frame pool.
 */
size_t RecordingFramePoolFree(const PRECORDINGFRAMEPOOL pPool)
{
    return pPool->cFrames - (RecordingCircBufUsed(&pPool->CircBuf) / pPool->cbFrame);
}

/**
 * Returns currently readable frames for a given reader.
 *
 * @returns Readable frames for @a idRdr.
 * @param   pPool               Frame pool.
 * @param   idRdr               Reader ID.
 */
size_t RecordingFramePoolReadable(const PRECORDINGFRAMEPOOL pPool, uint32_t idRdr)
{
    return RecordingCircBufReadable(&pPool->CircBuf, idRdr) / pPool->cbFrame;
}

/**
 * Returns currently used frames of a frame pool.
 *
 * This reflects the not-yet-read frames of all connected readers.
 *
 * @returns Used frames.
 * @param   pPool               Frame pool.
 */
size_t RecordingFramePoolUsed(const PRECORDINGFRAMEPOOL pPool)
{
    return RecordingCircBufUsed(&pPool->CircBuf) / pPool->cbFrame;
}

/**
 * Returns total capacity (in frames) of a frame pool.
 *
 * @returns Total frames.
 * @param   pPool               Frame pool.
 */
size_t RecordingFramePoolSize(const PRECORDINGFRAMEPOOL pPool)
{
    return pPool->cFrames /* constant, hence no atomic reading needed */;
}

/**
 * Returns frame pool pressure in percent.
 *
 * @returns Pressure [0..100].
 * @param   pPool               Frame pool.
 * @param   idRdr               Reader ID to use, or
 *                              Use RECORDINGFRAMEPOOL_PRESSURE_READER_ALL for global used occupancy.
 */
uint8_t RecordingFramePoolGetPressure(const RECORDINGFRAMEPOOL *pPool, uint32_t idRdr)
{
    if (!pPool)
        return 0;

    size_t const cCap = pPool->cFrames;
    if (!cCap || !pPool->cbFrame)
        return 0;

    size_t cUsed;
    if (idRdr == RECORDINGFRAMEPOOL_PRESSURE_READER_ALL)
    {
        /* Note: RecordingCircBuf* APIs are not const-correct yet, so cast is intentional here. */
        cUsed = RecordingCircBufUsed((PRECORDINGCIRCBUF)&pPool->CircBuf) / pPool->cbFrame;
    }
    else
    {
        /* Reader-visible backlog (for per-stream pressure decisions). */
        cUsed = RecordingFramePoolReadable((PRECORDINGFRAMEPOOL)pPool, idRdr);
    }

    return (uint8_t)RT_MIN((cUsed * 100) / cCap, (size_t)100);
}


/*********************************************************************************************************************************
 * Recording Stats                                                                                                               *
 ********************************************************************************************************************************/

#ifdef VBOX_WITH_RECORDING_STATS
/**
 * Updates generic pressure statistics from a used/capacity sample.
 */
void RecordingStatsPoolPressureSample(PRECORDINGPOOLPRESSURESTATS pStats, size_t cUsed, size_t cCapacity)
{
    pStats->cSamples++;
    if (!cCapacity)
        return;

    uint64_t const uPct = RT_MIN((uint64_t)(((uint64_t)cUsed * 100) / cCapacity), UINT64_C(100));
    if (uPct >= 50)
        pStats->cGe50++;
    if (uPct >= 75)
        pStats->cGe75++;
    if (uPct >= 90)
        pStats->cGe90++;
    if (uPct >= 100)
        pStats->cGe100++;
    if (uPct > pStats->uPctMax)
        pStats->uPctMax = uPct;
}

/**
 * Logs one pool-pressure statistics block.
 *
 * @param   pszPrefix           Prefix to identify the log context.
 * @param   pszPoolName         Pool name to print.
 * @param   pStats              Pressure statistics to log.
 * @param   cUsed               Current used entries for this pool.
 * @param   cCapacity           Total capacity entries for this pool.
 */
void RecordingStatsPoolPressureLog(const char *pszPrefix, const char *pszPoolName,
                                   const RECORDINGPOOLPRESSURESTATS *pStats,
                                   size_t cUsed, size_t cCapacity)
{
    AssertPtrReturnVoid(pStats);

    uint32_t const uPctCur = cCapacity ? (uint32_t)RT_MIN((cUsed * 100) / cCapacity, (size_t)100) : 0;

    LogRel(("Recording: %s: %s samples=%RU64 ge50=%RU64 ge75=%RU64 ge90=%RU64 ge100=%RU64 max=%RU64%% "
            "cur=%RU32%% (%zu/%zu) drops=%RU64\n",
            pszPrefix, pszPoolName,
            pStats->cSamples,
            pStats->cGe50, pStats->cGe75, pStats->cGe90, pStats->cGe100,
            pStats->uPctMax,
            uPctCur, cUsed, cCapacity,
            pStats->cDrops));
}
#endif /* VBOX_WITH_RECORDING_STATS */

