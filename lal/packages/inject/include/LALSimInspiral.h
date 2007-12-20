#include <lal/LALDatatypes.h>

/**
 * Computes the (s)Y(l,m) spin-weighted spherical harmonic.
 *
 * Implements Equations (II.9)-(II.13) of
 * D. A. Brown, S. Fairhurst, B. Krishnan, R. A. Mercer, R. K. Kopparapu,
 * L. Santamaria, and J. T. Whelan,
 * "Data formats for numerical relativity waves",
 * arXiv:0709.0093v1 (2007).
 *
 * Currently only supports s=-2, l=2 modes.
 */
COMPLEX16 XLALSpinWeightedSphericalHarmonic(REAL8 theta, REAL8 phi, int s, int l, int m);

int XLALSimAddMode(REAL8TimeSeries *hplus, REAL8TimeSeries *hcross, COMPLEX16TimeSeries *hmode, REAL8 theta, REAL8 phi, int l, int m, int sym);

/**
 * Computes the rate of increase of the orbital frequency for a post-Newtonian
 * inspiral.  This function returns dx/dt rather than the true angular
 * acceleration.
 *
 * Implements Equation (6) of
 * Yi Pan, Alessandra Buonanno, Yanbei Chen, and Michele Vallisneri,
 * "A physical template family for gravitational waves from precessing
 * binaries of spinning compact objects: Application to single-spin binaries"
 * arXiv:gr-qc/0310034v3 (2007).
 *
 * Note: this equation is actually dx/dt rather than (domega/dt)/(omega)^2
 * so the leading coefficient is different.  Also, this function applies
 * for non-spinning objects.
 *
 * Compare the overall coefficient, with nu=1/4, to Equation (45) of
 * Michael Boyle, Duncan A. Brown, Lawrence E. Kidder, Abdul H. Mroue, 
 * Harald P. Pfeiﬀer, Mark A. Scheel, Gregory B. Cook, and Saul A. Teukolsky 
 * "High-accuracy comparison of numerical relativity simulations with
 * post-Newtonian expansions"
 * arXiv:0710.0158v1 (2007).
 */
REAL8 XLALSimInspiralPNAngularAcceleration(REAL8 x, REAL8 m1, REAL8 m2, int O);

/**
 * Computes the orbital angular velocity from the quantity x.
 * This is from the definition of x.
 *
 * Implements Equation (46) of
 * Michael Boyle, Duncan A. Brown, Lawrence E. Kidder, Abdul H. Mroue, 
 * Harald P. Pfeiﬀer, Mark A. Scheel, Gregory B. Cook, and Saul A. Teukolsky 
 * "High-accuracy comparison of numerical relativity simulations with
 * post-Newtonian expansions"
 * arXiv:0710.0158v1 (2007).
 */
REAL8 XLALSimInspiralPNAngularVelocity(REAL8 x, REAL8 m1, REAL8 m2);

/**
 * Computes the orbital energy at a fixed frequency and pN order.
 *
 * Implements Equation (152) of
 * Luc Blanchet,
 * "Gravitational Radiation from Post-Newtonian Sources and Inspiralling
 * Compact Binaries",
 * http://www.livingreviews.org/lrr-2006-4/index.html
 *
 * This is the same as Equation (10) (where the spin of the objects
 * is zero) of:
 * Yi Pan, Alessandra Buonanno, Yanbei Chen, and Michele Vallisneri,
 * "A physical template family for gravitational waves from precessing
 * binaries of spinning compact objects: Application to single-spin binaries"
 * arXiv:gr-qc/0310034v3 (2007). 
 * Note: this equation is actually dx/dt rather than (domega/dt)/(omega)^2
 * so the leading coefficient is different.
 */
REAL8 XLALSimInspiralPNEnergy(REAL8 x, REAL8 m1, REAL8 m2, int O);

int XLALSimInspiralPNEvolveOrbitTaylorT4(REAL8TimeSeries **x, REAL8TimeSeries **phi, LIGOTimeGPS *tc, REAL8 deltaT, REAL8 m1, REAL8 m2, REAL8 fmin, int O);

/**
 * Computes h(2,2) mode of spherical harmonic decomposition of
 * the post-Newtonian inspiral waveform.
 *
 * Implements Equation (79) of:
 * Lawrence E. Kidder, "Using Full Information When Computing Modes of
 * Post-Newtonian Waveforms From Inspiralling Compact Binaries in Circular
 * Orbit", arXiv:0710.0614v1 [gr-qc].
 */
COMPLEX16 XLALSimInspiralPNMode22(REAL8 x, REAL8 phi, REAL8 m1, REAL8 m2, REAL8 r, int O);

COMPLEX16TimeSeries *XLALCreateSimInspiralPNModeCOMPLEX16TimeSeries(REAL8TimeSeries *x, REAL8TimeSeries *phi, REAL8 m1, REAL8 m2, REAL8 r, int O, int l, int m);

int XLALSimInspiralPN(REAL8TimeSeries **hplus, REAL8TimeSeries **hcross, LIGOTimeGPS *tc, REAL8 deltaT, REAL8 m1, REAL8 m2, REAL8 fmin, REAL8 r, REAL8 i, int O);

int XLALSimInspiralPNRestricted(REAL8TimeSeries **hplus, REAL8TimeSeries **hcross, LIGOTimeGPS *tc, REAL8 deltaT, REAL8 m1, REAL8 m2, REAL8 fmin, REAL8 r, REAL8 i, int O);
