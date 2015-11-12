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

#include <cstdio>
#include <math.h>
#include <fstream>
#include <iostream>

#include <GL/glew.h>

#ifdef __APPLE__
#include <glut.h>
#else
#include <GL/glut.h>
#endif

#include <luma_decoder.h>
#include "luma_exception.h"
#include "arg_parser.h"

#include "config.h"



// === Necessary globals =======================================================

namespace global
{
// Dequantization shader loaded on compile time
const char* dequant_src[] = {
#include "lumaplay_dequantizer_glsl.h"
};

// GUI shader loaded on compile time
const char* gui_src[] = {
#include "lumaplay_gui_glsl.h"
};

// GUI background texture loaded on compile time
const unsigned char gui_texture[] = {
#include "lumaplay_gui_texture.h"
};

// State variables for controlling interaction in GLUT callback functions
// Need to be defined globally, since variables cannot be passed to GLUT callbacks
struct GlutState
{
    GlutState() : pausePlayback(0), doTmo(0), seek(0), showTimings(0), ldrSim(0),
        mousePressL1(0), mousePressL2(0), fullScreen(0), firstClick(0),
        dispTimeTot(0.0f), totTimeTot(0.0f), exposure(1.0f), fps(-1.0f), 
        frameDuration(40.0f), inactiveTime(-1e10f), guiTime(2000.0f), 
        frameNr(0), guiButton(0)
    {}
    
    bool hbd, pausePlayback, doTmo, seek, showTimings, ldrSim, mousePressL1, 
         mousePressL2, fullScreen, firstClick;
         
    float dispTimeTot, totTimeTot, exposure, fps, frameDuration,
          duration, inactiveTime, guiTime;
          
    unsigned int windNr, stride[3], frameNr, guiButton,
                 width[3], height[3], widthGUI, heightGUI, widthWin,
                 heightWin, widthWin0, heightWin0,
                 sx, sy, x0, y0, sxGUI, syGUI, x0GUI, y0GUI,
                 Sx0, Sx1, Sx2, Sx3, Sy0, Sy1;
                 
    timeval start1, start2, stop;
    
    LumaDecoder decoder;
    
    GLfloat strideRatio;
    GLuint dequantProg, guiProg, texC1, texC2, texC3, texGUI1, texGUI2, texM;
} state;
}

// =============================================================================



// === Setup and initialization ================================================
int loadTexture(GLuint t);
bool setupShader(GLuint &shader, GLuint &program, const GLchar** shader_src);
void setupGUI();
void init(float gammaVal, float userScaling);
// =============================================================================



// === Utility functions for use during playback ===============================
bool getFrame();
void resetGuiTime();
void seekToTime(float time);
bool positionSlider(unsigned int x);
// =============================================================================



// === Callback functions ======================================================
void display();
void reshape(int w, int h);
void doubleClickRegistration(int);
void mouse(int button, int state, int xi, int yi);
void mouseMotion(int x, int);
void mousePassiveMotion(int xi, int yi);
void keyboard(unsigned char key, int x, int);
// =============================================================================





int main(int argc, char** argv)
{
    std::string inputFile;
    float gammaVal = 2.2f, userScaling = 1.0f;

    // Application usage info
    std::string info = 
        std::string("lumaplay -- Playback a high dynamic range (HDR) video\n\n") +
        std::string("Usage: lumaplay --input <hdr_video>\n\n") +
        std::string("Interaction options:\n") +
        std::string("  Ctr+Q/Ctr+W/Esc\tQuit application\n") +
        std::string("  Space\t\t\tPause video\n") +
        std::string("  s\t\t\tSeek to position indicated by mouse pointers horizontal position.\n") +
        std::string("  \t\t\tLeft border of video is start of video, and right border is end of video.\n") +
        std::string("  +\t\t\tIncrease exposure\n") +
        std::string("  -/=\t\t\tDecrease exposure\n") +
        std::string("  t\t\t\tApply simple sigmoid tone curve\n") +
        std::string("  l\t\t\tSimulate low dynamic range (LDR) video\n") +
        std::string("  z\t\t\tShow timings\n");
            
    std::string postInfo = std::string("\nExample: lumaplay --input hdr_video.mkv -s 0.2 -g 1.8 -fps 25\n\n") +
                           std::string("See man page for more information.");
    
    try
    {
        ArgParser argHolder(info, postInfo);
        
        // Input arguments
        argHolder.add(&inputFile,         "--input",     "-i",   "Input HDR video", 0);
        argHolder.add(&gammaVal,          "--gamma",     "-g",   "Display gamma value", 2.2f);
        argHolder.add(&userScaling,       "--scaling",   "-s",   "Scaling to apply to video", 1.0f);
        argHolder.add(&global::state.fps, "--framerate", "-fps", "Framerate (frames/s)", global::state.fps);
        
        // Parse arguments
        if (!argHolder.read(argc, argv))
            return 0;    
        
        // Initialize decoder    
        if (!global::state.decoder.initialize(inputFile.c_str()))
            return 1;
        
        // Store some useful decoder parameters
        LumaDecoderParams params = global::state.decoder.getParams();
        global::state.hbd = params.highBitDepth;
        global::state.stride[0] = params.stride[0];
        global::state.stride[1] = params.stride[1];
        global::state.stride[2] = params.stride[2];
        global::state.width[0] = params.width[0];
        global::state.width[1] = params.width[1];
        global::state.width[2] = params.width[2];
        global::state.height[0] = params.height[0];
        global::state.height[1] = params.height[1];
        global::state.height[2] = params.height[2];
        global::state.duration = global::state.decoder.getReader()->getDuration();
        global::state.strideRatio = ((float)global::state.stride[0])/global::state.width[0];
        if (global::state.hbd) global::state.strideRatio /= 2;
        
        // Initialize GLUT and GLEW
        glutInit(&argc, argv);
        glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
        glutInitWindowSize(global::state.width[0], global::state.height[0]);
        glutInitWindowPosition(100, 100);
        global::state.windNr = glutCreateWindow("Luma HDRv player");
        glewInit();
        fprintf(stderr, "OpenGL version : %s\n",  (char*)glGetString(GL_VERSION));
        
        // Initialize textures, shaders, etc.
        init(gammaVal, userScaling);
        
        // Specify GLUT callback functions
        glutDisplayFunc(display);
        glutReshapeFunc(reshape);
        glutKeyboardFunc(keyboard);
        glutMouseFunc(mouse);
        glutMotionFunc(mouseMotion);
        glutPassiveMotionFunc(mousePassiveMotion);
        
        // Start the rendering loop
        glutMainLoop();
    }
    catch (ParserException &e)
    {
        fprintf(stderr, "\nlumaplay input error: %s\n", e.what());
        return 1;
    }
    catch (LumaException &e)
    {
        fprintf(stderr, "\nlumaplay decoding error: %s\n", e.what());
        return 1;
    }
    catch (std::exception & e)
    {
        fprintf(stderr, "\nlumaplay error: %s\n", e.what());
        return 1;
    }
    
    return 0;
}





// === Setup and initialization ================================================

// Setup a GL texture
int loadTexture(GLuint t)
{
    GLuint object;
    glGenTextures(1,&object);
    glBindTexture(t, object);
    glTexParameterf(t,GL_TEXTURE_MIN_FILTER,GL_LINEAR); //GL_NEAREST);
    glTexParameterf(t,GL_TEXTURE_MAG_FILTER,GL_LINEAR); //GL_NEAREST);
    glTexParameterf(t,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
    glTexParameterf(t,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
    return object;
}

// Setup a GLSL shader
bool setupShader(GLuint &shader, GLuint &program, const GLchar** shader_src)
{
    shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(shader, 1, shader_src, NULL);
    glCompileShader(shader);
    
    GLint flag;
    
    glGetShaderiv(shader, GL_COMPILE_STATUS, &flag);
    
    if (!flag)
    {
        fprintf(stderr, "Error! Failed to compile shader.\n");
        
        GLint length;
        GLchar* log;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
        log = (GLchar*)malloc(length);
        glGetShaderInfoLog(shader, length, &length, log);
        fprintf(stderr, "\n%s\n", log);
        
        return 0;
    }
    
    program = glCreateProgram();
    
    glAttachShader(program, shader);
    glLinkProgram(program);
    
    glGetProgramiv(program, GL_LINK_STATUS, &flag);
    
    if (!flag)
    {
        fprintf(stderr, "Error! Failed to link program.\n");
        
        GLint length;
        GLchar* log;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
        log = (GLchar*)malloc(length);
        glGetShaderInfoLog(shader, length, &length, log);
        fprintf(stderr, "\n%s\n", log);
        
        return 0;
    }
    
    return 1;
}

// Setup textures for GUI
void setupGUI()
{
    // First indices are texture size
    unsigned int sx = global::gui_texture[0]|global::gui_texture[1]<<8,
                 sy = global::gui_texture[2]|global::gui_texture[3]<<8, 
                 sz = global::gui_texture[4];
    
    // Texture format depends on number of channels
    GLint format;
    switch (sz)
    {
    case 1:
        format = GL_RED;
        break;
    case 3:
        format = GL_RGB;
        break;
    case 4:
        format = GL_RGBA;
        break;
    default:
        throw LumaException("unsupported number of channels in GUI texture");
        break;
    }
    
    global::state.texGUI1 = loadTexture(GL_TEXTURE_2D);
    global::state.texGUI2 = loadTexture(GL_TEXTURE_2D);
    
    // Initialize textures
    glBindTexture(GL_TEXTURE_2D,global::state.texGUI1);
    glTexImage2D(GL_TEXTURE_2D, 0, format, sx, sy, 0, format, GL_UNSIGNED_BYTE, global::gui_texture+5);
    
    glBindTexture(GL_TEXTURE_2D,global::state.texGUI2);
    glTexImage2D(GL_TEXTURE_2D, 0, format, sx, sy, 0, format, GL_UNSIGNED_BYTE, global::gui_texture+sx*sy*sz+5);
    
    global::state.widthGUI = sx;
    global::state.heightGUI = sy;
}

// Initialization of e.g. textures and shaders
void init(float gammaVal, float userScaling)
{
    glClearColor(0.0,0.0,0.0,0.0);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0,100,100,1.0,-1.0,1.0);
    glEnable(GL_BLEND);
    glEnable(GL_TEXTURE_2D);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    
    global::state.texC1 = loadTexture(GL_TEXTURE_2D);
    global::state.texC2 = loadTexture(GL_TEXTURE_2D);
    global::state.texC3 = loadTexture(GL_TEXTURE_2D);
    global::state.texM = loadTexture(GL_TEXTURE_1D);
    
    glBindTexture(GL_TEXTURE_1D,global::state.texM);
    glTexImage1D(GL_TEXTURE_1D,0,GL_R32F,global::state.decoder.getQuantizer()->getSize(),
                 0,GL_RED,GL_FLOAT,global::state.decoder.getQuantizer()->getMapping());
    
    // Load textures for GUI
    setupGUI();
    
    GLuint shaderDequant, shaderGUI;
    LumaDecoderParams params = global::state.decoder.getParams();
    
    // Dequantization shader
    if (!setupShader(shaderDequant, global::state.dequantProg, (const GLchar**)(&global::dequant_src)))
        throw LumaException("error in dequantization shader");
    
    // GUI shader
    if (!setupShader(shaderGUI, global::state.guiProg, (const GLchar**)(&global::gui_src)))
        throw LumaException("error in GUI shader");
    
    // Dequantization shader params
    glUseProgram(global::state.dequantProg);
    glUniform1i(glGetUniformLocation(global::state.dequantProg, "texC1"), 0);
    glUniform1i(glGetUniformLocation(global::state.dequantProg, "texC2"), 1);
    glUniform1i(glGetUniformLocation(global::state.dequantProg, "texC3"), 2);
    glUniform1i(glGetUniformLocation(global::state.dequantProg, "texM"), 3);
    
    float normalization = global::state.hbd ? 65535.0f : 255.0f;
    float maxVal = global::state.decoder.getQuantizer()->getSize() / normalization;
    float maxValColor = (pow(2, params.colorBitDepth) - 1) / normalization;
    unsigned int colorSpaces[] = {LumaQuantizer::CS_LUV, LumaQuantizer::CS_RGB,
                                  LumaQuantizer::CS_YCBCR, LumaQuantizer::CS_XYZ};
    unsigned int colorSpace = params.colorSpace;
    
    glUniform1f(glGetUniformLocation(global::state.dequantProg, "strideRatio"), global::state.strideRatio);
    glUniform1f(glGetUniformLocation(global::state.dequantProg, "maxVal"), maxVal);
    glUniform1f(glGetUniformLocation(global::state.dequantProg, "maxValColor"), maxValColor);
    glUniform1f(glGetUniformLocation(global::state.dequantProg, "exposure"), global::state.exposure);
    glUniform1f(glGetUniformLocation(global::state.dequantProg, "gamma"), gammaVal);
    glUniform1f(glGetUniformLocation(global::state.dequantProg, "scaling"), params.preScaling / userScaling);
    glUniform1iv(glGetUniformLocation(global::state.dequantProg, "colorSpaces"), 4, (GLint*)(colorSpaces));
    glUniform1i(glGetUniformLocation(global::state.dequantProg, "colorSpace"), colorSpace);
    glUniform1i(glGetUniformLocation(global::state.dequantProg, "doTmo"), global::state.doTmo);
    glUniform1i(glGetUniformLocation(global::state.dequantProg, "ldrSim"), global::state.ldrSim);
    
    // GUI shader params
    glUseProgram(global::state.guiProg);
    glUniform1i(glGetUniformLocation(global::state.guiProg, "texGUI"), 4);
    glUniform1f(glGetUniformLocation(global::state.guiProg, "time"), 0.0f);
    glUniform1i(glGetUniformLocation(global::state.guiProg, "button"), 0);
}

// =============================================================================





// === Utility functions for use during playback ===============================

// Get a frame from the decoder
bool getFrame()
{
    // End of clip. Start from first frame
    if (!global::state.decoder.run())
    {
        global::state.decoder.seekToTime(0);
        global::state.inactiveTime -= (global::state.frameNr-1)*global::state.frameDuration;
        global::state.frameNr = 0;
        if (!global::state.decoder.run())
            return 0;
    }
    
    // Get frame and frame duration
    global::state.decoder.getFrame();
    global::state.frameDuration = global::state.decoder.getReader()->getFrameDuration();
    global::state.frameNr++;
    
    return 1;
}

// inactiveTime stores the timestamp for the last interaction with the window
void resetGuiTime()
{
    global::state.inactiveTime = (global::state.frameNr-1)*global::state.frameDuration;
}

// Seeking to a time in the video sequence
void seekToTime(float time)
{
    global::state.decoder.seekToTime(time);
    global::state.frameNr = time*(global::state.duration-global::state.frameDuration)
                                /global::state.frameDuration + 1;
    global::state.seek = true;
    resetGuiTime();
    glutPostRedisplay();
}

// Interaction with time and exposure sliders
bool positionSlider(unsigned int x)
{
    if (x > global::state.Sx1 && x < global::state.Sx2)
    {
        if (global::state.mousePressL1)
        {
            seekToTime(float(x-global::state.Sx1)/(global::state.Sx2-global::state.Sx1));
            return 1;
        }
        else if (global::state.mousePressL2)
        {
            global::state.exposure = pow(2.0f, 20*float(x-global::state.Sx1)
                /(global::state.Sx2-global::state.Sx1) - 10);
            glUniform1f(glGetUniformLocation(global::state.dequantProg, "exposure"), global::state.exposure);
            resetGuiTime();
            glutPostRedisplay();
            return 1;
        }
    }
    
    return 0;
}

// =============================================================================





// === Callback functions ======================================================

// On redraw
void display()
{
    // Timings, for information and synchronization of framerate
    if (global::state.frameNr > 0 && !global::state.pausePlayback)
    {
        gettimeofday(&global::state.stop, NULL);
        
        float dispTime = (global::state.stop.tv_sec-global::state.start2.tv_sec)*1000.0f 
                       + (global::state.stop.tv_usec-global::state.start2.tv_usec)/1000.0f;
        float totTime = (global::state.stop.tv_sec-global::state.start1.tv_sec)*1000.0f
                      + (global::state.stop.tv_usec-global::state.start1.tv_usec)/1000.0f;
        
        global::state.dispTimeTot = (global::state.dispTimeTot*global::state.frameNr +  dispTime)
                                   /(global::state.frameNr+1);
        global::state.totTimeTot = (global::state.totTimeTot*global::state.frameNr +  totTime)
                                   /(global::state.frameNr+1);
        
        if (global::state.showTimings)
        {
            fprintf(stderr, "DISPLAY TIME: %f (avg = %f)\n",
                    dispTime, global::state.dispTimeTot);
            fprintf(stderr, "TOTAL TIME:   %f (avg = %f), %f fps\n",
                    totTime, global::state.totTimeTot, 1000.0f/totTime);
        }
        
        // Waiting time for next frame
        float sleepTime = global::state.frameDuration-totTime, t = 0.0f;
        if (global::state.fps > 0.0f)
            sleepTime = 1000.0f/global::state.fps-totTime;
        
        // Very ugly way to accomplish the sought frame rate
        //TODO: use glutTimerFunc instead...?
        while (sleepTime > t)
        {
            gettimeofday(&global::state.start1, NULL);
            t = (global::state.start1.tv_sec-global::state.stop.tv_sec)*1000.0f
              + (global::state.start1.tv_usec-global::state.stop.tv_usec)/1000.0f;
        }
    }
    
    gettimeofday(&global::state.start1, NULL);
    
    bool gotFrame = 0;
    
    // Get next frame, if video is not paused
    if (!global::state.pausePlayback || global::state.seek)
    {
        gotFrame = getFrame();
        global::state.seek = false;
    }
    
    gettimeofday(&global::state.start2, NULL);
    
    glClear(GL_COLOR_BUFFER_BIT);
    
    // Load frame to textures, one per color channel
    if (gotFrame)
    {
        if (global::state.hbd)
        {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D,global::state.texC1);
            glTexImage2D(GL_TEXTURE_2D,0,GL_R16,global::state.stride[0]/2,global::state.height[0],
                         0,GL_RED,GL_UNSIGNED_SHORT,(unsigned short*)(global::state.decoder.getBuffer()[0]));
            
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D,global::state.texC2);
            glTexImage2D(GL_TEXTURE_2D,0,GL_R16,global::state.stride[1]/2,global::state.height[1],
                         0,GL_RED,GL_UNSIGNED_SHORT,(unsigned short*)(global::state.decoder.getBuffer()[1]));
            
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D,global::state.texC3);
            glTexImage2D(GL_TEXTURE_2D,0,GL_R16,global::state.stride[2]/2,global::state.height[2],
                         0,GL_RED,GL_UNSIGNED_SHORT,(unsigned short*)(global::state.decoder.getBuffer()[2]));
        }
        else
        {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D,global::state.texC1);
            glTexImage2D(GL_TEXTURE_2D,0,GL_R8,global::state.stride[0],global::state.height[0],
                         0,GL_RED,GL_UNSIGNED_BYTE,global::state.decoder.getBuffer()[0]);
            
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D,global::state.texC2);
            glTexImage2D(GL_TEXTURE_2D,0,GL_R8,global::state.stride[1],global::state.height[1],
                         0,GL_RED,GL_UNSIGNED_BYTE,global::state.decoder.getBuffer()[1]);
            
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D,global::state.texC3);
            glTexImage2D(GL_TEXTURE_2D,0,GL_R8,global::state.stride[2],global::state.height[2],
                         0,GL_RED,GL_UNSIGNED_BYTE,global::state.decoder.getBuffer()[2]);
        }
        
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_1D,global::state.texM);
    }
    
    glActiveTexture(GL_TEXTURE4);
    
    // GUI texture
    if (global::state.pausePlayback)
        glBindTexture(GL_TEXTURE_2D,global::state.texGUI2);
    else
        glBindTexture(GL_TEXTURE_2D,global::state.texGUI1);
    
    // Run dequantization shader on the frame textures
    glUseProgram(global::state.dequantProg);
    glBegin(GL_QUADS);
        glTexCoord2f(0,0);
        glVertex2f(global::state.x0,global::state.y0);
        glTexCoord2f(1,0);
        glVertex2f(global::state.x0 + global::state.sx*global::state.strideRatio,global::state.y0);
        glTexCoord2f(1,1);
        glVertex2f(global::state.x0 + global::state.sx*global::state.strideRatio, global::state.y0 + global::state.sy);
        glTexCoord2f(0,1);
        glVertex2f(global::state.x0, global::state.y0 + global::state.sy);
    glEnd();
    
    // Run GUI shader on the GUI texture, if it is supposed to be displayed
    if ((global::state.frameNr-1)*global::state.frameDuration - global::state.inactiveTime < global::state.guiTime)
    {
        glUseProgram(global::state.guiProg);
        glUniform1f(glGetUniformLocation(global::state.guiProg, "time"),
            std::min(1.0f, (global::state.frameNr-1)*global::state.frameDuration
                           /(global::state.duration-global::state.frameDuration)));
        glUniform1f(glGetUniformLocation(global::state.guiProg, "exposure"), global::state.exposure);
        glUniform1i(glGetUniformLocation(global::state.guiProg, "button"), global::state.guiButton);
        glBegin(GL_QUADS);
            glTexCoord2f(0,0);
            glVertex2f(global::state.x0GUI,global::state.y0GUI);
            glTexCoord2f(1,0);
            glVertex2f(global::state.x0GUI + global::state.sxGUI,global::state.y0GUI);
            glTexCoord2f(1,1);
            glVertex2f(global::state.x0GUI + global::state.sxGUI,
                       global::state.y0GUI + global::state.syGUI);
            glTexCoord2f(0,1);
            glVertex2f(global::state.x0GUI, global::state.y0GUI + global::state.syGUI);
        glEnd();
        
        glUseProgram(global::state.dequantProg);
    }
    
    //glFlush();
    glutSwapBuffers();
    
    if (gotFrame && !global::state.pausePlayback)
        glutPostRedisplay();
}

// On window reshape
void reshape(int w, int h)
{
    global::state.widthWin = w;
    global::state.heightWin = h;
    
    // Calculate video position and size within the window
    float sx = w, sy = h;
    if (sx/sy >= (float)global::state.width[0]/global::state.height[0])
        sx = ((float)(h*global::state.width[0]))/global::state.height[0];
    else
        sy = ((float)(w*global::state.height[0]))/global::state.width[0];
    
    float x0 = (w-sx)/2.0f;
    float y0 = (h-sy)/2.0f;
    
    global::state.sx = round(100.0f*sx/w);
    global::state.sy = round(100.0f*sy/h);
    global::state.x0 = round(100.0f*x0/w);
    global::state.y0 = round(100.0f*y0/h);
    
    // Calculate GUI size and position
    global::state.sxGUI = 0.9f*w;
    global::state.syGUI = 0.3f*h;
    global::state.sxGUI = std::min(global::state.syGUI*(float)global::state.widthGUI/global::state.heightGUI, 
                                   (float)std::min(global::state.widthGUI, global::state.sxGUI));
    global::state.syGUI = global::state.sxGUI*(float)global::state.heightGUI/global::state.widthGUI;
    global::state.x0GUI = (w-global::state.sxGUI)/2;
    global::state.y0GUI = h - 1.5f*global::state.syGUI;
    
    // Calculate positions of GUI buttons/sliders
    global::state.Sx0 = global::state.x0GUI;
    global::state.Sx1 = global::state.x0GUI + 0.066f*global::state.sxGUI;
    global::state.Sx2 = global::state.x0GUI + 0.91f*global::state.sxGUI;
    global::state.Sx3 = global::state.x0GUI + global::state.sxGUI;
    global::state.Sy0 = global::state.y0GUI;
    global::state.Sy1 = global::state.y0GUI + global::state.syGUI;
    
    global::state.sxGUI = 100*global::state.sxGUI/w;
    global::state.syGUI = 100*global::state.syGUI/h;
    global::state.x0GUI = 100*global::state.x0GUI/w;
    global::state.y0GUI = 100*global::state.y0GUI/h;
    
    glViewport(0,0,w,h);
}

// On double click time out. To determine when a double click occures
void doubleClickRegistration(int)
{
    global::state.firstClick = false;
}

// On mouse click
void mouse(int button, int state, int xi, int yi)
{
    unsigned int x = xi, y = yi;
    
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN)
    {
        // Click on the sliders
        if (x > global::state.Sx1 && x < global::state.Sx2)
        {
            if (y > global::state.Sy0 && y < 0.7*global::state.Sy0 + 0.3*global::state.Sy1)
                global::state.mousePressL1 = true;
            else if (y > 0.7*global::state.Sy0 + 0.3*global::state.Sy1 && y < global::state.Sy1)
                global::state.mousePressL2 = true;
            
            positionSlider(x);
        }
        
        // Click on play/pause button
        if (x > global::state.Sx0 && x < global::state.Sx1 
            && y > global::state.Sy0 && y < global::state.Sy1)
        {
            global::state.pausePlayback = !global::state.pausePlayback;
            glutPostRedisplay();
        }
        
        // Click on maximize button, or by double clicking in the window
        else if (global::state.firstClick ||
                 (x > global::state.Sx2 && x < global::state.Sx3 
                  && y > global::state.Sy0 && y < global::state.Sy1) )
        {
            if (global::state.fullScreen)
                glutReshapeWindow(global::state.widthWin0, global::state.heightWin0);
            else
            {
                global::state.widthWin0 = global::state.widthWin;
                global::state.heightWin0 = global::state.heightWin;
                glutFullScreen();
            }
            
            global::state.fullScreen = !global::state.fullScreen;
        }
        
        else
        {
            global::state.firstClick = true;
            glutTimerFunc(300, doubleClickRegistration, 0);
        }
    }
    else
    {
        global::state.mousePressL1 = false;
        global::state.mousePressL2 = false;
    }
}

// On mouse click and movement
void mouseMotion(int x, int)
{
    resetGuiTime();
    
    positionSlider(x);
}

// On mouse movement
void mousePassiveMotion(int xi, int yi)
{
    unsigned int x = xi, y = yi;
    resetGuiTime();
    
    // Locate if and where the mouse is above the GUI
    if (x > global::state.Sx0 && x < global::state.Sx1 && 
        y > global::state.Sy0 && y < global::state.Sy1) // Pause/play button
        global::state.guiButton = 1;
    else if (x > global::state.Sx1 && x < global::state.Sx2 &&
             y > global::state.Sy0 && 
             y < 0.7*global::state.Sy0 + 0.3*global::state.Sy1) // Time slider
        global::state.guiButton = 2;
    else if (x > global::state.Sx1 && x < global::state.Sx2 && 
             y > 0.7*global::state.Sy0 + 0.3*global::state.Sy1 && 
             y < global::state.Sy1) // Exposure slider
        global::state.guiButton = 3;
    else if (x > global::state.Sx2 && x < global::state.Sx3 && 
             y > global::state.Sy0 && y < global::state.Sy1) // Maximize button
        global::state.guiButton = 4;
    else
        global::state.guiButton = 0;
    
    if (global::state.pausePlayback)
        glutPostRedisplay();
}

// On keybord press
void keyboard(unsigned char key, int x, int)
{
    switch(key)
    {
    // Exit
    case 27:
    case 17:
    case 23:
        glutDestroyWindow(global::state.windNr);
        exit(0);
        break;
    // Pause/play
    case ' ':
        global::state.pausePlayback = !global::state.pausePlayback;
        resetGuiTime();
        glutPostRedisplay();
        break;
    // Increase exposure
    case '+':
    case '=':
        global::state.exposure *= 1.5f;
        glUniform1f(glGetUniformLocation(global::state.dequantProg, "exposure"),
                    global::state.exposure);
        resetGuiTime();
        glutPostRedisplay();
        break;
    // Decrease exposure
    case '-':
        global::state.exposure /= 1.5f;
        glUniform1f(glGetUniformLocation(global::state.dequantProg, "exposure"),
                    global::state.exposure);
        resetGuiTime();
        glutPostRedisplay();
        break;
    // Apply tone curve
    case 't':
        global::state.doTmo = !global::state.doTmo;
        glUniform1i(glGetUniformLocation(global::state.dequantProg, "doTmo"),
                    global::state.doTmo);
        glutPostRedisplay();
        break;
    // Simulate an LDR video
    case 'l':
        global::state.ldrSim = !global::state.ldrSim;
        glUniform1i(glGetUniformLocation(global::state.dequantProg, "ldrSim"),
                    global::state.ldrSim);
        glutPostRedisplay();
        break;
    // Seeking by mouse position
    case 's':
        seekToTime(float(x)/global::state.widthWin);
        break;
    // Show simple timings
    case 'z':
        global::state.showTimings = !global::state.showTimings;
        break;
    }
}

// =============================================================================

