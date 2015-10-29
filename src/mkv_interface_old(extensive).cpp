#include "mkv_interface.h"

MkvInterface::MkvInterface()
{
    m_writeDefaultValues = false;
    m_frameCount = 1;
    m_file = NULL;
    
    m_attachments = NULL;
    
    m_upperElementa = m_upperElementb = 0;
    m_allowDummy = true;
    aStream = NULL;
    m_element0 = m_element1 = m_element2a = m_element2b = m_element3a = m_element3b = m_element4a = m_element4b = NULL;
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
    
    /*
    if (m_element0 != NULL) delete m_element0;
    if (m_element1 != NULL) delete m_element1;
    if (m_element2a != NULL) delete m_element2a;
    if (m_element2b != NULL) delete m_element2b;
    if (m_element3a != NULL) delete m_element3a;
    if (m_element3b != NULL) delete m_element3b;
    if (m_element4a != NULL) delete m_element4a;
    if (m_element4b != NULL) delete m_element4b;
    */
}

void MkvInterface::openWrite(const char *outputFile, const unsigned int w, const unsigned int h)
{
    try
    {
        // write the head of the file (with everything already configured)
        m_file = new StdIOCallback(outputFile, MODE_CREATE);

        ///// Writing EBML test
        EbmlHead FileHead;

        EDocType & MyDocType = GetChild<EDocType>(FileHead);
        *static_cast<EbmlString *>(&MyDocType) = "matroska";

        EDocTypeVersion & MyDocTypeVer = GetChild<EDocTypeVersion>(FileHead);
        *(static_cast<EbmlUInteger *>(&MyDocTypeVer)) = MATROSKA_VERSION;

        EDocTypeReadVersion & MyDocTypeReadVer = GetChild<EDocTypeReadVersion>(FileHead);
        *(static_cast<EbmlUInteger *>(&MyDocTypeReadVer)) = 1;

        FileHead.Render(*m_file, m_writeDefaultValues);

        // size is unknown and will always be, we can render it right away
        uint64 SegmentSize = m_fileSegment.WriteHead(*m_file, 5, m_writeDefaultValues);
        
        // Tracks
        KaxTracks & MyTracks = GetChild<KaxTracks>(m_fileSegment);

        // reserve some space for the Meta Seek written at the end
        EbmlVoid Dummy;
        Dummy.SetSize(300); // 300 octets
        Dummy.Render(*m_file, m_writeDefaultValues);

        // fill the mandatory Info section -----------------------------------------
        KaxInfo & MyInfos = GetChild<KaxInfo>(m_fileSegment);
        KaxTimecodeScale & TimeScale = GetChild<KaxTimecodeScale>(MyInfos);
        *(static_cast<EbmlUInteger *>(&TimeScale)) = TIMECODE_SCALE;

        KaxDuration & SegDuration = GetChild<KaxDuration>(MyInfos);
        *(static_cast<EbmlFloat *>(&SegDuration)) = 0.0;

        *((EbmlUnicodeString *)&GetChild<KaxMuxingApp>(MyInfos))  = L"libmatroska";
        *((EbmlUnicodeString *)&GetChild<KaxWritingApp>(MyInfos)) = L"mkv_interface";
        GetChild<KaxWritingApp>(MyInfos).SetDefaultSize(25);

        filepos_t InfoSize = MyInfos.Render(*m_file);
        m_metaSeek.IndexThis(MyInfos, m_fileSegment);

        // fill track 1 params -----------------------------------------------------
        m_track = &GetChild<KaxTrackEntry>(MyTracks);
        m_track->SetGlobalTimecodeScale(TIMECODE_SCALE);

        KaxTrackNumber & MyTrack2Number = GetChild<KaxTrackNumber>(*m_track);
        *(static_cast<EbmlUInteger *>(&MyTrack2Number)) = 1;

        KaxTrackUID & MyTrack2UID = GetChild<KaxTrackUID>(*m_track);
        *(static_cast<EbmlUInteger *>(&MyTrack2UID)) = 13;

        *(static_cast<EbmlUInteger *>(&GetChild<KaxTrackType>(*m_track))) = track_video;

        KaxCodecID & MyTrack2CodecID = GetChild<KaxCodecID>(*m_track);
        *static_cast<EbmlString *>(&MyTrack2CodecID) = "V_VP9";

        m_track->EnableLacing(false);

        // video specific params
        KaxTrackVideo & MyTrack2Video = GetChild<KaxTrackVideo>(*m_track);

        KaxVideoPixelHeight & MyTrack2PHeight = GetChild<KaxVideoPixelHeight>(MyTrack2Video);
        *(static_cast<EbmlUInteger *>(&MyTrack2PHeight)) = h;

        KaxVideoPixelWidth & MyTrack2PWidth = GetChild<KaxVideoPixelWidth>(MyTrack2Video);
        *(static_cast<EbmlUInteger *>(&MyTrack2PWidth)) = w;
        
        /*
        // fill track 2 params -----------------------------------------------------
        KaxTrackEntry & MetaTrack = GetNextChild<KaxTrackEntry>(MyTracks, *m_track);
        //m_track->SetGlobalTimecodeScale(TIMECODE_SCALE);

        KaxTrackNumber & MetaTrackNumber = GetChild<KaxTrackNumber>(MetaTrack);
        *(static_cast<EbmlUInteger *>(&MetaTrackNumber)) = 99;

        KaxTrackUID & MetaTrackUID = GetChild<KaxTrackUID>(MetaTrack);
        *(static_cast<EbmlUInteger *>(&MetaTrackUID)) = 99;

        *(static_cast<EbmlUInteger *>(&GetChild<KaxTrackType>(MetaTrack))) = track_complex;

        KaxCodecID & MetaTrackCodecID = GetChild<KaxCodecID>(MetaTrack);
        *static_cast<EbmlString *>(&MetaTrackCodecID) = "No codec";
        */

        uint64 TrackSize = MyTracks.Render(*m_file, m_writeDefaultValues);

        m_metaSeek.IndexThis(MyTracks, m_fileSegment);

        m_cues.SetGlobalTimecodeScale(TIMECODE_SCALE);
    
    }
    catch (std::exception & Ex)
    {
        fprintf(stderr, "%s\n", Ex.what());
    } 

}

void MkvInterface::openRead(const char *inputFile)
{
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
    catch (std::exception & Ex)
    {
        fprintf(stderr, "%s\n", Ex.what());
    } 
}

void MkvInterface::close()
{
    m_file->close();
    delete m_file;
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
        *static_cast<EbmlUnicodeString *>(&MyAttachedFile_Filename) = (const wchar_t*)(L"HDRV meta data");

        KaxMimeType & MyAttachedFile_MimieType  = GetChild<KaxMimeType>(*MyAttachedFile);
        *static_cast<EbmlString *>(&MyAttachedFile_MimieType) = (const char *)("");

        KaxFileDescription & MyKaxFileDescription  = GetChild<KaxFileDescription>(*MyAttachedFile);
        *static_cast<EbmlUnicodeString *>(&MyKaxFileDescription) = (const wchar_t*)(description);

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
        m_attachments->Render(*m_file, m_writeDefaultValues);
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

void MkvInterface::addFrame(const uint8_t *frame_buffer, unsigned int buffer_size)
{
    KaxCluster cluster;
    cluster.SetParent(m_fileSegment); // mandatory to store references in this Cluster
    cluster.SetPreviousTimecode(0, TIMECODE_SCALE); // the first timecode here
    cluster.EnableChecksum();

    KaxBlockGroup *MyNewBlock, *MyLastBlockTrk1 = NULL;

    DataBuffer *data7 = new DataBuffer((binary *)frame_buffer, buffer_size);
    cluster.AddFrame(*m_track, 40 * m_frameCount++ * TIMECODE_SCALE, *data7, MyNewBlock, LACING_NONE);
    if (MyNewBlock != NULL)
        MyLastBlockTrk1 = MyNewBlock;

    uint64 ClusterSize = cluster.Render(*m_file, m_cues, m_writeDefaultValues);
    cluster.ReleaseFrames();
    m_metaSeek.IndexThis(cluster, m_fileSegment);
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
    
    while (!clusterFound)
    {
        if (m_element1 == NULL)
            m_element1 = aStream->FindNextElement(m_element0->Generic().Context, m_upperElementa, 0xFFFFFFFFL, m_allowDummy);
        else
        {
            if (m_upperElementa > 0) {
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
                
                //m_element1 = aStream->FindNextElement(m_element0->Generic().Context, m_upperElementa, 0, m_allowDummy);
                m_element1 = aStream->FindNextElement(m_element0->Generic().Context, m_upperElementa, 0xFFFFFFFFL, m_allowDummy);
            }
        }
        
        if (m_element1 == NULL || m_upperElementa > 0)
            break;
        if (m_upperElementa < 0)
            m_upperElementa = 0;

        if (EbmlId(*m_element1) == EbmlVoid::ClassInfos.GlobalId)
        {
            printf("- Void found\n");
        }
        else if (EbmlId(*m_element1) == KaxTracks::ClassInfos.GlobalId)
        {
            // found the Tracks element
            printf("- Segment Tracks found\n");
            // handle the data in Tracks here.
            // poll for new tracks and handle them
            m_element2a = aStream->FindNextElement(m_element1->Generic().Context, m_upperElementa, 0xFFFFFFFFL, m_allowDummy);

            while (m_element2a != NULL) 
            {
                if (m_upperElementa > 0)
                    break;
                if (m_upperElementa < 0)
                    m_upperElementa = 0;
                
                if (EbmlId(*m_element2a) == KaxTrackEntry::ClassInfos.GlobalId)
                {
                    m_element3a = aStream->FindNextElement(m_element2a->Generic().Context, m_upperElementa, 0xFFFFFFFFL, m_allowDummy);
                    
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
                            printf("Track # %d\n", uint8(TrackNum));
                        }
                        
                        // Track type
                        else if (EbmlId(*m_element3a) == KaxTrackType::ClassInfos.GlobalId)
                        {
                            KaxTrackType & TrackType = *static_cast<KaxTrackType*>(m_element3a);
                            TrackType.ReadData(aStream->I_O());
                            printf("Track type : ");
                            switch(uint8(TrackType))
                            {
                                case track_audio:
                                    printf("Audio");
                                    break;
                                case track_video:
                                    printf("Video");
                                    break;
                                default:
                                    printf("unknown");
                            }
                            printf("\n");
                        }

                        else if (EbmlId(*m_element3a) == KaxTrackFlagLacing::ClassInfos.GlobalId)
                        {
                            printf("Flag Lacing\n");
                        }
                        else if (EbmlId(*m_element3a) == KaxCodecID::ClassInfos.GlobalId)
                        {
                            KaxCodecID & CodecID = *static_cast<KaxCodecID*>(m_element3a);
                            CodecID.ReadData(aStream->I_O());
                            printf("Codec ID   : %s\n", std::string(CodecID).c_str());
                        }
                        else if (EbmlId(*m_element3a) == KaxTrackUID::ClassInfos.GlobalId)
                        {
                            KaxTrackUID & TrackUID = *static_cast<KaxTrackUID*>(m_element3a);
                            TrackUID.ReadData(aStream->I_O());
                            printf("Track UID   : %d\n", (unsigned int)TrackUID);
                        }
                        else if (EbmlId(*m_element3a) == KaxTrackVideo::ClassInfos.GlobalId)
                        {
                            KaxTrackVideo & TrackVideo = *static_cast<KaxTrackVideo*>(m_element3a);
	                        for (unsigned int j = 0; j<TrackVideo.ListSize(); j++)
	                        {
	                            if (EbmlId(*TrackVideo[j]) == KaxVideoFlagInterlaced::ClassInfos.GlobalId) 
		                        {
			                        KaxVideoFlagInterlaced &Interlaced = *static_cast<KaxVideoFlagInterlaced*>(TrackVideo[j]);
			                        printf("Interlaced : %d\n", (unsigned int)Interlaced);
		                        }
		                        else if (EbmlId(*TrackVideo[j]) == KaxVideoPixelWidth::ClassInfos.GlobalId)
		                        {
			                        KaxVideoPixelWidth &PixelWidth = *static_cast<KaxVideoPixelWidth*>(TrackVideo[j]);
			                        printf("PixelWidth : %d\n", (unsigned int)PixelWidth);
		                        }
		                        else if (EbmlId(*TrackVideo[j]) == KaxVideoPixelHeight::ClassInfos.GlobalId)
		                        {
			                        KaxVideoPixelHeight &PixelHeight = *static_cast<KaxVideoPixelHeight*>(TrackVideo[j]);
			                        printf("PixelHeight : %d\n", (unsigned int)PixelHeight);
		                        }
		                        else if (EbmlId(*TrackVideo[j]) == KaxVideoFrameRate::ClassInfos.GlobalId)
		                        {
			                        KaxVideoFrameRate &FrameRate = *static_cast<KaxVideoFrameRate*>(TrackVideo[j]);
			                        printf("FrameRate : %f\n", (float)FrameRate);
		                        }				
	                        }
                        }

                        if (m_upperElementa > 0)
                        {
                            assert(0 == 1); // impossible to be here ?
                            m_upperElementa--;
                            delete m_element3a;
                            m_element3a = m_element4a;
                            if (m_upperElementa > 0)
                                break;
                        } 
                        else
                        {
                            m_element3a->SkipData(*aStream, m_element3a->Generic().Context);
                            delete m_element3a;

                            m_element3a = aStream->FindNextElement(m_element2a->Generic().Context, m_upperElementa, 0xFFFFFFFFL, m_allowDummy);
                        }
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
        }
        else if (EbmlId(*m_element1) == KaxAttachments::ClassInfos.GlobalId)
        {
            printf("- Attachments found\n");
            
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
                            printf("File UID   : %d\n", (unsigned int)FileUID);
                            
                            m_attachmentID.push_back((unsigned int)FileUID);
                        }
                        else if (EbmlId(*m_element3a) == KaxFileName::ClassInfos.GlobalId)
                        {
                            printf("File name found\n");
                        }
                        else if (EbmlId(*m_element3a) == KaxMimeType::ClassInfos.GlobalId)
                        {
                            KaxMimeType & MimeType = *static_cast<KaxMimeType*>(m_element3a);
                            MimeType.ReadData(aStream->I_O());
                            printf("Mime type   : %s\n", std::string(MimeType).c_str());
                        }
                        else if (EbmlId(*m_element3a) == KaxFileDescription::ClassInfos.GlobalId)
                        {
                            printf("File description found\n");
                        }
                        else if (EbmlId(*m_element3a) == KaxFileData::ClassInfos.GlobalId)
                        {
                            printf("File data found\n");
                            KaxFileData & FileData = *static_cast<KaxFileData*>(m_element3a);
                            FileData.ReadData(aStream->I_O());
                            
                            unsigned int buffer_size = FileData.GetSize();
                            binary *buffer = new binary[buffer_size];
                            memcpy((void*)buffer, (void*)FileData.GetBuffer(), buffer_size);
                            m_attachmentBuffer.push_back(buffer);
                            m_attachmentSize.push_back(buffer_size);
                        }

                        if (m_upperElementa > 0)
                        {
                            assert(0 == 1); // impossible to be here ?
                            m_upperElementa--;
                            delete m_element3a;
                            m_element3a = m_element4a;
                            if (m_upperElementa > 0)
                                break;
                        } 
                        else
                        {
                            m_element3a->SkipData(*aStream, m_element3a->Generic().Context);
                            delete m_element3a;

                            m_element3a = aStream->FindNextElement(m_element2a->Generic().Context, m_upperElementa, 0xFFFFFFFFL, m_allowDummy);
                        }
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
        }
        else if (EbmlId(*m_element1) == KaxInfo::ClassInfos.GlobalId)
        {
            printf("- Segment Informations found\n");
        }
        else if (EbmlId(*m_element1) == KaxCluster::ClassInfos.GlobalId)
        {
            printf("- Segment Clusters found\n");
            clusterFound = 1;
        }
        else if (EbmlId(*m_element1) == KaxCues::ClassInfos.GlobalId)
        {
            printf("- Cue entries found\n");
        }
        else if (EbmlId(*m_element1) == KaxSeekHead::ClassInfos.GlobalId)
        {
            printf("- Meta Seek found\n");
        }
        else if (EbmlId(*m_element1) == KaxChapters::ClassInfos.GlobalId)
        {
            printf("- Chapters found\n");
        }
        else if (EbmlId(*m_element1) == KaxTags::ClassInfos.GlobalId)
        {
            printf("- Tags found\n");
        }
        else
        {
            printf("- Nothing found\n");
        }

        printf("\n");
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
    uint32 sizeInCrc;
    uint64 crcPositionStart = 0;
    
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
            printf("Cluster timecode found\n");
            KaxClusterTimecode & ClusterTime = *static_cast<KaxClusterTimecode*>(m_element2b);
            ClusterTime.ReadData(aStream->I_O());
            clusterTimecode = uint32(ClusterTime);
            segmentCluster->InitTimecode(clusterTimecode, TIMECODE_SCALE);
        }
        else  if (EbmlId(*m_element2b) == KaxBlockGroup::ClassInfos.GlobalId)
        {
            printf("Block Group found\n");
            
            frameFound = 1;
        } 
        else if (EbmlId(*m_element2b) == EbmlCrc32::ClassInfos.GlobalId)
        {
            printf("Cluster CheckSum !\n");
            pChecksum = static_cast<EbmlCrc32*>(m_element2b);
            pChecksum->ReadData(aStream->I_O());
            segmentCluster->ForceChecksum( pChecksum->GetCrc32() ); // not use later
            sizeInCrc = 0;
            crcPositionStart = aStream->I_O().getFilePointer();
        }
    }
    
    /*
    if (pChecksum != NULL)
    {
        EbmlCrc32 ComputedChecksum;
        uint64 currPosition = aStream->I_O().getFilePointer();
        uint64 crcPositionEnd = m_element2b->GetElementPosition();
        binary *SupposedBufferInCrc = new binary [crcPositionEnd - crcPositionStart];
        aStream->I_O().setFilePointer(crcPositionStart);
        aStream->I_O().readFully(SupposedBufferInCrc, crcPositionEnd - crcPositionStart);
        aStream->I_O().setFilePointer(currPosition);
        ComputedChecksum.FillCRC32(SupposedBufferInCrc, crcPositionEnd - crcPositionStart);
        delete [] SupposedBufferInCrc;
        if (pChecksum->GetCrc32() == ComputedChecksum.GetCrc32())
            printf(" ++ CheckSum verification succeeded ++");
        else
            printf(" ++ CheckSum verification FAILED !!! ++");
        delete pChecksum;
        pChecksum = NULL;
    }
    */
    
    return frameFound;
}

//unsigned int MkvInterface::getFrame(uint8_t *frame_buffer)
const uint8_t *MkvInterface::getFrame(unsigned int & buffer_size)
{
    const uint8_t *frame_buffer = NULL;
    
    KaxBlockGroup & aBlockGroup = *static_cast<KaxBlockGroup*>(m_element2b);
    //              aBlockGroup.ClearElement();
    // Extract the valuable data from the Block

    aBlockGroup.Read(*aStream, KaxBlockGroup::ClassInfos.Context, m_upperElementb, m_element3b, m_allowDummy);
    KaxBlock * dataBlock = static_cast<KaxBlock *>(aBlockGroup.FindElt(KaxBlock::ClassInfos));
    if (dataBlock != NULL)
    {
        //                dataBlock->ReadData(aStream->I_O());
        dataBlock->SetParent(*static_cast<KaxCluster *>(m_element1));
        printf("   Track # %d / %d frame%s / Timecode %d\n",dataBlock->TrackNum(), dataBlock->NumberFrames(), (dataBlock->NumberFrames() > 1)?"s":"", dataBlock->GlobalTimecode());
        
        if (dataBlock->NumberFrames() > 1)
            fprintf(stderr, "Warning! Multiple frames per block not supported. Reading first frame in block.\n");
        //for (unsigned int f=0; f<dataBlock->NumberFrames(); f++)
        //{
            DataBuffer data = dataBlock->GetBuffer(0);
            frame_buffer = (uint8_t*)data.Buffer();
            buffer_size = data.Size();
            
            //frame_buffer = (uint8_t*)malloc(buffer_size);
            //memcpy((void*)frame_buffer, (void*)data.Buffer(), buffer_size);
        //}
    }
    else 
        printf("   A BlockGroup without a Block !!!");

    KaxBlockDuration * BlockDuration = static_cast<KaxBlockDuration *>(aBlockGroup.FindElt(KaxBlockDuration::ClassInfos));
    if (BlockDuration != NULL)
        printf("  Block Duration %d scaled ticks : %ld ns\n", uint32(*BlockDuration), uint32(*BlockDuration) * TIMECODE_SCALE);

    KaxReferenceBlock * RefTime = static_cast<KaxReferenceBlock *>(aBlockGroup.FindElt(KaxReferenceBlock::ClassInfos));
    if (RefTime != NULL)
        printf("  Reference frame at scaled (%d) timecode %ld\n", int32(*RefTime), int32(int64(*RefTime) * TIMECODE_SCALE));
    
    return frame_buffer;
}


