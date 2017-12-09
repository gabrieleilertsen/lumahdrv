/**
 * \class LumaFrame
 *
 * \brief Convenience structure for images.
 *
 * Convenience structure for Luma frames during encoding/decoding.
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

#ifndef LUMAFRAME_H
#define LUMAFRAME_H

#include <cstddef>

struct LumaFrame
{
    LumaFrame(unsigned int w = 0, unsigned int h = 0, unsigned int c = 3) : 
        height(h), width(w), channels(c), buffer(NULL)
    {
        if (height && width && channels)
            init();
    }
    
    ~LumaFrame() { clear(); }
    
    void clear()
    {
        if (buffer != NULL)
        {
            delete[] buffer;
            buffer = NULL;
        }
    }
    
    bool init()
    {
        if (!height && !width && !channels)
            return 0;
            
        clear();
        
        buffer = new float[channels*height*width];
        
        return 1;
    }
    
    float* getChannel(unsigned int c)
    {
        return buffer + c*height*width;
    }
    
    unsigned int height, width, channels;
    float *buffer;
};

#endif //LUMAFRAME_H
