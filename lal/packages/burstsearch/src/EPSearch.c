/*
 * $Id$
 *
 * Copyright (C) 2007  Brady, P. and Kipp Cannon
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
 * Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */


#include <math.h>
#include <stdio.h>
#include <lal/LALAtomicDatatypes.h>
#include <lal/BandPassTimeSeries.h>
#include <lal/EPSearch.h>
#include <lal/FrequencySeries.h>
#include <lal/LALDatatypes.h>
#include <lal/LALRCSID.h>
#include <lal/LALStatusMacros.h>
#include <lal/LALStdlib.h>
#include <lal/LIGOMetadataTables.h>
#include <lal/RealFFT.h>
#include <lal/ResampleTimeSeries.h>
#include <lal/TFTransform.h>
#include <lal/TimeFreqFFT.h>
#include <lal/TimeSeries.h>
#include <lal/Units.h>
#include <lal/XLALError.h>


NRCSID(EPSEARCHC, "$Id$");


/*
 * Delete a SnglBurstTable linked list
 */


static void XLALDestroySnglBurstTable(SnglBurstTable *head)
{
	SnglBurstTable *event;

	while(head) {
		event = head;
		head = head->next;
		XLALFree(event);
	}
}


/*
 * Generate a linked list of burst events from a time series.
 */


SnglBurstTable *XLALEPSearch(
	struct XLALEPSearchDiagnostics *diagnostics,
	const REAL8TimeSeries *tseries,
	REAL8Window *window,
	REAL8 flow,
	REAL8 bandwidth,
	REAL8 confidence_threshold,
	REAL8 fractional_stride,
	REAL8 maxTileBandwidth,
	REAL8 maxTileDuration
)
{ 
	static const char func[] = "XLALEPSearch";
	SnglBurstTable *head = NULL;
	int errorcode = 0;
	int start_sample;
	COMPLEX16FrequencySeries *fseries = NULL;
	REAL8FFTPlan *fplan;
	REAL8FFTPlan *rplan;
	REAL8FrequencySeries *psd;
	REAL8TimeSeries *cuttseries;
	REAL8TimeFrequencyPlane *plane;
	struct ExcessPowerTemplateBank *template_bank = NULL;

	/*
	 * Construct forward and reverse FFT plans, storage for the PSD,
	 * the time-frequency plane, and a tiling.  Note that the flat part
	 * of the Tukey window needs to match the locations of the tiles as
	 * specified by the tiling_start parameter of XLALCreateTFPlane.
	 */

	fplan = XLALCreateForwardREAL8FFTPlan(window->data->length, 1);
	rplan = XLALCreateReverseREAL8FFTPlan(window->data->length, 1);
	psd = XLALCreateREAL8FrequencySeries("PSD", &tseries->epoch, 0, 0, &lalDimensionlessUnit, window->data->length / 2 + 1);
	plane = XLALCreateTFPlane(window->data->length, tseries->deltaT, flow, bandwidth, fractional_stride, maxTileBandwidth, maxTileDuration);
	if(!fplan || !rplan || !psd || !plane) {
		errorcode = XLAL_EFUNC;
		goto error;
	}

#if 0
	/* diagnostic code to replace the input time series with stationary
	 * Gaussian white noise.  the normalization is such that it yields
	 * unit variance frequency components without a call to the
	 * whitening function. */
	{
	unsigned i;
	static RandomParams *rparams = NULL;
	if(!rparams)
		rparams = XLALCreateRandomParams(0);
	XLALNormalDeviates(tseries->data, rparams);
	for(i = 0; i < tseries->data->length; i++)
		tseries->data->data[i] *= sqrt(0.5 / tseries->deltaT);
	}
#endif
#if 0
	/* diagnostic code to disable the tapering window */
	{
	unsigned i = plane->window->data->length;
	XLALDestroyREAL8Window(plane->window);
	plane->window = XLALCreateRectangularREAL8Window(i);
	}
#endif

	/*
	 * Compute the average spectrum.
	 *
	 * FIXME: is using windowShift here correct?  we have to, otherwise
	 * the time series' lengths are inconsistent
	 */

	if(XLALREAL8AverageSpectrumMedian(psd, tseries, plane->window->data->length, plane->window_shift, plane->window, fplan) < 0) {
		errorcode = XLAL_EFUNC;
		goto error;
	}

	if(diagnostics)
		diagnostics->XLALWriteLIGOLwXMLArrayREAL8FrequencySeries(diagnostics->LIGOLwXMLStream, NULL, psd);

	/*
	 * Loop over data applying excess power method.
	 */

	for(start_sample = 0; start_sample + plane->window->data->length <= tseries->data->length; start_sample += plane->window_shift) {
		/*
		 * Extract a window-length of data from the time series,
		 * compute its DFT, then free it.
		 */

		cuttseries = XLALCutREAL8TimeSeries(tseries, start_sample, plane->window->data->length);
		if(!cuttseries) {
			errorcode = XLAL_EFUNC;
			goto error;
		}
		XLALPrintInfo("%s(): analyzing %u samples (%.9lf s) at offset %u (%.9lf s) from epoch %d.%09u s\n", func, cuttseries->data->length, cuttseries->data->length * cuttseries->deltaT, start_sample, start_sample * cuttseries->deltaT, tseries->epoch.gpsSeconds, tseries->epoch.gpsNanoSeconds);
		if(diagnostics)
			diagnostics->XLALWriteLIGOLwXMLArrayREAL8TimeSeries(diagnostics->LIGOLwXMLStream, NULL, cuttseries);

		XLALPrintInfo("%s(): computing the Fourier transform\n", func);
		fseries = XLALWindowedREAL8ForwardFFT(cuttseries, plane->window, fplan);
		XLALDestroyREAL8TimeSeries(cuttseries);
		if(!fseries) {
			errorcode = XLAL_EFUNC;
			goto error;
		}

		/*
		 * Normalize the frequency series to the average PSD.
		 */

#if 1
		XLALPrintInfo("%s(): normalizing to the average spectrum\n", func);
		if(!XLALWhitenCOMPLEX16FrequencySeries(fseries, psd, plane->flow, plane->flow + plane->channels * plane->deltaF)) {
			errorcode = XLAL_EFUNC;
			goto error;
		}
		if(diagnostics)
			diagnostics->XLALWriteLIGOLwXMLArrayCOMPLEX16FrequencySeries(diagnostics->LIGOLwXMLStream, "whitened", fseries);
#endif

		/*
		 * Construct the time-frequency plane's channel filters.
		 *
		 * FIXME:  this could be moved out of the loop for a
		 * performance boost.
		 */

		XLALPrintInfo("%s(): constructing channel filters ...\n", func);
		if(XLALTFPlaneMakeChannelFilters(fseries, plane, psd)) {
			errorcode = XLAL_EFUNC;
			goto error;
		}

		/*
		 * Compute the time-frequency plane from the frequency
		 * series.
		 */

#if 1
		XLALPrintInfo("%s(): projecting data onto time-frequency plane\n", func);
		if(XLALFreqSeriesToTFPlane(plane, fseries, rplan)) {
			errorcode = XLAL_EFUNC;
			goto error;
		}
		XLALDestroyCOMPLEX16FrequencySeries(fseries);
		fseries = NULL;

		/*
		 * Compute the excess power for each time-frequency tile
		 * using the data in the time-frequency plane, and add
		 * those whose confidence is above threshold to the trigger
		 * list.  Note that because it is possible for there to be
		 * 0 triggers found, we can't check for errors by testing
		 * for head == NULL.
		 */

		XLALPrintInfo("%s(): computing the excess power for each tile\n", func);
		XLALClearErrno();
		head = XLALComputeExcessPower(plane, head, confidence_threshold);
		if(xlalErrno) {
			errorcode = XLAL_EFUNC;
			goto error;
		}
#else
		/* FIXME: decide what to do about this */

		/*
		 * Construct the template bank
		 *
		 * FIXME:  this could be moved out of the loop for a
		 * performance boost.
		 */

		XLALPrintInfo("%s(): constructing template bank\n", func);
		template_bank = XLALCreateExcessPowerTemplateBank(fseries, plane, psd);
		if(!template_bank) {
			errorcode = XLAL_EFUNC;
			goto error;
		}

		/*
		 * Project frequency series onto templates
		 */

		XLALPrintInfo("%s(): projecting frequency series onto template bank\n", func);
		XLALClearErrno();
		head = XLALExcessPowerProject(fseries, plane, template_bank, head, confidence_threshold, rplan);
		if(xlalErrno) {
			errorcode = XLAL_EFUNC;
			goto error;
		}
		XLALDestroyCOMPLEX16FrequencySeries(fseries);
		fseries = NULL;
		XLALDestroyExcessPowerTemplateBank(template_bank);
		template_bank = NULL;
#endif
	}

	/*
	 * Memory clean-up.
	 */

	XLALPrintInfo("%s(): done\n", func);

	error:
	XLALDestroyREAL8FFTPlan(fplan);
	XLALDestroyREAL8FFTPlan(rplan);
	XLALDestroyREAL8FrequencySeries(psd);
	XLALDestroyCOMPLEX16FrequencySeries(fseries);
	XLALDestroyExcessPowerTemplateBank(template_bank);
	XLALDestroyTFPlane(plane);
	if(errorcode) {
		XLALDestroySnglBurstTable(head);
		XLAL_ERROR_NULL(func, errorcode);
	}
	return(head);
}


/*
 * Condition the time series prior to analysis by the power code
 */


int XLALEPConditionData(
	REAL8TimeSeries *series,
	REAL8 flow,
	REAL8 resampledeltaT,
	INT4 corruption
)
{
	static const char func[] = "XLALEPConditionData";
	const REAL8 epsilon = 1.0e-3;

	XLALPrintInfo("%s(): conditioning %u samples (%.9f s) at GPS time %d%.09u s ...\n", func, series->data->length, series->data->length * series->deltaT, series->epoch.gpsSeconds, series->epoch.gpsNanoSeconds);

	/*
	 * High-pass filter the time series.
	 */

	if(flow > 0.0) {
		PassBandParamStruc highpassParam;
		highpassParam.nMax = 8;
		highpassParam.f2 = flow;
		highpassParam.f1 = -1.0;
		highpassParam.a2 = 0.9;
		highpassParam.a1 = -1.0;
		if(XLALButterworthREAL8TimeSeries(series, &highpassParam))
			XLAL_ERROR(func, XLAL_EFUNC);
	}

	/*
	 * Resample the time series if necessary
	 */

	if(fabs(resampledeltaT - series->deltaT) / series->deltaT >= epsilon)
		if(XLALResampleREAL8TimeSeries(series, resampledeltaT))
			XLAL_ERROR(func, XLAL_EFUNC);

	/*
	 * Chop off the ends of the time series.
	 */

	if(!XLALShrinkREAL8TimeSeries(series, corruption, series->data->length - 2 * corruption))
		XLAL_ERROR(func, XLAL_EFUNC);

	/*
	 * Done.
	 */

	XLALPrintInfo("%s(): %u samples (%.9f s) at GPS time %d.%09u s remain after conditioning\n", func, series->data->length, series->data->length * series->deltaT, series->epoch.gpsSeconds, series->epoch.gpsNanoSeconds);

	return(0);
}
