//____________________________________________________________________________
/*!

\class    genie::FortranWrapperQELPXSec

\brief    Computes quasielastic neutrino-nucleus differential cross sections
          using a wrapper for an external Fortran code. Intended for
          use with a spectral function nuclear model.
          This is a concrete implementation of the XSecAlgorithmI interface. \n

\author   Syrian Truong <struong@fnal.gov>
\author   Steven Gardiner <gardiner \at fnal.gov>
          Fermi National Acclerator Laboratory

\created  July 7, 2021

\cpright  Copyright (c) 2003-2021, The GENIE Collaboration
          For the full text of the license visit http://copyright.genie-mc.org
          or see $GENIE/LICENSE
*/
//____________________________________________________________________________

#ifndef _FORTRAN_WRAPPER_QEL_CROSS_SECTION_H_
#define _FORTRAN_WRAPPER_QEL_CROSS_SECTION_H_

#include <string>
#include <complex>

#include "Math/IFunction.h"

#include "Framework/EventGen/XSecAlgorithmI.h"
#include "Physics/QuasiElastic/XSection/LeptonTensor.h"
#include "Physics/QuasiElastic/XSection/FreeNucleonTensor.h"
#include "Physics/NuclearState/NuclearModelI.h"
#include "Physics/NuclearState/PauliBlocker.h"

namespace genie {

class FortranWrapperQELPXSec : public XSecAlgorithmI {

public:

  FortranWrapperQELPXSec();
  FortranWrapperQELPXSec(std::string config);
  virtual ~FortranWrapperQELPXSec();

  // XSecAlgorithmI interface implementation
  double XSec            (const Interaction* i, KinePhaseSpace_t k) const;
  double Integral        (const Interaction* i) const;
  bool ValidProcess      (const Interaction* i) const;

  // Override the Algorithm::Configure methods to load configuration
  // data to private data members
  void Configure (const Registry& config);
  void Configure (std::string param_set);

private:
  void LoadConfig (void);
};

} // genie namespace

#endif
