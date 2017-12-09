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

#include "luma_decoder.h"
#include "luma_exception.h"

#include <cstdio>
#include <math.h>
#include <algorithm>

LumaDecoder::LumaDecoder(const char *inputFile, bool verbose)
{
    m_initialized = false;
    
    if (inputFile != NULL)
    {
        m_input = inputFile;
        initialize(inputFile, verbose);
    }
}

LumaDecoder::~LumaDecoder()
{
    if (m_initialized && vpx_codec_destroy(&m_codec))
        fprintf(stderr, "Failed to destroy vpx codec\n");
}

bool LumaDecoder::initialize(const char *inputFile, bool verbose)
{
    if (inputFile == NULL)
        return false;
    
    m_input = inputFile;
    
    // Open Matroska file
    m_reader.openRead(inputFile);
    m_reader.setVerbose(verbose);
    
    // Read attachments
    binary *buffer;
    unsigned int ind = 0, id, buffer_size, mapping_size;
    float *mapping;
    bool ptfBDFound = 0, colorBDFound = 0, ptfFound = 0, csFound = 0, mappingFound = 0;
    while (m_reader.getAttachment(ind++, &buffer, id, buffer_size))
    {
        switch (id)
        {
        case 430:
            m_params.ptfBitDepth = *((unsigned int*)buffer);
            ptfBDFound = 1;
            break;
        case 431:
            m_params.colorBitDepth = *((unsigned int*)buffer);
            colorBDFound = 1;
            break;
        case 432:
            m_params.ptf = *((LumaQuantizer::ptf_t*)buffer);
            ptfFound = 1;
            break;
        case 433:
            m_params.colorSpace = *((LumaQuantizer::colorSpace_t*)buffer);
            csFound = 1;
            break;
        case 434:
            mapping = (float*)buffer;
            mapping_size = buffer_size;
            mappingFound = 1;
            break;
        case 435:
            m_params.preScaling = *((float*)buffer);
            break;
        case 436:
            m_params.maxLum = ((float*)buffer)[0];
            m_params.minLum = ((float*)buffer)[1];
            break;
        }
    }
    
    if (!(ptfBDFound && colorBDFound && ptfFound && csFound && mappingFound))
    {
        std::string msg = "Failed to locate Luma HDRv meta data in '" + std::string(inputFile) + "'";
        throw LumaException(msg.c_str());
    }
    
    // Initialize quantizer
    m_quant.setQuantizer(m_params.ptf, m_params.ptfBitDepth, m_params.colorSpace, m_params.colorBitDepth, m_params.maxLum, m_params.minLum);
    memcpy((void*)m_quant.getMapping(), (void*)mapping, mapping_size);
    
    // Initialize VPX codec
    const vpx_codec_iface_t *(*const vpx_decoder)() = &vpx_codec_vp9_dx;

    fprintf(stderr, "\nDecoding options:\n");
    fprintf(stderr, "-------------------------------------------------------------------\n");
    fprintf(stderr, "Transfer function (PTF):   %s\n", ptfFound ? m_quant.name(m_params.ptf).c_str() : "--");
    if (m_params.ptf == LumaQuantizer::PTF_PQ || m_params.ptf == LumaQuantizer::PTF_LOG || m_params.ptf == LumaQuantizer::PTF_LINEAR)
        fprintf(stderr, "Encoding luminance range:  %.4f-%.2f\n", m_quant.getMinLum(), m_quant.getMaxLum());
    fprintf(stderr, "Color space:               %s\n", csFound ? m_quant.name(m_params.colorSpace).c_str() : "--");
    fprintf(stderr, "PTF bit depth:             %d\n", ptfBDFound ? m_params.ptfBitDepth : -1);
    fprintf(stderr, "Color bit depth:           %d\n", colorBDFound ? m_params.colorBitDepth : -1);
    fprintf(stderr, "Codec:                     %s\n", vpx_codec_iface_name(vpx_decoder()));
    fprintf(stderr, "-------------------------------------------------------------------\n\n");

    vpx_codec_dec_cfg_t cfg;
    cfg.threads = 4;
    if (vpx_codec_dec_init(&m_codec, vpx_decoder(), &cfg, 0))
        fprintf(stderr, "Failed to initialize decoder.\n");
    
    // Get first frame
    m_firstFrame = false;
    m_initialized = true;
    if (!run())
    {
        m_initialized = false;
        return false;
    }
    
    m_firstFrame = true;
    
    m_params.highBitDepth = m_vpxFrame->fmt & VPX_IMG_FMT_HIGHBITDEPTH;
    const int m = (m_params.highBitDepth ? 2 : 1);    
    m_params.stride = m_vpxFrame->stride;
    m_params.profile = m_vpxFrame->x_chroma_shift ? 2*m-2 : 2*m-1;
    m_params.width[0] = m_vpxFrame->d_w;
    m_params.width[1] = m_vpxFrame->x_chroma_shift > 0 ? (m_vpxFrame->d_w + 1) >> m_vpxFrame->x_chroma_shift : m_vpxFrame->d_w;
    m_params.width[2] = m_vpxFrame->x_chroma_shift > 0 ? (m_vpxFrame->d_w + 1) >> m_vpxFrame->x_chroma_shift : m_vpxFrame->d_w;
    m_params.height[0] = m_vpxFrame->d_h;
    m_params.height[1] = m_vpxFrame->y_chroma_shift > 0 ? (m_vpxFrame->d_h + 1) >> m_vpxFrame->y_chroma_shift : m_vpxFrame->d_h;
    m_params.height[2] = m_vpxFrame->y_chroma_shift > 0 ? (m_vpxFrame->d_h + 1) >> m_vpxFrame->y_chroma_shift : m_vpxFrame->d_h;
    
    return true;
}

bool LumaDecoder::run()
{
    if (!m_initialized)
        if(!initialize(m_input))
            return false;
    
    if (m_firstFrame)
    {
        m_firstFrame = false;
        return true;
    }
    
    //timeval start, stop;
    //gettimeofday(&start, NULL);
    
    m_vpxFrame = NULL;
    vpx_codec_iter_t iter = NULL;
    
    if (!m_reader.readFrame()) // Reading frame failed, probably EOF
        return false;

    unsigned int frame_size = 0;
    const uint8_t *frame = m_reader.getFrame(frame_size);
    
    if (vpx_codec_decode(&m_codec, frame, frame_size, NULL, 0))
        throw LumaException("Failed to decode frame");
    
    if ((m_vpxFrame = vpx_codec_get_frame(&m_codec, &iter)) == NULL)
        throw("Failed to get decoded frame");
    
    //gettimeofday(&stop, NULL);
    //fprintf(stderr, "DECODING TIME: %f\n", (stop.tv_usec-start.tv_usec)/1000.0f);
	
	return true;
}


void LumaDecoder::getVpxChannels()
{
    for (unsigned int plane=0; plane<3; plane++)
    {
        float *dest = m_frame.getChannel(plane);
        unsigned char *buf = m_vpxFrame->planes[plane];
        //unsigned short *bufS = (unsigned short*)(m_vpxFrame->planes[plane]);
        const int w = m_params.width[plane], h = m_params.height[plane], stride = m_params.stride[plane], profile = m_params.profile;
        
        //fprintf(stderr, "--plane %d: %dx%d. stride = %d, bit depth = %d, profile = %d\n", plane, w, h, stride, profile>1?16:8, profile);
        
        size_t ind1, ind2;
        float val;
        for (int y=0; y<h; y++)
        {
            for (int x=0; x<w; x++)
            {
                if (profile>1)
                	val = m_quant.dequantize(buf[2*x+y*stride+1]*256.0f + buf[2*x+y*stride], plane);
                	//val = m_quant.dequantize((float)((buf[m*x+y*stride+1]<<8) | buf[m*x+y*stride]), plane);
                	//val = m_quant.dequantize(bufS[x+y*stride/2], plane);
                else
                	val = m_quant.dequantize(buf[x+y*stride], plane);
                
                if (plane && (profile == 2 || profile == 0))
                {
                    ind1 = 2*x+4*y*w;
                    ind2 = ind1 + 2*w;
                    dest[ind1] = dest[ind1+1] = dest[ind2] = dest[ind2+1] = val;
                }
                else
                    dest[x+y*w] = val;
            }
        }
    }
}
