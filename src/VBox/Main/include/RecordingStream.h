/* $Id: RecordingStream.h 113380 2026-03-13 10:01:45Z andreas.loeffler@oracle.com $ */
/** @file
 * Recording stream code header.
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

#ifndef MAIN_INCLUDED_RecordingStream_h
#define MAIN_INCLUDED_RecordingStream_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#ifdef IN_VBOXSVC
# error "Using RecordingStream.h is prohibited in VBoxSVC!"
#endif

#include <map>
#include <vector>

#include <iprt/types.h>

#include <iprt/circbuf.h>
#include <iprt/critsect.h>
#include <iprt/req.h>
#include <iprt/semaphore.h>

#ifdef LOG_GROUP
# undef LOG_GROUP
#endif
#define LOG_GROUP LOG_GROUP_RECORDING
#include "LoggingNew.h"

#include "RecordingInternals.h"

class Console;
class WebMWriter;
class RecordingContext;

/**
 * Class for managing a recording stream.
 *
 * A recording stream represents one entity to record (e.g. on screen / monitor),
 * so there is a 1:1 mapping (stream <-> monitors).
 */
class RecordingStream
{
    struct RECORDINGCMD;

public:

    RecordingStream(RecordingContext *pCtx, uint32_t uScreen, const ComPtr<IRecordingScreenSettings> &ScreenSettings, PRECORDINGFRAMEPOOL paPoolsCommonEnc);

    virtual ~RecordingStream(void);

    int InitVideo(const ComPtr<IRecordingScreenSettings> &ScreenSettings);
#ifdef VBOX_WITH_AUDIO_RECORDING
    int InitAudio(const ComPtr<IRecordingScreenSettings> &ScreenSettings, PRECORDINGCODEC pCodecAudio);
#endif

public:

    static DECLCALLBACK(int) Thread(RTTHREAD hThreadSelf, void *pvUser);

    int ThreadWorker(int rcWait, RTMSINTERVAL msTimeout);
    int Notify(void);
    int SendAudioFrame(const void *pvData, size_t cbData, uint64_t msTimestamp);
    int SendCursorPos(uint8_t idCursor, PRECORDINGPOS pPos, uint64_t msTimestamp);
    int SendCursorShape(uint8_t idCursor, PRECORDINGVIDEOFRAME pShape, uint64_t msTimestamp);
    int SendVideoFrame(PRECORDINGVIDEOFRAME pFrame, uint64_t msTimestamp);
    int SendScreenChange(PRECORDINGSURFACEINFO pInfo, uint64_t msTimestamp, bool fForce = false);

    int Start(void);
    int Stop(void);

    const ComPtr<IRecordingScreenSettings> &GetSettings(void) const;
    uint16_t GetID(void) const { return this->m_uScreenID; };
#ifdef VBOX_WITH_AUDIO_RECORDING
    PRECORDINGCODEC GetAudioCodec(void) { return this->m_pCodecAudio; };
#endif
    PRECORDINGCODEC GetVideoCodec(void) { return &this->m_CodecVideo; };

    bool IsLimitReached(uint64_t msTimestamp) const;
    bool IsFeatureEnabled(RecordingFeature_T enmFeature) const;
    bool NeedsUpdate(uint64_t msTimestamp) const;

public:

    static DECLCALLBACK(int) codecWriteDataCallback(PRECORDINGCODEC pCodec, const void *pvData, size_t cbData, uint64_t msAbsPTS, uint32_t uFlags, void *pvUser);

protected:

    int open(const ComPtr<IRecordingScreenSettings> &ScreenSettings);
    int close(void);

    int initCommon(RecordingContext *pCtx, uint32_t uScreen, const ComPtr<IRecordingScreenSettings> &ScreenSettings, PRECORDINGFRAMEPOOL paPoolsCommonEnc);
    int uninitCommon(void);

    int uninitVideo(void);
#ifdef VBOX_WITH_AUDIO_RECORDING
    int uninitAudio(void);
#endif

#ifdef VBOX_WITH_RECORDING_STATS
    void statsVideoOnFrameQueued(PRECORDINGVIDEOFRAME pVideoFrame);
    void statsVideoLogBuckets(const char *pszPrefix) const;
#endif

    inline bool isLimitReachedInternal(uint64_t msTimestamp) const;
    inline bool isVideoEnabled(void) const;
    inline bool isAudioEnabled(void) const;

    int iterateInternal(uint64_t msTimestamp);

    int cmdQueueAddFrame(PRECORDINGFRAME pFrame, PRECORDINGFRAMEPOOL pPool, RECORDINGCMD *pCmd);
    int cmdQueueAddFrame(PRECORDINGFRAME pFrame, PRECORDINGFRAMEPOOL pPool);
    uint8_t cmdQueueGetPressure(void) const;

    struct RECORDINGPKT;
    RECORDINGPKT &packetLookupOrCreate(uint64_t msTimestamp, size_t cFramesExpected);

    int videoEnqueueFrame(PRECORDINGVIDEOFRAME pVideoFrame, uint64_t msTimestamp);

    int process(RTMSINTERVAL msTimeout);
    int codecWriteToWebM(PRECORDINGCODEC pCodec, const void *pvData, size_t cbData, uint64_t msAbsPTS, uint32_t uFlags);

    bool shouldDropFrame(RECORDINGFRAME_TYPE enmType) const;

    void lock(void);
    void unlock(void);

protected:

    /**
     * Enumeration for a recording stream state.
     */
    enum RECORDINGSTREAMSTATE
    {
        /** Stream not initialized. */
        RECORDINGSTREAMSTATE_UNINITIALIZED = 0,
        /** Stream was initialized. */
        RECORDINGSTREAMSTATE_INITIALIZED   = 1,
        /** Stream was started (recording active). */
        RECORDINGSTREAMSTATE_STARTED       = 2,
        /** Stream is in paused state. */
        RECORDINGSTREAMSTATE_PAUSED        = 3,
        /** Stream is stopping. */
        RECORDINGSTREAMSTATE_STOPPING      = 4,
        /** Stream has been stopped (non-continuable). */
        RECORDINGSTREAMSTATE_STOPPED       = 5,
        /** The usual 32-bit hack. */
        RECORDINGSTREAMSTATE_32BIT_HACK    = 0x7fffffff
    };

    /**
     * Structure for maintaining a recording command.
     */
    typedef struct RECORDINGCMD
    {
        /** Timestamp (PTS, in ms). */
        uint64_t msTimestamp;
        /** Pool index (frame type). */
        uint8_t  idxPool;
    } RECORDINGCMD;
    /** Pointer to a RECORDINGCMD struct. */
    typedef RECORDINGCMD *PRECORDINGCMD;

    /**
     * Structure for maintaining a recording packet.
     *
     * A recording packet contains 2 or more frames with the same timestamp.
     */
    typedef struct RECORDINGPKT
    {
        /** Monotonically increasing paket ID. */
        uint64_t                       idPkt;
        /** Packet timestamp (PTS, in ms). */
        uint64_t                       msTimestamp;
        /** Timestamp when first member was seen. */
        uint64_t                       tsFirstSeenMs;
        /** Expected number of frames in this packet. */
        size_t                         cExpected;
        /** Number of frames seen so far. */
        size_t                         cSeen;
    } RECORDINGPKT;
    /** Pointer to a RECORDINGPKT struct. */
    typedef RECORDINGPKT *PRECORDINGPKT;

    /** Pointer (weak) to console object.
     *  Needed for STAM. */
    Console * const         m_pConsole;
    /** Recording context this stream is associated to. */
    RecordingContext       *m_pCtx;
    /** The current state. */
    RECORDINGSTREAMSTATE    m_enmState;
    struct
    {
        /** File handle to use for writing. */
        RTFILE              m_hFile;
        /** Pointer to WebM writer instance being used. */
        WebMWriter         *m_pWEBM;
    } File;
    /** Track number of audio stream.
     *  Set to UINT8_MAX if not being used. */
    uint8_t             m_uTrackAudio;
    /** Track number of video stream.
     *  Set to UINT8_MAX if not being used. */
    uint8_t             m_uTrackVideo;
    /** Screen ID. */
    uint16_t            m_uScreenID;
    /** Critical section to serialize access. */
    RTCRITSECT          m_CritSect;
    /** Semaphore to signal this stream's worker thread. */
    RTSEMEVENT          m_WaitEvent;
    /** Stream worker thread. */
    RTTHREAD            m_Thread;
    /** Shutdown indicator for stream worker thread. */
    bool                m_fShutdown;
    /** Timestamp (in ms) of when recording has been started. */
    uint64_t            m_tsStartMs;
#ifdef VBOX_WITH_AUDIO_RECORDING
    /** Pointer to audio codec instance data to use.
     *
     *  We multiplex audio data from the recording context to all streams,
     *  to avoid encoding the same audio data for each stream. We ASSUME that
     *  all audio data of a VM will be the same for each stream at a given
     *  point in time.
     *
     *  Might be NULL if not being used. */
    PRECORDINGCODEC     m_pCodecAudio;
    /** Pointer to the recording context's commonly encoded frame pools.
     *  Might be NULL if not being used. */
    PRECORDINGFRAMEPOOL m_paPoolsCommonEnc;
#endif /* VBOX_WITH_AUDIO_RECORDING */
#ifdef VBOX_WITH_STATISTICS
    /** STAM values. */
    struct
    {
        STAMCOUNTER     cVideoFramesAdded;
        STAMCOUNTER     cVideoFramesToEncode;
        STAMCOUNTER     cVideoFramesEncoded;
        STAMCOUNTER     cVideoFramesSkipped;
# ifdef VBOX_WITH_AUDIO_RECORDING
        STAMCOUNTER     cAudioFramesAdded;
        /* Note: STAM values for frames to encode / encoded / housekeeping
                 will be handled in the recording context, as this is common data
                 which needs to be multiplexed for now. */
        STAMCOUNTER     cAudioFramesToEncode;
        STAMCOUNTER     cAudioFramesEncoded;
        STAMPROFILE     profileFnProcessAudio;
# endif
        STAMPROFILE     profileFnProcessTotal;
        STAMPROFILE     profileFnProcessVideo;
    } m_STAM;
#endif /* VBOX_WITH_STATISTICS */
    /** Video codec instance data to use. */
    RECORDINGCODEC      m_CodecVideo;
    /** Screen settings to use. */
    ComPtr<IRecordingScreenSettings>
                        m_Settings;
    /** Video-specific runtime data. */
    struct
    {
        /** Current surface screen info being used.
         *  Can be changed by a SendScreenChange() call. */
        RECORDINGSURFACEINFO ScreenInfo;
#ifdef VBOX_WITH_RECORDING_STATS
        /** Internal, non-STAM video stats. */
        struct
        {
            /** Total amount of dirty video bytes queued so far (sum of pVideoFrame->cbBuf). */
            uint64_t                   cbDirtyTotal;
            /** Approximate number of video frames currently pending in this stream's queue path. */
            uint64_t                   cPendingVideo;
            /** Dirty-size distribution buckets (frame count):
             *  [0]=<=16K, [1]=<=64K, [2]=<=256K, [3]=<=1M, [4]=<=4M, [5]=>4M. */
            uint64_t                   acDirtyBuckets[6];
            /** Generic video-pool pressure statistics (samples / >=50/75/90/100 / max). */
            RECORDINGPOOLPRESSURESTATS PoolPressure;
            /** Maximum observed command-queue occupancy percentage [0..100]. */
            uint64_t                   uCmdQPctMax;
        } Stats;
#endif
    } m_Video;
    /** Cached (const) settings from IRecordingScreen. Same naming (minus notation).
     *  Kept around for speed reasons during runtime. */
    struct
    {
        BOOL                   fEnabled;
        RecordingFeatureMap    mapFeatures;
        RecordingDestination_T enmDestination;
        ULONG                  uMaxTime;
        ULONG                  uMaxFileSize;
    } m_SettingsCache;
    /** The stream's frame pools.
     *  Used to keep (re-)allocation of passed-in frames. */
#ifdef DEBUG
    public:
#endif
    RECORDINGFRAMEPOOL  m_aFramePool[RECORDINGFRAME_TYPE_MAX];
    /** The command pool.
     *  It holds RECORDINGCMD entries to know which pool / timestamp / group to process next. */
    PRTCIRCBUF          m_pCmdCircBuf;
    /** Next packet ID.
     *  Monotonically increasing. */
    uint64_t            m_uPacketNext;
    /** Deferred packet map.
     *  Key is the timecode (PTS, in ms).
     *  Used by stream worker only. */
    std::map<uint64_t, RECORDINGPKT>
                        m_mapPkts;
};

/** Vector of recording streams. */
typedef std::vector <RecordingStream *> RecordingStreams;

#endif /* !MAIN_INCLUDED_RecordingStream_h */

