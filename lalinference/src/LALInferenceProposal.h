/* 
 *  LALInferenceProposal.h:  Bayesian Followup, jump proposals.
 *
 *  Copyright (C) 2011 Ilya Mandel, Vivien Raymond, Christian Roever,
 *  Marc van der Sluys, John Veitch, Will M. Farr
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

/**
 * \author Ilya Mandel, Vivien Raymond, Christian Roever, Marc van der Sluys, John Veitch, and Will M. Farr.
 * \date 2011
 * \file
 * \ingroup LALInference
 * \brief Jump proposals for exploring the GW signal parameter space. 
 *
 * For exploring the parameter space of GW signals, it is convenient
 * to use many different types of jumps.  For example, we supply in
 * this module proposals that change one parameter at a time, based on
 * a gaussian distribution centered at the current value with a fixed
 * width; proposals that change a combination of parameters along a
 * one-dimensional curve in parameter space; proposals that change a
 * combination of parameters along a sub-manifold of parameter space;
 * and proposals that change all parameters at once, attempting to
 * generate a jump along the directions in parameter space
 * corresponding to eigenvectors of the covariance matrix; etc.
 *
 * Good jump proposals involve a combination of moves, or
 * sub-proposals, chosen with various weights.  Having such a combined
 * jump proposal can cause difficulties with standard Metropolis
 * sampling in an MCMC, depending on how the combined proposal is
 * implemented.  The reason involves the ratio of forward to backward
 * jump probabilities that is used in the acceptance probability for a
 * jump in Metropolis sampling.  Many proposals are symmetric, having
 * equal forward and backward jump probabilities, but not all.
 *
 * If the combined proposal is implemented as a random choice among a
 * list of sub-proposals, then the proper jump probability is a
 * combination of the jump probabilities from all sub-propsals.  That
 * is, to properly compute the jump probability for such a combined
 * proposal, every sub-proposal---whether it individually is symmetric
 * or not---must be queried after each jump for its individual jump
 * probability, and these must be combined into the correct jump
 * probability for the overall proposal.  The situation becomes even
 * more complex when the sub-proposals act on sub-manifolds of
 * parameter space of differing dimensions, because then the
 * individual proposal jump probabilities involve varying numbers of
 * delta-functions that act to restrict the support of the jump
 * probability to the proper sub-manifold.  Ugh.
 *
 * We avoid this problem by implementing our combined proposal as a
 * cyclic sequence of sub-proposals.  At each MCMC step, only one of
 * the sub-proposals is used, and therefore only this proposal's jump
 * probability is relevant to the computation of the forward and
 * backward jump probabilities.  At the next step, the next proposal
 * in the cycle is called, etc.  The LALInferenceCyclicProposal()
 * function implements this procedure.  To add a number of copies of a
 * proposal function to the end of the cycle, use
 * LALInferenceAddProposalToCycle(); once all desired sub-proposals
 * are added to the cycle, call LALInferenceRandomizeProposalCycle()
 * in order to ensure that the ordering of the sub-proposals is
 * random.
 */

#ifndef LALInferenceProposal_h
#define LALInferenceProposal_h

#include <lal/LALInference.h>

#define MAX_STRLEN 512
#define ADAPTSUFFIX "adapt_sigma"
#define ACCEPTSUFFIX "accepted"
#define PROPOSEDSUFFIX "proposed"

extern const char *cycleArrayName;
extern const char *cycleArrayLengthName;
extern const char *cycleArrayCounterName;


/* Proposal Names */
extern const char *singleAdaptProposalName;
extern const char *singleProposalName;
extern const char *orbitalPhaseJumpName;
extern const char *inclinationDistanceName;
extern const char *covarianceEigenvectorJumpName;
extern const char *skyLocWanderJumpName;
extern const char *differentialEvolutionFullName;
extern const char *differentialEvolutionMassesName;
extern const char *differentialEvolutionSpinsName;
extern const char *differentialEvolutionExtrinsicName;
extern const char *drawApproxPriorName;
extern const char *skyReflectDetPlaneName;
extern const char *rotateSpinsName;
extern const char *polarizationPhaseJumpName;
extern const char *distanceQuasiGibbsProposalName;
extern const char *orbitalPhaseQuasiGibbsProposalName;
extern const char *extrinsicParamProposalName;
extern const char *KDNeighborhoodProposalName;

/** The name of the variable that will store the name of the current
    proposal function. */
extern const char *LALInferenceCurrentProposalName;

/** Adds \a weight copies of the proposal \a prop to the end of the
    proposal cycle. 

    After adding all desired sub-proposals, call
    LALInferenceRandomizeProposalCycle() to randomize the order of
    sub-proposal application. */
void
LALInferenceAddProposalToCycle(LALInferenceRunState *runState, const char *propName, LALInferenceProposalFunction *prop, UINT4 weight);

/** Randomizes the order of the proposals in the proposal cycle. */
void 
LALInferenceRandomizeProposalCycle(LALInferenceRunState *runState);

/** Proposes a jump from the next proposal in the proposal cycle.*/
void
LALInferenceCyclicProposal(LALInferenceRunState *runState, LALInferenceVariables *proposedParams);

/** Completely remove the current proposal cycle, freeing the associated memory. */
void
LALInferenceDeleteProposalCycle(LALInferenceRunState *runState);

/** A reasonable default proposal.  Uses adaptation if the --adapt
    command-line flag active. */
void LALInferenceDefaultProposal(LALInferenceRunState *runState, LALInferenceVariables *proposedParams);
void LALInferencetempProposal(LALInferenceRunState *runState, LALInferenceVariables *proposedParams);

/** Proposal for rapid sky localization.  Used when --rapidSkyLoc
    is specified. */
void LALInferenceRapidSkyLocProposal(LALInferenceRunState *runState, LALInferenceVariables *proposedParams);

/** Proposal for finding max temperature for PTMCMC. */
void LALInferencePTTempTestProposal(LALInferenceRunState *runState, LALInferenceVariables *proposedParams);

/** Proposal for after annealing is over. */
void LALInferencePostPTProposal(LALInferenceRunState *runState, LALInferenceVariables *proposedParams);

/** Non-adaptive, sigle-variable update proposal with reasonable
    widths in each dimension. */
void LALInferenceSingleProposal(LALInferenceRunState *runState, LALInferenceVariables *proposedParams);

/** Like LALInferenceSingleProposal() but will use adaptation if the
    --adapt command-line flag given. */
void LALInferenceSingleAdaptProposal(LALInferenceRunState *runState, LALInferenceVariables *proposedParams);

/** Chooses a detector at random, and keeps the amplitude coefficient
    in that detector, A = (fPlus*iPlus + fCross*iCross)/d, constant
    while choosing a different inclination and distance. */
void LALInferenceInclinationDistance(LALInferenceRunState *runState, LALInferenceVariables *proposedParams);

/** Increments the orbital phase by pi. */
void LALInferenceOrbitalPhaseJump(LALInferenceRunState *runState, LALInferenceVariables *proposedParams);

/** Polarization-phase exact degeneracy. */
void LALInferencePolarizationPhaseJump(LALInferenceRunState *runState, LALInferenceVariables *proposedParams);

/** Choose a random covariance matrix eigenvector to jump along. */
void LALInferenceCovarianceEigenvectorJump(LALInferenceRunState *runState, LALInferenceVariables *proposedParams);

/** Jump around by 0.01 radians in angle on the sky */
void LALInferenceSkyLocWanderJump(LALInferenceRunState *runState, LALInferenceVariables *proposedParams);

void LALInferenceAdaptationProposal(LALInferenceRunState *runState, LALInferenceVariables *proposedParams);
void LALInferenceAdaptationSingleProposal(LALInferenceRunState *runState, LALInferenceVariables *proposedParams);

/** Differential evolution, on all non-fixed, non-output parameters. */
void LALInferenceDifferentialEvolutionFull(LALInferenceRunState *state, LALInferenceVariables *proposedParams);

/** Perform differential evolution on the parameters of the given
    names (the names array should be terminated by a NULL pointer).
    If names == NULL, then perform a
    LALInferenceDifferentialEvolutionFull() step.*/
void LALInferenceDifferentialEvolutionNames(LALInferenceRunState *state, LALInferenceVariables *proposedParams, const char *names[]);

/** Perform differential evolution on only the mass parameters. */
void LALInferenceDifferentialEvolutionMasses(LALInferenceRunState *state, LALInferenceVariables *proposedParams);

/** Perform a differential evolution step on only the extrinsic
    parameters. */
void LALInferenceDifferentialEvolutionExtrinsic(LALInferenceRunState *state, LALInferenceVariables *proposedParams);

/** Perform a differential evolution step on only the spin variables. */
void LALInferenceDifferentialEvolutionSpins(LALInferenceRunState *state, LALInferenceVariables *proposedParams);

/** Draws from an approximation to the true prior.  Flat in all
    variables except for: Mc^(-11/6), flat in cos(co-latitudes), flat
    in sin(dec), dist^2.
    WARNING: This seems to break detailed balance for the LALInferenceProposalTest */
void LALInferenceDrawApproxPrior(LALInferenceRunState *runState, LALInferenceVariables *proposedParams);

/** Reflects the sky location through the plane formed by three
    detectors.  Should only be used when there are exactly three
    different locations for detectors. */
void LALInferenceSkyReflectDetPlane(LALInferenceRunState *runState, LALInferenceVariables *proposedParams);

/* Nested sampling wrappers. */
void NSFillMCMCVariables(LALInferenceVariables *proposedParams, LALInferenceVariables *priorArgs);
void NSWrapMCMCLALProposal(LALInferenceRunState *runState, LALInferenceVariables *proposedParams);

/** Rotate each spin by random angles about L. */
void LALInferenceRotateSpins(LALInferenceRunState *runState, LALInferenceVariables *proposedParams);

/** Samples from the analytic likelihood distribution on u = 1/d with
    all other variables fixed.  This behaves similarly to a Gibbs
    sampler for distance (though a Gibbs sampler would sample from the
    *posterior* in d, not the likelihood in u = 1/d). */
void LALInferenceDistanceQuasiGibbsProposal(LALInferenceRunState *runState, LALInferenceVariables *proposedParams);

/** Samples from the analytic likelihood distribution in orbital phase
    (log(L) ~ <d|d> + 2*Re(<d|h>)*cos(delta-phi) -
    2*Im(<d|h>)*sin(delta-phi) + <h|h>, where delta-phi is the orbital
    phase shift relative to the reference used for h.  This is
    effectively a Gibbs sampler for the phase coordinate. */
void LALInferenceOrbitalPhaseQuasiGibbsProposal(LALInferenceRunState *runState, LALInferenceVariables *proposedParams);

/** Uses a kD tree containing the previously-output points to propose
    the next sample.  The proposal chooses a stored point at random,
    finds the kD cell that contains this point and about 64 others,
    and then chooses the proposed point uniformly within the bounding
    box of the points contained in this sell. */
void LALInferenceKDNeighborhoodProposal(LALInferenceRunState *runState, LALInferenceVariables *proposedParams);

/** Proposal for the extrinsic parameters. Uses the sky reflection for 3
 independent detector locations, then computes the corresponding values
 of polarisation, inclination and distance for the proposed sky location.
 See Vivien's thesis for the details of the equations implemented.*/
void LALInferenceExtrinsicParamProposal(LALInferenceRunState *runState, LALInferenceVariables *proposedParams);

/** Helper function to update the adaptive steps after each jump. Set accepted=1 if accepted, 0 otherwise */
void LALInferenceUpdateAdaptiveJumps(LALInferenceRunState *runState, INT4 accepted, REAL8 targetAcceptance);
/** Helper function to setup the adaptive step proposals before the run */
void LALInferenceSetupAdaptiveProposals(LALInferenceRunState *state);

#endif
