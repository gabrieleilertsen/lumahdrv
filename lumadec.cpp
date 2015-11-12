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

#include <luma_decoder.h>
#include "exr_interface.h"
#include "pfs_interface.h"
#include "luma_exception.h"
#include "arg_parser.h"

#include <iostream>
#include <string.h>

#include "config.h"


// Determine file extension
bool hasExtension( const char *file_name, const char *extension )
{
    if( file_name == NULL )
        return false;
    size_t fn_len = strlen( file_name );
    size_t ex_len = strlen( extension );

    if( ex_len >= fn_len )
        return false;

    if( strcasecmp( file_name + fn_len - ex_len, extension ) == 0 )
        return true;  

    return false;
}

// Parse parameter options from command line
bool setParams(int argc, char* argv[], std::string &hdrFrames, std::string &inputFile, bool &verbose)
{
    // Application usage info
    std::string info = std::string("lumadec -- Decode a high dynamic range (HDR) video that has been encoded with the HDRv codec\n\n") +
                       std::string("Usage: lumadec --input <hdr_video> --output <hdr_frames>\n");
    std::string postInfo = std::string("\nExample: lumadec -i hdr_video.mkv -o hdr_frame_%05d.exr\n\n") +
                           std::string("See man page for more information.");
    ArgParser argHolder(info, postInfo);
    
    // Input arguments
    argHolder.add(&inputFile, "--input",   "-i", "Input HDR video", 0);
    argHolder.add(&hdrFrames, "--output",  "-o", "Output location of decoded HDR frames");
    argHolder.add(&verbose,   "--verbose", "-v", "Verbose mode");
    
    // Parse arguments
    if (!argHolder.read(argc, argv))
        return 0;

    return 1;
}

int main(int argc, char* argv[])
{
    std::string hdrFrames, inputFile;
    bool verbose = 0;
    
#ifdef HAVE_PFS
    PfsInterface pfs; // Needs to store state between frames
#endif
    
    try
    {
        if (!setParams(argc, argv, hdrFrames, inputFile, verbose))
            return 1;
        
        // Decoder
        LumaDecoder decoder(inputFile.c_str(), verbose);
        
        // Frame for retrieving and storing decoded frames
        LumaFrame *frame;
        
#define STRBUF_LEN 500
        char str[STRBUF_LEN];
        int decoded_frame_count = 0; 
        for (int f=1; ; f++)
        {
            fprintf(stderr, "Decoding frame %d... ", f);
            
            // Get decoded frame
            frame = decoder.decode();
            
            // No frame available (EOF)
            if (frame == NULL)
                break;
            
            fprintf(stderr, "done\n");
            decoded_frame_count++;
            
            // Write hdr frames
            if( hdrFrames.size() == 0 || hasExtension( hdrFrames.c_str(), "pfs" ) )
            {
#ifdef HAVE_PFS
                if( !pfs.writeFrame(hdrFrames.c_str(), *frame) )
                    break;
#else
                throw LumaException( "Compiled without pfstools support" );          
#endif
            }
            else
            {
                snprintf(str, STRBUF_LEN-1, hdrFrames.c_str(), f);
                //sprintf(str, hdrFrames.size() == 0 ? "output_%05d.exr" : hdrFrames.c_str(), f);
                if (!ExrInterface::writeFrame(str, *frame))
                    break;
            }
        }
        
        fprintf(stderr, "\n\nDecoding finished. %d frames decoded.\n", decoded_frame_count);
    }
    catch (ParserException &e)
    {
        fprintf(stderr, "\nlumadec input error: %s\n", e.what());
        return 1;
    }
    catch (LumaException &e)
    {
        fprintf(stderr, "\nlumadec decoding error: %s\n", e.what());
        return 1;
    }
    catch (std::exception & e)
    {
        fprintf(stderr, "\nlumadec error: %s\n", e.what());
        return 1;
    }
    
    return 0;
}

