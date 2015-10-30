#include <luma_encoder.h>
#include "exr_interface.h"

#include <string.h>

int main(int argc, char* argv[])
{
    if (argc > 1 && !(strcmp(argv[1], "-h") && strcmp(argv[1], "--help")) )
    {
        printf("Usage: ./test_simple_enc <hdr_frames> <start_frame> <end_frame> <output>\n");
        return 1;
    }
    
    char *hdrFrames = NULL, *outputFile = NULL;
    int startFrame = 1, endFrame = 5;
    
    if (argc > 1)
        hdrFrames = argv[1];
    if (argc > 2)
        startFrame = atoi(argv[2]);
    if (argc > 3)
        endFrame = atoi(argv[3]);
    if (argc > 4)
        outputFile = argv[4];
    
    
    LumaEncoder encoder;
    LumaEncoderParams params = encoder.getParams();
    params.profile = 2;
    params.bitrate = 1000;
    params.keyframeInterval = 0;
    params.bitDepth = 12;
    params.ptfBitDepth = 11;
    params.colorBitDepth = 8;
    params.lossLess = 0; // --> // 0;
    
    params.quantizerScale = 4;
    params.ptf = LumaQuantizer::PTF_PQ;
    params.colorSpace = LumaQuantizer::CS_LUV;
    
    // LDR
    //params.profile = 0;
    //params.bitDepth = 8;
    //params.ptfBitDepth = 8;
    
    encoder.setParams(params);
    
    char str[500];
    for (int f = startFrame; f <= endFrame; f++)
    {
        printf("Encoding frame %d.\n", f );
        
        LumaFrame frame;
        
        if (hdrFrames != NULL)
        {
            sprintf(str, hdrFrames, f);
            ExrInterface::readFrame(str, frame);
        }
        else
            ExrInterface::testFrame(frame);
        
        if (!encoder.initialized())
            encoder.initialize(outputFile == NULL ? "output.mkv" : outputFile, frame.width, frame.height);

        encoder.encode(&frame);
    }
    
    encoder.finish();
    printf("Encoding finished. %d frames encoded.\n", endFrame-startFrame+1);
    
    return 0;
}
