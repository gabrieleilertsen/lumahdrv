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
 * \author Rafal Mantiuk, mantiuk@gmail.com
 *
 * \date Oct 7 2015
 */

#include "pfs_interface.h"

#include <iostream>
#include <pfs.h>

#include "luma_exception.h"


PfsInterface::~PfsInterface()
{
  if( fh != stdin && fh != NULL ) 
    fclose( fh );
  
  delete pfsio;  
  
}


bool PfsInterface::readFrame(const char *inputFile, LumaFrame &frame)
{  
  if( pfsio == NULL )
    pfsio = new pfs::DOMIO();
  
  if( fh == NULL ) {
    if( inputFile == NULL || (strcmp( inputFile, "-" ) == 0) || (strcmp( inputFile, "" ) == 0) ) {      
      // Read from standard input
      fh = stdin;      
    } else {
      fh = fopen( inputFile, "rb" );      
    }
    if( fh == NULL ) 
      throw LumaException( "Cannot open input frames for reading" );      
    
  }

  try {
    
    pfs::Frame *pfs_frame = pfsio->readFrame( fh );
    if( pfs_frame == NULL ) return false; // No more frames
  
    pfs::Channel *X, *Y, *Z;
    pfs_frame->getXYZChannels( X, Y, Z );
    pfs::Channel *R = X, *G = Y, *B = Z; // Reuse the same memory

    if( X != NULL ) {           // Color, XYZ      
      pfs::transformColorSpace( pfs::CS_XYZ, X, Y, Z, pfs::CS_RGB, X, Y, Z );
      // At this point (X,Y,Z) = (R,G,B)    
    } else if( (Y = pfs_frame->getChannel( "Y" )) != NULL ) {
      // Luminance only    
      throw LumaException( "Luminance only frame not supported yet" );    
    } else
      throw LumaException( "Missing X, Y, Z channels in the PFS stream" );
  
  
    frame.width = pfs_frame->getWidth();  
    frame.height = pfs_frame->getHeight();
    frame.channels = 3;
    if (!frame.init())
      throw LumaException( "Cannot allocate memory for frame" );

    memcpy( frame.getChannel(0), R->getRawData(), frame.height*frame.width*sizeof(float) );
    memcpy( frame.getChannel(1), G->getRawData(), frame.height*frame.width*sizeof(float) );
    memcpy( frame.getChannel(2), B->getRawData(), frame.height*frame.width*sizeof(float) );  

    pfsio->freeFrame( pfs_frame );
    
    return true;
  
  }
  catch( pfs::Exception e )
  {
    throw LumaException( e.getMessage() );    
  }
  
}

bool PfsInterface::writeFrame(const char *outputFile, LumaFrame &frame)
{
  if( pfsio == NULL )
    pfsio = new pfs::DOMIO();
  
  if( fh == NULL ) {
    if( outputFile == NULL || (strcmp( outputFile, "-" ) == 0) || (strcmp( outputFile, "" ) == 0) ) {      
      // Write to standard output
      fh = stdout;
    } else {
      fh = fopen( outputFile, "wb" );      
    }
    if( fh == NULL ) 
      throw LumaException( "Cannot open output frames for writing" );      
    
  }
  
  try {
    pfs::Frame *pfs_frame = pfsio->createFrame(frame.width, frame.height);
    pfs::Channel *X, *Y, *Z;
    pfs_frame->createXYZChannels(X, Y, Z);

    memcpy( X->getRawData(), frame.getChannel(0), frame.height*frame.width*sizeof(float) );
    memcpy( Y->getRawData(), frame.getChannel(1), frame.height*frame.width*sizeof(float) );
    memcpy( Z->getRawData(), frame.getChannel(2), frame.height*frame.width*sizeof(float) );  
    
    pfs::transformColorSpace( pfs::CS_RGB, X, Y, Z, pfs::CS_XYZ, X, Y, Z );

    pfsio->writeFrame(pfs_frame, fh);
    pfsio->freeFrame(pfs_frame);

    return true;
  }
  catch( pfs::Exception e )
  {
    throw LumaException( e.getMessage() );    
  }
}


