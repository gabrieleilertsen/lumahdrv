/**
 * \class MkvInterface
 *
 * \brief Reading/writing of Matroska HDR videos.
 *
 * MkvInterface provides a interface for reading and writing of encoded HDR
 * video using the Matroska container. In addition to the encoded HDR video 
 * stream, the container also stores meta data needed for decoding.
 *
 *
 * This file is part of the LumaHDRv package.
 * -----------------------------------------------------------------------------
 * Copyright (c) 2015, The LumaHDRv authors.
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice, 
 *    this list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 
 * 3. Neither the name of the copyright holder nor the names of its contributors
 *    may be used to endorse or promote products derived from this software 
 *    without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE 
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
 * POSSIBILITY OF SUCH DAMAGE.
 * -----------------------------------------------------------------------------
 *
 * \author Gabriel Eilertsen, gabriel.eilertsen@liu.se
 *
 * \date Oct 7 2015
 */

#ifndef MKV_INTERFACE_H
#define MKV_INTERFACE_H

#include "ebml/StdIOCallback.h"

#include "ebml/EbmlHead.h"
#include "ebml/EbmlSubHead.h"
#include "ebml/EbmlVoid.h"
#include "matroska/FileKax.h"
#include "matroska/KaxSegment.h"
#include "matroska/KaxTracks.h"
#include "matroska/KaxCluster.h"
#include "matroska/KaxSeekHead.h"
#include "matroska/KaxCues.h"
#include "matroska/KaxInfoData.h"
#include "matroska/KaxVersion.h"

#include "ebml/EbmlStream.h"
#include "ebml/EbmlContexts.h"
#include "ebml/EbmlCrc32.h"
#include "matroska/KaxContexts.h"
#include "matroska/KaxBlockData.h"
#include "matroska/KaxCuesData.h"

#define TIMECODE_SCALE 1000000

using namespace LIBMATROSKA_NAMESPACE;

class MkvInterface
{
public:
    MkvInterface();
    ~MkvInterface();
    
    void openWrite(const char *outputFile, const unsigned int w, const unsigned int h, const float maxL, const float minL);
    void openRead(const char *inputFile);
    void close();
    void addAttachment(unsigned int uid, const binary* buffer, unsigned int buffer_size, const char* description = "--");
    void addFrame(const uint8 *frame_buffer, unsigned int buffer_size, bool isKey = true);
    bool readFrame();
    bool getAttachment(unsigned int ind, binary** buffer, unsigned int &id, unsigned int &buffer_size);
    void writeAttachments();
    const uint8 *getFrame(unsigned int & buffer_size);
    bool seekToTime(float tm, bool absolute = false);
    
    void setFramerate(float fps) { m_frameDuration = 1000.0f/fps; }
    void setVerbose(bool verbose) { m_verbose = verbose; }
    int getCurrentTime() { return m_currentTime; }
    int getFrameDuration() { return m_frameDuration; }
    int getDuration() { return m_duration; }
    
private:
    void flushCluster();
    bool findCluster();
    bool findBlockGroup();
    
    void handleTracksData();
    void handleAttachments();
    void handleSeekHead();
    void handleInfo();
    void handleCueData();
    
    bool m_writeMode;
    bool m_verbose;
    
    StdIOCallback *m_file;
    KaxTrackEntry *m_track;
    KaxSegment m_fileSegment;
    KaxSeekHead *m_metaSeek;
    KaxCues *m_cues;
    EbmlVoid *m_dummy;
    KaxAttachments *m_attachments;
    KaxAttached *m_prevAttachment;
    
    KaxCluster *m_cluster;
    KaxBlockGroup *m_blockGroup, *m_blockGroupPrev;
    
    std::vector<uint8*> m_frameBuffer;
    
    uint64 m_filePosition;
    uint64 m_elementPosition;
    uint64 m_cuePosition;
    
    bool m_writeDefaultValues;
    unsigned int m_frameCount;
    float m_frameDuration;
    float m_duration;
    float m_timecode;
    
    std::vector<binary*> m_attachmentBuffer;
    std::vector<unsigned int> m_attachmentID;
    std::vector<unsigned int> m_attachmentSize;
    std::vector<uint64> m_timeStamps;
    std::vector<uint64> m_keyPositions;
    
    EbmlStream *aStream;
    EbmlElement *m_element0;
    EbmlElement *m_element1;
    EbmlElement *m_element2a;
    EbmlElement *m_element2b;
    EbmlElement *m_element3a;
    EbmlElement *m_element3b;
    EbmlElement *m_element4a;
    EbmlElement *m_element4b;
    int m_upperElementa;
    int m_upperElementb;
    bool m_allowDummy;
    
    int m_trackNr, m_trackUID, m_currentTime;
};

#endif //MKV_INTERFACE_H
