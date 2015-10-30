#include <luma_decoder.h>
#include "exr_interface.h"

#include <string.h>

int main(int argc, char* argv[])
{
    if (argc < 3 || !strcmp(argv[1], "-h") || !strcmp(argv[1], "--help"))
    {
        printf("Usage: ./test_simple_dec <hdr_video> <hdr_frames>\n");
        return 1;
    }
    
    char *inputFile = argv[1];
    char *hdrFrames = argv[2];
    
    LumaDecoder decoder(inputFile);
    LumaFrame *frame;
    
    char str[500];
    int f;
    for (f=0; ; f++)
    {
        printf("Decoding frame %d.\n", f);
        
        frame = decoder.decode();
        
        // No frame available -> end of file
        if (frame == NULL)
            break;
        
        sprintf(str, hdrFrames == NULL ? "output_%05d.exr" : hdrFrames, f);
        printf("NN = %d, Print --> %d\n", frame->buffer == NULL, ExrInterface::writeFrame(str, *frame));
    }
    
    printf("Decoding finished. %d frames decoded.\n", f);
    
    return 0;
}
