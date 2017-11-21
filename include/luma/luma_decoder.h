/**
 * \brief The HDR video decoding classes and structures.
 *
 * LumaDecoderBase is a general virtual HDR video decoding interface, while 
 * LumaDecoder provides functionality for decoding VP9 encoded HDR videos. The 
 * input for decoding should be stored in a Matroska container (.mkv).
 *
 * The VPX codec: http://www.webmproject.org
 * Matroska container: http://www.matroska.org
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

#ifndef LUMA_DECODER_H
#define LUMA_DECODER_H

#include "luma_quantizer.h"
#include "mkv_interface.h"

#include "vpx_decoder.h"
#include "vp8dx.h"

#include <sys/time.h>

struct LumaDecoderParamsBase
{
    LumaDecoderParamsBase() : ptf(LumaQuantizer::PTF_PSI), colorSpace(LumaQuantizer::CS_LUV),
        preScaling(1.0f), minLum(0.005f), maxLum(1e4f)
    {}
    
    LumaQuantizer::ptf_t ptf;
    LumaQuantizer::colorSpace_t colorSpace;
    float preScaling, minLum, maxLum;
};


/**
 * \class LumaDecoderBase
 *
 * \brief Base class of HDRv decoding.
 *
 * LumaDecoderBase provides the virtual interface for decoding of HDR video.
 *
 */
 
class LumaDecoderBase
{
public:
    LumaDecoderBase(const char *inputFile = NULL, bool verbose = 0) : m_input(inputFile)
    {}

    virtual bool initialize(const char *inputFile, bool verbose = 0) = 0;
    virtual bool run() = 0;
    void seekToTime(float tm, bool absolute = false)
    {
        m_reader.seekToTime(tm, absolute);
    }
    
    virtual LumaFrame *decode() = 0;
    LumaQuantizer *getQuantizer() { return &m_quant; }
    MkvInterface *getReader() { return &m_reader; }
    LumaFrame *getFrame() { return &m_frame; }
    bool initialized() { return m_initialized; }
    
protected:
    bool m_initialized;
    const char* m_input;
    LumaQuantizer m_quant;
    MkvInterface m_reader;
    LumaFrame m_frame;
};





struct LumaDecoderParams : LumaDecoderParamsBase
{
    LumaDecoderParams() : ptfBitDepth(11), colorBitDepth(8), highBitDepth(true)
    {}
    
    unsigned int ptfBitDepth, colorBitDepth;
    bool highBitDepth;
    int *stride, profile, width[3], height[3];
};


/**
 * \class LumaDecoder
 *
 * \brief HDR video decoding using the VP9 codec.
 *
 * LumaDecoder uses the VP9 codec for decoding of HDR video. The HDR video is 
 * assumed to be stored in a Matroska containter.
 *
 * The VPX codec: http://www.webmproject.org
 * Matroska container: http://www.matroska.org
 *
 */
class LumaDecoder : public LumaDecoderBase
{
public:
    LumaDecoder(const char *inputFile = NULL, bool verbose = 0);
    ~LumaDecoder();

    bool initialize(const char *inputFile, bool verbose = 0);
    bool run();
    LumaFrame *decode()
    {
        if (!run())
            return NULL;
            
        if (!m_frame.width)
        {
            m_frame.width = m_vpxFrame->d_w;
            m_frame.height = m_vpxFrame->d_h;
            m_frame.channels = 3;
            m_frame.init();
        }
        
	    getVpxChannels();
        
        m_quant.transformColorSpace(&m_frame, false, m_params.preScaling);
        
        return &m_frame;
    }
    
    unsigned char **getBuffer() { return m_vpxFrame->planes; }
    LumaDecoderParams getParams() { return m_params; }
    void setParams(LumaDecoderParams params) { m_params = params; }
    
private:
    void getVpxChannels();

    vpx_codec_ctx_t m_codec;
    vpx_image_t *m_vpxFrame;
    LumaDecoderParams m_params;
    
    bool m_firstFrame;
};

#endif //LUMA_DECODER_H
