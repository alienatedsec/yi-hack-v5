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
// Copyright (c) 1996-2022 Live Networks, Inc.  All rights reserved.
// A source object for AAC audio files in ADTS format
// C++ header

#ifndef _ADTS_AUDIO_FIFO_SOURCE_HH
#define _ADTS_AUDIO_FIFO_SOURCE_HH

#ifndef _FRAMED_FILE_SOURCE_HH
#include "FramedFileSource.hh"
#endif

class ADTSAudioFifoSource: public FramedFileSource {
public:
  static ADTSAudioFifoSource* createNew(UsageEnvironment& env,
				       char const* fileName);

  unsigned samplingFrequency() const { return fSamplingFrequency; }
  unsigned numChannels() const { return fNumChannels; }
  char const* configStr() const { return fConfigStr; }
      // returns the 'AudioSpecificConfig' for this stream (in ASCII form)

private:
  ADTSAudioFifoSource(UsageEnvironment& env, FILE* fid, u_int8_t profile,
		      u_int8_t samplingFrequencyIndex, u_int8_t channelConfiguration);
	// called only by createNew()

  virtual ~ADTSAudioFifoSource();

private:
  // redefined virtual functions:
  virtual void doGetNextFrame();

private:
  unsigned fSamplingFrequency;
  unsigned fNumChannels;
  unsigned fuSecsPerFrame;
  char fConfigStr[5];
};

#endif
