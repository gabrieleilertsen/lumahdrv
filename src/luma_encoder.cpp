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

#include "luma_encoder.h"
#include "luma_exception.h"

#include <math.h>
#include <algorithm>

LumaEncoder::LumaEncoder()
{
	m_frameCount = 0;

    m_initialized = false;
}

LumaEncoder::~LumaEncoder()
{
    if (m_initialized)
    {
        vpx_img_free(&m_rawFrame);
        if (vpx_codec_destroy(&m_codec))
	        fprintf(stderr, "Failed to destroy codec.\n");
	}
}

// Initialize encoder, given setup in the encoder parameters
bool LumaEncoder::initialize(const char *outputFile, const unsigned int w, const unsigned int h, bool verbose)
{
    // Initialize base. Creates a Matroska file for writing
    LumaEncoderBase::initialize(outputFile, w, h, m_params.maxLum, m_params.minLum);
    
    // Adjust profile for the specified bit depth (0-1 for 8 bits, and 2-3 for higher bit depths)
    if (m_params.profile > 1 && m_params.bitDepth == 8)
        m_params.profile -=2;
    if (m_params.profile < 2 && m_params.bitDepth > 8)
        m_params.profile +=2;
    
    // Initialize quantizer
    m_quant.setQuantizer(m_params.ptf, m_params.ptfBitDepth, m_params.colorSpace, m_params.colorBitDepth, m_params.maxLum, m_params.minLum);
    
    // Add attachments with meta data to Matroska file
    unsigned int *buffer1 = new unsigned int;
    *buffer1 = m_params.ptfBitDepth;
    m_writer.addAttachment(430, (const binary*)buffer1, sizeof(unsigned int), "PTF bit depth");
    
    unsigned int *buffer2 = new unsigned int;
    *buffer2 = m_params.colorBitDepth;
    m_writer.addAttachment(431, (const binary*)buffer2, sizeof(unsigned int), "Color bit depth");
    
    LumaQuantizer::ptf_t *buffer3 = new LumaQuantizer::ptf_t;
    *buffer3 = m_params.ptf;
    m_writer.addAttachment(432, (const binary*)buffer3, sizeof(LumaQuantizer::ptf_t), "PTF description");
    
    LumaQuantizer::colorSpace_t *buffer4 = new LumaQuantizer::colorSpace_t;
    *buffer4 = m_params.colorSpace;
    m_writer.addAttachment(433, (const binary*)buffer4, sizeof(LumaQuantizer::colorSpace_t), "Color space");
    
    float *buffer5 = new float[m_quant.getSize()];
    memcpy((void*)buffer5, (void*)m_quant.getMapping(), (m_quant.getSize())*sizeof(float));
    m_writer.addAttachment(434, (const binary*)buffer5, (m_quant.getSize())*sizeof(float), "PTF");
    
    float *buffer6 = new float;
    *buffer6 = m_params.preScaling;
    m_writer.addAttachment(435, (const binary*)buffer6, sizeof(float), "Scaling");

    float *buffer7 = new float[2];
    buffer7[0] = m_params.maxLum; buffer7[1] = m_params.minLum;
    m_writer.addAttachment(436, (const binary*)buffer7, 2*sizeof(float), "Luminance range");
    
    m_writer.writeAttachments();
    
    // Framerate for timecodes    
    m_writer.setFramerate(m_params.fps);
    
    m_writer.setVerbose(verbose);
    
    // Initialize VPX codec
    vpx_codec_err_t res;
	vpx_codec_enc_cfg_t cfg;
	const vpx_codec_iface_t *(*const vpx_encoder)() = &vpx_codec_vp9_cx;

    if (w <= 0 || h <= 0 || (w % 2) != 0 || (h % 2) != 0)
        throw LumaException("Invalid frame size");

    if (m_params.profile == 0 && !vpx_img_alloc(&m_rawFrame, VPX_IMG_FMT_I420, w, h, 32))
	    throw LumaException("Failed to allocate 8 bit 420 image");
    else if (m_params.profile == 1 && !vpx_img_alloc(&m_rawFrame, VPX_IMG_FMT_I444, w, h, 32))
	    throw LumaException("Failed to allocate 8 bit 444 image");
    else if (m_params.profile == 2 && !vpx_img_alloc(&m_rawFrame, VPX_IMG_FMT_I42016, w, h, 32))
	    throw LumaException("Failed to allocate 16 bit 420 image");
    else if (m_params.profile == 3 && !vpx_img_alloc(&m_rawFrame, VPX_IMG_FMT_I44416, w, h, 32))
	    throw LumaException("Failed to allocate 16 bit 444 image");

    res = vpx_codec_enc_config_default(vpx_encoder(), &cfg, 0);
    if (res)
	    throw LumaException("Failed to get default codec config");

    cfg.g_threads = 6;
    cfg.rc_min_quantizer = m_params.quantizerScale;
    cfg.rc_max_quantizer = m_params.quantizerScale;
    cfg.g_w = w;
    cfg.g_h = h;
    cfg.g_timebase.num = 1;
    cfg.g_timebase.den = 25;
    cfg.rc_target_bitrate = m_params.bitrate;
    cfg.g_error_resilient = 0;
    cfg.g_pass = VPX_RC_ONE_PASS;
    cfg.rc_end_usage = VPX_VBR;
    cfg.g_lag_in_frames = 0;
    cfg.rc_end_usage = VPX_Q;
    cfg.kf_max_dist = 25;
    cfg.kf_mode = VPX_KF_AUTO;
    cfg.g_profile = m_params.profile;
    
    fprintf(stderr, "Encoding options:\n");
    fprintf(stderr, "-------------------------------------------------------------------\n");
    fprintf(stderr, "Transfer function (PTF):   %s\n", m_quant.name(m_params.ptf).c_str());
    fprintf(stderr, "Color space:               %s\n", m_quant.name(m_params.colorSpace).c_str());
    fprintf(stderr, "PTF bit depth:             %d\n", m_params.ptfBitDepth);
    fprintf(stderr, "Color bit depth:           %d\n", m_params.colorBitDepth);
    if (m_params.ptf == LumaQuantizer::PTF_PQ || m_params.ptf == LumaQuantizer::PTF_LOG || m_params.ptf == LumaQuantizer::PTF_LINEAR)
        fprintf(stderr, "Encoding luminance range:  %.4f-%.2f\n", m_quant.getMinLum(), m_quant.getMaxLum());
    fprintf(stderr, "Encoding profile:          %d (4%d%d)\n", m_params.profile, (m_params.profile%2==0) ? 2 : 4, (m_params.profile%2==0) ? 2 : 4);
    fprintf(stderr, "Encoding bit depth:        ");
    if (m_params.bitDepth == 8 || m_params.profile < 2)
    {
        cfg.g_bit_depth = VPX_BITS_8;
        fprintf(stderr, "8\n");
    }
    else if (m_params.bitDepth == 10)
    {
        cfg.g_bit_depth = VPX_BITS_10;
        fprintf(stderr, "10\n");
    }
    else
    {
        cfg.g_bit_depth = VPX_BITS_12;
        fprintf(stderr, "12\n");
    }
    fprintf(stderr, "Codec:                     %s\n", vpx_codec_iface_name(vpx_encoder()));
    fprintf(stderr, "Output:                    %s\n", outputFile);
    fprintf(stderr, "-------------------------------------------------------------------\n\n");

    int flags = m_params.profile < 2 ? 0 : VPX_CODEC_USE_HIGHBITDEPTH;
    if (vpx_codec_enc_init(&m_codec, vpx_encoder(), &cfg, flags))
	    throw LumaException("Failed to initialize vpxEncoder\n");

    // Let the codec know if CbCr color space is used, for third party decoding purposes
    if (m_params.colorSpace == LumaQuantizer::CS_YCBCR && vpx_codec_control(&m_codec, VP9E_SET_COLOR_SPACE, 5))
        fprintf(stderr, "Warning! Failed to set color space of encoder. Color primaries may not be recognized during decoding.\n\n");
	
	if (m_params.lossLess && vpx_codec_control(&m_codec, VP9E_SET_LOSSLESS, 1))
	    throw LumaException("Failed to use lossless mode\n");
	
    m_initialized = true;
    return true;
}

// Set vpx encoding channels from input frame
void LumaEncoder::setChannels(LumaFrame *frame)
{
    setVpxChannel(&m_rawFrame, frame->getChannel(0), 0);
	setVpxChannel(&m_rawFrame, frame->getChannel(1), 1);
	setVpxChannel(&m_rawFrame, frame->getChannel(2), 2);
}

// Run the encoder
bool LumaEncoder::run()
{
	int flags = 0;
	
	// Force key frame?
	if (m_params.keyframeInterval > 0 && m_frameCount % m_params.keyframeInterval == 0)
		flags = VPX_EFLAG_FORCE_KF;
    
    // Start encoder
	encode_frame_vpx(&m_codec, &m_rawFrame, m_frameCount++, flags);
	
	return true;
}

// Finish encoding buffered frames
void LumaEncoder::finish()
{
    // Flush vpx encoder.
    while (encode_frame_vpx(&m_codec, NULL, -1, 0)) {}; 
    
    LumaEncoderBase::finish();
}


// Encoding of one frame
int LumaEncoder::encode_frame_vpx(vpx_codec_ctx_t *codec,
                                  vpx_image_t *img,
                                  int frame_index,
                                  int flags)
{
    int got_pkts = 0;
    vpx_codec_iter_t iter = NULL;
    const vpx_codec_cx_pkt_t *pkt = NULL;
    const vpx_codec_err_t res = vpx_codec_encode(codec, img, frame_index, 1,
                                                 flags, VPX_DL_GOOD_QUALITY); // VPX_DL_GOOD_QUALITY/VPX_DL_REALTIME
    if (res != VPX_CODEC_OK)
        fprintf(stderr, "Failed to encode frame\n");

    while ((pkt = vpx_codec_get_cx_data(codec, &iter)) != NULL)
    {
        got_pkts = 1;

        if (pkt->kind == VPX_CODEC_CX_FRAME_PKT)
        {
            const int keyframe = (pkt->data.frame.flags & VPX_FRAME_IS_KEY) != 0;
            
            m_writer.addFrame((const uint8_t*)pkt->data.frame.buf, pkt->data.frame.sz, keyframe);
            //fprintf(stderr, keyframe ? "K" : ".");
            fflush(stdout);
        }
    }

    return got_pkts;
}

// Convert a frame buffer to a vpx frame
void LumaEncoder::setVpxChannel(vpx_image_t *dest, const float *src, int plane)
{
    unsigned char *buf = dest->planes[plane];
    const int stride = dest->stride[plane];
    
    // Width and height depends on chroma sub sampling for the color channels
    const int w = (plane > 0 && dest->x_chroma_shift > 0) ?
                  (dest->d_w + 1) >> dest->x_chroma_shift : dest->d_w;
    const int h = (plane > 0 && dest->y_chroma_shift > 0) ? 
                  (dest->d_h + 1) >> dest->y_chroma_shift : dest->d_h;
    
    const int m = ((dest->fmt & VPX_IMG_FMT_HIGHBITDEPTH) ? 2 : 1);
    const int profile = m_params.profile;
    
    //fprintf(stderr, "--plane %d: %dx%d. stride = %d, bit depth = %dx8, profile = %d\n", plane, w, h, stride, m, profile);
    
    float avg = 0.0f;
    
    size_t ind1, ind2;
    float res;
    for (int y=0; y<h; y++)
    {
        for (int x=0; x<w; x++)
        {
            // Color sub sampling, as simple average
            if (plane && (profile == 2 || profile == 0))
            {
                    ind1 = 2*x+4*y*w;
                    ind2 = ind1 + 2*w; //2*x+(2*y+1)*2*w;
                    res = 0.25f*(src[ind1] + src[ind1+1] + src[ind2] + src[ind2+1]);
            }
            else
            {
                res = src[x+y*w];
                avg += res;
            }
            
            // Quantize the pixel
       		res = m_quant.quantize(res, plane);
            
            // For high bit depths, split pixel into separate bytes (big endian)
            if (profile > 1)
            {
                unsigned char bl = res/256;
                unsigned char bh = res - bl*256;
                buf[m*x+y*stride+1] = bl; //* ((unsigned char *)&val); //low byte
                buf[m*x+y*stride] = bh; //* ((unsigned char *)((&val)+1)); //high byte
            }
            else
                buf[m*x+y*stride] = res;
        }
    }
    
    // Warn if average luminace is < 1, which could imply uncalibrated input
    avg /= (w*h);
    if (!plane && avg <= 1.0f)
        fprintf(stderr, "\n\tWarning! Mean luminance is %f cd/m2. Is input calibrated to physical units? \n", avg);
}

