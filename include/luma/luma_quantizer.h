/**
 * \class LumaQuantizer
 *
 * \brief HDR video quantization algorithms.
 *
 * LumaQuantizer provides algorithms to map an HDR input to/from the range 
 * provided by the specific codec used for encoding/decoding. This includes
 * both luminance and color transformations.
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

#ifndef LUMA_QUANTIZER_H
#define LUMA_QUANTIZER_H

#include "luma_frame.h"

#include <string>

const float ptf_jnd_ferwerda_10bit[] = {
#include "ptfs/ptf_jnd_ferwerda_10bit.h"
};

const float ptf_jnd_ferwerda_11bit[] = {
#include "ptfs/ptf_jnd_ferwerda_11bit.h"
};

const float ptf_jnd_ferwerda_12bit[] = {
#include "ptfs/ptf_jnd_ferwerda_12bit.h"
};

const float ptf_jnd_hdrvdp_10bit[] = {
#include "ptfs/ptf_jnd_hdrvdp_10bit.h"
};

const float ptf_jnd_hdrvdp_11bit[] = {
#include "ptfs/ptf_jnd_hdrvdp_11bit.h"
};

const float ptf_jnd_hdrvdp_12bit[] = {
#include "ptfs/ptf_jnd_hdrvdp_12bit.h"
};

static const float rgb2xyzMat[3][3] =
{ { 0.412424f, 0.357579f, 0.180464f },
  { 0.212656f, 0.715158f, 0.072186f },
  { 0.019332f, 0.119193f, 0.950444f } };

static const float xyz2rgbMat[3][3] =
{ {  3.240708f, -1.537259f, -0.498570f },
  { -0.969257f,  1.875995f,  0.041555f },
  {  0.055636f, -0.203996f,  1.057069f } };

class LumaQuantizer
{
public:
    LumaQuantizer();
    ~LumaQuantizer();
    
    enum ptf_t {PTF_PSI, PTF_PQ, PTF_LOG, PTF_JND_HDRVDP, PTF_LINEAR};
    enum colorSpace_t {CS_LUV, CS_RGB, CS_YCBCR, CS_XYZ};
    static std::string name(ptf_t ptf);
    static std::string name(colorSpace_t cs);
    
    void setQuantizer(ptf_t ptf, unsigned int bitdepth,
                      colorSpace_t cs, unsigned int bitdepthC,
                      float maxLum, float minLum);
    
    float quantize(const float val, const unsigned int ch) const;
    float dequantize(const float val, const unsigned int ch) const;
    
    bool transformColorSpace(LumaFrame *frame, bool toCs, float sc);
    const float *getMapping(){ return m_mapping; }
    unsigned int getSize() { return m_maxVal; }
    float getMaxLum() { return m_Lmax; };
    float getMinLum() { return m_Lmin; };
private:
    void setMappingPQ();
    void setMappingLog();
    void setMappingJNDHDRVDP();
    void setMappingPsi();
    float transformPQ(float val, bool encode);
    float transformLog(float val, bool encode);
    
    colorSpace_t m_colorSpace;
    float* m_mapping;
    
    float m_Lmax, m_Lmin;
    
    unsigned int m_maxVal, m_maxValColor, m_bitdepth, m_bitdepthColor;
};

#endif //LUMA_QUANTIZER_H
