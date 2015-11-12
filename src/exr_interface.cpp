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

#include "exr_interface.h"
#include "luma_exception.h"

#include <ImfRgbaFile.h>
#include <ImfArray.h>

using namespace Imf;
using namespace Imath;


// Generate a test frame
bool ExrInterface::testFrame(LumaFrame &frame, unsigned int w, unsigned int h)
{
    frame.width = w;
    frame.height = h;
    frame.channels = 3;
    if (!frame.init())
        throw LumaException("Cannot allocate memory for input frame");
    
    for (size_t y=0; y<h; y++)
        for (size_t x=0; x<w; x++)
        {
            frame.getChannel(0)[x+y*w] = y<h/5 ? (y<h/10 ? 10000.0f*((float)(x*x))/(w*w) : 10000.0f*( (20*x)/w )/20.0f) : 
                                     10000.0f*( ((20*y/h)%2)^((30*x/w)%2) );
            frame.getChannel(1)[x+y*w] = y<h/5 ? (y<h/10 ? 10000.0f*((float)(x*x))/(w*w) : 10000.0f*( (20*x)/w )/20.0f) : 
                                     10000.0f*((20*y/h)%2)*((float)(y*y))/(h*h);
            frame.getChannel(2)[x+y*w] = y<h/5 ? (y<h/10 ? 10000.0f*((float)(x*x))/(w*w) : 10000.0f*( (20*x)/w )/20.0f) : 
                                     10000.0f*((20*y/h)%2)*((float)(x*x))/(w*w);
        }
    
    return 1;
}

// Read an exr frame from file
bool ExrInterface::readFrame(const char *inputFile, LumaFrame &frame)
{
    try
    {
        Array2D<Rgba> pixels;
        RgbaInputFile file(inputFile);
        Box2i dw = file.dataWindow();
        unsigned int w  = dw.max.x - dw.min.x + 1,
                     h = dw.max.y - dw.min.y + 1;
        pixels.resizeErase(h, w);
        file.setFrameBuffer(&pixels[0][0] - dw.min.x - dw.min.y*w, 1, w);
        file.readPixels(dw.min.y, dw.max.y);
        
        frame.width = w;
        frame.height = h;
        frame.channels = 3;
        if (!frame.init())
            throw LumaException("Cannot allocate memory for input frame");
        
        switch (file.channels())
        {
            case WRITE_RGB:
            case WRITE_RGBA:
            {
                for (size_t y=0; y<h; y++)
                    for (size_t x=0; x<w; x++)
                    {
                        Rgba &p = pixels[y][x];
                        frame.getChannel(0)[x+y*w] = p.r;
                        frame.getChannel(1)[x+y*w] = p.g;
                        frame.getChannel(2)[x+y*w] = p.b;
                    }
	        }
	        break;
	        case WRITE_R:
	        {
                for (size_t y=0; y<h; y++)
                    for (size_t x=0; x<w; x++)
                    {
                        Rgba &p = pixels[y][x];
                        frame.getChannel(0)[x+y*w] = p.r;
                        frame.getChannel(1)[x+y*w] = p.r;
                        frame.getChannel(2)[x+y*w] = p.r;
                    }
	        }
	        break;
	        case WRITE_G:
	        {
                for (size_t y=0; y<h; y++)
                    for (size_t x=0; x<w; x++)
                    {
                        Rgba &p = pixels[y][x];
                        frame.getChannel(0)[x+y*w] = p.g;
                        frame.getChannel(1)[x+y*w] = p.g;
                        frame.getChannel(2)[x+y*w] = p.g;
                    }
	        }
	        break;
	        case WRITE_B:
	        {
                for (size_t y=0; y<h; y++)
                    for (size_t x=0; x<w; x++)
                    {
                        Rgba &p = pixels[y][x];
                        frame.getChannel(0)[x+y*w] = p.b;
                        frame.getChannel(1)[x+y*w] = p.b;
                        frame.getChannel(2)[x+y*w] = p.b;
                    }
	        }
	        break;
	        default:
	            throw LumaException("Reading of luminance only frames not yet supported");
	            break;
	    }
    }
    catch (const std::exception &e)
    {
        throw LumaException(e.what());
    }
    
    return 1;
}

// Write an exr frame to file
bool ExrInterface::writeFrame(const char *outputFile, LumaFrame &frame)
{
    if (frame.buffer == NULL)
        throw LumaException("Frame does not contain any data");
    
    try
    {
        unsigned int w = frame.width, h = frame.height;
        Array2D<Rgba> pixels;
        pixels.resizeErase(h, w);
        for (size_t y=0; y<h; y++)
	        for (size_t x=0; x<w; x++)
	        {
	            Rgba &p = pixels[y][x];
	            p.r = frame.getChannel(0)[x+y*w];
	            p.g = frame.getChannel(1)[x+y*w];
	            p.b = frame.getChannel(2)[x+y*w];
	            p.a = 0;
	        }
        
        RgbaOutputFile file(outputFile, w, h, WRITE_RGB);
        file.setFrameBuffer(&pixels[0][0], 1, w);
        file.writePixels(h);
    }
    catch (const std::exception &e)
    {
        throw LumaException(e.what());
    }
    
    return 1;
}


