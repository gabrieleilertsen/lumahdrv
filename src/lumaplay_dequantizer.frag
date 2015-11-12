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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ‘AS IS’ 
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
 * @author Gabriel Eilertsen, gabriel.eilertsen@liu.se
 *
 * @date Oct 7 2015
 */

#version 120

uniform sampler2D texC1, texC2, texC3;
uniform sampler1D texM;
uniform float maxVal, maxValColor, exposure, gamma, scaling, strideRatio;

uniform int colorSpaces[4], colorSpace, doTmo, ldrSim;

const mat3 xyz2rgb = mat3( 3.240708, -0.969257,  0.055636,
                          -1.537259,  1.875995, -0.203996,
                          -0.498570,  0.041555,  1.057069);

// Perceptual quantizer
float transformPQ(float val, bool encode)
{
    const float L = 10000.0,
                m = 78.8438, n = 0.1593,
                c1 = 0.8359, c2 = 18.8516, c3 = 18.6875;
    
    if (encode)
    {
        float Lp = pow(val/L, n);
        return pow((c1 + c2*Lp) / (1 + c3*Lp), m);
    }
    else
    {
        float Vp = pow(val, 1.0/m);
        return L * pow( max(0.0, (Vp-c1)) / (c2-c3*Vp), 1.0/n);
    }
}

void main (void)  
{
    // Ignore pixels outside image
    if (gl_TexCoord[0].s > 1.0/strideRatio)
    {
        gl_FragColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }
    
    // Get pixel value from textures
    vec3 C = vec3(texture2D(texC1, gl_TexCoord[0].st)[0],
                  texture2D(texC2, gl_TexCoord[0].st)[0],
                  texture2D(texC3, gl_TexCoord[0].st)[0]);
    
    // Dequantization
    C.x = texture1D(texM, max(min(C.x, maxVal), 0.0)/maxVal)[0];
    
    if (colorSpace == colorSpaces[1] || colorSpace == colorSpaces[3])
    {
        C.y = texture1D(texM, max(min(C.y, maxVal), 0.0)/maxVal)[0];
        C.z = texture1D(texM, max(min(C.z, maxVal), 0.0)/maxVal)[0];
    }
    else
    {
        C.y = C.y/maxValColor;
        C.z = C.z/maxValColor;
    }
    
    vec3 RGB;
    
    // Color transformation
    if (colorSpace == colorSpaces[0]) // LUV
    {
        float L = C.x;
        float u = C.y*255.0/410.0;
        float v = C.z*255.0/410.0;
        
        // LUV --> XYZ
        float x = 9.0*u / (6.0*u - 16.0*v + 12.0);
        float y = 4.0*v / (6.0*u - 16.0*v + 12.0);
        
        vec3 XYZ;
        XYZ[1] = max(min(L, 100000000.0), 0.0001);
        XYZ[0] = max(min(x/y * L, 100000000.0), 0.0001);
        XYZ[2] = max(min((1.0-x-y)/y * L, 100000000.0), 0.0001);
        
        // XYZ --> RGB
        RGB = xyz2rgb * XYZ;
    }
    else if (colorSpace == colorSpaces[1]) // RGB
    {
        RGB = C;
    }
    else if (colorSpace == colorSpaces[2]) // YCbCr
    {
        // According to BT.2020
        int index = 0;
        float y = transformPQ(C.x, true);
        y = (255.0*y - 16.0) / 219.0;
        
        RGB.b = y + 1.8814 * (255.0*C.y - 128.0) / 224.0;
        RGB.r = y + 1.4746 * (255.0*C.z - 128.0) / 224.0;
        RGB.g = (y - 0.2627*RGB.r - 0.0593*RGB.b) / 0.6780;
        
        RGB = max(vec3(0.0), min(vec3(1.0), RGB));
        
        RGB.r = transformPQ(RGB.r, false);
        RGB.g = transformPQ(RGB.g, false);
        RGB.b = transformPQ(RGB.b, false);
    }
    else if (colorSpace == colorSpaces[3]) // XYZ
    {
        RGB = xyz2rgb * C;
    }
    
    if (ldrSim > 0)
        RGB = exposure*max(vec3(1),min(vec3(256),floor(256*RGB/scaling)))/256;
    else
        RGB = RGB*exposure/scaling;
    
    if (doTmo > 0)
    {
        float n = 0.8, sig = 0.8;
        RGB = pow(RGB, vec3(n)) / (pow(RGB, vec3(n)) + vec3(pow(sig, n)));
    }

    gl_FragColor = vec4(pow(RGB, vec3(1.0/gamma)), 1.0);
}


