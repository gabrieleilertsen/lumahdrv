/**
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

#include "mkv_interface.h"
#include "luma_exception.h"

MkvInterface::MkvInterface()
{
    m_writeDefaultValues = false;
    m_frameCount = 0;
    m_frameDuration = 40.0f;
    m_duration = 0.0f;
    m_file = NULL;
    m_currentTime = 0;
    
    m_attachments = NULL;
    m_cues = NULL;
    m_metaSeek = NULL;
    m_dummy = NULL;
    m_cluster = NULL;
    m_blockGroup = m_blockGroupPrev = NULL;
    
    m_upperElementa = m_upperElementb = 0;
    m_allowDummy = true;
    aStream = NULL;
    m_element0 = m_element1 = m_element2a = m_element2b = m_element3a = m_element3b = m_element4a = m_element4b = NULL;
    
    m_trackNr = m_trackUID = -1;
    m_filePosition = m_elementPosition = 0;
    
    m_timecode = 0;
    
    m_verbose = 0;
}

MkvInterface::~MkvInterface()
{
    if (m_file != NULL)
        close();
    
    if (aStream != NULL)
        delete aStream;
    aStream = NULL;
    
    for (size_t i=0; i<m_attachmentBuffer.size(); i++)
        delete[] m_attachmentBuffer.at(i);
    m_attachmentBuffer.clear();
}

void MkvInterface::openWrite(const char *outputFile, const unsigned int w, const unsigned int h)
{
    m_writeMode = 1;
    
    try
    {
        m_metaSeek = &GetChild<KaxSeekHead>(m_fileSegment);
        
        m_file = new StdIOCallback(outputFile, MODE_CREATE);

        // EBML head
        EbmlHead FileHead;
        *static_cast<EbmlUInteger *>(&GetChild<EVersion>(FileHead)) = 1;
        *static_cast<EbmlUInteger *>(&GetChild<EReadVersion>(FileHead)) = 1;
        *static_cast<EbmlUInteger *>(&GetChild<EMaxIdLength>(FileHead)) = 4;
        *static_cast<EbmlUInteger *>(&GetChild<EMaxSizeLength>(FileHead)) = 8;
        *static_cast<EbmlString *>(&GetChild<EDocType>(FileHead)) = "matroska";
        *static_cast<EbmlUInteger *>(&GetChild<EDocTypeVersion>(FileHead)) = 4;
        *static_cast<EbmlUInteger *>(&GetChild<EDocTypeReadVersion>(FileHead)) = 2;
        FileHead.Render(*m_file, m_writeDefaultValues);

        // size is unknown and will always be, we can render it right away
        m_fileSegment.WriteHead(*m_file, 5, m_writeDefaultValues);
        
        // Tracks
        KaxTracks & MyTracks = GetChild<KaxTracks>(m_fileSegment);

        // reserve some space for the Meta Seek written at the end
        m_dummy = &GetChild<EbmlVoid>(m_fileSegment);
        m_dummy->SetSize(4096); // in octets
        m_dummy->Render(*m_file, m_writeDefaultValues);

        // fill the mandatory Info section -----------------------------------------
        KaxInfo & MyInfos = GetChild<KaxInfo>(m_fileSegment);
        
        *static_cast<EbmlUInteger *>(&GetChild<KaxTimecodeScale>(MyInfos)) = TIMECODE_SCALE;
        *static_cast<EbmlFloat *>(&GetChild<KaxDuration>(MyInfos)) = 1000.0;
        UTFstring str;
        str.SetUTF8(outputFile);
		*static_cast<EbmlUnicodeString *>(&GetChild<KaxSegmentFilename>(MyInfos)) = str.c_str();
		
        str.SetUTF8(std::string("libebml v") + EbmlCodeVersion + std::string(" + libmatroska v") + KaxCodeVersion);
        *static_cast<EbmlUnicodeString *>(&GetChild<KaxMuxingApp>(MyInfos))  = str.c_str();
        *static_cast<EbmlUnicodeString *>(&GetChild<KaxWritingApp>(MyInfos)) = L"luma mkv_interface";
        GetChild<KaxWritingApp>(MyInfos).SetDefaultSize(128);
        MyInfos.Render(*m_file, m_writeDefaultValues);
        m_metaSeek->IndexThis(MyInfos, m_fileSegment);

        // fill track 1 params -----------------------------------------------------
        m_track = &GetChild<KaxTrackEntry>(MyTracks);
        m_track->SetGlobalTimecodeScale(TIMECODE_SCALE);

        *static_cast<EbmlUInteger *>(&GetChild<KaxTrackNumber>(*m_track)) = 1;
        *static_cast<EbmlUInteger *>(&GetChild<KaxTrackUID>(*m_track)) = 13;
        *static_cast<EbmlUInteger *>(&GetChild<KaxTrackType>(*m_track)) = track_video;
        *static_cast<EbmlUInteger *>(&GetChild<KaxTrackMinCache>(*m_track)) = 1;
        *static_cast<EbmlString *>(&GetChild<KaxCodecID>(*m_track)) = "V_VP9";
        //binary b[2] = {'x','a'};
        //(static_cast<EbmlBinary *>(&GetChild<KaxCodecPrivate>(*m_track)))->CopyBuffer(b,2);
        m_track->EnableLacing(true);

        // video specific params
        
        KaxTrackVideo & MyTrack2Video = GetChild<KaxTrackVideo>(*m_track);
        *(static_cast<EbmlUInteger *>(&GetChild<KaxVideoPixelHeight>(MyTrack2Video))) = h;
        *(static_cast<EbmlUInteger *>(&GetChild<KaxVideoPixelWidth>(MyTrack2Video))) = w;
        *(static_cast<EbmlUInteger *>(&GetChild<KaxVideoDisplayHeight>(MyTrack2Video))) = h;
        *(static_cast<EbmlUInteger *>(&GetChild<KaxVideoDisplayWidth>(MyTrack2Video))) = w;

        /*
        KaxVideoColour & MyTrack2Color = GetChild<KaxVideoColour>(MyTrack2Video);
        *(static_cast<EbmlUInteger *>(&GetChild<KaxVideoColourMatrix>(MyTrack2Color))) = 9;
        *(static_cast<EbmlUInteger *>(&GetChild<KaxVideoColourRange>(MyTrack2Color))) = 1;
        *(static_cast<EbmlUInteger *>(&GetChild<KaxVideoColourTransferCharacter>(MyTrack2Color))) = 16;
        *(static_cast<EbmlUInteger *>(&GetChild<KaxVideoColourPrimaries>(MyTrack2Color))) = 9;
        *(static_cast<EbmlUInteger *>(&GetChild<KaxVideoColourMaxCLL>(MyTrack2Color))) = 1000;
        *(static_cast<EbmlUInteger *>(&GetChild<KaxVideoColourMaxFALL>(MyTrack2Color))) = 300;

        KaxVideoColourMasterMeta & MyColorMeta = GetChild<KaxVideoColourMasterMeta>(MyTrack2Color);
        *(static_cast<EbmlFloat *>(&GetChild<KaxVideoRChromaX>(MyColorMeta))) = 0.68;
        *(static_cast<EbmlFloat *>(&GetChild<KaxVideoRChromaY>(MyColorMeta))) = 0.32;
        *(static_cast<EbmlFloat *>(&GetChild<KaxVideoGChromaX>(MyColorMeta))) = 0.265;
        *(static_cast<EbmlFloat *>(&GetChild<KaxVideoGChromaY>(MyColorMeta))) = 0.69;
        *(static_cast<EbmlFloat *>(&GetChild<KaxVideoBChromaX>(MyColorMeta))) = 0.15;
        *(static_cast<EbmlFloat *>(&GetChild<KaxVideoBChromaY>(MyColorMeta))) = 0.06;
        *(static_cast<EbmlFloat *>(&GetChild<KaxVideoWhitePointChromaX>(MyColorMeta))) = 0.3127;
        *(static_cast<EbmlFloat *>(&GetChild<KaxVideoWhitePointChromaY>(MyColorMeta))) = 0.329;
        *(static_cast<EbmlFloat *>(&GetChild<KaxVideoLuminanceMax>(MyColorMeta))) = 1000;
        *(static_cast<EbmlFloat *>(&GetChild<KaxVideoLuminanceMin>(MyColorMeta))) = 0.01;
        */

        MyTracks.Render(*m_file, m_writeDefaultValues);

        m_metaSeek->IndexThis(MyTracks, m_fileSegment);

        m_cues = &GetChild<KaxCues>(m_fileSegment);
        m_cues->SetGlobalTimecodeScale(TIMECODE_SCALE);
    }
    catch (std::exception & e)
    {
        throw LumaException(e.what());
    } 

}

void MkvInterface::openRead(const char *inputFile)
{
    m_writeMode = 0;
    
    fprintf(stderr, "\nReading '%s':\n", inputFile);
    fprintf(stderr, "---------------------------------------------------\n");
    
    try
    {
        m_file = new StdIOCallback(inputFile, MODE_READ);
        aStream = new EbmlStream(*m_file);

        // find the EBML head in the file
        m_element0 = aStream->FindNextID(EbmlHead::ClassInfos, 0xFFFFFFFFL);
        if (m_element0 != NULL)
        {
            m_element0->SkipData(*aStream, EbmlHead_Context);
            if (m_element0 != NULL)
                delete m_element0;
        }
        
        // find the segment to read
        m_element0 = aStream->FindNextID(KaxSegment::ClassInfos, 0xFFFFFFFFL);
        findCluster();
    }
    catch (std::exception &e)
    {
        throw LumaException(e.what());
    }
    fprintf(stderr, "---------------------------------------------------\n");
}

void MkvInterface::close()
{
    flushCluster();
    
    if (m_file != NULL && m_writeMode)
    {
        if (m_cues != NULL && m_cues->ListSize() > 0)
        {
            m_cues->Render(*m_file, m_writeDefaultValues);
            m_metaSeek->IndexThis(*m_cues, m_fileSegment);
        }
        
        KaxInfo & MyInfos = GetChild<KaxInfo>(m_fileSegment);

        KaxDuration &Duration = GetChild<KaxDuration>(MyInfos);
        *(static_cast<EbmlFloat *>(&Duration)) = m_frameDuration * m_frameCount;

        MyInfos.UpdateSize();

        uint64 origPos = m_file->getFilePointer();
        m_file->setFilePointer(MyInfos.GetElementPosition());
        MyInfos.Render(*m_file, m_writeDefaultValues);
        m_file->setFilePointer(origPos);
        
        // write the seek head
        m_dummy->ReplaceWith(*m_metaSeek, *m_file);

        /*
        KaxTags &tags = GetChild<KaxTags>(m_fileSegment);
        KaxTag &tag = GetChild<KaxTag>(tags);

        KaxTagTargets &targets = GetChild<KaxTagTargets>(tag);
        *(static_cast<EbmlUInteger *>(&GetChild<KaxTagTargetTypeValue>(targets))) = 50;
        *(static_cast<EbmlUInteger *>(&GetChild<KaxTagTrackUID>(targets))) = 13;
        *(static_cast<EbmlString *>(&GetChild<KaxTagTargetType>(targets))) = "MOVIE";

        KaxTagSimple &stag_l1 = GetChild<KaxTagSimple>(tag);
        *(static_cast<EbmlUnicodeString *>(&GetChild<KaxTagName>(stag_l1))) = L"NUMBER_OF_FRAMES";
        *(static_cast<EbmlUnicodeString *>(&GetChild<KaxTagString>(stag_l1))) = L"2";

        tags.Render(*m_file, m_writeDefaultValues);
        */
        
        m_file->setFilePointer(0, seek_end);
	    if (m_fileSegment.ForceSize(m_file->getFilePointer() -
                                    m_fileSegment.GetElementPosition() -
                                    m_fileSegment.HeadSize()))
            m_fileSegment.OverwriteHead(*m_file);
    }
    
    if (m_file != NULL)
    {
        m_file->close();
        delete m_file;
    }
    m_file = NULL;
}

void MkvInterface::addAttachment(unsigned int uid, const binary* buffer, unsigned int buffer_size, const char* description)
{
    try
    {
        KaxAttached *MyAttachedFile;
        
        // Attachments
        if (m_attachments == NULL)
        {
            m_attachments = &GetChild<KaxAttachments>(m_fileSegment);
            MyAttachedFile = &GetChild<KaxAttached>(*m_attachments);
        }
        else
            MyAttachedFile = &GetNextChild<KaxAttached>(*m_attachments, *m_prevAttachment);
        
        m_prevAttachment = MyAttachedFile;
        
        KaxFileUID & MyKaxFileUID = GetChild<KaxFileUID>(*MyAttachedFile);
        *static_cast<EbmlUInteger *>(&MyKaxFileUID) = uid;

        KaxFileName & MyAttachedFile_Filename  = GetChild<KaxFileName>(*MyAttachedFile);
        *static_cast<EbmlUnicodeString *>(&MyAttachedFile_Filename) = (const wchar_t*)(L"Luma HDRV meta data");

        KaxMimeType & MyAttachedFile_MimieType  = GetChild<KaxMimeType>(*MyAttachedFile);
        *static_cast<EbmlString *>(&MyAttachedFile_MimieType) = (const char *)("");

        KaxFileDescription & MyKaxFileDescription  = GetChild<KaxFileDescription>(*MyAttachedFile);
        
        UTFstring str;
        str.SetUTF8(description);
        *static_cast<EbmlUnicodeString *>(&MyKaxFileDescription) = str.c_str();

        KaxFileData & MyAttachedFile_FileData  = GetChild<KaxFileData>(*MyAttachedFile);
        
        MyAttachedFile_FileData.SetBuffer(buffer, buffer_size);
    }
    catch (std::exception & Ex)
    {
        fprintf(stderr, "%s\n", Ex.what());
    }
}

void MkvInterface::writeAttachments()
{
    if (m_attachments != NULL)
    {
        m_attachments->Render(*m_file, m_writeDefaultValues);
        m_metaSeek->IndexThis(*m_attachments, m_fileSegment);
    }
}

bool MkvInterface::getAttachment(unsigned int ind, binary** buffer, unsigned int &id, unsigned int &buffer_size)
{
    if (ind >= m_attachmentID.size())
        return 0;
    
    *buffer = m_attachmentBuffer.at(ind);
    id = m_attachmentID.at(ind);
    buffer_size = m_attachmentSize.at(ind);
    
    return 1;
}

void MkvInterface::addFrame(const uint8 *frame_buffer, unsigned int buffer_size, bool isKey)
{
    m_timecode = m_frameDuration * m_frameCount;
    if (m_verbose) fprintf(stderr, "Frame %d (%f)\n", m_frameCount+1, m_timecode);
    
    // switch to new cluster for every key frame
    if (isKey)
    {
        flushCluster();
        
        // new cluster
        m_cluster = new KaxCluster();
        m_cluster->SetParent(m_fileSegment);
        m_cluster->SetGlobalTimecodeScale(TIMECODE_SCALE);
        m_cluster->SetPreviousTimecode(m_timecode, TIMECODE_SCALE);
        m_cluster->InitTimecode(m_timecode, TIMECODE_SCALE);
        m_cluster->EnableChecksum();
        
        KaxClusterTimecode & MyClusterTimeCode = GetChild<KaxClusterTimecode>(*m_cluster);
		*(static_cast<EbmlUInteger *>(&MyClusterTimeCode)) = m_timecode;// * m_timecodeScale;
    }
    
    // for each frame, create new block group in current cluster
    m_blockGroup = &m_cluster->GetNewBlock();		
    m_blockGroup->SetParent(*m_cluster);
    m_blockGroup->SetParentTrack(*m_track);

    // copy frame buffer, since this must be kept in memory until the current cluster is flushed
    uint8 *fb = new uint8[buffer_size];
    memcpy((void*)fb, (void*)frame_buffer, buffer_size);
    DataBuffer *data = new DataBuffer((binary *)fb, buffer_size);
    m_frameBuffer.push_back(fb);
    
    //KaxBlock &MyKaxBlock = GetChild<KaxBlock>(*m_blockGroup);
	//MyKaxBlock.SetParent(*m_cluster);	
    
    // frame without reference (key/I frame)
    if (m_blockGroupPrev == NULL)
    {
        if (m_verbose) fprintf(stderr, "\tKey frame (I)\n");
        //MyKaxBlock.AddFrame(*m_track, m_timecode * TIMECODE_SCALE, *data, LACING_AUTO);
        m_blockGroup->AddFrame(*m_track, m_timecode * TIMECODE_SCALE, *data, LACING_AUTO);
        
        m_blockGroupPrev = m_blockGroup;
    }
    // frame with reference to last key frame (P frame)
    else
    {
        if (m_verbose) fprintf(stderr, "\tIntermediate frame (P)\n");
        
        //KaxReferenceBlock &MyKaxReferenceBlock = GetChild<KaxReferenceBlock>(*m_blockGroup);
	    //MyKaxReferenceBlock.SetReferencedBlock(*m_blockGroupPrev);
	    //MyKaxReferenceBlock.SetParentBlock(*m_blockGroup);
        //MyKaxBlock.AddFrame(*m_track, m_timecode * TIMECODE_SCALE, *data, LACING_AUTO);
        
        m_blockGroup->AddFrame(*m_track, m_timecode * TIMECODE_SCALE, *data, *m_blockGroupPrev, LACING_AUTO);
    }
    if (m_verbose) fprintf(stderr, "\tdone!\n");
    
    KaxBlockDuration &MyKaxBlockDuration = GetChild<KaxBlockDuration>(*m_blockGroup);
	*(static_cast<EbmlUInteger *>(&MyKaxBlockDuration)) = m_frameDuration;
    
    m_frameCount++;
}

void MkvInterface::flushCluster()
{
    if (m_blockGroupPrev != NULL)
    {
        KaxBlockBlob *Blob = new KaxBlockBlob(BLOCK_BLOB_NO_SIMPLE);
        Blob->SetBlockGroup(*m_blockGroupPrev);
        m_cues->AddBlockBlob(*Blob);
    }

    if (m_cluster != NULL)
    {
        m_cluster->Render(*m_file, *m_cues, m_writeDefaultValues);
        m_cluster->ReleaseFrames();
        
        while (!m_frameBuffer.empty())
        {
            uint8 *data = m_frameBuffer.back();
            m_frameBuffer.pop_back();
            delete[] data;
        }
        
        // we don't really need clusters in the seek head
        //assert(m_metaSeek->GetSize()+75 < m_dummy->GetSize());
        //m_metaSeek->IndexThis(*m_cluster, m_fileSegment);
        //m_metaSeek->UpdateSize();
        
        // cluster deletion seems to be handled automatically when using reference frames
        // TODO: is this true, or is this a potential memory leak?
        //delete m_cluster;
    }
    
    m_cluster = NULL;
    m_blockGroup = m_blockGroupPrev = NULL;
}

bool MkvInterface::readFrame()
{
    if (m_element0 == NULL)
        return 0;
        
    try
    {
        if (findBlockGroup())
            return 1;
        
        if (findCluster())
            return findBlockGroup();
    }
    catch (std::exception & Ex)
    {
        fprintf(stderr, "%s\n", Ex.what());
    }
    
    return 0;
}

bool MkvInterface::findCluster()
{
    m_element2b = NULL;
    
    if (EbmlId(*m_element0) != KaxSegment::ClassInfos.GlobalId)
        return 0;
        
    bool clusterFound = 0;
    
    //fprintf(stderr, "FILE POS = %d, ELEMENT POS = %d\n", m_file->getFilePointer(), m_element0->GetElementPosition());
    
    while (!clusterFound)
    {
        if (m_element1 == NULL)
            m_element1 = aStream->FindNextElement(m_element0->Generic().Context, m_upperElementa, 0xFFFFFFFFL, m_allowDummy);
        else
        {
            if (m_upperElementa > 0)
            {
                m_upperElementa--;
                delete m_element1;
                m_element1 = m_element2a;
                if (m_upperElementa > 0)
                    break;
            }
            else
            {
                m_element1->SkipData(*aStream, m_element1->Generic().Context);
                delete m_element1;
                
                m_element1 = aStream->FindNextElement(m_element0->Generic().Context, m_upperElementa, 0xFFFFFFFFL, m_allowDummy);
            }
        }
        
        if (m_element1 == NULL || m_upperElementa > 0)
            break;
        if (m_upperElementa < 0)
            m_upperElementa = 0;
        
        //fprintf(stderr, "FILE POS = %d, ELEMENT1 POS = %d (%d)\n", m_file->getFilePointer(), m_element1->GetElementPosition(), m_elementPosition);

        if (EbmlId(*m_element1) == KaxInfo::ClassInfos.GlobalId)
        {
            fprintf(stderr, "File info:\n");
            handleInfo();
        }
        else if (EbmlId(*m_element1) == KaxTracks::ClassInfos.GlobalId)
        {
            // found the Tracks element
            fprintf(stderr, "Segment Tracks:\n");
            handleTracksData();
        }
        else if (EbmlId(*m_element1) == KaxAttachments::ClassInfos.GlobalId)
        {
            fprintf(stderr, "Attachments:\n");
            handleAttachments();
        }
        else if (EbmlId(*m_element1) == KaxCues::ClassInfos.GlobalId)
        {
            if (m_verbose) fprintf(stderr, "Cue entries:\n");
            handleCueData();
        }
        else if (EbmlId(*m_element1) == KaxSeekHead::ClassInfos.GlobalId)
        {
            if (m_verbose) fprintf(stderr, "Meta seek head:\n");
            handleSeekHead();
        }
        else if (EbmlId(*m_element1) == KaxCluster::ClassInfos.GlobalId)
        {
            if (m_verbose) fprintf(stderr, "Segment Cluster found\n");
            clusterFound = 1;
        }
    }

    return clusterFound;
}

bool MkvInterface::findBlockGroup()
{
    if (m_element1 == NULL)
        return 0;
        
    bool frameFound = 0;
    
    KaxCluster *segmentCluster = static_cast<KaxCluster *>(m_element1);
    uint32 clusterTimecode;
    EbmlCrc32 *pChecksum = NULL;
    
    while (!frameFound)
    {
        if (m_element2b == NULL)
            m_element2b = aStream->FindNextElement(m_element1->Generic().Context, m_upperElementb, 0xFFFFFFFFL, m_allowDummy);
        else
        {
            if (m_upperElementb > 0)
            {
                m_upperElementb--;
                delete m_element2b;
                m_element2b = m_element3b;
                if (m_upperElementb > 0)
                    break;
            }
            else 
            {
                m_element2b->SkipData(*aStream, m_element2b->Generic().Context);
                if (m_element2b != pChecksum)
                    delete m_element2b;

                m_element2b = aStream->FindNextElement(m_element1->Generic().Context, m_upperElementb, 0xFFFFFFFFL, m_allowDummy);
            }
        }
        
        if (m_element2b == NULL || m_upperElementb > 0)
            break;
        if (m_upperElementb < 0)
            m_upperElementb = 0;
        
        if (EbmlId(*m_element2b) == KaxClusterTimecode::ClassInfos.GlobalId)
        {
            //fprintf(stderr, "Cluster timecode found\n");
            KaxClusterTimecode & ClusterTime = *static_cast<KaxClusterTimecode*>(m_element2b);
            ClusterTime.ReadData(aStream->I_O());
            clusterTimecode = uint32(ClusterTime);
            m_currentTime = clusterTimecode;
            segmentCluster->InitTimecode(clusterTimecode, TIMECODE_SCALE);
            
            if (m_verbose) fprintf(stderr, "\tCluster time code = %d\n", clusterTimecode);
        }
        else  if (EbmlId(*m_element2b) == KaxBlockGroup::ClassInfos.GlobalId)
        {
            if (m_verbose) fprintf(stderr, "\tBlock Group found\n");
            frameFound = 1;
        }
    }
    
    return frameFound;
}

const uint8 *MkvInterface::getFrame(unsigned int & buffer_size)
{
    const uint8 *frame_buffer = NULL;
    
    KaxBlockGroup & aBlockGroup = *static_cast<KaxBlockGroup*>(m_element2b);
    //              aBlockGroup.ClearElement();
    // Extract the valuable data from the Block

    aBlockGroup.Read(*aStream, KaxBlockGroup::ClassInfos.Context, m_upperElementb, m_element3b, m_allowDummy);
    KaxBlock * dataBlock = static_cast<KaxBlock *>(aBlockGroup.FindElt(KaxBlock::ClassInfos));
    if (dataBlock != NULL)
    {
        assert(dataBlock->TrackNum() == m_trackNr);
        
        dataBlock->SetParent(*static_cast<KaxCluster *>(m_element1));
        //fprintf(stderr, "   Track # %d / %d frame%s / Timecode %d\n",dataBlock->TrackNum(), dataBlock->NumberFrames(), (dataBlock->NumberFrames() > 1)?"s":"", dataBlock->GlobalTimecode());
        
        if (dataBlock->NumberFrames() > 1)
            fprintf(stderr, "Warning! Multiple frames per block not supported. Reading first frame in block.\n");
        //for (unsigned int f=0; f<dataBlock->NumberFrames(); f++)
        //{
            DataBuffer data = dataBlock->GetBuffer(0);
            frame_buffer = (uint8*)data.Buffer();
            buffer_size = data.Size();
            
            //frame_buffer = (uint8*)malloc(buffer_size);
            //memcpy((void*)frame_buffer, (void*)data.Buffer(), buffer_size);
        //}
    }
    else 
        fprintf(stderr, "Warning! A BlockGroup without a Block!\n");
    
    
    KaxBlockDuration * BlockDuration = static_cast<KaxBlockDuration *>(aBlockGroup.FindElt(KaxBlockDuration::ClassInfos));
    if (BlockDuration != NULL)
        m_frameDuration = uint32(*BlockDuration);
    //    fprintf(stderr, "  Block Duration %d scaled ticks : %ld ns\n", uint32(*BlockDuration), uint32(*BlockDuration) * TIMECODE_SCALE);
    /*
    KaxReferenceBlock * RefTime = static_cast<KaxReferenceBlock *>(aBlockGroup.FindElt(KaxReferenceBlock::ClassInfos));
    if (RefTime != NULL)
        fprintf(stderr, "  Reference frame at scaled (%d) timecode %ld\n", int32(*RefTime), int32(int64(*RefTime) * TIMECODE_SCALE));
    */
    
    return frame_buffer;
}

// TODO: why use seek position + 18?
bool MkvInterface::seekToTime(float tm, bool absolute)
{
    if (m_timeStamps.empty())
    {
        m_file->setFilePointer(m_cuePosition+18, seek_beginning); //seek_current , seek_end
        if (m_element1 != NULL)
            delete m_element1;
        m_element1 = NULL;
        m_upperElementa = 0;
        findCluster();
    }
    
    if (m_timeStamps.empty())
        return false;
    
    if (!absolute)
        tm = tm*m_duration;
    
    unsigned int pos = 0;
    int l = 0, r = m_timeStamps.size()-1;
    while( l+1 < r )
    {
	    int m = (l+r)/2;
	    if( tm < m_timeStamps.at(m) )
		    r = m;
	    else
		    l = m;
    }
    if ( tm - m_timeStamps.at(l) < m_timeStamps.at(r) - tm )
	    pos = l;
    else
	    pos = r;
    
    if (m_verbose) fprintf(stderr, "SEEK TO TIME %d (%f), POSITION %d (l=%d, r=%d, %d keypoints)\n\n",
                           (int)m_timeStamps.at(pos), tm, (int)m_keyPositions.at(pos), l , r, (int)m_timeStamps.size());
    
    m_file->setFilePointer(m_keyPositions.at(pos)+18, seek_beginning); //seek_current , seek_end
    if (m_element1 != NULL)
        delete m_element1;
    m_element1 = NULL;
    m_upperElementa = 0;
    findCluster();
    
    return true;
}


// ------------- EBML element handling -----------------------------------------

void MkvInterface::handleTracksData()
{
    // handle the data in Tracks here.
    // poll for new tracks and handle them
    m_element2a = aStream->FindNextElement(m_element1->Generic().Context, m_upperElementa, 0xFFFFFFFFL, m_allowDummy);

    while (m_element2a != NULL) 
    {
        if (m_upperElementa > 0) break;
        if (m_upperElementa < 0) m_upperElementa = 0;
        
        if (EbmlId(*m_element2a) == KaxTrackEntry::ClassInfos.GlobalId)
        {
            m_element3a = aStream->FindNextElement(m_element2a->Generic().Context, m_upperElementa, 0xFFFFFFFFL, m_allowDummy);
            
            bool videoTrackFound = false;
            int trackNr = -1, trackUID = -1;
            
            while (m_element3a != NULL)
            {
                if (m_upperElementa > 0)
                    break;
                if (m_upperElementa < 0)
                    m_upperElementa = 0;
                    
                // read the data we care about in a track
                // Track number
                if (EbmlId(*m_element3a) == KaxTrackNumber::ClassInfos.GlobalId)
                {
                    KaxTrackNumber & TrackNum = *static_cast<KaxTrackNumber*>(m_element3a);
                    TrackNum.ReadData(aStream->I_O());
                    trackNr = uint8(TrackNum);
                    fprintf(stderr, "\tTrack # %d\n", trackNr);
                }
                
                // Track type
                else if (EbmlId(*m_element3a) == KaxTrackType::ClassInfos.GlobalId)
                {
                    KaxTrackType & TrackType = *static_cast<KaxTrackType*>(m_element3a);
                    TrackType.ReadData(aStream->I_O());
                    fprintf(stderr, "\tTrack type : ");
                    switch(uint8(TrackType))
                    {
                        case track_audio:
                            fprintf(stderr, "Audio");
                            break;
                        case track_video:
                            fprintf(stderr, "Video");
                            videoTrackFound = true;
                            break;
                        default:
                            fprintf(stderr, "unknown");
                    }
                    fprintf(stderr, "\n");
                }

                else if (EbmlId(*m_element3a) == KaxTrackFlagLacing::ClassInfos.GlobalId)
                {
                    fprintf(stderr, "\tFlag Lacing\n");
                }
                else if (EbmlId(*m_element3a) == KaxCodecID::ClassInfos.GlobalId)
                {
                    KaxCodecID & CodecID = *static_cast<KaxCodecID*>(m_element3a);
                    CodecID.ReadData(aStream->I_O());
                    fprintf(stderr, "\tCodec ID   : %s\n", std::string(CodecID).c_str());
                }
                else if (EbmlId(*m_element3a) == KaxTrackUID::ClassInfos.GlobalId)
                {
                    KaxTrackUID & TrackUID = *static_cast<KaxTrackUID*>(m_element3a);
                    TrackUID.ReadData(aStream->I_O());
                    trackUID = (unsigned int)TrackUID;
                    fprintf(stderr, "\tTrack UID   : %d\n", trackUID);
                }
                else if (EbmlId(*m_element3a) == KaxTrackVideo::ClassInfos.GlobalId)
                {
                    KaxTrackVideo & TrackVideo = *static_cast<KaxTrackVideo*>(m_element3a);
                    for (unsigned int j = 0; j<TrackVideo.ListSize(); j++)
                    {
                        if (EbmlId(*TrackVideo[j]) == KaxVideoFlagInterlaced::ClassInfos.GlobalId) 
                        {
	                        KaxVideoFlagInterlaced &Interlaced = *static_cast<KaxVideoFlagInterlaced*>(TrackVideo[j]);
	                        Interlaced.ReadData(aStream->I_O());
	                        fprintf(stderr, "\tInterlaced : %d\n", (unsigned int)Interlaced);
                        }
                        else if (EbmlId(*TrackVideo[j]) == KaxVideoPixelWidth::ClassInfos.GlobalId)
                        {
	                        KaxVideoPixelWidth &PixelWidth = *static_cast<KaxVideoPixelWidth*>(TrackVideo[j]);
	                        PixelWidth.ReadData(aStream->I_O());
	                        fprintf(stderr, "\tPixelWidth : %d\n", (unsigned int)PixelWidth);
                        }
                        else if (EbmlId(*TrackVideo[j]) == KaxVideoPixelHeight::ClassInfos.GlobalId)
                        {
	                        KaxVideoPixelHeight &PixelHeight = *static_cast<KaxVideoPixelHeight*>(TrackVideo[j]);
	                        PixelHeight.ReadData(aStream->I_O());
	                        fprintf(stderr, "\tPixelHeight : %d\n", (unsigned int)PixelHeight);
                        }
                        else if (EbmlId(*TrackVideo[j]) == KaxVideoFrameRate::ClassInfos.GlobalId)
                        {
	                        KaxVideoFrameRate &FrameRate = *static_cast<KaxVideoFrameRate*>(TrackVideo[j]);
	                        FrameRate.ReadData(aStream->I_O());
	                        fprintf(stderr, "\tFrameRate : %f\n", (float)FrameRate);
                        }				
                    }
                }
                
                m_element3a->SkipData(*aStream, m_element3a->Generic().Context);
                delete m_element3a;

                m_element3a = aStream->FindNextElement(m_element2a->Generic().Context, m_upperElementa, 0xFFFFFFFFL, m_allowDummy);
            }
            
            if (videoTrackFound)
            {
                m_trackNr = trackNr;
                m_trackUID = trackUID;
            }
        }
        if (m_upperElementa > 0)
        {
            m_upperElementa--;
            delete m_element2a;
            m_element2a = m_element3a;
            if (m_upperElementa > 0)
                break;
        } 
        else
        {
            m_element2a->SkipData(*aStream, m_element2a->Generic().Context);
            delete m_element2a;

            m_element2a = aStream->FindNextElement(m_element1->Generic().Context, m_upperElementa, 0xFFFFFFFFL, m_allowDummy);
        }
    }
    fprintf(stderr, "\n");
}

void MkvInterface::handleAttachments()
{
    m_element2a = aStream->FindNextElement(m_element1->Generic().Context, m_upperElementa, 0xFFFFFFFFL, m_allowDummy);

    while (m_element2a != NULL) 
    {
        if (m_upperElementa > 0)
            break;
        if (m_upperElementa < 0)
            m_upperElementa = 0;
        
        if (EbmlId(*m_element2a) == KaxAttached::ClassInfos.GlobalId)
        {
            m_element3a = aStream->FindNextElement(m_element2a->Generic().Context, m_upperElementa, 0xFFFFFFFFL, m_allowDummy);
            
            std::string attName, attDesc;
            
            while (m_element3a != NULL)
            {
                if (m_upperElementa > 0)
                    break;
                if (m_upperElementa < 0)
                    m_upperElementa = 0;
                
                if (EbmlId(*m_element3a) == KaxFileUID::ClassInfos.GlobalId)
                {
                    KaxFileUID & FileUID = *static_cast<KaxFileUID*>(m_element3a);
                    FileUID.ReadData(aStream->I_O());
                    //fprintf(stderr, "\tFile UID   : %d\n", (unsigned int)FileUID);
                    
                    m_attachmentID.push_back((unsigned int)FileUID);
                }
                else if (EbmlId(*m_element3a) == KaxFileName::ClassInfos.GlobalId)
                {
                    KaxFileName & FileName = *static_cast<KaxFileName*>(m_element3a);
                    FileName.ReadData(aStream->I_O());
                    //fprintf(stderr, "\tFile name   : %s\n", UTFstring(FileName).GetUTF8().c_str());
                    attName = UTFstring(FileName).GetUTF8();
                }
                else if (EbmlId(*m_element3a) == KaxMimeType::ClassInfos.GlobalId)
                {
                    KaxMimeType & MimeType = *static_cast<KaxMimeType*>(m_element3a);
                    MimeType.ReadData(aStream->I_O());
                    //fprintf(stderr, "\tMime type   : %s\n", std::string(MimeType).c_str());
                }
                else if (EbmlId(*m_element3a) == KaxFileDescription::ClassInfos.GlobalId)
                {
                    KaxFileDescription & FileDescription = *static_cast<KaxFileDescription*>(m_element3a);
                    FileDescription.ReadData(aStream->I_O());
                    //fprintf(stderr, "\tFile description   : %s\n", UTFstring(FileDescription).GetUTF8().c_str());
                    attDesc = UTFstring(FileDescription).GetUTF8();
                }
                else if (EbmlId(*m_element3a) == KaxFileData::ClassInfos.GlobalId)
                {
                    //fprintf(stderr, "\tFile data found\n");
                    KaxFileData & FileData = *static_cast<KaxFileData*>(m_element3a);
                    FileData.ReadData(aStream->I_O());
                    
                    unsigned int buffer_size = FileData.GetSize();
                    binary *buffer = new binary[buffer_size];
                    memcpy((void*)buffer, (void*)FileData.GetBuffer(), buffer_size);
                    m_attachmentBuffer.push_back(buffer);
                    m_attachmentSize.push_back(buffer_size);
                }
                
                m_element3a->SkipData(*aStream, m_element3a->Generic().Context);
                delete m_element3a;

                m_element3a = aStream->FindNextElement(m_element2a->Generic().Context, m_upperElementa, 0xFFFFFFFFL, m_allowDummy);
            }
            
            fprintf(stderr, "\t%s (UID:%d) : %s\n", attName.c_str(), m_attachmentID.back(), attDesc.c_str());
        }
        if (m_upperElementa > 0)
        {
            m_upperElementa--;
            delete m_element2a;
            m_element2a = m_element3a;
            if (m_upperElementa > 0)
                break;
        } 
        else
        {
            m_element2a->SkipData(*aStream, m_element2a->Generic().Context);
            delete m_element2a;

            m_element2a = aStream->FindNextElement(m_element1->Generic().Context, m_upperElementa, 0xFFFFFFFFL, m_allowDummy);
        }
    }
}

void MkvInterface::handleSeekHead()
{
    KaxSeekHead *MetaSeek = static_cast<KaxSeekHead *>(m_element1);
    // read it in memory
    MetaSeek->Read(*aStream, KaxSeekHead::ClassInfos.Context, m_upperElementa, m_element2a, m_allowDummy);
    
    if (!MetaSeek->CheckMandatory())
        fprintf(stderr, "Warning! Some mandatory elements in the seek head are missing!\n");
    
    for (unsigned int Index0=0; Index0 < MetaSeek->ListSize(); Index0++)
    {
        if ((*MetaSeek)[Index0]->Generic().GlobalId == KaxSeek::ClassInfos.GlobalId)
        {
            if (m_verbose) fprintf(stderr, "\tSeek Point\n");
            KaxSeek & SeekPoint = *static_cast<KaxSeek *>((*MetaSeek)[Index0]);
            KaxSeekID *SeekID;
            KaxSeekPosition *SeekPos;
            for (unsigned int Index1=0; Index1 < SeekPoint.ListSize(); Index1++)
            {
                if (SeekPoint[Index1]->Generic().GlobalId == KaxSeekID::ClassInfos.GlobalId)
                {
                    SeekID = static_cast<KaxSeekID *>(SeekPoint[Index1]);
                    if (m_verbose)
                    {
                        fprintf(stderr, "\t\tSeek ID : ");
                        for (unsigned int i=0; i<SeekID->GetSize(); i++)
                            fprintf(stderr, "[%02X]", SeekID->GetBuffer()[i]);
                        fprintf(stderr, "\n");
                    }
                }
                else if (SeekPoint[Index1]->Generic().GlobalId == KaxSeekPosition::ClassInfos.GlobalId)
                {
                    SeekPos = static_cast<KaxSeekPosition *>(SeekPoint[Index1]);
                    if (m_verbose) fprintf(stderr, "\t\tSeek position %d\n", uint32(*SeekPos));
                }
                
                if (EbmlId(SeekID->GetBuffer(), SeekID->GetSize()) == KaxCues::ClassInfos.GlobalId)
                    m_cuePosition = uint64(*SeekPos);
            }
        }
    }
}

void MkvInterface::handleInfo()
{
    m_element2a = aStream->FindNextElement(m_element1->Generic().Context, m_upperElementa, 0xFFFFFFFFL, m_allowDummy);
    
    while (m_element2a != NULL) 
    {
        if (m_upperElementa > 0)
            break;
        if (m_upperElementa < 0)
            m_upperElementa = 0;
    
        //fprintf(stderr, "\tFILE POS = %d, ELEMENT POS = %d\n", m_file->getFilePointer(), m_element2a->GetElementPosition());
        
        if (m_verbose)
        {
            fprintf(stderr, "\tEBML ID: ");
            for (unsigned int i=0; i<EbmlId(*m_element2a).Length; i++)
                fprintf(stderr, "[%02X]", (EbmlId(*m_element2a).Value >> (8*(1-i))) & 0xFF);
            fprintf(stderr, "\n");
        }
        
        if (EbmlId(*m_element2a) == KaxTimecodeScale::ClassInfos.GlobalId)
        {
            KaxTimecodeScale *TimeScale = static_cast<KaxTimecodeScale*>(m_element2a);
            TimeScale->ReadData(aStream->I_O());
            fprintf(stderr, "\tTimecode Scale : %d\n", uint32(*TimeScale));
        }
        else if (EbmlId(*m_element2a) == KaxDuration::ClassInfos.GlobalId)
        {
            KaxDuration *Duration = static_cast<KaxDuration*>(m_element2a);
            Duration->ReadData(aStream->I_O());
            m_duration = Duration->GetValue();
            fprintf(stderr, "\tSegment duration : %0.2fs\n", 1000*m_duration/TIMECODE_SCALE);
        }
        else if (EbmlId(*m_element2a) == KaxDateUTC::ClassInfos.GlobalId)
        {
            fprintf(stderr, "\tDate UTC\n");
        }
        else if (EbmlId(*m_element2a) == KaxSegmentFilename::ClassInfos.GlobalId)
        {
            KaxSegmentFilename *fn = static_cast<KaxSegmentFilename*>(m_element2a);
            fn->ReadData(aStream->I_O());
            fprintf(stderr, "\tSegment filename : %s\n", UTFstring(*fn).GetUTF8().c_str());
        }
        else if (EbmlId(*m_element2a) == KaxMuxingApp::ClassInfos.GlobalId)
        {
            KaxMuxingApp *pApp = static_cast<KaxMuxingApp*>(m_element2a);
            pApp->ReadData(aStream->I_O());
            fprintf(stderr, "\tMuxing app : %s\n", UTFstring(*pApp).GetUTF8().c_str());
        }
        else if (EbmlId(*m_element2a) == KaxWritingApp::ClassInfos.GlobalId)
        {
            KaxWritingApp *pApp = static_cast<KaxWritingApp*>(m_element2a);
            pApp->ReadData(aStream->I_O());
            fprintf(stderr, "\tWriting app : %s\n", UTFstring(*pApp).GetUTF8().c_str());
        }
        else
        {
            fprintf(stderr, "\tOther...\n");
        }
        
        if (m_upperElementa > 0)
        {
            m_upperElementa--;
            delete m_element2a;
            m_element2a = m_element3a;
            if (m_upperElementa > 0)
                break;
        } 
        else
        {
            m_element2a->SkipData(*aStream, m_element2a->Generic().Context);
            delete m_element2a;

            m_element2a = aStream->FindNextElement(m_element1->Generic().Context, m_upperElementa, 0xFFFFFFFFL, m_allowDummy);
        }
    }
    fprintf(stderr, "\n");
}

void MkvInterface::handleCueData()
{
    KaxCues *CuesEntry = static_cast<KaxCues *>(m_element1);
    CuesEntry->SetGlobalTimecodeScale(TIMECODE_SCALE);
    
    // read everything in memory
    CuesEntry->Read(*aStream, KaxCues::ClassInfos.Context, m_upperElementa, m_element2a, m_allowDummy); // build the entries in memory
    if (!CuesEntry->CheckMandatory())
        fprintf(stderr, "Warning! Some mandatory elements in the cues are missing!\n");
    
    CuesEntry->Sort();
    
    // display the elements read
    unsigned int Index0;
    for (Index0 = 0; Index0<CuesEntry->ListSize() ;Index0++)
    {
        if ((*CuesEntry)[Index0]->Generic().GlobalId == KaxCuePoint::ClassInfos.GlobalId) 
        {
            if (m_verbose) fprintf(stderr, "\tCue Point\n");

            KaxCuePoint & CuePoint = *static_cast<KaxCuePoint *>((*CuesEntry)[Index0]);
            
            unsigned int Index1;
            for (Index1 = 0; Index1<CuePoint.ListSize() ;Index1++)
            {
                uint64 timeStamp;
                if (CuePoint[Index1]->Generic().GlobalId == KaxCueTime::ClassInfos.GlobalId)
                {
                    KaxCueTime & CueTime = *static_cast<KaxCueTime *>(CuePoint[Index1]);
                    timeStamp = uint64(CueTime);
                    if (m_verbose) fprintf(stderr, "\t\tTime %ld\n", (unsigned long)(timeStamp * TIMECODE_SCALE));
                }
                else if (CuePoint[Index1]->Generic().GlobalId == KaxCueTrackPositions::ClassInfos.GlobalId)
                {
                    KaxCueTrackPositions & CuePos = *static_cast<KaxCueTrackPositions *>(CuePoint[Index1]);
                    if (m_verbose) fprintf(stderr, "\t\tPositions\n");

                    uint16 track;
                    uint64 position = 0;
                    unsigned int Index2;
                    for (Index2 = 0; Index2<CuePos.ListSize() ;Index2++)
                    {
                        if (CuePos[Index2]->Generic().GlobalId == KaxCueTrack::ClassInfos.GlobalId)
                        {
                            KaxCueTrack & CueTrack = *static_cast<KaxCueTrack *>(CuePos[Index2]);
                            track = uint16(CueTrack);
                            if (m_verbose) fprintf(stderr, "\t\t\tTrack %d\n", track);
                        }
                        else if (CuePos[Index2]->Generic().GlobalId == KaxCueClusterPosition::ClassInfos.GlobalId)
                        {
                            KaxCueClusterPosition & CuePoss = *static_cast<KaxCueClusterPosition *>(CuePos[Index2]);
                            position = uint64(CuePoss);
                            if (m_verbose) fprintf(stderr, "\t\t\tCluster position %d\n", (int)position);
                        }
                        else if (CuePos[Index2]->Generic().GlobalId == KaxCueReference::ClassInfos.GlobalId)
                        {
                            KaxCueReference & CueRefs = *static_cast<KaxCueReference *>(CuePos[Index2]);
                            fprintf(stderr, "\t\t\tReference\n");

                            unsigned int Index3;
                            for (Index3 = 0; Index3<CueRefs.ListSize() ;Index3++)
                            {
                                if (CueRefs[Index3]->Generic().GlobalId == KaxCueRefTime::ClassInfos.GlobalId)
                                {
                                    KaxCueRefTime & CueTime = *static_cast<KaxCueRefTime *>(CueRefs[Index3]);
                                    if (m_verbose) fprintf(stderr, "\t\t\t\tTime %d\n", (int)uint32(CueTime));
                                }
                                else if (CueRefs[Index3]->Generic().GlobalId == KaxCueRefCluster::ClassInfos.GlobalId)
                                {
                                    KaxCueRefCluster & CueClust = *static_cast<KaxCueRefCluster *>(CueRefs[Index3]);
                                    if (m_verbose) fprintf(stderr, "\t\t\t\tCluster position %d\n", (int)uint64(CueClust));
                                }
                                else
                                    if (m_verbose) fprintf(stderr, "\t\t\t\t- found %s\n", CueRefs[Index3]->Generic().DebugName);
                            }
                        }
                        else
                            if (m_verbose) fprintf(stderr, "\t\t\t- found %s\n", CuePos[Index2]->Generic().DebugName);
                    }
                    if (track == m_trackNr && position > 0)
                    {
                        m_timeStamps.push_back(timeStamp);
                        m_keyPositions.push_back(position);
                    }
                }
                else
                    if (m_verbose) fprintf(stderr, "\t\t- found %s\n", CuePoint[Index1]->Generic().DebugName);
            }
        }
        else
            if (m_verbose) fprintf(stderr, "\t- found %s\n", (*CuesEntry)[Index0]->Generic().DebugName);
    }
}

