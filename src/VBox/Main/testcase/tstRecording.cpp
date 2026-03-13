/* $Id: tstRecording.cpp 113380 2026-03-13 10:01:45Z andreas.loeffler@oracle.com $ */
/** @file
 * Recording testcases.
 */

/*
 * Copyright (C) 2024-2026 Oracle and/or its affiliates.
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
#include <iprt/test.h>
#include <iprt/rand.h>

#include <VBox/err.h>

#include "RecordingInternals.h"
#include "RecordingUtils.h"


/** Tests centering / cropped centering of coordinates. */
static void testCenteredCrop(RTTEST hTest)
{
    RTTestSub(hTest, "testCenteredCrop");

    RECORDINGCODECPARMS Parms;

    #define INIT_CROP(/* Framebuffer width / height */   a_fw, a_fh, \
                      /* Video (codec) width / height */ a_vw, a_vh) \
        Parms.u.Video.uWidth  = a_vw; \
        Parms.u.Video.uHeight = a_vh; \
        Parms.u.Video.Scaling.u.Crop.m_iOriginX = int32_t(Parms.u.Video.uWidth  - a_fw) / 2; \
        Parms.u.Video.Scaling.u.Crop.m_iOriginY = int32_t(Parms.u.Video.uHeight - a_fh) / 2;

    #define TEST_CROP(/* Source In  */ a_in_sx, a_in_sy, a_in_sw, a_in_sh, \
                      /* Dest In    */ a_in_dx, a_in_dy, \
                      /* Source Out */ a_out_sx, a_out_sy, a_out_sw, a_out_sh, \
                      /* Dest Out   */ a_out_dx, a_out_dy, out_rc) \
        { \
            int32_t sx = a_in_sx; int32_t sy = a_in_sy; int32_t sw = a_in_sw; int32_t sh = a_in_sh; \
            int32_t dx = a_in_dx; int32_t dy = a_in_dy; \
            RTTEST_CHECK_RC (hTest, RecordingUtilsCoordsCropCenter(&Parms, &sx, &sy, &sw, &sh, &dx, &dy), out_rc); \
            RTTEST_CHECK_MSG(hTest, sx  == a_out_sx, (hTest, "Expected a_out_sx == %RI16, but got %RI16\n", a_out_sx, sx)); \
            RTTEST_CHECK_MSG(hTest, sy  == a_out_sy, (hTest, "Expected a_out_sy == %RI16, but got %RI16\n", a_out_sy, sy)); \
            RTTEST_CHECK_MSG(hTest, sw  == a_out_sw, (hTest, "Expected a_out_sw == %RI16, but got %RI16\n", a_out_sw, sw)); \
            RTTEST_CHECK_MSG(hTest, sh  == a_out_sh, (hTest, "Expected a_out_sh == %RI16, but got %RI16\n", a_out_sh, sh)); \
            RTTEST_CHECK_MSG(hTest, dx  == a_out_dx, (hTest, "Expected a_out_dx == %RI16, but got %RI16\n", a_out_dx, dx)); \
            RTTEST_CHECK_MSG(hTest, dy  == a_out_dy, (hTest, "Expected a_out_dy == %RI16, but got %RI16\n", a_out_dy, dy)); \
        }

    /*
     * No center / cropping performed (framebuffer and video codec are of the same size).
     */
    INIT_CROP(1024, 768, 1024, 768);
    TEST_CROP(0, 0, 1024, 768,
              0, 0,
              0, 0, 1024, 768,
              0, 0, VINF_SUCCESS);
    /* Source is bigger than allowed. */
    TEST_CROP(0, 0, 2048, 1536,
              0, 0,
              0, 0, 1024, 768,
              0, 0, VINF_SUCCESS);
    /* Source is bigger than allowed. */
    TEST_CROP(1024, 768, 2048, 1536,
              0, 0,
              1024, 768, 1024, 768,
              0, 0, VINF_SUCCESS);
    /* Check limits with custom destination. */
    TEST_CROP(0, 0, 1024, 768,
              512, 512,
              0, 0, 512, 256,
              512, 512, VINF_SUCCESS);
    TEST_CROP(512, 512, 1024, 768,
              512, 512,
              512, 512, 512, 256,
              512, 512, VINF_SUCCESS);
    TEST_CROP(512, 512, 1024, 768,
              1024, 768,
              512, 512, 0, 0,
              1024, 768, VWRN_RECORDING_ENCODING_SKIPPED);
    TEST_CROP(1024, 768, 1024, 768,
              1024, 768,
              1024, 768, 0, 0,
              1024, 768, VWRN_RECORDING_ENCODING_SKIPPED);

    /*
     * Framebuffer is twice the size of the video codec -- center crop the framebuffer.
     */
    INIT_CROP(2048, 1536, 1024, 768);
    TEST_CROP(0, 0, 2048, 1536,
              0, 0,
              512, 384, 1024, 768,
              0, 0, VINF_SUCCESS);

    TEST_CROP(1024, 768, 1024, 768,
              0, 0,
              1536, 1152, 512, 384,
              0, 0, VINF_SUCCESS);
    /* Check limits with custom destination. */
    TEST_CROP(1024, 768, 1024, 768,
              512, 384,
              1024, 768, 1024, 768,
              0, 0, VINF_SUCCESS);
    TEST_CROP(1024, 768, 1024, 768,
              512 + 42, 384 + 42,
              1024, 768, 1024 - 42, 768 - 42,
              42, 42, VINF_SUCCESS);
    TEST_CROP(1024, 768, 1024 * 2, 768 * 2,
              512, 384,
              1024, 768, 1024, 768,
              0, 0, VINF_SUCCESS);

    /*
     * Framebuffer is half the size of the video codec -- center (but not crop) the framebuffer within the video output.
     */
    INIT_CROP(1024, 768, 2048, 1536);
    TEST_CROP(0, 0, 1024, 768,
              0, 0,
              0, 0, 1024, 768,
              512, 384, VINF_SUCCESS);

#undef INIT_CROP
#undef TEST_CROP
}

static void tstRecCircBufSingleUse(RTTEST hTest)
{
    RTTestSub(hTest, "RecCircBuf: Single use");

    RECORDINGCIRCBUF Buf;
    RTTESTI_CHECK_RC(RecordingCircBufCreate(&Buf, 64), VINF_SUCCESS);

    uint32_t id = 0;
    RTTESTI_CHECK_RC(RecordingCircBufAddReader(&Buf, &id), VINF_SUCCESS);

    static const uint8_t s_abMsg[] = { 'h','e','l','l','o',0 };

    void  *pvW = NULL;
    size_t cbW = 0;
    RTTESTI_CHECK_RC(RecordingCircBufAcquireWrite(&Buf, sizeof(s_abMsg), &pvW, &cbW), VINF_SUCCESS);
    RTTESTI_CHECK(cbW >= sizeof(s_abMsg));
    memcpy(pvW, s_abMsg, sizeof(s_abMsg));
    RTTESTI_CHECK_RC(RecordingCircBufReleaseWrite(&Buf, sizeof(s_abMsg)), VINF_SUCCESS);

    const void *pvR = NULL;
    size_t cbR = 0;
    RTTESTI_CHECK_RC(RecordingCircBufAcquireRead(&Buf, id, sizeof(s_abMsg), &pvR, &cbR), VINF_SUCCESS);
    RTTESTI_CHECK(cbR >= sizeof(s_abMsg));
    RTTESTI_CHECK(memcmp(pvR, s_abMsg, sizeof(s_abMsg)) == 0);
    RTTESTI_CHECK_RC(RecordingCircBufReleaseRead(&Buf, id, sizeof(s_abMsg)), VINF_SUCCESS);

    RecordingCircBufDestroy(&Buf);
}

static void tstRecCircBufUnderflow(RTTEST hTest)
{
    RTTestSub(hTest, "RecCircBuf: Underflow (no data)");

    RECORDINGCIRCBUF Buf;
    RTTESTI_CHECK_RC(RecordingCircBufCreate(&Buf, 32), VINF_SUCCESS);

    uint32_t id = 0;
    RTTESTI_CHECK_RC(RecordingCircBufAddReader(&Buf, &id), VINF_SUCCESS);

    const void *pvR = (const void *)(uintptr_t)1;
    size_t cbR = 123;
    int rc = RecordingCircBufAcquireRead(&Buf, id, 8, &pvR, &cbR);
    RTTESTI_CHECK_RC(rc, VERR_TRY_AGAIN);
    RTTESTI_CHECK(pvR == NULL);
    RTTESTI_CHECK(cbR == 0);

    RecordingCircBufDestroy(&Buf);
}

static void tstRecCircBufMultiFanoutAndReclaim(RTTEST hTest)
{
    RTTestSub(hTest, "RecCircBuf: Multi reader (fanout + reclaim)");

    RECORDINGCIRCBUF Buf;
    RTTESTI_CHECK_RC(RecordingCircBufCreate(&Buf, 64), VINF_SUCCESS);

    uint32_t idA = 0, idB = 0;
    RTTESTI_CHECK_RC(RecordingCircBufAddReader(&Buf, &idA), VINF_SUCCESS);
    RTTESTI_CHECK_RC(RecordingCircBufAddReader(&Buf, &idB), VINF_SUCCESS);
    RTTESTI_CHECK(idA != idB);

    uint8_t abW[20];
    for (unsigned i = 0; i < sizeof(abW); i++) abW[i] = (uint8_t)i;

    void  *pvW = NULL;
    size_t cbW = 0;
    RTTESTI_CHECK_RC(RecordingCircBufAcquireWrite(&Buf, sizeof(abW), &pvW, &cbW), VINF_SUCCESS);
    RTTESTI_CHECK(cbW >= sizeof(abW));
    memcpy(pvW, abW, sizeof(abW));
    RTTESTI_CHECK_RC(RecordingCircBufReleaseWrite(&Buf, sizeof(abW)), VINF_SUCCESS);

    /* A reads all. */
    const void *pvR = NULL;
    size_t cbR = 0;
    RTTESTI_CHECK_RC(RecordingCircBufAcquireRead(&Buf, idA, sizeof(abW), &pvR, &cbR), VINF_SUCCESS);
    RTTESTI_CHECK(cbR >= sizeof(abW));
    RTTESTI_CHECK(memcmp(pvR, abW, sizeof(abW)) == 0);
    RTTESTI_CHECK_RC(RecordingCircBufReleaseRead(&Buf, idA, sizeof(abW)), VINF_SUCCESS);

    /* B reads (up to) multiple chunks (handles wrap-boundary limiting). */
    uint8_t abB[20];
    size_t cbDone = 0;
    while (cbDone < sizeof(abB))
    {
        RTTESTI_CHECK_RC(RecordingCircBufAcquireRead(&Buf, idB, sizeof(abB) - cbDone, &pvR, &cbR), VINF_SUCCESS);
        RTTESTI_CHECK(cbR > 0);

        size_t cbTake = cbR;
        if (cbTake > sizeof(abB) - cbDone)
            cbTake = sizeof(abB) - cbDone;

        memcpy(&abB[cbDone], pvR, cbTake);
        RTTESTI_CHECK_RC(RecordingCircBufReleaseRead(&Buf, idB, cbTake), VINF_SUCCESS);
        cbDone += cbTake;
    }
    RTTESTI_CHECK(memcmp(abB, abW, sizeof(abW)) == 0);

    /* Underflow for both. */
    int rc = RecordingCircBufAcquireRead(&Buf, idA, 1, &pvR, &cbR);
    RTTESTI_CHECK_RC(rc, VERR_TRY_AGAIN);

    rc = RecordingCircBufAcquireRead(&Buf, idB, 1, &pvR, &cbR);
    RTTESTI_CHECK_RC(rc, VERR_TRY_AGAIN);

    RecordingCircBufDestroy(&Buf);
}

static void tstRecCircBufOverflowSlowReader(RTTEST hTest)
{
    RTTestSub(hTest, "RecCircBuf: Overflow (slow reader prevents reclaim)");

    RECORDINGCIRCBUF Buf;
    RTTESTI_CHECK_RC(RecordingCircBufCreate(&Buf, 8), VINF_SUCCESS);

    uint32_t idFast = 0, idSlow = 0;
    RTTESTI_CHECK_RC(RecordingCircBufAddReader(&Buf, &idFast), VINF_SUCCESS);
    RTTESTI_CHECK_RC(RecordingCircBufAddReader(&Buf, &idSlow), VINF_SUCCESS);
    RTTESTI_CHECK(idFast != idSlow);

    uint8_t abW1[8] = {0,1,2,3,4,5,6,7};

    void  *pvW = NULL;
    size_t cbW = 0;
    RTTESTI_CHECK_RC(RecordingCircBufAcquireWrite(&Buf, sizeof(abW1), &pvW, &cbW), VINF_SUCCESS);
    RTTESTI_CHECK(cbW >= sizeof(abW1));
    memcpy(pvW, abW1, sizeof(abW1));
    RTTESTI_CHECK_RC(RecordingCircBufReleaseWrite(&Buf, sizeof(abW1)), VINF_SUCCESS);

    /* Still full: extra write should fail. */
    int rc = RecordingCircBufAcquireWrite(&Buf, 1, &pvW, &cbW);
    RTTESTI_CHECK_RC(rc, VERR_TRY_AGAIN);

    /* Fast consumes all; slow doesn't => still full. */
    const void *pvR = NULL;
    size_t cbR = 0;
    RTTESTI_CHECK_RC(RecordingCircBufAcquireRead(&Buf, idFast, 8, &pvR, &cbR), VINF_SUCCESS);
    RTTESTI_CHECK(cbR == 8);
    RTTESTI_CHECK(memcmp(pvR, abW1, 8) == 0);
    RTTESTI_CHECK_RC(RecordingCircBufReleaseRead(&Buf, idFast, 8), VINF_SUCCESS);

    rc = RecordingCircBufAcquireWrite(&Buf, 1, &pvW, &cbW);
    RTTESTI_CHECK(pvW == NULL);
    RTTESTI_CHECK(cbW == 0);
    RTTESTI_CHECK_RC(rc, VERR_TRY_AGAIN);

    /* Slow consumes all; next acquire-write can reclaim and succeed. */
    RTTESTI_CHECK_RC(RecordingCircBufAcquireRead(&Buf, idSlow, 8, &pvR, &cbR), VINF_SUCCESS);
    RTTESTI_CHECK(cbR == 8);
    RTTESTI_CHECK(memcmp(pvR, abW1, 8) == 0);
    RTTESTI_CHECK_RC(RecordingCircBufReleaseRead(&Buf, idSlow, 8), VINF_SUCCESS);

    rc = RecordingCircBufAcquireWrite(&Buf, 1, &pvW, &cbW);
    RTTESTI_CHECK_RC(rc, VINF_SUCCESS);
    RTTESTI_CHECK_RETV(cbW == 1);
    *(uint8_t *)pvW = 0xaa;
    RTTESTI_CHECK_RC(RecordingCircBufReleaseWrite(&Buf, 1), VINF_SUCCESS);

    /* Both see the new byte. */
    RTTESTI_CHECK_RC(RecordingCircBufAcquireRead(&Buf, idFast, 1, &pvR, &cbR), VINF_SUCCESS);
    RTTESTI_CHECK(cbR == 1);
    RTTESTI_CHECK(*(uint8_t const *)pvR == 0xaa);
    RTTESTI_CHECK_RC(RecordingCircBufReleaseRead(&Buf, idFast, 1), VINF_SUCCESS);

    RTTESTI_CHECK_RC(RecordingCircBufAcquireRead(&Buf, idSlow, 1, &pvR, &cbR), VINF_SUCCESS);
    RTTESTI_CHECK(cbR == 1);
    RTTESTI_CHECK(*(uint8_t const *)pvR == 0xaa);
    RTTESTI_CHECK_RC(RecordingCircBufReleaseRead(&Buf, idSlow, 1), VINF_SUCCESS);

    RecordingCircBufDestroy(&Buf);
}

static void tstRecCircBufOverflowResolvedByRemovingSlow(RTTEST hTest)
{
    RTTestSub(hTest, "RecCircBuf: Overflow resolved by removing slow reader");

    RECORDINGCIRCBUF Buf;
    RTTESTI_CHECK_RC(RecordingCircBufCreate(&Buf, 8), VINF_SUCCESS);

    uint32_t idFast = 0, idSlow = 0;
    RTTESTI_CHECK_RC(RecordingCircBufAddReader(&Buf, &idFast), VINF_SUCCESS);
    RTTESTI_CHECK_RC(RecordingCircBufAddReader(&Buf, &idSlow), VINF_SUCCESS);

    uint8_t abW1[8] = { 10,11,12,13,14,15,16,17 };

    void  *pvW = NULL;
    size_t cbW = 0;
    RTTESTI_CHECK_RC(RecordingCircBufAcquireWrite(&Buf, 8, &pvW, &cbW), VINF_SUCCESS);
    RTTESTI_CHECK(cbW >= 8);
    memcpy(pvW, abW1, 8);
    RTTESTI_CHECK_RC(RecordingCircBufReleaseWrite(&Buf, 8), VINF_SUCCESS);

    /* Fast consumes all. */
    const void *pvR = NULL;
    size_t cbR = 0;
    RTTESTI_CHECK_RC(RecordingCircBufAcquireRead(&Buf, idFast, 8, &pvR, &cbR), VINF_SUCCESS);
    RTTESTI_CHECK(cbR == 8);
    RTTESTI_CHECK_RC(RecordingCircBufReleaseRead(&Buf, idFast, 8), VINF_SUCCESS);

    /* Still full (slow hasn't consumed). */
    int rc = RecordingCircBufAcquireWrite(&Buf, 1, &pvW, &cbW);
    RTTESTI_CHECK_RC(rc, VERR_TRY_AGAIN);

    /* Remove slow => writer can reclaim and proceed. */
    RTTESTI_CHECK_RC(RecordingCircBufRemoveReader(&Buf, idSlow), VINF_SUCCESS);

    rc = RecordingCircBufAcquireWrite(&Buf, 1, &pvW, &cbW);
    RTTESTI_CHECK_RETV(cbW == 1);
    RTTESTI_CHECK_RC(rc, VINF_SUCCESS);
    *(uint8_t *)pvW = 0xbb;
    RTTESTI_CHECK_RC(RecordingCircBufReleaseWrite(&Buf, 1), VINF_SUCCESS);

    /* Fast reads the new byte. */
    RTTESTI_CHECK_RC(RecordingCircBufAcquireRead(&Buf, idFast, 1, &pvR, &cbR), VINF_SUCCESS);
    RTTESTI_CHECK(cbR == 1);
    RTTESTI_CHECK(*(uint8_t const *)pvR == 0xbb);
    RTTESTI_CHECK_RC(RecordingCircBufReleaseRead(&Buf, idFast, 1), VINF_SUCCESS);

    RecordingCircBufDestroy(&Buf);
}

static void tstRecCircBufRecFrames(RTTEST hTest)
{
    RTTestSub(hTest, "RecCircBuf: Recording frames");

    for (int t = 0; t < 32; t++)
    {
        size_t const cbFrame = sizeof(RECORDINGFRAME) + RTRandU32Ex(0, _4K);
        size_t const cFrames = RTRandU32Ex(1, 1024);

        RTTestPrintf(hTest, RTTESTLVL_ALWAYS, "Testing %zu frames, each %zu bytes (%zu bytes total)\n",
                     cFrames, cbFrame, cFrames * cbFrame);

        RECORDINGCIRCBUF Buf;
        RTTESTI_CHECK_RC_RETV(RecordingCircBufCreate(&Buf, cFrames * cbFrame), VINF_SUCCESS);
        RTTESTI_CHECK((RecordingCircBufSize(&Buf) % cbFrame) == 0); /* Size must be an integral of cbFrame. */
        uint32_t idRdrA, idRdrB;
        RTTESTI_CHECK_RC_RETV(RecordingCircBufAddReader(&Buf, &idRdrA), VINF_SUCCESS);
        RTTESTI_CHECK_RC_RETV(RecordingCircBufAddReader(&Buf, &idRdrB), VINF_SUCCESS);

        size_t cOverwriteOldStuff = RTRandU32Ex(1, 4);

        size_t cToWrite = cFrames * cOverwriteOldStuff;
        size_t cWrittenTotal = 0;
        size_t cToReadA = cFrames * cOverwriteOldStuff;
        size_t cReadTotalA = 0;
        size_t cToReadB = cFrames * cOverwriteOldStuff;
        size_t cReadTotalB = 0;
        while (cToWrite || cToReadA || cToReadB)
        {
            size_t const cCurToWrite = RTRandU32Ex(0, cToWrite);
            RTTestPrintf(hTest, RTTESTLVL_ALWAYS, "Writing %zu frames (cWrittenTotal=%zu)\n", cCurToWrite, cWrittenTotal);
            size_t cWritten = 0;
            for (size_t i = 0; i < cCurToWrite; i++)
            {
                void  *pvW = NULL;
                size_t cbW = 0;
                int rc = RecordingCircBufAcquireWrite(&Buf, cbFrame, &pvW, &cbW);
                if (RT_SUCCESS(rc))
                {
                    RTTESTI_CHECK_MSG_RETV(cbW == cbFrame, ("\tFrame: #%zu: Written %zu, expected %zu\n", i, cbW, cbFrame));
                    RTTESTI_CHECK_RC_RETV(rc, VINF_SUCCESS);
                    RTTESTI_CHECK_RC_RETV(RecordingCircBufReleaseWrite(&Buf, cbW), VINF_SUCCESS);
                    cWritten++;
                }
                else
                {
                    if (rc == VERR_TRY_AGAIN)
                        RTTestPrintf(hTest, RTTESTLVL_ALWAYS, "\tFrame: #%zu: Buffer full, skipping\n", i);
                    break;
                }
            }
            cToWrite -= cWritten;
            cWrittenTotal += cWritten;

            RTTESTI_CHECK((RecordingCircBufUsed(&Buf) % cbFrame) == 0); /* Writes must be an integral of cbFrame. */

            /* Reader A */
            size_t cCurToRead = RTRandU32Ex(0, cToReadA);
                   cCurToRead = RT_MIN(cCurToRead, cWrittenTotal - cReadTotalA);
            size_t cRead = 0;
            for (size_t i = 0; i < cCurToRead; i++)
            {
                const void *pvR;
                size_t      cbR;
                int rc = RecordingCircBufAcquireRead(&Buf, idRdrA, cbFrame, &pvR, &cbR);
                if (RT_SUCCESS(rc))
                {
                    RTTESTI_CHECK_MSG_RETV(cbR == cbFrame, ("\tA: Frame #%zu: Got %zu bytes, expected %zu\n", i, cbR, cbFrame));
                    RecordingCircBufReleaseRead(&Buf, idRdrA, cbR);
                    cRead++;
                }
                else
                {
                    if (rc == VERR_TRY_AGAIN)
                        RTTestPrintf(hTest, RTTESTLVL_ALWAYS, "\tA: Frame: #%zu: Buffer empty, skipping\n", i);
                    break;
                }
            }
            cToReadA -= cRead;
            cReadTotalA += cRead;
            RTTestPrintf(hTest, RTTESTLVL_ALWAYS, "\tA: Read %zu frames (cReadTotal=%zu)\n", cRead, cReadTotalA);

            /* Reader B */
            cCurToRead = RTRandU32Ex(0, cToReadB);
            cCurToRead = RT_MIN(cCurToRead, cWrittenTotal - cReadTotalB);
            cRead      = 0;
            for (size_t i = 0; i < cCurToRead; i++)
            {
                const void *pvR;
                size_t      cbR;
                int rc = RecordingCircBufAcquireRead(&Buf, idRdrB, cbFrame, &pvR, &cbR);
                if (RT_SUCCESS(rc))
                {
                    RTTESTI_CHECK_MSG_RETV(cbR == cbFrame, ("\tB: Frame #%zu: Got %zu bytes, expected %zu\n", i, cbR, cbFrame));
                    RecordingCircBufReleaseRead(&Buf, idRdrB, cbR);
                    cRead++;
                }
                else
                {
                    if (rc == VERR_TRY_AGAIN)
                        RTTestPrintf(hTest, RTTESTLVL_ALWAYS, "\tB: Frame: #%zu: Buffer empty, skipping\n", i);
                    break;
                }
            }
            cToReadB -= cCurToRead;
            cReadTotalB += cCurToRead;
            RTTestPrintf(hTest, RTTESTLVL_ALWAYS, "\tB: Read %zu frames (cReadTotal=%zu)\n", cRead, cReadTotalB);
        }

        RTTESTI_CHECK(cWrittenTotal == cReadTotalA);
        RTTESTI_CHECK(cWrittenTotal == cReadTotalB);
        RecordingCircBufDestroy(&Buf);
    }
}

int main()
{
    RTTEST     hTest;
    RTEXITCODE rcExit = RTTestInitAndCreate("tstRecording", &hTest);
    if (rcExit == RTEXITCODE_SUCCESS)
    {
        RTTestBanner(hTest);

        testCenteredCrop(hTest);

        tstRecCircBufSingleUse(hTest);
        tstRecCircBufUnderflow(hTest);
        tstRecCircBufMultiFanoutAndReclaim(hTest);
        tstRecCircBufOverflowSlowReader(hTest);
        tstRecCircBufOverflowResolvedByRemovingSlow(hTest);
        tstRecCircBufRecFrames(hTest);

        rcExit = RTTestSummaryAndDestroy(hTest);
    }
    return rcExit;
}

