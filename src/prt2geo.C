/*
 * Copyright (c) 2012
 *	Side Effects Software Inc.  All rights reserved.
 *
 * Redistribution and use of Houdini Development Kit samples in source and
 * binary forms, with or without modification, are permitted provided that the
 * following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. The name of Side Effects Software may not be used to endorse or
 *    promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY SIDE EFFECTS SOFTWARE `AS IS' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN
 * NO EVENT SHALL SIDE EFFECTS SOFTWARE BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *----------------------------------------------------------------------------
 */


#include <stdio.h>
#include <cstdlib>
#include <iostream>

//PRT includes
#include <prtio/prt_ifstream.hpp>
#include <prtio/prt_ofstream.hpp>

#if !defined(WIN32) && !defined(_WIN64) && __WORDSIZE == 64
#define INT64 long int
#else
#define INT64 long long
#endif

// Houdini includes
#include <CMD/CMD_Args.h>
#include <UT/UT_Assert.h>
#include <GEO/GEO_AttributeHandle.h>
#include <GU/GU_Detail.h>
#include <GU/GU_PrimPart.h>

static void
usage(const char *program)
{
    cerr << "Usage: " << program << " sourcefile dstfile\n";
    cerr << "The extension of the source/dest will be used to determine" << endl;
}


bool
loadPRT(const std::string& prtFile, GU_Detail *gdp)
{
    struct{
		float pos[3];
		float vel[3];
		float col[3];
		float density;
		INT64 id;
	} theParticle = { {0,0,0}, {0,0,0}, {1.f,1.f,1.f}, 0.f, -1 };

    prtio::prt_ifstream stream( prtFile );
    
    //We demand a "Position" channel exist, otherwise it throws an exception.
    stream.bind( "Position", theParticle.pos, 3 );

    //GEO_AttributeHandle		name_gah;

    GU_PrimParticle *PRT;
    
    bool prtEnd = 0;
    if (PRT = GU_PrimParticle::build(gdp, 200))
    {
      GA_Primitive::const_iterator it;
      PRT->beginVertex(it);
      do
      {
	prtEnd = stream.read_next_particle(); //read a particle
	gdp->setPos3(it.getPointOffset(), theParticle.pos[0], theParticle.pos[1], theParticle.pos[2]); // feed position
        it.advance();
      }
      while (!it.atEnd() || !prtEnd);
    }
    stream.close();

    // All done successfully
    return true;
}



// Convert a volume into a toy ascii voxel format.

int
main(int argc, char *argv[])
{
    CMD_Args		 args;
    GU_Detail		 gdp;

    args.initialize(argc, argv);

    if (args.argc() != 3)
    {
		usage(argv[0]);
		return 1;
    }

    UT_String		inputname, outputname;

    inputname.harden(argv[1]);
    outputname.harden(argv[2]);

    // Get data into gdp
    loadPRT((const char *) inputname, &gdp);

    // Save our result.
#if defined(HOUDINI_11)
    gdp.save((const char *) outputname, 0, 0);
#else
    gdp.save(outputname, NULL);
#endif
    
    return 0;
}
