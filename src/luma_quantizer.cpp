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

#include "luma_quantizer.h"

#include <stdio.h>
#include <math.h>
#include <algorithm>

LumaQuantizer::LumaQuantizer()
{
    m_Lmax = 10000.0f;
    m_Lmin = 0.005f;
    
    m_mapping = NULL;
    m_colorSpace = CS_LUV;
}

LumaQuantizer::~LumaQuantizer()
{
    if (m_mapping != NULL)
        delete[] m_mapping;
}

// Names of perceptual transfer functions
std::string LumaQuantizer::name(ptf_t ptf)
{
    std::string name;
    switch (ptf)
    {
    case PTF_PQ:
        name = "Perceptual quantizer (PQ, SMPTE ST 2084)";
        break;
    case PTF_LOG:
        name = "Logarithmic";
        break;
    case PTF_JND_HDRVDP:
        name = "JND HDR-VDP";
        break;
    case PTF_PSI:
        name = "Perceptual - Ferwerda's t.v.i.";
        break;
    case PTF_LINEAR:
        name = "Linear scaling";
        break;
    default:
        name = "Undefined";
    }
    
    return name;
}

// Names of color spaces
std::string LumaQuantizer::name(colorSpace_t cs)
{
    std::string name;
    switch (cs)
    {
    case CS_LUV:
        name = "Lu'v'";
        break;
    case CS_RGB:
        name = "RGB";
        break;
    case CS_YCBCR:
        name = "YCbCr (ITU-R BT.2020)";
        break;
    case CS_XYZ:
        name = "XYZ";
        break;
    default:
        name = "Undefined";
    }
    
    return name;
}

// Define PQ mapping
void LumaQuantizer::setMappingPQ()
{
    for (size_t i=0; i<=m_maxVal; i++)
	    m_mapping[i] = transformPQ((float)i/m_maxVal, 0);
}

// Define log mapping
void LumaQuantizer::setMappingLog()
{
    for (size_t i=0; i<=m_maxVal; i++)
        m_mapping[i] = transformLog((float)i/m_maxVal, 0);
}

// Define JND HDR-VDP2 mapping
void LumaQuantizer::setMappingJNDHDRVDP()
{
    const float *mapping;
    switch (m_bitdepth)
    {
    case 10:
        mapping = ptf_jnd_hdrvdp_10bit;
        break;
    case 11:
        mapping = ptf_jnd_hdrvdp_11bit;
        break;
    case 12:
    default:
        mapping = ptf_jnd_hdrvdp_12bit;
        break;
    }
    
    for (size_t i=0; i<=m_maxVal; i++)
        m_mapping[i] = mapping[i];
}

// Define mapping from original HDR video encdoing paper
void LumaQuantizer::setMappingPsi()
{
    const float *mapping;
    switch (m_bitdepth)
    {
    case 10:
        mapping = ptf_jnd_ferwerda_10bit;
        break;
    case 11:
        mapping = ptf_jnd_ferwerda_11bit;
        break;
    case 12:
    default:
        mapping = ptf_jnd_ferwerda_12bit;
        break;
    }
    
    for (size_t i=0; i<=m_maxVal; i++)
        m_mapping[i] = mapping[i];
}

// Specify the quanizer to use, which includes ptf, color space and their respective bit depths
void LumaQuantizer::setQuantizer(ptf_t ptf, unsigned int bitdepth,
                                 colorSpace_t cs, unsigned int bitdepthC,
                                 float maxLum, float minLum)
{
    if (m_mapping != NULL)
        delete[] m_mapping;
    
    m_bitdepth = bitdepth;
    m_maxVal = (int)pow(2.0f,(float)bitdepth)-1;
    m_colorSpace = cs;
    m_bitdepthColor = bitdepthC;
    m_maxValColor = (int)pow(2.0f,(float)bitdepthC)-1;
    m_Lmax = maxLum;
    m_Lmin = minLum;
    
    m_mapping = new float[m_maxVal+1];
    
    switch (ptf)
    {
    case PTF_PQ:
        setMappingPQ();
        break;
    case PTF_LOG:
        setMappingLog();
        break;
    case PTF_JND_HDRVDP:
        setMappingJNDHDRVDP();
        break;
    case PTF_LINEAR:
        for (size_t i=0; i<=m_maxVal; i++)
	        m_mapping[i] = m_Lmax*((float)i/m_maxVal);
	    break;
    case PTF_PSI:
    default:
        setMappingPsi();
        break;
    }
    
    //for (int i = 0; i<=m_maxVal; i++)
    //    fprintf(stderr, "%0.8f, ", m_mapping[i]);
}

// Run quantizer on a pixel value
float LumaQuantizer::quantize(const float val, const unsigned int ch) const
{
    float res;
    
	if (ch == 0 || m_colorSpace == CS_RGB || m_colorSpace == CS_XYZ)
    {
	    //binary search for best fitting luminance
	    int l = 0, r = m_maxVal;
	    while( l+1 < r )
	    {
		    int m = (l+r)/2;
		    if( val < m_mapping[m] )
			    r = m;
		    else
			    l = m;
	    }
	    //assert( r - l == 1 );
	    if ( val - m_mapping[l] < m_mapping[r] - val )
		    res = (float)l;
	    else
		    res = (float)r;
	}
	else
	{
	    res = floor(m_maxValColor*val + 0.5f);
        res = std::max(0.0f, std::min((float)m_maxValColor, res));
    }
	
	return res;
}

// Run dequantizer on a pixel value
float LumaQuantizer::dequantize(const float val, const unsigned int ch) const
{
    float res;
    
	if (ch == 0 || m_colorSpace == CS_RGB || m_colorSpace == CS_XYZ)
    {	
	    if( val < 0 )
		    res = m_mapping[0];
	    else if( val >= m_maxVal )
		    res = m_mapping[m_maxVal];
	    else
		    res = m_mapping[(int)val];	
    }
    else
        res = std::max(val/m_maxValColor, 1e-10f);    
    
	return res;
}

// Color transformation of a frame
bool LumaQuantizer::transformColorSpace(LumaFrame *frame, bool toCs, float sc)
{
    if (toCs)
    {
        switch (m_colorSpace)
        {
        case CS_XYZ:
            //fprintf(stderr, "Color transformation: RGB --> XYZ\n");
            {
            int index = 0;
            float R, G, B;
            for( unsigned int r = 0; r < frame->height; r++ )
                for( unsigned int c = 0; c < frame->width; c++, index++ )
                {
                    R = frame->getChannel(0)[index]*sc;
                    G = frame->getChannel(1)[index]*sc;
                    B = frame->getChannel(2)[index]*sc;
                    
                    frame->getChannel(0)[index] = std::max(std::min(rgb2xyzMat[0][0]*R + rgb2xyzMat[0][1]*G + rgb2xyzMat[0][2]*B, 100000000.0f), 0.0001f);
                    frame->getChannel(1)[index] = std::max(std::min(rgb2xyzMat[1][0]*R + rgb2xyzMat[1][1]*G + rgb2xyzMat[1][2]*B, 100000000.0f), 0.0001f);
                    frame->getChannel(2)[index] = std::max(std::min(rgb2xyzMat[2][0]*R + rgb2xyzMat[2][1]*G + rgb2xyzMat[2][2]*B, 100000000.0f), 0.0001f);
                }
            }
            break;
        case CS_LUV:
            //fprintf(stderr, "Color transformation: RGB --> LUV\n");
            {
            int index = 0;
            float R, G, B, X, Y, Z, sum, x, y;
            for( unsigned int r = 0; r < frame->height; r++ )
                for( unsigned int c = 0; c < frame->width; c++, index++ )
                {
                    // RGB --> XYZ
                    R = frame->getChannel(0)[index]*sc;
                    G = frame->getChannel(1)[index]*sc;
                    B = frame->getChannel(2)[index]*sc;
                    X = std::max(std::min(rgb2xyzMat[0][0]*R + rgb2xyzMat[0][1]*G + rgb2xyzMat[0][2]*B, 100000000.0f), 0.0001f);
                    Y = std::max(std::min(rgb2xyzMat[1][0]*R + rgb2xyzMat[1][1]*G + rgb2xyzMat[1][2]*B, 100000000.0f), 0.0001f);
                    Z = std::max(std::min(rgb2xyzMat[2][0]*R + rgb2xyzMat[2][1]*G + rgb2xyzMat[2][2]*B, 100000000.0f), 0.0001f);
            
                    // XYZ -> LUV
                    sum = X + Y + Z;
                    x = X/sum;
                    y = Y/sum;
                    frame->getChannel(0)[index] = Y;
                    frame->getChannel(1)[index] = 4.0f*x/(-2.0f*x + 12.0f*y + 3.0f) * 410.f/255.0f;
                    frame->getChannel(2)[index] = 9.0f*y/(-2.0f*x + 12.0f*y + 3.0f) * 410.f/255.0f;
                }
            }
            break;
        case CS_YCBCR:
            //fprintf(stderr, "Color transformation: RGB --> YCbCr\n");
            {
            // According to BT.2020
            unsigned int index = 0;
            /*
            float maxU = 0.0f, maxV = 0.0f;
            float minU = 1e10f, minV = 1e10f;
            float meanU = 0.0f, meanV = 0.0f;
            */
            float R, G, B, y;
            for( unsigned int r = 0; r < frame->height; r++ )
                for( unsigned int c = 0; c < frame->width; c++, index++ )
                {
                    R = transformPQ(std::max(frame->getChannel(0)[index]*sc, 1e-10f), 1);
                    G = transformPQ(std::max(frame->getChannel(1)[index]*sc, 1e-10f), 1);
                    B = transformPQ(std::max(frame->getChannel(2)[index]*sc, 1e-10f), 1);
                    
                    y = 0.2627f*R + 0.6780f*G + 0.0593f*B;
                
                    frame->getChannel(0)[index] = transformPQ((219.0f*y + 16.0f) / 255.0f, 0);
                    frame->getChannel(1)[index] = (224.0f*( (B - y) / 1.8814f ) + 128.0f) / 255.0f;
                    frame->getChannel(2)[index] = (224.0f*( (R - y) / 1.4746f ) + 128.0f) / 255.0f;
                    
                    /*
                    maxU = std::max(maxU, frame->getChannel(1)[index]);
                    maxV = std::max(maxV, frame->getChannel(2)[index]);
                    minU = std::min(minU, frame->getChannel(1)[index]);
                    minV = std::min(minV, frame->getChannel(2)[index]);
                    meanU += frame->getChannel(1)[index];
                    meanV += frame->getChannel(2)[index];
                    */
                }
            //fprintf(stderr, "max CbCr = [%f, %f]\n", maxU, maxV);
            //fprintf(stderr, "min CbCr = [%f, %f]\n", minU, minV);
            //fprintf(stderr, "mean CbCr = [%f, %f]\n", meanU/index, meanV/index);
            }
            break;
        case CS_RGB:
            //fprintf(stderr, "Color transformation: RGB --> RGB\n");
            {
            unsigned int index = 0;
            for( unsigned int r = 0; r < frame->height; r++ )
                for( unsigned int c = 0; c < frame->width; c++, index++ )
                {
                    frame->getChannel(0)[index]*=sc;
                    frame->getChannel(1)[index]*=sc;
                    frame->getChannel(2)[index]*=sc;
                }
            break;
            }
        default:
            fprintf(stderr, "Error! Unrecognized color transformation XYZ --> ?\n");
            return false;
            break;
        }
    }
    else
    {
        switch (m_colorSpace)
        {
        case CS_XYZ:
            //fprintf(stderr, "Color transformation: XYZ --> RGB\n");
            {   
            unsigned int index = 0;
            float X, Y, Z;
            for( unsigned int r = 0; r < frame->height; r++ )
                for( unsigned int c = 0; c < frame->width; c++, index++ )
                {
                    X = frame->getChannel(0)[index];
                    Y = frame->getChannel(1)[index];
                    Z = frame->getChannel(2)[index];
                    
                    frame->getChannel(0)[index] = (xyz2rgbMat[0][0]*X + xyz2rgbMat[0][1]*Y + xyz2rgbMat[0][2]*Z)/sc;
                    frame->getChannel(1)[index] = (xyz2rgbMat[1][0]*X + xyz2rgbMat[1][1]*Y + xyz2rgbMat[1][2]*Z)/sc;
                    frame->getChannel(2)[index] = (xyz2rgbMat[2][0]*X + xyz2rgbMat[2][1]*Y + xyz2rgbMat[2][2]*Z)/sc;
                }
            }
            break;
        case CS_LUV:
            //fprintf(stderr, "Color transformation: LUV --> RGB\n");
            {
            unsigned int index = 0;
            float L, u, v, x, y, X, Y, Z;
            for( unsigned int r = 0; r < frame->height; r++ )
                for( unsigned int c = 0; c < frame->width; c++, index++ )
                {
                    // LUV --> XYZ
                    L = frame->getChannel(0)[index];
                    u = frame->getChannel(1)[index]*255.0f/410.0f;
                    v = frame->getChannel(2)[index]*255.0f/410.0f;
                    
                    x = 9.0f*u / (6.0f*u - 16.0f*v + 12.0f);
                    y = 4.0f*v / (6.0f*u - 16.0f*v + 12.0f);
                    Y = std::max(std::min(L, 100000000.0f), 0.0001f);
                    X = std::max(std::min(x/y * L, 100000000.0f), 0.0001f);
                    Z = std::max(std::min((1.0f-x-y)/y * L, 100000000.0f), 0.0001f);
                    
                    // XYZ --> RGB
                    frame->getChannel(0)[index] = (xyz2rgbMat[0][0]*X + xyz2rgbMat[0][1]*Y + xyz2rgbMat[0][2]*Z)/sc;
                    frame->getChannel(1)[index] = (xyz2rgbMat[1][0]*X + xyz2rgbMat[1][1]*Y + xyz2rgbMat[1][2]*Z)/sc;
                    frame->getChannel(2)[index] = (xyz2rgbMat[2][0]*X + xyz2rgbMat[2][1]*Y + xyz2rgbMat[2][2]*Z)/sc;
                }
            }
            break;
        case CS_RGB:
            //fprintf(stderr, "Color transformation: RGB --> RGB\n");
            {
            unsigned int index = 0;
            for( unsigned int r = 0; r < frame->height; r++ )
                for( unsigned int c = 0; c < frame->width; c++, index++ )
                {
                    frame->getChannel(0)[index]/=sc;
                    frame->getChannel(1)[index]/=sc;
                    frame->getChannel(2)[index]/=sc;
                }
            break;
            }
            break;
        case CS_YCBCR:
            //fprintf(stderr, "Color transformation: YCbCr --> RGB\n");
            {
            // According to BT.2020
            unsigned int index = 0;
            //float maxC[] = {0.0f, 0.0f, 0.0f};
            //float minC[] = {1e10f, 1e10f, 1e10f};
            float y, red, green, blue;
            for( unsigned int r = 0; r < frame->height; r++ )
                for( unsigned int c = 0; c < frame->width; c++, index++ )
                {
                    y = transformPQ(frame->getChannel(0)[index], 1);
                    y = (255.0f*y - 16.0f) / 219.0f;
                    blue = y + 1.8814f * (255.0f*frame->getChannel(1)[index] - 128.0f) / 224.0f;
                    red = y + 1.4746f * (255.0f*frame->getChannel(2)[index] - 128.0f) / 224.0f;
                    green = (y - 0.2627f*red - 0.0593f*blue) / 0.6780f;
                    
                    red = std::max(0.0f, std::min(1.0f, red));
                    green = std::max(0.0f, std::min(1.0f, green));
                    blue = std::max(0.0f, std::min(1.0f, blue));
                    
                    frame->getChannel(0)[index] = transformPQ(red, 0)/sc;
                    frame->getChannel(1)[index] = transformPQ(green, 0)/sc;
                    frame->getChannel(2)[index] = transformPQ(blue, 0)/sc;
                    
                    /*
                    maxC[0] = std::max(maxC[0], frame->getChannel(0)[index]);
                    maxC[1] = std::max(maxC[1], frame->getChannel(1)[index]);
                    maxC[2] = std::max(maxC[2], frame->getChannel(2)[index]);
                    minC[0] = std::min(minC[0], frame->getChannel(0)[index]);
                    minC[1] = std::min(minC[1], frame->getChannel(1)[index]);
                    minC[2] = std::min(minC[2], frame->getChannel(2)[index]);
                    */
                }
            //fprintf(stderr, "max rgb = [%f, %f, %f]\n", maxC[0], maxC[1], maxC[2]);
            //fprintf(stderr, "min rgb = [%f, %f, %f]\n", minC[0], minC[1], minC[2]);
            }
            break;
        default:
            fprintf(stderr, "Error! Unrecognized color transformation ? --> RGB\n");
            return false;
            break;
        }
    }
    
    return true;
}

// PQ function
float LumaQuantizer::transformPQ(float val, bool encode)
{
    const float L = m_Lmax,
	      		m = 78.8438, n = 0.1593,
	      		c1 = 0.8359, c2 = 18.8516, c3 = 18.6875;
    	
	if (encode)
	{
	    float Lp = pow(val/L, n);
        return pow((c1 + c2*Lp) / (1 + c3*Lp), m);
	}
	else
	{
	    float Vp = pow(val, 1.0f/m);
        return L * pow( std::max(0.0f, (Vp-c1)) / (c2-c3*Vp), 1.0f/n);
    }
}

// Log function
float LumaQuantizer::transformLog(float val, bool encode)
{
    if (encode)
        return (log10(val) - log10(m_Lmin)) / (log10(m_Lmax)-log10(m_Lmin));
    else
        return pow( 10.0f, val*(log10(m_Lmax)-log10(m_Lmin)) + log10(m_Lmin) );
}

