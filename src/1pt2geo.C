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
#include <iostream>
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
make1Pt(GU_Detail *gdp)
{
    GEO_AttributeHandle		name_gah;

    GU_PrimParticle     *prt;
	float				 i = 0;

    if (partsys = GU_PrimParticle::build(gdp, 4))
    {
        GA_Primitive::const_iterator it;
        partsys->beginVertex(it);
        do
        {
            // Initially all particles spring from (3,1,1):
            gdp->setPos3(it.getPointOffset(), 3, i, i+0.5);
			i++;
            it.advance();
        }
        while (!it.atEnd());
    }

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

    if (args.argc() != 2)
    {
		usage(argv[0]);
		return 1;
    }

    UT_String		outputname;

    outputname.harden(argv[1]);

	// Get data into gdp
	make1Pt	(&gdp);

	// Save our result.
#if defined(HOUDINI_11)
	gdp.save((const char *) outputname, 0, 0);
#else
	gdp.save(outputname, NULL);
#endif
    
    return 0;
}
