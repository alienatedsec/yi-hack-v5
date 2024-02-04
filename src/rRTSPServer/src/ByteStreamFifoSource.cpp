/**********
This library is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the
Free Software Foundation; either version 3 of the License, or (at your
option) any later version. (See <http://www.gnu.org/copyleft/lesser.html>.)

This library is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
more details.

You should have received a copy of the GNU Lesser General Public License
along with this library; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
**********/
// "liveMedia"
// Copyright (c) 1996-2023 Live Networks, Inc.  All rights reserved.
// A file source that is a plain byte stream (rather than frames)
// Implementation

#include "ByteStreamFifoSource.hh"
#include "presentationTime.hh"
#include "InputFile.hh"
#include "GroupsockHelper.hh"

#include <fcntl.h>

////////// ByteStreamFifoSource //////////

extern int debug;

ByteStreamFifoSource*
ByteStreamFifoSource::createNew(UsageEnvironment& env, char const* fileName,
                                unsigned preferredFrameSize,
                                unsigned playTimePerFrame) {
    int flags;
    unsigned char null[4];
    FILE* fid = OpenInputFile(env, fileName);
    if (fid == NULL) return NULL;

    // Set non blocking
    if ((flags = fcntl(fileno(fid), F_GETFL, 0)) < 0) {
        fclose(fid);
        return NULL;
    };
    if (fcntl(fileno(fid), F_SETFL, flags | O_NONBLOCK) != 0) {
        fclose(fid);
        return NULL;
    };

    // Clean fifo content
    while (fread(null, 1, sizeof(null), fid) > 0) {}

    // Restore old blocking
    if (fcntl(fileno(fid), F_SETFL, flags) != 0) {
        fclose(fid);
        return NULL;
    };

    ByteStreamFifoSource* newSource
        = new ByteStreamFifoSource(env, fid, preferredFrameSize, playTimePerFrame);

    return newSource;
}

ByteStreamFifoSource*
ByteStreamFifoSource::createNew(UsageEnvironment& env, FILE* fid,
                                unsigned preferredFrameSize,
                                unsigned playTimePerFrame) {
    if (fid == NULL) return NULL;

    ByteStreamFifoSource* newSource = new ByteStreamFifoSource(env, fid, preferredFrameSize, playTimePerFrame);

    return newSource;
}

ByteStreamFifoSource::ByteStreamFifoSource(UsageEnvironment& env, FILE* fid,
                                           unsigned preferredFrameSize,
                                           unsigned playTimePerFrame)
    : FramedFileSource(env, fid), fFileSize(0), fPreferredFrameSize(preferredFrameSize),
      fPlayTimePerFrame(playTimePerFrame), fLastPlayTime(0),
      fHaveStartedReading(False), fLimitNumBytesToStream(False), fNumBytesToStream(0) {
#ifndef READ_FROM_FILES_SYNCHRONOUSLY
    makeSocketNonBlocking(fileno(fFid));
#endif
}

ByteStreamFifoSource::~ByteStreamFifoSource() {
    if (fFid == NULL) return;

#ifndef READ_FROM_FILES_SYNCHRONOUSLY
    envir().taskScheduler().disableBackgroundHandling(fileno(fFid));
#endif

    CloseInputFile(fFid);
}

void ByteStreamFifoSource::doGetNextFrame() {
    if (feof(fFid) || (fLimitNumBytesToStream && fNumBytesToStream == 0)) {
        handleClosure();
        return;
    }

#ifdef READ_FROM_FILES_SYNCHRONOUSLY
    doReadFromFile();
#else
    if (!fHaveStartedReading) {
        // Await readable data from the file:
        envir().taskScheduler().setBackgroundHandling(fileno(fFid), SOCKET_READABLE,
               (TaskScheduler::BackgroundHandlerProc*)&fileReadableHandler, this);
        fHaveStartedReading = True;
    }
#endif
}

void ByteStreamFifoSource::doStopGettingFrames() {
    envir().taskScheduler().unscheduleDelayedTask(nextTask());
#ifndef READ_FROM_FILES_SYNCHRONOUSLY
    envir().taskScheduler().disableBackgroundHandling(fileno(fFid));
    fHaveStartedReading = False;
#endif
}

void ByteStreamFifoSource::fileReadableHandler(ByteStreamFifoSource* source, int /*mask*/) {
    if (!source->isCurrentlyAwaitingData()) {
      source->doStopGettingFrames(); // we're not ready for the data yet
      return;
    }
    source->doReadFromFile();
}

void ByteStreamFifoSource::doReadFromFile() {
    // Try to read as many bytes as will fit in the buffer provided (or "fPreferredFrameSize" if less)
    if (fLimitNumBytesToStream && fNumBytesToStream < (u_int64_t)fMaxSize) {
        fMaxSize = (unsigned)fNumBytesToStream;
    }
    if (fPreferredFrameSize > 0 && fPreferredFrameSize < fMaxSize) {
        fMaxSize = fPreferredFrameSize;
    }
#ifdef READ_FROM_FILES_SYNCHRONOUSLY
    fFrameSize = fread(fTo, 1, fMaxSize, fFid);
#else
    fFrameSize = read(fileno(fFid), fTo, fMaxSize);
#endif
    if (fFrameSize == 0) {
        handleClosure();
        return;
    }
    fNumBytesToStream -= fFrameSize;

    // Set the 'presentation time':
    if (fPlayTimePerFrame > 0 && fPreferredFrameSize > 0) {
        if (fPresentationTime.tv_sec == 0 && fPresentationTime.tv_usec == 0) {
            // This is the first frame, so use the current time:
            gettimeofday(&fPresentationTime, NULL);
        } else {
#ifndef PRES_TIME_CLOCK
            // Increment by the play time of the previous data:
            unsigned uSeconds	= fPresentationTime.tv_usec + fLastPlayTime;
            fPresentationTime.tv_sec += uSeconds/1000000;
            fPresentationTime.tv_usec = uSeconds%1000000;
#else
            // Use system clock to set presentation time
            gettimeofday(&fPresentationTime, NULL);
#endif
        }

        // Remember the play time of this data:
        fLastPlayTime = (fPlayTimePerFrame*fFrameSize)/fPreferredFrameSize;
        fDurationInMicroseconds = fLastPlayTime;
    } else {
        // We don't know a specific play time duration for this data,
        // so just record the current time as being the 'presentation time':
        gettimeofday(&fPresentationTime, NULL);
        fDurationInMicroseconds = fPlayTimePerFrame;
    }
    if (debug & 4) fprintf(stderr, "h264 frame - fPresentationTime, sec = %ld, usec = %ld\n", fPresentationTime.tv_sec, fPresentationTime.tv_usec);

    // Inform the reader that he has data:
#ifdef READ_FROM_FILES_SYNCHRONOUSLY
    // To avoid possible infinite recursion, we need to return to the event loop to do this:
    nextTask() = envir().taskScheduler().scheduleDelayedTask(0,
                                (TaskFunc*)FramedSource::afterGetting, this);
#else
    // Because the file read was done from the event loop, we can call the
    // 'after getting' function directly, without risk of infinite recursion:
    FramedSource::afterGetting(this);
#endif
}
