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

#include "arg_parser.h"
#include "luma_exception.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sstream>
#include <math.h>



//=== Flag =====================================================================
void ArgParser::add(bool *var,
                    std::string name, std::string shortName, std::string description)
{
    ArgHolder arg(name, shortName, description, true);
    arg.varFLAG = var;
    arg.argType = ARG_FLAG;
    m_args.push_back(arg);
}
//==============================================================================


             
//=== Unsigned int =============================================================
void ArgParser::add(unsigned int *var,
                    std::string name, std::string shortName, std::string description,
                    bool optional)
{
    ArgHolder arg(name, shortName, description, optional);
    arg.varUINT = var;
    arg.argType = ARG_UINT;
    m_args.push_back(arg);
}

void ArgParser::add(unsigned int *var,
                    std::string name, std::string shortName, std::string description,
                    unsigned int mn, unsigned int mx,
                    bool optional)
{
    add(var, name, shortName, description, optional);
    int ind = m_args.size() - 1;
    m_args[ind].minUINT = mn;
    m_args[ind].maxUINT = mx;
    m_args[ind].checkRange = true;
}

void ArgParser::add(unsigned int *var,
                    std::string name, std::string shortName, std::string description,
                    unsigned int validArgs[], int nrValid,
                    bool optional)
{
    add(var, name, shortName, description, optional);
    int ind = m_args.size() - 1;
    m_args[ind].validUINT = std::vector<unsigned int>(validArgs, validArgs+nrValid);
    m_args[ind].checkValid = true;
}
//==============================================================================



//=== Float ====================================================================
void ArgParser::add(float *var,
                    std::string name, std::string shortName, std::string description,
                    bool optional)
{
    ArgHolder arg(name, shortName, description, optional);
    arg.varFLOAT = var;
    arg.argType = ARG_FLOAT;
    m_args.push_back(arg);
}

void ArgParser::add(float *var,
                    std::string name, std::string shortName, std::string description,
                    float mn, float mx,
                    bool optional)
{
    add(var, name, shortName, description, optional);
    int ind = m_args.size() - 1;
    m_args[ind].minFLOAT = mn;
    m_args[ind].maxFLOAT = mx;
    m_args[ind].checkRange = true;
}

void ArgParser::add(float *var,
                    std::string name, std::string shortName, std::string description,
                    float validArgs[], int nrValid,
                    bool optional)
{
    add(var, name, shortName, description, optional);
    int ind = m_args.size() - 1;
    m_args[ind].validFLOAT = std::vector<float>(validArgs, validArgs+nrValid);
    m_args[ind].checkValid = true;
}
//==============================================================================



//=== String ===================================================================
void ArgParser::add(std::string *var,
                    std::string name, std::string shortName, std::string description,
                    bool optional)
{
    ArgHolder arg(name, shortName, description, optional);
    arg.varSTRING = var;
    arg.argType = ARG_STRING;
    m_args.push_back(arg);
}

void ArgParser::add(std::string *var,
                    std::string name, std::string shortName, std::string description,
                    std::string validArgs[], int nrValid,
                    bool optional)
{
    add(var, name, shortName, description, optional);
    int ind = m_args.size() - 1;
    m_args[ind].validSTRING = std::vector<std::string>(validArgs, validArgs+nrValid);
    m_args[ind].checkValid = true;
}
//==============================================================================



// Display specified general and parameter specific information
void ArgParser::displayInfo()
{
    fprintf(stderr, "%s\nAvailable options:\n", m_preInfo.c_str());
    for (size_t i=0; i<m_args.size(); i++)
    {
        std::string type;
        switch (m_args[i].argType)
        {
        case ARG_UINT:
            type = " <int>";
            break;
        case ARG_FLOAT:
            type = " <float>";
            break;
        case ARG_STRING:
            type = " <string>";
            break;
        case ARG_FLAG:
        case ARG_NONE:
            type = "";
            break;
        }
        fprintf(stderr, "  %-15s  %-25s\t:  %s\n",
                (m_args[i].shortName + type + ",").c_str(),
                (m_args[i].name + type).c_str(),
                m_args[i].description.c_str());
    }
    fprintf(stderr, "%s\n", m_postInfo.c_str());
}



// Read arguments
bool ArgParser::read(int argc, char* argv[])
{
    for (int i=1; i<argc; i++)
    {
        bool argFound = false;
        for (size_t a=0; a<m_args.size(); a++)
        {
            // Show help text and abort
            if ( !(strcmp(argv[i], "--help") && strcmp(argv[i], "-help") &&
                   strcmp(argv[i], "--h") && strcmp(argv[i], "-h")) )
            {
                displayInfo();
                return 0;
            }
            
            if ( !(strcmp(argv[i], m_args[a].name.c_str()) && strcmp(argv[i], m_args[a].shortName.c_str())) )
            {
                argFound = true;
                
                if (m_args[a].argType != ARG_FLAG && argc <= ++i)
                {
                    std::string msg = "No value provided for input option '" + std::string(argv[i-1]) + "'";
                    throw ParserException(msg);
                }
                
                switch(m_args[a].argType)
                {
                case ARG_FLAG:
                    *(m_args[a].varFLAG) = true;
                    break;
                case ARG_UINT:
                    {
                    unsigned int arg = atoi(argv[i]);
                    if (m_args[a].checkRange && (arg < m_args[a].minUINT || arg > m_args[a].maxUINT) )
                    {
                        std::ostringstream ss;
                        ss << "Argument '" << argv[i-1] << "' with value '" << arg << "' is out of range. ",
                        ss << "Valid range is [" << m_args[a].minUINT << ", " << m_args[a].maxUINT << "]";
                        throw ParserException(ss.str().c_str());
                    }
                    if (m_args[a].checkValid)
                    {
                        bool found = false;
                        for (size_t v=0; v<m_args[a].validUINT.size(); v++)
                            if (arg == m_args[a].validUINT[v])
                            {
                                found = true;
                                break;
                            }
                        if (!found)
                        {
                            std::ostringstream ss;
                            ss << "Input '" << arg << "' for argument '" << argv[i-1] << "' is not valid. Valid values are: ";
                            for (size_t v=0; v<m_args[a].validUINT.size(); v++)
                                ss << m_args[a].validUINT[v] << " ";
                            throw ParserException(ss.str().c_str());
                        }
                        
                    }
                    *(m_args[a].varUINT) = arg;
                    }
                    break;
                case ARG_FLOAT:
                    {
                    float arg = atof(argv[i]);
                    if (m_args[a].checkRange && (arg < m_args[a].minFLOAT || arg > m_args[a].maxFLOAT) )
                    {
                        std::ostringstream ss;
                        ss << "Argument '" << argv[i-1] << "' with value '" << arg << "' is out of range. ",
                        ss << "Valid range is [" << m_args[a].minFLOAT << ", " << m_args[a].maxFLOAT << "]";
                        throw ParserException(ss.str().c_str());
                    }
                    if (m_args[a].checkValid)
                    {
                        bool found = false;
                        for (size_t v=0; v<m_args[a].validFLOAT.size(); v++)
                            if (fabs(arg - m_args[a].validFLOAT[v]) < ARG_EPS)
                            {
                                found = true;
                                break;
                            }
                        if (!found)
                        {
                            std::ostringstream ss;
                            ss << "Input '" << arg << "' for argument '" << argv[i-1] << "' is not valid. Valid values are: ";
                            for (size_t v=0; v<m_args[a].validFLOAT.size(); v++)
                                ss << m_args[a].validFLOAT[v] << " ";
                            throw ParserException(ss.str().c_str());
                        }
                        
                    }
                    *(m_args[a].varFLOAT) = arg;
                    }
                    break;
                case ARG_STRING:
                    {
                    std::string arg = argv[i];
                    if (m_args[a].checkValid)
                    {
                        bool found = false;
                        for (size_t v=0; v<m_args[a].validSTRING.size(); v++)
                            if (!(strcmp(arg.c_str(), m_args[a].validSTRING[v].c_str())))
                            {
                                found = true;
                                break;
                            }
                        if (!found)
                        {
                            std::ostringstream ss;
                            ss << "Input '" << arg << "' for argument '" << argv[i-1] << "' is not valid. Valid values are: ";
                            for (size_t v=0; v<m_args[a].validSTRING.size(); v++)
                                ss << m_args[a].validSTRING[v] << " ";
                            throw ParserException(ss.str().c_str());
                        }
                        
                    }
                    *(m_args[a].varSTRING) = arg;
                    }
                    break;
                default:
                    throw ParserException("Unrecognized input option type");
                    break;
                }
            }
        }
        
        if (!argFound)
        {
            std::string msg = "The argument '" + std::string(argv[i]) + "' is not a valid input option";
            throw ParserException(msg);
        }
    }
    
    // Are all required arguments provided?
    for (size_t a=0; a<m_args.size(); a++)
    {
        bool argFound = false;
        for (int i=1; i<argc; i++)
        {
            if ( !(strcmp(argv[i], m_args[a].name.c_str()) && strcmp(argv[i], m_args[a].shortName.c_str())) )
            {
                if (m_args[a].argType != ARG_FLAG)
                    i++;
                
                argFound = true;
            }
        }
        if (!m_args[a].optional && !argFound)
        {
            std::string msg = "Missing required option '" + m_args[a].name + "'";
            throw ParserException(msg);
        }
    }
    
    return 1;
}


