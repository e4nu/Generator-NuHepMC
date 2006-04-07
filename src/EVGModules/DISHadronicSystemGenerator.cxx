//____________________________________________________________________________
/*!

\class   genie::DISHadronicSystemGenerator

\brief   Generates the final state hadronic system in v DIS interactions.

         Is a concrete implementation of the EventRecordVisitorI interface.

\author  Costas Andreopoulos <C.V.Andreopoulos@rl.ac.uk>
         CCLRC, Rutherford Appleton Laboratory

\created October 03, 2004

*/
//____________________________________________________________________________

#include <TMCParticle6.h>

#include "Conventions/Constants.h"
#include "EVGCore/EVGThreadException.h"
#include "EVGModules/DISHadronicSystemGenerator.h"
#include "Fragmentation/HadronizationModelI.h"
#include "GHEP/GHepStatus.h"
#include "GHEP/GHepParticle.h"
#include "GHEP/GHepRecord.h"
#include "GHEP/GHepFlags.h"
#include "Messenger/Messenger.h"
#include "Numerical/RandomGen.h"
#include "PDG/PDGLibrary.h"
#include "PDG/PDGCodes.h"
#include "PDG/PDGUtils.h"
#include "Utils/PrintUtils.h"

using namespace genie;
using namespace genie::constants;
using namespace genie::utils::print;

//___________________________________________________________________________
DISHadronicSystemGenerator::DISHadronicSystemGenerator() :
HadronicSystemGenerator("genie::DISHadronicSystemGenerator")
{

}
//___________________________________________________________________________
DISHadronicSystemGenerator::DISHadronicSystemGenerator(string config) :
HadronicSystemGenerator("genie::DISHadronicSystemGenerator", config)
{

}
//___________________________________________________________________________
DISHadronicSystemGenerator::~DISHadronicSystemGenerator()
{

}
//___________________________________________________________________________
void DISHadronicSystemGenerator::ProcessEventRecord(GHepRecord * evrec) const
{
// This method generates the final state hadronic system

  //-- If the struck nucleon was within a nucleus, then add the final state
  //   nucleus at the EventRecord
  this->AddTargetNucleusRemnant(evrec);

  //-- Add an entry for the DIS Pre-Fragm. Hadronic State
  this->AddFinalHadronicSyst(evrec);

  //-- Add the fragmentation products
  this->AddFragmentationProducts(evrec);
}
//___________________________________________________________________________
void DISHadronicSystemGenerator::AddFragmentationProducts(
                                                    GHepRecord * evrec) const
{
// Calls a hadronizer and adds the fragmentation products at the GHEP

  //-- Compute the hadronic system invariant mass
  TLorentzVector p4Had = this->Hadronic4pLAB(evrec);
  double W = p4Had.M();

  Interaction * interaction = evrec->GetInteraction();
  interaction->GetKinematicsPtr()->SetW(W);

  //-- Run the hadronization model and get the fragmentation products:
  //   A collection of ROOT TMCParticles (equiv. to a LUJETS record)
  TClonesArray * plist = fHadronizationModel->Hadronize(interaction);

  if(!plist) {
     LOG("DISHadronicVtx", pWARN) 
                  << "Got an empty particle list. Hadronizer failed!";
     LOG("DISHadronicVtx", pWARN) 
                    << "Quitting the current event generation thread";

     evrec->EventFlags()->SetBitNumber(kNoAvailablePhaseSpace, true);

     genie::exceptions::EVGThreadException exception;
     exception.SetReason("Not enough phase space for hadronizer");
     exception.SwitchOnFastForward();
     throw exception;

     return;
  }

  //-- Velocity for the [Hadronic CM] -> [LAB] active Lorentz transform
  TVector3 beta = this->HCM2LAB(evrec);

  //-- Translate the fragmentation products from TMCParticles to
  //   GHepParticles and copy them to the event record.

  int mom = evrec->FinalStateHadronicSystemPosition();
  assert(mom!=-1);

  TMCParticle * p = 0;
  TIter particle_iter(plist);

  while( (p = (TMCParticle *) particle_iter.Next()) ) {

     // the fragmentation products are generated in the final state
     // hadronic CM Frame - take each particle back to the LAB frame
     TLorentzVector p4(p->GetPx(), p->GetPy(), p->GetPz(), p->GetEnergy());
     p4.Boost(beta);

     // copy the particle to the event record
     int          pdgc   = p->GetKF();
     GHepStatus_t status = GHepStatus_t(p->GetKS());
     TLorentzVector v4(0,0,0,0); // dummy position 4-vector

     evrec->AddParticle(pdgc, status, mom,-1,-1,-1, p4, v4);

  } // fragmentation-products-iterator

  plist->Delete();
  delete plist;
}
//___________________________________________________________________________
void DISHadronicSystemGenerator::Configure(const Registry & config)
{
  Algorithm::Configure(config);
  this->LoadConfig();
}
//____________________________________________________________________________
void DISHadronicSystemGenerator::Configure(string config)
{
  Algorithm::Configure(config);
  this->LoadConfig();
}
//____________________________________________________________________________
void DISHadronicSystemGenerator::LoadConfig(void)
{
// Load sub-algorithms and config data to reduce the number of registry
// lookups

  fHadronizationModel = 0;

  //-- Get the requested hadronization model
  fHadronizationModel = dynamic_cast<const HadronizationModelI *> (
           this->SubAlg("hadronization-alg-name", "hadronization-param-set"));

  assert(fHadronizationModel);
}
//____________________________________________________________________________

