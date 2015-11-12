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
 * @date Nov 12 2015
 */

#version 120

uniform sampler2D texGUI;
uniform float time, exposure;
uniform int button;

void main (void)  
{
    float x0 = 0.069, x1 = 0.92,
          y0 = 0.25, y1 = 0.37;
    
    // Get pixel value from textures
    vec2 texC = gl_TexCoord[0].st;
    vec4 C = texture2D(texGUI, texC);
    
    // Time slider
    if (button == 2) { y0 -= 0.02; y1 += 0.02; }
    if (texC.s > x0 && texC.s < x0+time*(x1-x0) && texC.t < y1 && texC.t > y0)
        C.rgb = vec3(0.3, 0.3, 1.0);
    
    // Exposure normalization
    float expMin = -10.0, expMax = 10.0;
    float e = max(0.0, min(1.0, (log2(exposure)-expMin)/(expMax-expMin)));
    
    // Exposure slider
    y0 = 0.65; y1 = 0.77;
    if (button == 3) { y0 -= 0.02; y1 += 0.02; }
    if (texC.s > x0 && texC.s < x0+e*(x1-x0) && texC.t < y1 && texC.t > y0)
        C.rgb = vec3(1.0, 0.3, 0.3);
    
    // Buttons highlighting
    if ( ((button == 1 && texC.s < x0) || (button == 4 && texC.s > x0+(x1-x0)) ) && C.g > 0.3)
        C.rgb = vec3(0.3, 0.3, C.b);
    
    
    gl_FragColor = C;
}


