#include "soltrace.h"

#ifdef ST_CONSOLE_APP

#define _CONSOLE

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWidgets headers)
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include <wx/app.h>
#include <wx/cmdline.h>
#include <wx/dir.h>
#include <wx/filename.h>

#include "project.h"
#include "trace.h"

// ============================================================================
// implementation
// ============================================================================

static const wxCmdLineEntryDesc cmdLineDesc[] =
{
    { wxCMD_LINE_SWITCH, "h", "help", "show this help message", wxCMD_LINE_VAL_NONE, wxCMD_LINE_OPTION_HELP },
    { wxCMD_LINE_SWITCH, "i", "info", "Show usage info and documentation", wxCMD_LINE_VAL_NONE },
    { wxCMD_LINE_OPTION, "f", "file", "Run an stinput file", wxCMD_LINE_VAL_STRING, wxCMD_LINE_OPTION_MANDATORY },
    { wxCMD_LINE_OPTION, "r", "rays", "Required ray intersections (=1e4)", wxCMD_LINE_VAL_NUMBER },
    { wxCMD_LINE_OPTION, "m", "maxrays", "Max. number of sun rays (=100*rays)", wxCMD_LINE_VAL_NUMBER },
    { wxCMD_LINE_OPTION, "c", "cpu", "Number of CPU's (=16)", wxCMD_LINE_VAL_NUMBER},
    { wxCMD_LINE_OPTION, "s", "seed", "Random seed (=-1)", wxCMD_LINE_VAL_NUMBER},
    { wxCMD_LINE_OPTION, "p", "sunshape", "Enable sunshape (=1)", wxCMD_LINE_VAL_NUMBER},
    { wxCMD_LINE_OPTION, "e", "error", "Enable optical error (=1)", wxCMD_LINE_VAL_NUMBER},
    { wxCMD_LINE_OPTION, "t", "tower", "Run as power tower (=1)", wxCMD_LINE_VAL_NUMBER},
    { wxCMD_LINE_OPTION, "o", "out", "File to write ray data", wxCMD_LINE_VAL_STRING},
    { wxCMD_LINE_OPTION, "x", "nbinx", "Number of flux bins in X", wxCMD_LINE_VAL_NUMBER},
    { wxCMD_LINE_OPTION, "y", "nbiny", "Number of flux bins in Y", wxCMD_LINE_VAL_NUMBER},
    { wxCMD_LINE_OPTION, "z", "final", "Report final rays only (=1)", wxCMD_LINE_VAL_NUMBER},
    { wxCMD_LINE_OPTION, "d", "dni", "DNI [kW/m2] (=1)", wxCMD_LINE_VAL_NUMBER},

    { wxCMD_LINE_PARAM, 0, 0, "Stage.element number(s) for data reporting", wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_MULTIPLE},
    //{ wxCMD_LINE_OPTION, "s", "summary", "File to write summary", wxCMD_LINE_VAL_STRING},
    //{ wxCMD_LINE_OPTION, "s", "script", "Run an lk script file", wxCMD_LINE_VAL_STRING},
    //{ wxCMD_LINE_SWITCH, "i", "interactive", "Interactive mode"},
    { wxCMD_LINE_NONE }
};

int main(int argc, char **argv)
{
    wxApp::CheckBuildOptions(WX_BUILD_OPTIONS_SIGNATURE, "program");

    wxInitializer initializer;
    if ( !initializer )
    {
        fprintf(stderr, "Failed to initialize the wxWidgets library, aborting.");
        return -1;
    }
    
    //default values
    wxString fname = ""; //"C:/Users/mwagner/Documents/NREL/projects/SolTrace-git/app/deploy/x64/A1.stinput";
    wxString fnout = ""; //trace.out";
    wxString fnsum = "";
    long rays = (int)1e4;
    long maxrays = 100*rays;
    long threads = 16;
    long l_seed = -1;
    long l_sunshape = 1;
    long l_error = 1;
    long l_tower = 1;
    long l_nbinx=20;
    long l_nbiny=20;
    long l_final = 1;
    double dni=1.;

    wxCmdLineParser parser(cmdLineDesc, argc, argv);

    parser.Parse();
    
    if( parser.Found("i") )
    {
        wxPrintf(
            "This interface allows execution of SolTrace project files via command line.\n"
            "Example usage is as follows:\n"
            "$ soltrace_cmd -f MyProject.stinput -r 100000 -c 3 -p 0 2.2 2.3 2.4 2.6\n"
            ".. This can be interpreted as \"run soltrace file 'MyProject.stinput' with\n"
            "100,000 rays, 3 threads, and no sunshape, then export flux information for\n"
            "second stage elements 2, 3, 4, and 6.\"\n\n"
            "Another example:\n"
            "$ soltrace_cmd -f \"C:\\Another\\project with\\spaces.stinput\" -o \"C:\\Another\\project with\\output.csv\" 1.1\n"
            ".. this can be interpreted as \"run soltrace file spaces.stinput, and output\n"
            "a ray data file 'output.csv' at the specified paths, and save data for stage\n"
            " 1 element 1.\""
            );

        return 0;
    }

    if( parser.Found("f", &fname) )
    {
        //check that file exists
        if(! ::wxFileExists( fname ) )
        {
            wxPrintf( "\nInput file not found! Invalid path." );
            return 0;
        }
    }

    parser.Found("r", &rays);
    parser.Found("m", &maxrays);
    parser.Found("c", &threads);
    parser.Found("s", &l_seed);
    parser.Found("p", &l_sunshape);
    parser.Found("e", &l_error);
    parser.Found("t", &l_tower);
    parser.Found("x", &l_nbinx);
    parser.Found("y", &l_nbiny);
    parser.Found("z", &l_final);
    parser.Found("d", &dni);

            
    if( parser.Found("o", &fnout) )
    {
        //check that file exists
        if(! ::wxDir::Exists( wxPathOnly(fnout) ) )
        {
            wxPrintf( "\nRay data file path not found!" );
            return 0;
        }
    }

    if( parser.Found("s", &fnsum) )
    {
        //check that file exists
        if(! ::wxDir::Exists( wxPathOnly(fnsum) ) )
        {
            wxPrintf( "\nSummary file path not found!" );
            return 0;
        }
    }

    if ( argc == 1 )
    {
        // If there were no command-line options supplied, emit a message
        // otherwise it's not obvious that the sample ran successfully
        wxPrintf("Welcome to the SolTrace 'console' interface!\n");
        wxPrintf("For more information, run again with the --help option\n");
        return 0;
    }

    if( parser.Found("h") )
        return 0;

    // create and execute according to the commands
    FILE *fp_in = fopen( fname.c_str(), "r" );
	
	Project project;
    project.Read( fp_in );
    
    fclose(fp_in); 
    
    //type conversion
    int seed = (int)l_seed;
    bool sunshape = l_sunshape == 1L;
    bool error = l_error == 1L;
    bool tower = l_tower == 1L;
    int nbinx = (int)l_nbinx;
    int nbiny = (int)l_nbiny;
    bool final = l_final == 1L;

    wxArrayString ref_errors;

    int msec = RunTraceMultiThreaded( &project, 
            (int)rays,
			(int)maxrays,
			threads,
			&seed,
			sunshape,
			error,
            tower,
			ref_errors,
            true);


    //------------------------------------------------------------------------------
    if(! fnout.IsEmpty() )
    {
        FILE *fp_out = fopen( fnout.c_str(), "w" );
	    if ( !fp_out )
	    {
		    wxPrintf("Could not open file for writing:\n%s\n", fnout);
		    return 0;
	    }

        //create output
        if ( msec < 0 )
		    wxPrintf( wxJoin( ref_errors, '\n' ) );

        // by default, all ray data is in stage coordinates
	    RayData &rd = project.Results;
	
	    int nwrite = rd.PrepareExport( RayData::COORD_GLOBAL, 0 );
	
	    // progress update every 0.5 %
	    int nupdate = nwrite / 200;
	    size_t nwr = 0;
	    size_t bytes = 0;
	
	    double Pos[3], Cos[3];
	    int Elm, Stg, Ray;

	    fputs( "Pos X,Pos Y,Pos Z,Cos X,Cos Y,Cos Z,Element,Stage,Ray Number\n", fp_out );
	    while( rd.GetNextExport( project, Pos, Cos, Elm, Stg, Ray ) )
	    {
		    /*if ( nwr % nupdate == 0 )
		    {
			    bool proceed = pd.Update( (int)( 100*((double)nwr)/((double)nwrite) ), 
					    "Writing data to " + dlg.GetPath() + wxString::Format(" (%.2lf MB)", bytes*0.000001 ) );
			    if( !proceed )
				    break;
		    }*/

		    int nb = fprintf( fp_out, "%lg,%lg,%lg,%lg,%lg,%lg,%d,%d,%d\n",
			    Pos[0], Pos[1], Pos[2],
			    Cos[0], Cos[1], Cos[2],
			    Elm, Stg, Ray );

		    if ( nb < 0 ) break; // file error, disk space issue?

		    bytes += nb;
		    nwr++;
	    }

	    if ( ferror( fp_out ) )
		    wxPrintf(wxString("An error occurred exporting the CSV data file: ") + strerror(errno));
    
        fclose(fp_out);
    }

    ////------------------------------------------------------------------------------
    //if(! fnsum.IsEmpty() )
    //{
    //    //write summary data from the run

    //    FILE *fp_sum = fopen( fnsum.c_str(), "w" );
	   // if ( !fp_sum )
	   // {
		  //  wxPrintf("Could not open file for writing:\n%s\n", fnsum);
		  //  return 0;
	   // }

	   // 
    //    

    //    fclose(fp_sum);
    //}


    //------------------------------------------------------------------------------
    size_t nelout = parser.GetParamCount();
    wxPrintf( "\nReporting %d elements...", (int)nelout );
    
    std::vector<int> e_list;
    for( size_t i=0; i<nelout; i++)
    {
        wxString pdat = parser.GetParam(i);
        wxArrayString dat = wxSplit(pdat, '.');
        if( dat.size() != 2 )
        {
            wxPrintf("\nInvalid stage.element argument number %d. Expecting two integers separated by period (e.g., 5.10), got %s.", (int)i+1, pdat.c_str());
            continue;
        }
        int st = std::atoi(dat[0].c_str());
        int el = std::atoi(dat[1].c_str());

        ElementStatistics es(project);
        double minx,miny,maxx,maxy;
        if(! es.Compute(st, el, nbinx, nbiny, true, final, dni, minx, miny, maxx, maxy) )
        {
            wxPrintf("\nError in flux map plot parameters for st/el %s", pdat.c_str());
            continue;
        }

	    RayData &rd = project.Results;

        //open a new file for writing
        wxFileName feout;
        wxString fnamepath = wxPathOnly(fname);
        wxString fnoutpath = wxPathOnly(fnout);

        if( fnoutpath.IsEmpty() && fnamepath.IsEmpty() )
        {
            feout.AssignDir("./");
        }
        else
        {
            if( fnout.IsEmpty() )
                feout.AppendDir( fnamepath );
            else
                feout.AppendDir( fnoutpath );
        }

        feout.SetExt("csv");
        feout.SetName(wxString::Format("element_output_%s", pdat.c_str()));

        FILE *fp_e = fopen( feout.GetFullPath().c_str(), "w" );
        if( ! fp_e )
        {
            wxPrintf("\n\nCould not open the element file %s for writing. Make sure the file is not write-protected or opened in another program.", feout.GetFullPath().c_str() );
            continue;
        }

        fprintf(fp_e, "Sun ray count,%d\n", rd.SunRayCount );
        fprintf(fp_e, "Over box of dimensions,%lg,%lg\n", rd.SunXMax - rd.SunXMin, rd.SunYMax - rd.SunYMin );
        fprintf(fp_e, "Power per ray,%lg\n", es.PowerPerRay);
        fprintf(fp_e, "Peak flux,%lg\n", es.PeakFlux);
        fprintf(fp_e, "Peak flux uncertainty pct+/-,%lg\n", es.PeakFluxUncertainty);
        fprintf(fp_e, "Min flux,%lg\n", es.MinFlux);
        fprintf(fp_e, "Sigma flux,%lg\n",  es.SigmaFlux);
        fprintf(fp_e, "Avg. flux,%lg\n", es.AveFlux);
        fprintf(fp_e, "Avg. flux uncertainty pct+/-,%lg\n", es.AveFluxUncertainty);
        fprintf(fp_e, "Uniformity,%lg\n", es.Uniformity);
        fprintf(fp_e, "Power of plotted rays,%lg\n", es.NumberOfRays*es.PowerPerRay);
        fprintf(fp_e, "Centroid,%lg,%lg,%lg\n", es.Centroid[0], es.Centroid[1], es.Centroid[2]);

        fprintf(fp_e, "\nFlux\n,");

        double dx = (maxx - minx)/(double)nbinx;
        double dy = (maxy - miny)/(double)nbiny;
        
        //x axis
        for( int i=0; i<nbinx; i++ )
            fprintf(fp_e, "%f%s", minx + ((double)i+0.5)*dx, i==nbinx-1 ? "\n" : "," );

        for( int i=0; i<nbiny; i++ )
        {
            //yaxis
            fprintf(fp_e, "%f,", maxy - ((double)i+0.5)*dy );
            //values
            for( int j=0; j<nbinx; j++ )
                fprintf(fp_e, "%f%s", es.fluxGrid(j,nbiny-1-i)*es.zScale, j==nbinx-1 ? "\n" : "," );
        }
    
        fclose(fp_e);
    }
    
    
    //------------------------------------------------------------------------------

    return 0;
}



#endif