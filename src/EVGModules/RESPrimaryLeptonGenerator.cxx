//____________________________________________________________________________
/*!

\class   genie::RESPrimaryLeptonGenerator

\brief   Generates the final state primary lepton in v RES interactions.

         Is a concrete implementation of the EventRecordVisitorI interface.

\author  Costas Andreopoulos <C.V.Andreopoulos@rl.ac.uk>
         CCLRC, Rutherford Appleton Laboratory

\created October 03, 2004

*/
//____________________________________________________________________________

#include <TMath.h>

#include "EVGModules/RESPrimaryLeptonGenerator.h"
#include "GHEP/GHepRecord.h"
#include "Interaction/Interaction.h"

using namespace genie;

//___________________________________________________________________________
RESPrimaryLeptonGenerator::RESPrimaryLeptonGenerator() :
PrimaryLeptonGenerator("genie::RESPrimaryLeptonGenerator")
{

}
//___________________________________________________________________________
RESPrimaryLeptonGenerator::RESPrimaryLeptonGenerator(string config) :
PrimaryLeptonGenerator("genie::RESPrimaryLeptonGenerator", config)
{

}
//___________________________________________________________________________
RESPrimaryLeptonGenerator::~RESPrimaryLeptonGenerator()
{

}
//___________________________________________________________________________
void RESPrimaryLeptonGenerator::ProcessEventRecord(GHepRecord * evrec) const
{
// This method generates the final state primary lepton

  //-- Get the interaction & initial state objects

  Interaction * interaction = evrec->GetInteraction();
  const InitialState & init_state = interaction->GetInitialState();

  //-- Figure out the Final State Lepton PDG Code

  int pdgc = interaction->GetFSPrimaryLepton()->PdgCode();

  //-- RES Kinematics: Compute the lepton energy and the scattering
  //   angle with respect to the incoming neutrino

  //auxiliary params:
  double Ev   = init_state.GetProbeE(kRfStruckNucAtRest);
  double M    = init_state.GetTarget().StruckNucleonP4()->M(); // can be off m/shell
  double ml   = interaction->GetFSPrimaryLepton()->Mass();
  double Q2   = interaction->GetKinematics().Q2();
  double W    = interaction->GetKinematics().W();
  double M2   = TMath::Power(M, 2);
  double ml2  = TMath::Power(ml,2);
  double W2   = TMath::Power(W, 2);

  //Compute outgoing lepton energy
  double El  = Ev - 0.5 * (W2 - M2 + Q2) / M;

  //Compute outgoing lepton scat. angle with respect to the incoming v
  double pl  = TMath::Sqrt( TMath::Max(0., El*El-ml2) );
  assert (pl > 0);
  double cThSc = (El - 0.5*(Q2+ml2)/Ev) / pl; // cos(theta-scat) [-1,1]
  assert( TMath::Abs(cThSc) <= 1 );

  //-- Rotate its 4-momentum to the nucleon rest frame
  //   unit' = R(Theta0,Phi0) * R(ThetaSc,PhiSc) * R^-1(Theta0,Phi0) * unit
  TLorentzVector * pl4 = P4InNucRestFrame(evrec, cThSc, El);

  //-- Boost it to the lab frame
  TVector3 * beta = NucRestFrame2Lab(evrec);
  pl4->Boost(*beta); // active Lorentz transform
  delete beta;

  //-- Create a GHepParticle and add it to the event record
  //   (use the insertion method at the base PrimaryLeptonGenerator visitor)
  this->AddToEventRecord(evrec, pdgc, pl4);

  delete pl4;

  // set final state lepton polarization
  this->SetPolarization(evrec);
}
//___________________________________________________________________________

