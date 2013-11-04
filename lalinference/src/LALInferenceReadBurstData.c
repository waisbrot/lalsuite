/* 
 *  LALInferenceReadData.c:  Bayesian Followup functions
 *
 *  Copyright (C) 2013 Salvatore Vitale
 *
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

#include <stdio.h>
#include <stdlib.h>
#include <lal/LALStdio.h>
#include <lal/LALStdlib.h>
#include <lal/LALInspiral.h>
#include <lal/LALCache.h>
#include <lal/LALFrStream.h>
#include <lal/TimeFreqFFT.h>
#include <lal/LALDetectors.h>
#include <lal/AVFactories.h>
#include <lal/ResampleTimeSeries.h>
#include <lal/TimeSeries.h>
#include <lal/FrequencySeries.h>
#include <lal/Units.h>
#include <lal/Date.h>
#include <lal/StringInput.h>
#include <lal/VectorOps.h>
#include <lal/Random.h>
#include <lal/LALNoiseModels.h>
#include <lal/XLALError.h>
#include <lal/GenerateInspiral.h>
#include <lal/LIGOLwXMLRead.h>
#include <lal/LIGOLwXMLInspiralRead.h>
#include <lal/SeqFactories.h>
#include <lal/DetectorSite.h>
#include <lal/GenerateInspiral.h>
#include <lal/GeneratePPNInspiral.h>
#include <lal/SimulateCoherentGW.h>
#include <lal/Inject.h>
#include <lal/LIGOMetadataTables.h>
#include <lal/LIGOMetadataUtils.h>
#include <lal/LIGOMetadataInspiralUtils.h>
#include <lal/LIGOMetadataRingdownUtils.h>
#include <lal/LALInspiralBank.h>
#include <lal/FindChirp.h>
#include <lal/LALInspiralBank.h>
#include <lal/GenerateInspiral.h>
#include <lal/NRWaveInject.h>
#include <lal/GenerateInspRing.h>
#include <lal/LALErrno.h>
#include <math.h>
#include <lal/LALInspiral.h>
#include <lal/LALSimulation.h>
#include <lal/LALInference.h>
#include <lal/LALInferenceLikelihood.h>
#include <lal/LALInferenceTemplate.h>
#include <lal/LIGOLwXMLBurstRead.h>
#include <lal/GenerateBurst.h>
#include <lal/LALSimBurst.h>
#include <lal/LALInferenceReadBurstData.h>
#include <lal/LALSimNoise.h>

#define LALINFERENCE_DEFAULT_FLOW "40.0"
//typedef void (NoiseFunc)(LALStatus *statusPtr,REAL8 *psd,REAL8 f);
static void PrintBurstSNRsToFile(LALInferenceIFOData *IFOdata , REAL8 trigtime);
void InjectSineGaussianFD(LALInferenceIFOData *IFOdata, SimBurst *inj_table, ProcessParamsTable *commandLine);
char *BurstSNRpath = NULL;

static const LALUnit strainPerCount={0,{0,0,0,0,0,1,-1},{0,0,0,0,0,0,0}};
 struct fvec {
	REAL8 f;
	REAL8 x;
};

typedef void (NoiseFunc)(LALStatus *statusPtr,REAL8 *psd,REAL8 f);

void LALInferenceInjectBurstSignal(LALInferenceRunState *irs, ProcessParamsTable *commandLine)
{
	LALStatus status;
	memset(&status,0,sizeof(status));
	SimBurst *injTable=NULL;
  SimBurst *injEvent=NULL;
  TimeSlide *tslide=NULL;
	INT4 Ninj=0;
	INT4 event=0;
	UINT4 i=0,j=0;
	int si=0;
  LIGOTimeGPS injstart;
	REAL8 SNR=0.0,NetworkSNR=0.0;// previous_snr=0.0; 
	memset(&injstart,0,sizeof(LIGOTimeGPS));
	COMPLEX16FrequencySeries *injF=NULL;
	ProcessParamsTable *ppt=NULL;
	REAL8 bufferLength = 2048.0; /* Default length of buffer for injections (seconds) */
	//UINT4 bufferN=0;
	LIGOTimeGPS bufferStart;

	LALInferenceIFOData *IFOdata=irs->data;
	LALInferenceIFOData *thisData=IFOdata->next;
	REAL8 minFlow=IFOdata->fLow;
	REAL8 MindeltaT=IFOdata->timeData->deltaT;
  //REAL8 InjSampleRate=1.0/MindeltaT;
  //REAL4TimeSeries *injectionBuffer=NULL;
 // REAL8 padding=0.4; //default, set in LALInferenceReadData()
	  
	while(thisData){
          minFlow   = minFlow>thisData->fLow ? thisData->fLow : minFlow;
          MindeltaT = MindeltaT>thisData->timeData->deltaT ? thisData->timeData->deltaT : MindeltaT;
          thisData  = thisData->next;
	}
  
	thisData=IFOdata;
	
	if(!LALInferenceGetProcParamVal(commandLine,"--inj")) {fprintf(stdout,"No injection file specified, not injecting\n"); return;}
	if(LALInferenceGetProcParamVal(commandLine,"--event")){
	    event= atoi(LALInferenceGetProcParamVal(commandLine,"--event")->value);
	    fprintf(stdout,"Injecting event %d\n",event);
	}
	else
	    fprintf(stdout,"WARNING: you did not give --event. Injecting event 0 of the xml table, which may not be what you want!\n");
  if(LALInferenceGetProcParamVal(commandLine,"--snrpath")){
    ppt = LALInferenceGetProcParamVal(commandLine,"--snrpath");
    BurstSNRpath = calloc(strlen(ppt->value)+1,sizeof(char));
    memcpy(BurstSNRpath,ppt->value,strlen(ppt->value)+1);
    fprintf(stdout,"Writing SNRs in %s\n",BurstSNRpath)     ;
	}
	injTable=XLALSimBurstTableFromLIGOLw(LALInferenceGetProcParamVal(commandLine,"--inj")->value,0,0);
	REPORTSTATUS(&status);
  Ninj=-1;
  while(injTable){Ninj++;injTable=injTable->next;}
	if(Ninj < event){ 
	    fprintf(stderr,"Error reading event %d from %s\n",event,LALInferenceGetProcParamVal(commandLine,"--inj")->value);
	    exit(1);
  }
	injTable=XLALSimBurstTableFromLIGOLw(LALInferenceGetProcParamVal(commandLine,"--inj")->value,0,0);
	while(si<event) {si++; injTable = injTable->next;} /* Select event */
	injEvent = injTable;
	injEvent->next = NULL;
  tslide=XLALTimeSlideTableFromLIGOLw(LALInferenceGetProcParamVal(commandLine,"--inj")->value);
	REPORTSTATUS(&status);
	//if(status.statusCode!=0) {fprintf(stderr,"Error generating injection!\n"); REPORTSTATUS(&status); }
    
    /* If it is the case, inject burst in the FreqDomain */
    int FDinj=0;
    if (injTable){
        if(!strcmp("SineGaussianF",injEvent->waveform)) FDinj=1;
    }
    
	if (LALInferenceGetProcParamVal(commandLine,"--FDinjections") || FDinj==1)
    {
         InjectSineGaussianFD(thisData, injEvent, commandLine);
         return;
    }
    
	/* Begin loop over interferometers */
	while(thisData){
		
		/*InjSampleRate=1.0/thisData->timeData->deltaT;*/
		if(LALInferenceGetProcParamVal(commandLine,"--injectionsrate")){
    fprintf(stderr,"ERROR: injectionsrate is not supported yet. You can wait or add this functionality\n");
    exit(1);}
    //InjSampleRate=atof(LALInferenceGetProcParamVal(commandLine,"--injectionsrate")->value);
	//	COMPLEX16FrequencySeries *resp = (COMPLEX8FrequencySeries *) XLALCreateCOMPLEX8FrequencySeries("response",&thisData->timeData->epoch,		  0.0,  thisData->freqData->deltaF, &strainPerCount, thisData->freqData->data->length);
		
		//for(i=0;i<resp->data->length;i++) {resp->data->data[i]=crect((REAL8)1.0,0.0);}
		/* Originally created for injecting into DARM-ERR, so transfer function was needed.  
		But since we are injecting into h(t), the transfer function from h(t) to h(t) is 1.*/

		//bufferN = (UINT4) (bufferLength*InjSampleRate);
		memcpy(&bufferStart,&thisData->timeData->epoch,sizeof(LIGOTimeGPS));
		XLALGPSAdd(&bufferStart,(REAL8) thisData->timeData->data->length * thisData->timeData->deltaT);
		XLALGPSAdd(&bufferStart,-bufferLength);
		
		//injectionBuffer=(REAL4TimeSeries *)XLALCreateREAL4TimeSeries(thisData->detector->frDetector.prefix, &bufferStart, 0.0, 1.0/InjSampleRate, &lalADCCountUnit, bufferN);
		char series_name[256];
    sprintf(series_name,"%s:injection",thisData->name);
    REAL8TimeSeries *inj8Wave=(REAL8TimeSeries *)XLALCreateREAL8TimeSeries(series_name,
                                                                           &thisData->timeData->epoch,
                                                                           0.0,
                                                                           thisData->timeData->deltaT,
                                                                           //&lalDimensionlessUnit,
                                                                           &lalStrainUnit,
                                                                           thisData->timeData->data->length);
		if(!inj8Wave) XLAL_ERROR_VOID(XLAL_EFUNC);
		/* This marks the sample in which the real segment starts, within the buffer */
		//for(i=0;i<injectionBuffer->data->length;i++) injectionBuffer->data->data[i]=0.0;
		for(i=0;i<inj8Wave->data->length;i++) inj8Wave->data->data[i]=0.0;
		//INT4 realStartSample=(INT4)((thisData->timeData->epoch.gpsSeconds - injectionBuffer->epoch.gpsSeconds)/thisData->timeData->deltaT);
		//realStartSample+=(INT4)((thisData->timeData->epoch.gpsNanoSeconds - injectionBuffer->epoch.gpsNanoSeconds)*1e-9/thisData->timeData->deltaT);

		
   
   
      //REAL8TimeSeries *hplus=NULL;  /**< +-polarization waveform */
      //REAL8TimeSeries *hcross=NULL; /**< x-polarization waveform */
      //REAL8TimeSeries       *signalvecREAL8=NULL;
      REAL8 Q, centre_frequency;
      
      Q=injEvent->q;
      centre_frequency=injEvent->frequency;
    /* Check that 2*width_gauss_envelope is inside frequency range */
    if ((centre_frequency + 3.0*centre_frequency/Q)>=  1.0/(2.0*thisData->timeData->deltaT))
    {
    fprintf(stdout, "WARNING: Your sample rate is too low to ensure a good analysis for a SG centered at f0=%lf and with Q=%lf. Consider increasing it to more than %lf. Exiting...\n",centre_frequency,Q,2.0*(centre_frequency + 3.0*centre_frequency/Q));
    //exit(1);
    }
    if ((centre_frequency -3.0*centre_frequency/Q)<=  thisData->fLow)
    {
    fprintf(stdout, "WARNING: The low frenquency tail of your SG centered at f0=%lf and with Q=%lf will lie below the low frequency cutoff. Whit your current settings and parameters the minimum f0 you can analyze without cuts is %lf.\n Continuing... \n",centre_frequency,Q,centre_frequency -3.0*centre_frequency/Q);
    //exit(1);
    }
    XLALBurstInjectSignals(inj8Wave,injEvent,tslide,NULL);
     
//      XLALResampleREAL8TimeSeries(hplus,thisData->timeData->deltaT);
    
   
   /*   for(i=0;i<signalvecREAL8->data->length;i++){
        if(isnan(signalvecREAL8->data->data[i])) {signalvecREAL8->data->data[i]=0.0;printf("isnan %d\n",i);}
      }
      
      if(signalvecREAL8->data->length > thisData->timeData->data->length-(UINT4)ceil((2.0*padding)/thisData->timeData->deltaT)){
        fprintf(stderr, "WARNING: waveform length = %u is longer than thisData->timeData->data->length = %d minus the window width = %d (total of %d points available).\n", signalvecREAL8->data->length, thisData->timeData->data->length, (INT4)ceil((2.0*padding)/thisData->timeData->deltaT) , thisData->timeData->data->length-(INT4)ceil((2.0*padding)/thisData->timeData->deltaT));
        fprintf(stderr, "The waveform injected is %f seconds long. Consider increasing the %f seconds segment length (--seglen) to be greater than %f. (in %s, line %d)\n",signalvecREAL8->data->length * thisData->timeData->deltaT , thisData->timeData->data->length * thisData->timeData->deltaT, signalvecREAL8->data->length * thisData->timeData->deltaT + 2.0*padding , __FILE__, __LINE__);
      }
*/
     
    
    
    //XLALDestroyREAL4TimeSeries(injectionBuffer);
    
    injF=(COMPLEX16FrequencySeries *)XLALCreateCOMPLEX16FrequencySeries("injF",
										&thisData->timeData->epoch,
										0.0,
										thisData->freqData->deltaF,
										&lalDimensionlessUnit,
										thisData->freqData->data->length);
    if(!injF) {
      XLALPrintError("Unable to allocate memory for injection buffer\n");
      XLAL_ERROR_VOID(XLAL_EFUNC);
    }
    
        /* Window the data */
    REAL4 WinNorm = sqrt(thisData->window->sumofsquares/thisData->window->data->length);
        for(j=0;j<inj8Wave->data->length;j++){
           inj8Wave->data->data[j]*=thisData->window->data->data[j]; /* /WinNorm; */ /* Window normalisation applied only in freq domain */
         }
       
    XLALREAL8TimeFreqFFT(injF,inj8Wave,thisData->timeToFreqFFTPlan);
    /*for(j=0;j<injF->data->length;j++) printf("%lf\n",injF->data->data[j].re);*/
    
    if(thisData->oneSidedNoisePowerSpectrum){
        UINT4 upper=thisData->fHigh/injF->deltaF;
	for(SNR=0.0,j=thisData->fLow/injF->deltaF;j<upper;j++){
	  SNR+=pow(creal(injF->data->data[j]),2.0)/thisData->oneSidedNoisePowerSpectrum->data->data[j];
	  SNR+=pow(cimag(injF->data->data[j]),2.0)/thisData->oneSidedNoisePowerSpectrum->data->data[j];
	}
        SNR*=4.0*injF->deltaF;
    }
    thisData->SNR=sqrt(SNR);
    NetworkSNR+=SNR;
    
    //if (thisData->SNR > previous_snr) {best_ifo_snr=highest_snr_index;    previous_snr=thisData->SNR;}
    //highest_snr_index++;

    if (!(BurstSNRpath==NULL)){ /* If the user provided a path with --snrpath store a file with injected SNRs */
    REAL8 trigtime=injEvent->time_geocent_gps.gpsSeconds + 1e-9*injEvent->time_geocent_gps.gpsNanoSeconds;
    PrintBurstSNRsToFile(IFOdata , trigtime);
    }
    /* Actually inject the waveform */
    for(j=0;j<inj8Wave->data->length;j++) thisData->timeData->data->data[j]+=inj8Wave->data->data[j];
    fprintf(stdout,"Injected SNR in detector %s = %.1f\n",thisData->name,thisData->SNR);
    char filename[256];
    sprintf(filename,"%s_timeInjection.dat",thisData->name);
    FILE* file=fopen(filename, "w");
    for(j=0;j<inj8Wave->data->length;j++){   
      fprintf(file, "%.6f\t%lg\n", XLALGPSGetREAL8(&thisData->timeData->epoch) + thisData->timeData->deltaT*j, inj8Wave->data->data[j]);
    }
    fclose(file);
    sprintf(filename,"%s_freqInjection.dat",thisData->name);
    file=fopen(filename, "w");
    for(j=0;j<injF->data->length;j++){   
      thisData->freqData->data->data[j]+=crect(creal(injF->data->data[j])/WinNorm,cimag(injF->data->data[j])/WinNorm);
      fprintf(file, "%lg %lg \t %lg\n", thisData->freqData->deltaF*j, creal(injF->data->data[j]), cimag(injF->data->data[j]));
    }
    fclose(file);
    
    XLALDestroyREAL8TimeSeries(inj8Wave);
    XLALDestroyCOMPLEX16FrequencySeries(injF);
    thisData=thisData->next;
    }
    NetworkSNR=sqrt(NetworkSNR);
    fprintf(stdout,"Network SNR of event %d = %.1f\n",event,NetworkSNR);
     thisData=IFOdata;
    
    
    return;
}

/** Fill the variables passed in vars with the parameters of the injection passed in event
    will over-write and destroy any existing parameters. Param vary type will be fixed */
void LALInferenceBurstInjectionToVariables(SimBurst *theEventTable, LALInferenceVariables *vars)
{
    if(!vars) {
	XLALPrintError("Encountered NULL variables pointer");
   	XLAL_ERROR_VOID(XLAL_EINVAL);
	}
    /* Destroy existing parameters */
    if(vars->head!=NULL) LALInferenceClearVariables(vars);
    REAL8 q = theEventTable->q;
    REAL8 psi = theEventTable->psi;
    REAL8 injGPSTime = XLALGPSGetREAL8(&(theEventTable->time_geocent_gps));
    REAL8 hrss = theEventTable->hrss;
    REAL8 loghrss=log(hrss);
    REAL8 f0 = theEventTable->frequency;
    REAL8 pol_angle = theEventTable->pol_ellipse_angle;
    REAL8 eccentricity = theEventTable->pol_ellipse_e;
    REAL8 duration=theEventTable->duration;
    REAL8 dec = theEventTable->dec;
    REAL8 ra = theEventTable->ra;
    
    LALInferenceAddVariable(vars, "Q", &q, LALINFERENCE_REAL8_t, LALINFERENCE_PARAM_FIXED);
    LALInferenceAddVariable(vars, "frequency", &f0, LALINFERENCE_REAL8_t, LALINFERENCE_PARAM_FIXED);
    LALInferenceAddVariable(vars, "time", &injGPSTime, LALINFERENCE_REAL8_t, LALINFERENCE_PARAM_FIXED);
    LALInferenceAddVariable(vars, "hrss", &hrss, LALINFERENCE_REAL8_t, LALINFERENCE_PARAM_FIXED);
    LALInferenceAddVariable(vars, "polar_angle", &pol_angle, LALINFERENCE_REAL8_t, LALINFERENCE_PARAM_FIXED);
    LALInferenceAddVariable(vars, "eccentricity", &eccentricity, LALINFERENCE_REAL8_t, LALINFERENCE_PARAM_FIXED);
    LALInferenceAddVariable(vars, "polarisation", &(psi), LALINFERENCE_REAL8_t, LALINFERENCE_PARAM_FIXED);
    LALInferenceAddVariable(vars, "declination", &dec, LALINFERENCE_REAL8_t, LALINFERENCE_PARAM_FIXED);
    LALInferenceAddVariable(vars, "rightascension", &ra, LALINFERENCE_REAL8_t, LALINFERENCE_PARAM_FIXED);
    LALInferenceAddVariable(vars, "loghrss", &loghrss, LALINFERENCE_REAL8_t, LALINFERENCE_PARAM_FIXED);
    LALInferenceAddVariable(vars,"duration",&duration,LALINFERENCE_REAL8_t, LALINFERENCE_PARAM_FIXED);
}

static void PrintBurstSNRsToFile(LALInferenceIFOData *IFOdata ,REAL8 injtime){
    char SnrName[300];
    char ListOfIFOs[10]="";
    REAL8 NetSNR=0.0;
    
    LALInferenceIFOData *thisData=IFOdata;
    int nIFO=0;

    while(thisData){
         sprintf(ListOfIFOs,"%s%s",ListOfIFOs,thisData->name);
         thisData=thisData->next;
	nIFO++;
        }
    
    sprintf(SnrName,"%s/snr_%s_%10.3f.dat",BurstSNRpath,ListOfIFOs,injtime);
    FILE * snrout = fopen(SnrName,"w");
    if(!snrout){
	fprintf(stderr,"Unable to open the path %s for writing SNR files\n",BurstSNRpath);
	exit(1);
    }
    
    thisData=IFOdata; // restart from the first IFO
    while(thisData){
        fprintf(snrout,"%s:\t %4.2f\n",thisData->name,thisData->SNR);
        NetSNR+=(thisData->SNR*thisData->SNR);
        thisData=thisData->next;
    }		
    if (nIFO>1){  fprintf(snrout,"Network:\t");
    fprintf(snrout,"%4.2f\n",sqrt(NetSNR));}
    fclose(snrout);
}

void InjectSineGaussianFD(LALInferenceIFOData *IFOdata, SimBurst *inj_table, ProcessParamsTable *commandLine)
///*-------------- Inject in Frequency domain -----------------*/
{
    
    fprintf(stdout,"Injecting SineGaussian in the frequency domain\n");
    fprintf(stdout,"REMEMBER!!!!!! I HARD CODED h_plus=0 in LALSimBurst.c. Remember to restore  it .\n");
    /* Inject a gravitational wave into the data in the frequency domain */ 
    LALStatus status;
    memset(&status,0,sizeof(LALStatus));
    (void) commandLine;
    LALInferenceVariables *modelParams=NULL;
    LALInferenceIFOData * tmpdata=IFOdata;
    REAL8 Q =0.0;
    REAL8 hrss,loghrss = 0.0;
    REAL8 centre_frequency= 0.0;
    REAL8 polar_angle=0.0;
    REAL8 eccentricity=0.0;
    REAL8 latitude=0.0;
    REAL8 polarization=0.0;
    REAL8 injtime=0.0;
    REAL8 longitude;
	//LALInferenceIFOData *thisData=NULL;
    tmpdata->modelParams=XLALCalloc(1,sizeof(LALInferenceVariables));
    modelParams=tmpdata->modelParams;
    memset(modelParams,0,sizeof(LALInferenceVariables));

    Q=inj_table->q;
    centre_frequency=inj_table->frequency;
    hrss=inj_table->hrss;
    polarization=inj_table->psi;
    polar_angle=inj_table->pol_ellipse_angle;
    eccentricity=inj_table->pol_ellipse_e; // salvo
    loghrss=log(hrss);
    injtime=inj_table->time_geocent_gps.gpsSeconds + 1e-9*inj_table->time_geocent_gps.gpsNanoSeconds;
    latitude=inj_table->dec;
    longitude=inj_table->ra;
    LALInferenceAddVariable(tmpdata->modelParams, "loghrss",&loghrss,LALINFERENCE_REAL8_t,LALINFERENCE_PARAM_LINEAR);
    LALInferenceAddVariable(tmpdata->modelParams, "Q",&Q,LALINFERENCE_REAL8_t, LALINFERENCE_PARAM_LINEAR);  
    LALInferenceAddVariable(tmpdata->modelParams, "rightascension",&longitude,LALINFERENCE_REAL8_t, LALINFERENCE_PARAM_CIRCULAR);  
    LALInferenceAddVariable(tmpdata->modelParams, "declination",&latitude,LALINFERENCE_REAL8_t, LALINFERENCE_PARAM_CIRCULAR);  
    LALInferenceAddVariable(tmpdata->modelParams, "polarisation",&polarization,LALINFERENCE_REAL8_t, LALINFERENCE_PARAM_CIRCULAR);  
    LALInferenceAddVariable(tmpdata->modelParams, "time",&injtime,LALINFERENCE_REAL8_t, LALINFERENCE_PARAM_LINEAR);
    LALInferenceAddVariable(tmpdata->modelParams, "polar_angle",&polar_angle,LALINFERENCE_REAL8_t, LALINFERENCE_PARAM_CIRCULAR);
    LALInferenceAddVariable(tmpdata->modelParams, "eccentricity",&eccentricity,LALINFERENCE_REAL8_t,LALINFERENCE_PARAM_LINEAR);
    LALInferenceAddVariable(tmpdata->modelParams, "frequency",&centre_frequency,LALINFERENCE_REAL8_t,LALINFERENCE_PARAM_LINEAR);
    
      
    COMPLEX16FrequencySeries *freqModelhCross=NULL;
   freqModelhCross=XLALCreateCOMPLEX16FrequencySeries("freqDatahC",&(tmpdata->timeData->epoch),0.0,tmpdata->freqData->deltaF,&lalDimensionlessUnit,tmpdata->freqData->data->length);
    COMPLEX16FrequencySeries *freqModelhPlus=NULL;
    freqModelhPlus=XLALCreateCOMPLEX16FrequencySeries("freqDatahP",&(tmpdata->timeData->epoch),0.0,tmpdata->freqData->deltaF,&lalDimensionlessUnit,tmpdata->freqData->data->length);
    COMPLEX16FrequencySeries *freqTemplate=NULL;
    freqTemplate=XLALCreateCOMPLEX16FrequencySeries("freqTemplate",&(tmpdata->timeData->epoch),0.0,tmpdata->freqData->deltaF,&lalDimensionlessUnit,tmpdata->freqData->data->length);
    tmpdata->freqModelhPlus=freqModelhPlus;
    tmpdata->freqModelhCross=freqModelhCross;
    
      tmpdata->modelDomain = LAL_SIM_DOMAIN_FREQUENCY;
    LALInferenceTemplateSineGaussianF(tmpdata);
    
     
    LALInferenceVariables *currentParams=IFOdata->modelParams;
       
    double Fplus, Fcross;
    double FplusScaled, FcrossScaled;
    REAL8 plainTemplateReal, plainTemplateImag;
    REAL8 templateReal, templateImag;
    int i, lower, upper;
    LALInferenceIFOData *dataPtr;
    double ra, dec, psi, gmst;
    LIGOTimeGPS GPSlal;
    double chisquared;
    double timedelay;  /* time delay b/w iterferometer & geocenter w.r.t. sky location */
    double timeshift;  /* time shift (not necessarily same as above)                   */
    double deltaT, deltaF, twopit, f, re, im;
    UINT4 j=0;
    REAL8 temp=0.0;
    REAL8 NetSNR=0.0;
    LALInferenceVariables intrinsicParams;

    /* determine source's sky location & orientation parameters: */
    ra        = *(REAL8*) LALInferenceGetVariable(currentParams, "rightascension"); /* radian      */
    dec       = *(REAL8*) LALInferenceGetVariable(currentParams, "declination");    /* radian      */
    psi       = *(REAL8*) LALInferenceGetVariable(currentParams, "polarisation");   /* radian      */
    /* figure out GMST: */
    //XLALGPSSetREAL8(&GPSlal, GPSdouble); //This is what used in the likelihood. It seems off by two seconds (should not make a big difference as the antenna patterns would not change much in such a short interval)
    XLALGPSSetREAL8(&GPSlal, injtime);
    //UandA.units    = MST_RAD;
    //UandA.accuracy = LALLEAPSEC_LOOSE;
    //LALGPStoGMST1(&status, &gmst, &GPSlal, &UandA);
    gmst=XLALGreenwichMeanSiderealTime(&GPSlal);
    intrinsicParams.head      = NULL;
    intrinsicParams.dimension = 0;
    LALInferenceCopyVariables(currentParams, &intrinsicParams);
    LALInferenceRemoveVariable(&intrinsicParams, "rightascension");
    LALInferenceRemoveVariable(&intrinsicParams, "declination");
    LALInferenceRemoveVariable(&intrinsicParams, "polarisation");
    LALInferenceRemoveVariable(&intrinsicParams, "time");
    
    /* loop over data (different interferometers): */
    dataPtr = IFOdata;
    for (j=0; j<freqTemplate->data->length; ++j){
          freqTemplate->data->data[j]=0.0+j*0.0;
      }
      
    while (dataPtr != NULL) {
       
        if (IFOdata->modelDomain == LAL_SIM_DOMAIN_TIME) {
      printf("There is a problem. You seem to be using a time domain model into the frequency domain injection function!. Exiting....\n"); 
        exit(1);
      }
        
      /*-- WF to inject is now in dataPtr->freqModelhPlus and dataPtr->freqModelhCross. --*/
      /* determine beam pattern response (F_plus and F_cross) for given Ifo: */
      XLALComputeDetAMResponse(&Fplus, &Fcross,
                               (const REAL4(*)[3]) dataPtr->detector->response,
             ra, dec, psi, gmst);
      /* signal arrival time (relative to geocenter); */
      timedelay = XLALTimeDelayFromEarthCenter(dataPtr->detector->location,
                                               ra, dec, &GPSlal);
      dataPtr->injtime=injtime;
      /* (negative timedelay means signal arrives earlier at Ifo than at geocenter, etc.) */
      /* amount by which to time-shift template (not necessarily same as above "timedelay"): */
      timeshift =  (injtime - (*(REAL8*) LALInferenceGetVariable(IFOdata->modelParams, "time"))) + timedelay;
      twopit    = LAL_TWOPI * (timeshift);
      /* Restore hrss (template has been calculated for hrss=1) effect in Fplus/Fcross: */
      FplusScaled  = Fplus *hrss ;
      FcrossScaled = Fcross*hrss;
      dataPtr->fPlus = FplusScaled;
      dataPtr->fCross = FcrossScaled;
      dataPtr->timeshift = timeshift;
      
      char InjFileName[50];
      sprintf(InjFileName,"FD_injection_%s.dat",dataPtr->name);
      FILE *outInj=fopen(InjFileName,"w");
      REAL8 starttime=IFOdata->epoch.gpsSeconds+1.0e-9 *IFOdata->epoch.gpsNanoSeconds;
      //printf("time + shift= %lf \n", time+(injtime - (*(REAL8*) LALInferenceGetVariable(IFOdata->modelParams, "time"))));
      //printf("IFOdata time %lf, Time data time %lf\n",time,IFOdata->timeData->epoch.gpsSeconds+1.0e-9 *IFOdata->timeData->epoch.gpsNanoSeconds);
       char TInjFileName[50];
      sprintf(TInjFileName,"TD_injection_%s.dat",dataPtr->name);
      FILE *outTInj=fopen(TInjFileName,"w");
      
       /* determine frequency range & loop over frequency bins: */
      deltaT = dataPtr->timeData->deltaT;
      deltaF = 1.0 / (((double)dataPtr->timeData->data->length) * deltaT);
      REAL8 time_env_2sigma= Q  / (LAL_TWOPI * centre_frequency);
      if (2.0*time_env_2sigma>1./deltaF)
          fprintf(stdout,"WARNING: 95 of the Gaussian envelop (%lf) is larger than seglen (%lf)!!\n",2.0*time_env_2sigma,1./deltaF);
      lower = (UINT4)ceil(dataPtr->fLow / deltaF);
      upper = (UINT4)floor(dataPtr->fHigh / deltaF);
       chisquared = 0.0;
      for (i=lower; i<=upper; ++i){
        /* derive template (involving location/orientation parameters) from given plus/cross waveforms: */
        
        plainTemplateReal =FplusScaled * creal(IFOdata->freqModelhPlus->data->data[i]) 
                            + FcrossScaled * creal(IFOdata->freqModelhCross->data->data[i]); //SALVO
        plainTemplateImag = FplusScaled * cimag(IFOdata->freqModelhPlus->data->data[i])  
                            + FcrossScaled * cimag(IFOdata->freqModelhCross->data->data[i]);
     
        f = ((double) i) * deltaF;
        /* real & imag parts of  exp(-2*pi*i*f*deltaT): */
        re = cos(twopit * f);
        im = - sin(twopit * f);
        templateReal = (plainTemplateReal*re - plainTemplateImag*im);
        templateImag = (plainTemplateReal*im + plainTemplateImag*re);
        freqTemplate->data->data[i]=crect(templateReal,templateImag);
        dataPtr->freqData->data->data[i]+=crect(templateReal,templateImag);
        temp = ((2.0/( deltaT*(double) dataPtr->timeData->data->length) * (templateReal*templateReal+templateImag*templateImag)) / dataPtr->oneSidedNoisePowerSpectrum->data->data[i]);
        chisquared  += temp;
        fprintf(outInj,"%lf %10.10e %10.10e\n",f,templateReal,templateImag);
      }
      printf("Injected SNR %.1f in IFO %s\n",sqrt(2.0*chisquared),dataPtr->name);
      NetSNR+=2.0*chisquared;
      dataPtr->SNR=sqrt(2.0*chisquared);
       
      dataPtr = dataPtr->next;
      
       fclose(outInj);
    

      /* Calculate IFFT and print it to file */
      REAL8TimeSeries* timeData=NULL;
      timeData=(REAL8TimeSeries *) XLALCreateREAL8TimeSeries("name",&IFOdata->timeData->epoch,0.0,(REAL8)IFOdata->timeData->deltaT,&lalDimensionlessUnit,(size_t)IFOdata->timeData->data->length);
       
       XLALREAL8FreqTimeFFT(timeData,freqTemplate,IFOdata->freqToTimeFFTPlan);
       for (j=0;j<timeData->data->length;j++){
           
           fprintf(outTInj,"%10.10e %10.10e \n",starttime+j*deltaT,timeData->data->data[j]);
           
           
           }
       fclose(outTInj);
      //if(!timeData) XLAL_ERROR_NULL(XLAL_EFUNC);
      
    
    
    
    }
    
      LALInferenceClearVariables(&intrinsicParams);
      printf("injected Network SNR %.1f \n",sqrt(NetSNR)); 
    NetSNR=sqrt(NetSNR); 

      if (!(BurstSNRpath==NULL)){ /* If the user provided a path with --snrpath store a file with injected SNRs */
          PrintBurstSNRsToFile(IFOdata ,injtime);
      }
          
    XLALDestroyCOMPLEX16FrequencySeries(freqModelhCross);
      XLALDestroyCOMPLEX16FrequencySeries(freqModelhPlus);

}