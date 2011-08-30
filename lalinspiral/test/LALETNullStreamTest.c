/*
*  Copyright (C) 2011 Walter Del Pozzo, Tjonnie Li
*
*  This program is free software; you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation; either version 2 of the License, or
*  (at your option) any later version.
*
*  This program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with with program; see the file COPYING. If not, write to the
*  Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston,
*  MA  02111-1307  USA
*/

/* <lalVerbatim> */
#include <stdlib.h>
#include <getopt.h>
#include <lal/LALETNullStream.h>

NRCSID( LALETNULLSTREAMTESTC, "$Id$" );

#define USAGE "Usage: LALETNullStreamTest [--duration REAL8] [--GPSstart INT] [--Nsegs INT] [--o output] \n \
	 [OPTIONAL] [--Srate REAL8 [DEFAULT:4096]]\n"

extern int lalDebugLevel;

/*******************************************************************************
 *
 *  GLOBAL VARIABLES
 * 
 ******************************************************************************/

// GLOBAL VARIABLES USED IN ARGUMENT PARSING - SETS THE DEFAULT VALUE


CHAR ifo[LALNameLength] = "NS";
CHAR channel_time[LALNameLength]="NS:STRAIN";
CHAR channel_freq[LALNameLength]="NS:INV_SPEC";

CHAR outfile[FILENAME_MAX];
REAL8 duration_global=100.0;
LIGOTimeGPS GPSStart_global; // NB: NO INFRASTRUCTURE FOR NANOSECOND INPUT!
UINT4 SampleRate_global=4096; // JUSTIFY CHOICE WDP: the data have been generated with this Srate
UINT4 Nsegs_global=10; 
/*******************************************************************************
 *
 *  FUNCTION PROTOTYPES
 * 
 ******************************************************************************/

void initialise(INT4 argc, char *argv[]);

/*******************************************************************************
 *
 *  MAIN FUNCTION
 * 
 ******************************************************************************/

int main( int argc, char *argv[] )
{
    /***************************************************************************
     *
     *  TEST FUNCTION TO COMPUTE THE ET NULLSTREAM
     * 
     **************************************************************************/
    
    /***************************************************************************
     *
     *  ERROR HANDLING
     * 
     **************************************************************************/
    
    static LALStatus status;
    lalDebugLevel = 0;
    
    /***************************************************************************
     *
     *  ARGUMENT PARSING
     * 
     **************************************************************************/
    
		initialise(argc,argv); 
		
    /***************************************************************************
     *
     *  DECLARE TEMPORARY VARIABLES
     *		- EXCERP FROM ComputeSingleDetectorInvPSDfromNullStream
     * 
     **************************************************************************/		
		
		UINT4 i=0; 
		UINT4 j;
		INT4 check=0; 
		
		REAL8 segDur = duration_global/(REAL8)Nsegs_global;
    UINT4 seglen = (UINT4)(segDur*SampleRate_global); // GET THE LENGTH OF A SEGMENT
    segDur = seglen/SampleRate_global; // UPDATE WITH THE TRUNCATED SEGLEN
  	UINT4 nSegs = (INT4)floor(duration_global/segDur); // UPDATE NSEGS TO ACCOMODATE SAMPLERATE
    UINT4 stride = seglen; /* Overlap the padding */
    //REAL8 strideDur = stride / SampleRate;
    REAL8 deltaF=(REAL8)SampleRate_global/seglen;
    REAL8 end_freq=10.0; /* cutoff frequency */   

    /***************************************************************************
     *
     *  EXAMPLE FETCHING THE INDIVIDUAL STREAMS
     *		- EXCERPT FROM THE FUNCTION "LALETNullStream"
     * 
     **************************************************************************/ 
    
    fprintf(stdout, "Fetching strain streams for individual detectors\n");
   fprintf(stdout, "%e | %d | %d | %e \n", segDur, seglen, nSegs, deltaF); 
    const CHAR *ChannelNames[3] = {"E1:STRAIN", "E2:STRAIN", "E3:STRAIN"};
    const CHAR *CacheFileNames[3] = {"/home/tania/cache/E1GW.cache","/home/tania/cache/E2GW.cache","/home/tania/cache/E3GW.cache"}; 
    //const CHAR *CacheFileNames[3] = {"/home/tania/cache/E1_GWOnlyNewton.cache","/home/tania/cache/E2_GWOnlyNewton.cache","/home/tania/cache/E3_GWOnlyNewton.cache"}; 
    
    REAL8TimeSeries *RawData[3]; // INDIVIDUAL DETECTOR STRAINS
    
    /* Read the data from disk into a vector (RawData) */
    for (i=0; i<3; i++) {
    		fprintf(stdout, "Fetching strain streams for individual detectors - Channel %s Cache %s\n", ChannelNames[i],CacheFileNames[i]);
        RawData[i] = ReadTimeSerieFromCache(CacheFileNames[i], ChannelNames[i],&GPSStart_global,duration_global);
        if(&RawData[i]==NULL){fprintf(stderr,"Error opening %s in %s\n",ChannelNames[i],CacheFileNames[i]); exit(-1);}
    }    
    
    // RESAMPLE DATA STREAMS - OPTIONAL
    if (((REAL8)SampleRate_global)!=1.0/(RawData[0]->deltaT)) fprintf(stdout, "Fetching strain streams for individual detectors - Resampling time series to from %d to %d Hz \n", (UINT4)(1.0/(RawData[0]->deltaT)), SampleRate_global);
    for(i=0;i<3;i++){
			if (((REAL8)SampleRate_global)!=1.0/(RawData[i]->deltaT)) {
				XLALResampleREAL8TimeSeries(RawData[i],1.0/SampleRate_global);
			}

		}    
   
 
    fprintf(stdout, "Fetching strain streams for individual detectors - done \n");
    
    /***************************************************************************
     *
     *  EXAMPLE CALCULATING INVPSD OF INDIVIDUAL STRAIN STREAMS
     *		- EXCERPT FROM THE FUNCTION "ComputeSingleDetectorInvPSDfromNullStream"
     * 
     **************************************************************************/    
    
    fprintf(stdout, "Calculating invPSD for individual detectors \n");
    REAL8FrequencySeries *invPSD_individual[3];
    
    for(i=0; i<3; i++){
    	
    	// CREATE PLANS FOR PSD CALCULATIONS
    	REAL8Window  *windowplan = XLALCreateTukeyREAL8Window(seglen,0.1*(REAL8)8.0*SampleRate_global/(REAL8)seglen);;
    	REAL8FFTPlan *fwdplan = XLALCreateForwardREAL8FFTPlan( seglen, 1 );
    	REAL8FFTPlan *revplan = XLALCreateReverseREAL8FFTPlan( seglen, 1 );
    
			// SHRINKING STRAIN STREAMS INTO INTEGER SEGMENTS
			fprintf(stdout,"... Shrinking - (lost %d samples from end)\n",RawData[i]->data->length-(seglen*nSegs));
			RawData[i]=(REAL8TimeSeries *)XLALShrinkREAL8TimeSeries(RawData[i],(size_t) 0, (size_t) seglen*nSegs);
			fprintf(stdout,"... Computing power spectrum, seglen %i stride %i\n",seglen,stride);
			
			// CALCULATE THE INVERSE SPECTRUM
			invPSD_individual[i] = XLALCreateREAL8FrequencySeries("inverse spectrum",&GPSStart_global,0.0,deltaF,&lalDimensionlessUnit,seglen/2+1); 
			check = XLALREAL8AverageSpectrumMedian(invPSD_individual[i],RawData[i],(UINT4)seglen,(UINT4)stride,windowplan,fwdplan);
			if (check) {fprintf(stderr,"Failed computing the XLALREAL8AverageSpectrumMedian with error %d \n", check);exit(-1);}
			
			// TRUNCATE INVERSE SPECTRUM
			check = XLALREAL8SpectrumInvertTruncate(invPSD_individual[i], end_freq, seglen, (seglen-stride)/4, fwdplan, revplan ); 
			if (check) {fprintf(stderr,"Failed computing XLALREAL8SpectrumInvertTruncate \n");exit(-1);}
			
			
			XLALDestroyREAL8Window(windowplan);
			XLALDestroyREAL8FFTPlan(fwdplan);
			XLALDestroyREAL8FFTPlan(revplan);
    }
    
    // PRINT TO FILE
    FILE *invpsd_individual_stream;
    char InvPSDIndividual_name[124];
		
		for(i=0;i<3;i++){
			sprintf(InvPSDIndividual_name,"%s%d%s%d%s","invpsd_E",i+1,"_",GPSStart_global.gpsSeconds,".dat");
			invpsd_individual_stream = fopen(InvPSDIndividual_name, "w");
			fprintf(stdout, "Calculating invPSD for individual detectors - print to %s \n", InvPSDIndividual_name);
			for(j=0;j<invPSD_individual[i]->data->length;j++) {
      	fprintf(invpsd_individual_stream,"%10.10lf %10.10e\n",j*invPSD_individual[i]->deltaF,invPSD_individual[i]->data->data[j]); 
    	}
		}	
		
		fclose(invpsd_individual_stream);
		
	//	exit(0);
    
    /***************************************************************************
     *
     *  EXAMPLE NULL STREAM COMPUTATION
     * 
     **************************************************************************/
    
    fprintf(stdout, "Computing Null stream \n");
    
    // GET NULL STREAM FROM GPSSTART_GLOBAL AND DURATION_GLOBAL
    REAL8TimeSeries *NullStream = LALETNullStream(&GPSStart_global,duration_global);
    
    /* write the null stream to a frame file */
    //LALFrWriteREAL8TimeSeries(&status,NullStream,&frSerie_time);
    
    // RESAMPLE TIMESERIES - OPTIONAL
    fprintf(stdout, "Computing Null stream - Resampling time series to %d Hz \n", SampleRate_global);
    if (SampleRate_global!=8192) {
			XLALResampleREAL8TimeSeries(NullStream,1.0/SampleRate_global);
		}
		
		fprintf(stdout, "Computing Null stream - done \n");   
     
    /***************************************************************************
     *
     *  COMPUTE PSD (PUT IN FUNCTION INSTEAD OF IN THE TEST FUNCTION)
     *      + OUTPUT TO FILE
     * 
     **************************************************************************/
     
    fprintf(stdout, "Computing single detector PSD from Null stream \n");
    
    // COMPUTE THE SINGLE DETECTOR PSD FROM THE NULLSTREAM
    REAL8FrequencySeries *inverse_spectrum = ComputeSingleDetectorInvPSDfromNullStream(&GPSStart_global, duration_global, SampleRate_global, Nsegs_global);
    
    fprintf(stdout, "Computing single detector PSD from Null stream - print to %s \n", outfile);
    // PRINT TO FILE
    FILE *psdout;
		psdout=fopen(outfile,"w");
    
    // PRINT FILE
		for(j=0;j<inverse_spectrum->data->length;j++) {
       fprintf(psdout,"%10.10lf %10.10e\n",j*inverse_spectrum->deltaF,inverse_spectrum->data->data[j]); 
    }
    fclose(psdout);
    
    fprintf(stdout, "Computing single detector PSD from Null stream - done \n");
    
     /***************************************************************************
     *
     *  CLEAN AND EXIT
     * 
     **************************************************************************/
      
    XLALDestroyREAL8TimeSeries(NullStream);
    XLALDestroyREAL8FrequencySeries(inverse_spectrum);
    
    return status.statusCode;
}

/*******************************************************************************
 *
 *  FUNCTION
 * 
 ******************************************************************************/

void initialise(int argc, char *argv[])
{
	int i;
	double GPS;
	
	/*	sprintf(outfile,"default.dat"); */
	/* Sets up global variables from the command line */
	
	static struct option long_options[]=
	{	{"duration",required_argument,0,'a'},
		{"GPSstart",required_argument,0,'b'},
		{"Nsegs",required_argument,0,'c'},
		{"Srate",required_argument,0,'d'},
    {"out",required_argument,0,'e'},
		{0,0,0,0}
	};
	
	if(argc<=1) {fprintf(stderr,USAGE); exit(-1);}
	
	while((i=getopt_long(argc,argv,"a:b:c:d:e",long_options,&i))!=-1)
		{ 
		switch(i) {
			case 'a':
				duration_global=atof(optarg);
				break;	
			case 'b':
				GPS=atof(optarg);
				XLALGPSSetREAL8(&GPSStart_global,GPS);
				break;
			case 'c':
				Nsegs_global=atoi(optarg);
				break;
			case 'd':
				SampleRate_global=atof(optarg);
				fprintf(stderr,"Setting the sample rate to %i. This might not be what you want!\n",SampleRate_global);
				break;
      case 'e':
				strcpy(outfile,optarg);
        break;  
			default:
				fprintf(stdout,USAGE); exit(0);
				break;
			}
		}
}
