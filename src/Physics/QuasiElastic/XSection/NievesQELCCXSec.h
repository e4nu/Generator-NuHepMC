//____________________________________________________________________________
/*!

\class    genie::NievesCCXSec

\brief    Computes the Quasi Elastic (QEL) total cross section. \n
          Is a concrete implementation of the XSecIntegratorI interface. \n

\author   Steven Gardiner <gardiner \at fnal.gov>
          Fermi National Accelerator Laboratory

\created  March 29, 2019

\cpright  Copyright (c) 2003-2019, The GENIE Collaboration
          For the full text of the license visit http://copyright.genie-mc.org
          or see $GENIE/LICENSE
*/
//____________________________________________________________________________

#ifndef _NIEVES_QEL_CC_TOTAL_XSEC_H_
#define _NIEVES_QEL_CC_TOTAL_XSEC_H_

#include "Physics/XSectionIntegration/XSecIntegratorI.h"
#include "Physics/QuasiElastic/XSection/QELUtils.h"

#include "TMath.h"
#include "Math/IFunction.h"
#include "Math/Integrator.h"

namespace genie {

class VertexGenerator;

namespace utils {
  namespace gsl   {

    class NievesQELdXSec : public ROOT::Math::IBaseFunctionMultiDim
    {
     public:
       NievesQELdXSec(const XSecAlgorithmI* xsec_model, const Interaction* interaction);
       virtual ~NievesQELdXSec();

       // ROOT::Math::IBaseFunctionMultiDim interface
       unsigned int NDim(void) const;
       double DoEval(const double* xin) const;
       ROOT::Math::IBaseFunctionMultiDim* Clone(void) const;

     private:
       const XSecAlgorithmI* fXSecModel;
       Interaction* fInteraction;
    };

  } // gsl   namespace
} // utils namespace

class NievesQELCCXSec : public XSecIntegratorI {

public:

  NievesQELCCXSec(void);
  NievesQELCCXSec(std::string config);

  /// XSecIntegratorI interface implementation
  double Integrate(const XSecAlgorithmI* model, const Interaction* i) const;

  /// Overload the Algorithm::Configure() methods to load private data
  /// members from configuration options
  void Configure(const Registry& config);
  void Configure(std::string config);


private:

  void LoadConfig (void);

  // XML configuration parameters
  std::string fGSLIntgType;
  double fGSLRelTol;
  unsigned int fGSLMaxEval;
  AlgId fVertexGenID;
};


} // genie namespace

#endif
