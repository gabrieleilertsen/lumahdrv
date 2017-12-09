/**
 * \brief The HDR video encoding classes and structures.
 *
 * LumaEncoderBase is a general virtual HDR video encoding interface, while 
 * LumaEncoder provides functionality for encoding HDR videos with the VP9 
 * codec. The encoded videos are stored in a Matroska container (.mkv).
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

#ifndef LUMA_ENCODER_H
#define LUMA_ENCODER_H

#include "luma_quantizer.h"
#include "mkv_interface.h"
#include "luma_frame.h"

#include "vpx_encoder.h"
#include "vp8cx.h"

struct LumaEncoderParamsBase
{
    LumaEncoderParamsBase() : 
        quantizerScale(2), ptfBitDepth(11), colorBitDepth(8), preScaling(1.0f), minLum(0.005f), maxLum(1e4f),
        fps(25.0f), ptf(LumaQuantizer::PTF_PQ), colorSpace(LumaQuantizer::CS_LUV)
    {}
    
    unsigned int quantizerScale, ptfBitDepth, colorBitDepth;
    float preScaling, fps, minLum, maxLum;
    LumaQuantizer::ptf_t ptf;
    LumaQuantizer::colorSpace_t colorSpace;
};


/**
 * \class LumaEncoderBase
 *
 * \brief Base class of HDRv encoding.
 *
 * LumaEncoderBase provides the virtual interface for encoding of HDR video.
 *
 */
class LumaEncoderBase
{
public:
    virtual bool initialize(const char *outputFile, 
                            const unsigned int w, const unsigned int h,
                            const float ma, const float mi,
                            bool verbose = 0)
    {
        m_writer.openWrite(outputFile, w, h, ma, mi);
        return true;
    }
    
    virtual bool run() = 0;
    virtual void setChannels(LumaFrame *frame) = 0;
    virtual bool encode(LumaFrame *frame) = 0;
    virtual void finish() { m_writer.close(); };
    
    bool initialized() { return m_initialized; }
    
protected:
    bool m_initialized;
    LumaQuantizer m_quant;
    MkvInterface m_writer;
};




// VPX HDRv encoder params
struct LumaEncoderParams : LumaEncoderParamsBase
{
    LumaEncoderParams() : 
        bitrate(10000), profile(2), keyframeInterval(0), bitDepth(12), lossLess(false)
    {}
    
    unsigned int bitrate, profile, keyframeInterval, bitDepth;
    bool lossLess;
};


/**
 * \class LumaEncoder
 *
 * \brief HDR video encoding using the VP9 codec.
 *
 * LumaEncoder uses the VP9 codec for encoding of HDR video. The encoded HDR 
 * video is stored in a Matroska containter.
 *
 * The VPX codec: http://www.webmproject.org
 * Matroska container: http://www.matroska.org
 *
 */
class LumaEncoder : public LumaEncoderBase
{
public:
    LumaEncoder();
    ~LumaEncoder();

    bool initialize(const char *outputFile, const unsigned int w, const unsigned int h, bool verbose = 0);
    bool run();
    void setChannels(LumaFrame *frame);
    bool encode(LumaFrame *frame)
    {
        m_quant.transformColorSpace(frame, true, m_params.preScaling);
        setChannels(frame);
        
        return run();
    }
    void finish();
    
    LumaEncoderParams getParams() { return m_params; }
    void setParams(LumaEncoderParams params) { m_params = params; };
    
private:
    int encode_frame_vpx(vpx_codec_ctx_t *codec,
                         vpx_image_t *img,
                         int frame_index,
                         int flags);
    void setVpxChannel(vpx_image_t *dest, const float *src, int plane);
    
    vpx_codec_ctx_t m_codec;
	vpx_image_t m_rawFrame;
	unsigned int m_frameCount;
	
    LumaEncoderParams m_params;
};

#endif //LUMA_ENCODER_H
