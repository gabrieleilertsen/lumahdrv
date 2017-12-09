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

#include <luma_encoder.h>
#include "exr_interface.h"
#include "pfs_interface.h"
#include "luma_exception.h"
#include "arg_parser.h"

#include <iostream>
#include <string.h>

#include "config.h"

// Input and output specific information
struct IOData
{
    IOData() : startFrame(1), endFrame(9999), stepFrame(1), verbose(0)
    {}
    
    std::string hdrFrames, outputFile;
    unsigned int startFrame, endFrame, stepFrame;
    bool verbose;
};

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

// Parse a frame range, given as <startFrame>:<step>:<endFrame>
bool getFrameRange(std::string &frames, unsigned int &start, unsigned int &step, unsigned int &end)
{
    int nrDelim = -1;
    std::string::size_type pos = -1;
    
    do
    {
        pos = frames.find_first_of(":", pos+1);
        nrDelim++;
    } while (pos != std::string::npos);

    if (nrDelim < 1 || nrDelim > 2)
        return 0;
    
    unsigned int *range[3] = {&start, &step, &end};
    char *startPtr = &frames[0], *endPtr;
    
    for (size_t i=0; i<3; i+=3-nrDelim)
    {
        *(range[i]) = strtol(startPtr, &endPtr, 10);
        if (startPtr == endPtr)
            return 0;
        startPtr = endPtr + 1;
    }
    
    return 1;
}

// Parse parameter options from command line
bool setParams(int argc, char* argv[], LumaEncoderParams *params, IOData *io)
{
    std::string frames;
    std::string ptf, ptfValues[] = {"PSI", "PQ", "LOG", "HDRVDP", "LINEAR"}; // valid ptf input values
    std::string cs, csValues[] = {"LUV", "RGB", "YCBCR", "XYZ"}; // valid color space input values
    unsigned int bdValues[] = {8, 10, 12}; // valid bit depths

    // Application usage info
    std::string info = std::string("lumaenc -- Compress a sequence of high dyncamic range (HDR) frames in to a Matroska (.mkv) HDR video\n\n") +
                       std::string("Usage: lumaenc --input <hdr_frames> \\\n") +
                       std::string("               --frames <start_frame:step:end_frame> \\\n") +
                       std::string("               --output <output>\n");
    std::string postInfo = std::string("\nExample: lumaenc -i hdr_frame_%05d.exr -f 1:100 -o hdr_video.mkv\n\n") +
                           std::string("See man page for more information.");
    ArgParser argHolder(info, postInfo);

    // Input arguments
    argHolder.add(&io->hdrFrames,            "--input",             "-i",   "Input HDR video sequence");
    argHolder.add(&io->outputFile,           "--output",            "-o",   "Output location of the compressed HDR video", 0);
    argHolder.add(&frames,                   "--frames",            "-f",   "Input frames, formatted as startframe:step:endframe");
    argHolder.add(&params->fps,              "--framerate",         "-fps", "Framerate of video stream, specified as frames/s");
    argHolder.add(&params->profile,          "--profile",           "-p",   "VP9 encoding profile", (unsigned int)(0), (unsigned int)(3));
    argHolder.add(&params->quantizerScale,   "--quantizer-scaling", "-q",   "Scaling of the encoding quantization", (unsigned int)(0), (unsigned int)(63));
    argHolder.add(&params->preScaling,       "--pre-scaling",       "-sc",  "Scaling of pixels to apply before tranformation and encoding", 0.0f, 1e20f);
    argHolder.add(&params->ptfBitDepth,      "--ptf-bitdepth",      "-pb",  "Bit depth of the perceptual transfer function", (unsigned int)(0), (unsigned int)(16));
    argHolder.add(&params->colorBitDepth,    "--color-bitdepth",    "-cb",  "Bit depth of the color channels", (unsigned int)(0), (unsigned int)(16));
    argHolder.add(&ptf,                      "--transfer-function", "-ptf", "The perceptual transfer function used for encoding", ptfValues, 5);
    argHolder.add(&cs,                       "--color-space",       "-cs",  "Color space for encoding", csValues, 4);
    argHolder.add(&params->maxLum,           "--max-luminance",     "-ma",  "Maximum luminance in encoding (for PQ and LOG transfer function)", 100.0f, 1e5f);
    argHolder.add(&params->minLum,           "--min-luminance",     "-mi",  "Minimum luminance in encoding (for PQ and LOG transfer function)", 1e-10f, 99.99f);
    argHolder.add(&params->bitrate,          "--bitrate",           "-b",   "HDR video stream target bandwidth, in Kb/s", (unsigned int)(0), (unsigned int)(9999));
    argHolder.add(&params->keyframeInterval, "--keyframe-interval", "-k",   "Interval between keyframes. 0 for automatic keyframes", (unsigned int)(0), (unsigned int)(9999));
    argHolder.add(&params->bitDepth,         "--encoding-bitdepth", "-eb",  "Encoding at 8, 10 or 12 bits", bdValues, 3);
    argHolder.add(&params->lossLess,         "--lossless",          "-l",   "Enable lossless encoding mode");
    argHolder.add(&io->verbose,              "--verbose",           "-v",   "Verbose mode");

    // Parse arguments
    if (!argHolder.read(argc, argv))
        return 0;
    
    // Check output format
    if (!hasExtension(io->outputFile.c_str(), ".mkv"))
        throw ParserException("Unsupported output format. HDR video should be stored as Matroska file (.mkv)");
    
    // Parse frame range
    if (frames.size() > 0 && !getFrameRange(frames, io->startFrame, io->stepFrame, io->endFrame))
        throw ParserException(std::string("Unable to parse frame range from '" + frames + "'. Valid format is startframe:step:endframe").c_str());
    
    // Valid frame range?
    if (io->endFrame < io->startFrame)
        throw ParserException(std::string("Invalid frame range '" + frames + "'. End frame should be >= start frame").c_str());

    // Translate input strings to enums
    if (!strcmp(ptf.c_str(), ptfValues[0].c_str()))
        params->ptf = LumaQuantizer::PTF_PSI;
    else if (!strcmp(ptf.c_str(), ptfValues[1].c_str()))
        params->ptf = LumaQuantizer::PTF_PQ;
    else if (!strcmp(ptf.c_str(), ptfValues[2].c_str()))
        params->ptf = LumaQuantizer::PTF_LOG;
    else if (!strcmp(ptf.c_str(), ptfValues[3].c_str()))
        params->ptf = LumaQuantizer::PTF_JND_HDRVDP;
    else if (!strcmp(ptf.c_str(), ptfValues[4].c_str()))
        params->ptf = LumaQuantizer::PTF_LINEAR;

    if (!strcmp(cs.c_str(), csValues[0].c_str()))
        params->colorSpace = LumaQuantizer::CS_LUV;
    else if (!strcmp(cs.c_str(), csValues[1].c_str()))
        params->colorSpace = LumaQuantizer::CS_RGB;
    else if (!strcmp(cs.c_str(), csValues[2].c_str()))
        params->colorSpace = LumaQuantizer::CS_YCBCR;
    else if (!strcmp(cs.c_str(), csValues[3].c_str()))
        params->colorSpace = LumaQuantizer::CS_XYZ;

    return 1;
}

int main(int argc, char* argv[])
{
    // Holder for input/output options
    IOData io;
    
    // Encoder and encoder params
    LumaEncoder encoder;
    LumaEncoderParams params = encoder.getParams();

#ifdef HAVE_PFS    
    PfsInterface pfs; // Needs to store state between frames
#endif

    try
    {
        // Read parameters from input
        if (!setParams(argc, argv, &params, &io))
            return 1;
        
        // Set the encoder parameters    
        encoder.setParams(params);

#define STRBUF_LEN 500
        char str[STRBUF_LEN];
        int encoded_frame_count = 0;    
        for (unsigned int f = io.startFrame; f <= io.endFrame; f+=io.stepFrame)
        {        
            LumaFrame frame;

            // Read hdr frames, if available
            if( io.hdrFrames.size() == 0 || hasExtension( io.hdrFrames.c_str(), "pfs" ) )
            {
#ifdef HAVE_PFS
                if( !pfs.readFrame(io.hdrFrames.c_str(), frame) )
                    break;
#else
                throw LumaException( "Compiled without pfstools support" );          
#endif
            }
            else if ( strcmp( io.hdrFrames.c_str(), "__test__" ) == 0 )
                ExrInterface::testFrame(frame);
            else // OpenEXR?          
            {
                snprintf(str, STRBUF_LEN-1, io.hdrFrames.c_str(), f);
                ExrInterface::readFrame(str, frame);
            }

            // Initialize encoder
            if (!encoder.initialized())
                encoder.initialize(io.outputFile.size() == 0 ? "output.mkv" : io.outputFile.c_str(), frame.width, frame.height, io.verbose);

            fprintf(stderr, "Encoding frame %d... ", f);

            // Run the encoder
            encoder.encode(&frame);
            encoded_frame_count++;
            fprintf(stderr, "done\n");
        }

        encoder.finish();
        fprintf(stderr, "\n\nEncoding finished. %d frames encoded.\n", encoded_frame_count);

    }
    catch (ParserException &e)
    {
        fprintf(stderr, "\nlumaenc input error: %s\n", e.what());
        return 1;
    }
    catch (LumaException &e)
    {
        fprintf(stderr, "\nlumaenc encoding error: %s\n", e.what());
        return 1;
    }
    catch (std::exception & e)
    {
        fprintf(stderr, "\nlumaenc error: %s\n", e.what());
        return 1;
    }


    return 0;
}

