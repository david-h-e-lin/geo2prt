 /*
 * THIS SOFTWARE IS PROVIDED `AS IS' AND ANY EXPRESS
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
#include <GA/GA_AttributeRef.h>
#include <GEO/GEO_AttributeHandle.h>
#include <GU/GU_Detail.h>
#include <GU/GU_PrimPart.h>

static void
usage(const char *program)
{
    cerr << "Usage: " << program << " sourcefile dstfile\n";
    cerr << "Converts the source prt file to the destination bgeo file." << endl;
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

    // Open PRT file
    prtio::prt_ifstream stream( prtFile );
    INT64 prtSize = sizeof(stream);
    cout << "Loading " << prtSize << " particles from PRT file..." << endl;
    
    std::vector<std::string> chanlist = stream.get_channels_list();
    int numchan = chanlist.size();
    cout << "PRT file contains these channels..." << endl;
    for(int c=0; c<numchan; c++){
      cout << chanlist[c] << endl;
    }
    
    //Bind channels
    //We demand a "Position" channel exist, otherwise it throws an exception.
    stream.bind( "Position", theParticle.pos, 3 );
    
    if( stream.has_channel( "Velocity" ) )
      stream.bind( "Velocity", theParticle.vel, 3 );
    if( stream.has_channel( "Color" ) )
      stream.bind( "Color", theParticle.col, 3 );
    if( stream.has_channel( "Density" ) )
      stream.bind( "Density", &theParticle.density, 1 );
    if( stream.has_channel( "ID" ) )
      stream.bind( "ID", &theParticle.id, 1 );

    GU_PrimParticle *PRT;
    
    if (PRT = GU_PrimParticle::build(gdp, 0))
    {
      // create an attribute and get its handle
      GA_RWAttributeRef v = gdp->addFloatTuple(GA_ATTRIB_POINT, "v", 3);
      v.setTypeInfo(GA_TYPE_VECTOR);
      GEO_AttributeHandle v_gah = gdp->getPointAttribute("v");
      
      GA_RWAttributeRef Cd = gdp->addFloatTuple(GA_ATTRIB_POINT, "Cd", 3);
      Cd.setTypeInfo(GA_TYPE_COLOR);
      GEO_AttributeHandle cd_gah = gdp->getPointAttribute("Cd");
      
      GA_RWAttributeRef density = gdp->addFloatTuple(GA_ATTRIB_POINT, "density", 1);
      GEO_AttributeHandle density_gah = gdp->getPointAttribute("density");
      
      GA_RWAttributeRef id = gdp->addIntTuple(GA_ATTRIB_POINT, "id", 1, GA_Defaults(0), 0, 0, GA_STORE_INT64);
      GEO_AttributeHandle id_gah = gdp->getPointAttribute("id");
    
      //map to attribute
      for(int i=0; i < prtSize; i++)
      {
	stream.read_next_particle(); //read a particle
	
	//create a point and append to gdp
	GEO_Point *pt;
	pt = gdp->appendPointElement();
	pt->setPos(theParticle.pos[0], theParticle.pos[1], theParticle.pos[2]); //set position
	
	v_gah.setElement(pt); //target attribute
	v_gah.setV3(UT_Vector3(theParticle.vel[0],theParticle.vel[1],theParticle.vel[2])); //set it
	
	cd_gah.setElement(pt); //target attribute
	cd_gah.setV3(UT_Vector3(theParticle.col[0],theParticle.col[1],theParticle.col[2])); //set it
	
	density_gah.setElement(pt); //target attribute
	density_gah.setValue(theParticle.density); //set it
	
	id_gah.setElement(pt); //target attribute
	id_gah.setValue(theParticle.id); //set it
	
	PRT->appendParticle(pt);
      }
    }
    stream.close();

    // All done successfully
    return true;
}


// Convert a PRT file to a BGEO file

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

    UT_String	inputname, outputname;

    inputname.harden(argv[1]);
    outputname.harden(argv[2]);

    // Get data into gdp
    loadPRT((const char *) inputname, &gdp);

    // Save our result.
    cout << "Saving to BGEO file..." << endl;
#if defined(HOUDINI_11)
    gdp.save((const char *) outputname, 0, 0);
#else
    gdp.save(outputname, NULL);
#endif
    
    return 0;
}
