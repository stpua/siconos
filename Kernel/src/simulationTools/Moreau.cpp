/* Siconos-Kernel version 1.1.4, Copyright INRIA 2005-2006.
 * Siconos is a program dedicated to modeling, simulation and control
 * of non smooth dynamical systems.
 * Siconos is a free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * Siconos is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Siconos; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Contact: Vincent ACARY vincent.acary@inrialpes.fr
 */
#include "Moreau.h"
// includes to be deleted thanks to factories:
#include "LagrangianLinearTIDS.h"
#include "LagrangianDS.h"
#include "LinearDS.h"
#include "LinearTIDS.h"

using namespace std;



// --- Default constructor ---
Moreau::Moreau(Strategy* newS): OneStepIntegrator("Moreau", newS)
{}

// --- xml constructor ---
Moreau::Moreau(OneStepIntegratorXML * osiXML, Strategy* newS):
  OneStepIntegrator("Moreau", newS)
{
  // Note: we do not call xml constructor of OSI, but default one, since we need to download theta and DS at the same time.

  if (osiXML == NULL)
    RuntimeException::selfThrow("Moreau::xml constructor - OneStepIntegratorXML object == NULL.");

  integratorXml = osiXML;
  MoreauXML * moreauXml = static_cast<MoreauXML*>(osiXML);

  // Required inputs: a list of DS and one theta per DS.
  // No xml entries at the time for sizeMem and W.

  if (!osiXML->hasDSList())
    RuntimeException::selfThrow("Moreau::xml constructor - DS list is missing in xml input file.");
  if (!moreauXml->hasThetaList())
    RuntimeException::selfThrow("Moreau::xml constructor - theta list is missing in xml input file.");

  vector<double> thetaXml;     // list of theta values
  // thetaXml[i] will correspond to the i-eme ds in the xml list. If all attributes is true in ds,
  // then theta values are sorted so to correspond to growing ds numbers order.

  if (moreauXml->hasAllTheta()) // if one single value for all theta
    thetaXml.push_back(moreauXml->getSingleTheta());
  else
    moreauXml->getTheta(thetaXml);

  NonSmoothDynamicalSystem * nsds = strategyLink->getModelPtr()->getNonSmoothDynamicalSystemPtr();

  if (osiXML->hasAllDS()) // if flag all=true is present -> get all ds from the nsds
  {
    // In nsds DS are saved in a vector.
    // Future version: saved them in a set? And then just call:
    // OSIDynamicalSystems = nsds->getDynamicalSystems();
    DSSet tmpDS = nsds->getDynamicalSystems();
    DSIterator it;
    unsigned int i = 0;
    for (it = tmpDS.begin(); it != tmpDS.end(); ++it)
    {
      OSIDynamicalSystems.insert(*it);
      // get corresponding theta. In xml they must be sorted in an order that corresponds to growing DS-numbers order.
      if (moreauXml->hasAllTheta()) // if one single value for all theta
        thetaMap[*it] = thetaXml[0];
      else
        thetaMap[*it] = thetaXml[i++];
    }
  }
  else
  {
    // get list of ds numbers implicated in the OSI
    vector<int> dsNumbers;
    osiXML->getDSNumbers(dsNumbers);
    unsigned int i = 0;
    // get corresponding ds and insert them into the set.
    vector<int>::iterator it;
    for (it = dsNumbers.begin(); it != dsNumbers.end(); ++it)
    {
      OSIDynamicalSystems.insert(nsds->getDynamicalSystemPtrNumber(*it));
      if (moreauXml->hasAllTheta()) // if one single value for all theta
        thetaMap[nsds->getDynamicalSystemPtrNumber(*it)] = thetaXml[0];
      else
        thetaMap[nsds->getDynamicalSystemPtrNumber(*it)] = thetaXml[i++];
    }
  }

  // W loading: not yet implemented
  if (moreauXml->hasWList())
    RuntimeException::selfThrow("Moreau::xml constructor - W matrix loading not yet implemented.");
}

// --- constructor from a minimum set of data ---
Moreau::Moreau(DynamicalSystem* newDS, const double& newTheta, Strategy* newS): OneStepIntegrator("Moreau", newS)
{
  if (strategyLink == NULL)
    RuntimeException::selfThrow("Moreau:: constructor (ds,theta,strategy) - strategy == NULL");

  OSIDynamicalSystems.insert(newDS);
  thetaMap[newDS] = newTheta;
}

Moreau::~Moreau()
{
  matIterator it;
  for (it = WMap.begin(); it != WMap.end(); ++it)
    if (isWAllocatedInMap[it->first]) delete it->second;
  WMap.clear();
  thetaMap.clear();

  DSIterator itDS;
  for (itDS = OSIDynamicalSystems.begin(); itDS != OSIDynamicalSystems.end(); ++itDS)
    if (*itDS != NULL && (*itDS)->getType() == LNLDS)(*itDS)->freeTmpWorkVector("LagNLDSMoreau");
}

void Moreau::setWMap(const mapOfMatrices& newMap)
{
  // check sizes.
  if (newMap.size() != OSIDynamicalSystems.size())
    RuntimeException::selfThrow("Moreau::setWMap(newMap): number of W matrices is different from number of DS.");

  // pointer links! No reallocation
  matIterator it;
  for (it = WMap.begin(); it != WMap.end(); ++it)
    if (isWAllocatedInMap[it->first]) delete it->second;

  WMap.clear();
  WMap = newMap;
  isWAllocatedInMap.clear();
  for (it = WMap.begin(); it != WMap.end(); ++it)
    isWAllocatedInMap[it->first] = false;
}

const SimpleMatrix Moreau::getW(DynamicalSystem* ds)
{
  if (ds == NULL)
    return *(WMap[0]);
  if (WMap[ds] == NULL)
    RuntimeException::selfThrow("Moreau::getW(ds): W[ds] == NULL.");
  return *(WMap[ds]);
}

SiconosMatrix* Moreau::getWPtr(DynamicalSystem* ds)
{
  if (ds == NULL)
    return WMap[0];
  if (WMap[ds] == NULL)
    RuntimeException::selfThrow("Moreau::getWPtr(ds): W[ds] == NULL.");
  return WMap[ds];
}

void Moreau::setW(const SiconosMatrix& newValue, DynamicalSystem* ds)
{
  unsigned int line = newValue.size(0);
  unsigned int col  = newValue.size(1);

  if (line != col) // Check that newValue is square
    RuntimeException::selfThrow("Moreau::setW(newVal,ds) - newVal is not square! ");

  if (ds == NULL)
    RuntimeException::selfThrow("Moreau::setW(newVal,ds) - ds == NULL.");

  unsigned int sizeW = ds->getDim(); // n for first order systems, ndof for lagrangian.
  if (line != sizeW) // check consistency between newValue and dynamical system size
    RuntimeException::selfThrow("Moreau::setW(newVal) - unconsistent dimension between newVal and dynamical system to be integrated ");

  if (WMap[ds] == NULL) // allocate a new W if required
  {
    WMap[ds] = new SimpleMatrix(newValue);
    isWAllocatedInMap[ds] = true;
  }
  else  // or fill-in an existing one if dimensions are consistent.
  {
    if (line == WMap[ds]->size(0) && col == WMap[ds]->size(1))
      *(WMap[ds]) = newValue;
    else
      RuntimeException::selfThrow("Moreau - setW: inconsistent dimensions with problem size for given input matrix W");
  }
}

void Moreau::setWPtr(SiconosMatrix *newPtr, DynamicalSystem* ds)
{
  unsigned int line = newPtr->size(0);
  unsigned int col  = newPtr->size(1);
  if (line != col) // Check that newPtr is square
    RuntimeException::selfThrow("Moreau::setWPtr(newVal) - newVal is not square! ");

  if (ds == NULL)
    RuntimeException::selfThrow("Moreau::setWPtr(newVal,ds) - ds == NULL.");

  unsigned int sizeW = ds->getDim(); // n for first order systems, ndof for lagrangian.
  if (line != sizeW) // check consistency between newValue and dynamical system size
    RuntimeException::selfThrow("Moreau::setW(newVal) - unconsistent dimension between newVal and dynamical system to be integrated ");

  if (isWAllocatedInMap[ds]) delete WMap[ds]; // free memory for previous W
  WMap[ds] = newPtr;                  // link with new pointer
  isWAllocatedInMap[ds] = false;
}

void Moreau::setThetaMap(const mapOfDouble& newMap) // useless function ?
{
  thetaMap = newMap;
}

const double Moreau::getTheta(DynamicalSystem* ds)
{
  if (ds == NULL)
    RuntimeException::selfThrow("Moreau::getTheta(ds) - ds == NULL");
  return thetaMap[ds];
}

void Moreau::setTheta(const double& newTheta, DynamicalSystem* ds)
{
  if (ds == NULL)
    RuntimeException::selfThrow("Moreau::setTheta(val,ds) - ds == NULL");
  thetaMap[ds] = newTheta;
}

void Moreau::initialize()
{
  OneStepIntegrator::initialize();
  // Get initial time
  double t0 = strategyLink->getTimeDiscretisationPtr()->getT0();
  // Compute W(t0) for all ds
  DSIterator it;
  for (it = OSIDynamicalSystems.begin(); it != OSIDynamicalSystems.end(); ++it)
  {
    computeW(t0, *it);
    if ((*it)->getType() == LNLDS)
      (*it)->allocateTmpWorkVector("LagNLDSMoreau", WMap[*it]->size(0));
  }
}

void Moreau::computeW(const double& t, DynamicalSystem* ds)
{

  if (ds == NULL)
    RuntimeException::selfThrow("Moreau::computeW(t,ds) - ds == NULL");


  double h = strategyLink->getTimeDiscretisationPtr()->getH(); // time step

  double theta = thetaMap[ds];
  // Check if W is allocated; if not, do allocation.
  if (WMap[ds] == NULL)
  {
    unsigned int sizeW = ds->getDim(); // n for first order systems, ndof for lagrangian.
    WMap[ds] = new SimpleMatrix(sizeW, sizeW);
    isWAllocatedInMap[ds] = true;
  }

  SiconosMatrix * W = WMap[ds];

  // === Lagrangian dynamical system
  if (ds->getType() == LNLDS)
  {
    LagrangianDS* d = static_cast<LagrangianDS*>(ds);
    // Compute Mass matrix (if loaded from plugin)
    d->computeMass();
    // Compute and get Jacobian (if loaded from plugin)
    d->computeJacobianQFInt(t);
    d->computeJacobianVelocityFInt(t);
    d->computeJacobianQNNL();
    d->computeJacobianVelocityNNL();

    SiconosMatrix *KFint, *KQNL, *CFint, *CQNL ;
    KFint = d->getJacobianQFIntPtr();
    KQNL  = d->getJacobianQNNLPtr();
    CFint = d->getJacobianVelocityFIntPtr();
    CQNL  = d->getJacobianVelocityNNLPtr();

    // Get Mass matrix
    SiconosMatrix *M = d->getMassPtr();

    // Compute W
    *W = *M + h * theta * (*CFint + *CQNL  + h * theta * (*KFint + *KQNL));
  }
  // === Lagrangian linear time invariant system ===
  else if (ds->getType() == LTIDS)
  {
    LagrangianDS* d = static_cast<LagrangianDS*>(ds);
    // Get K, C and Mass
    SiconosMatrix *M, *K, *C ;
    K = ((static_cast<LagrangianLinearTIDS*>(d))->getKPtr());
    C = ((static_cast<LagrangianLinearTIDS*>(d))->getCPtr());
    M = d->getMassPtr();

    // Compute W
    *W = *M + h * theta * (*C + h * theta* *K);
  }

  // === Linear dynamical system ===
  else if (ds->getType() == LDS || ds->getType() == LITIDS)
  {
    LinearDS* d = static_cast<LinearDS*>(ds);
    SiconosMatrix *I;
    unsigned int size = d->getN();
    // Deals with Mxdot
    if (d->getMxdotPtr() == NULL)
    {
      I = new SimpleMatrix(size, size);
      I->eye();
      *W = *I - (h * theta * (d->getA()));
      delete I;
    }
    else
    {
      I = d->getMxdotPtr();
      *W = *I - (h * theta * (d->getA()));
    }
  }

  // === ===
  else RuntimeException::selfThrow("Moreau::computeW - not yet implemented for Dynamical system type :" + ds->getType());

  // LU factorization of W
  W->PLUFactorizationInPlace();
  // At the time, W inverse is saved in Moreau object -> \todo: to be reviewed: use forwarBackward in OneStepNS formalize to avoid inversion of W => work on Mlcp
  W->PLUInverseInPlace();
}


void Moreau::computeFreeState()
{
  // get current time, theta and time step
  double t = strategyLink->getModelPtr()->getCurrentT();
  double h = strategyLink->getTimeDiscretisationPtr()->getH();
  // Previous time step (i)
  double told = t - h;
  /*
     \warning Access to previous values of source terms shall be provided
     \warning thanks to a dedicated memory or to the accurate value of old time instants
  */

  DSIterator it;
  double theta;
  SiconosMatrix * W;
  for (it = OSIDynamicalSystems.begin(); it != OSIDynamicalSystems.end(); ++it)
  {
    DynamicalSystem* ds = *it;
    theta = thetaMap[ds];
    W = WMap[ds];
    // Get the DS type
    string dstyp = ds->getType();

    if ((dstyp == LNLDS) || (dstyp == LTIDS))
    {

      // -- Get the DS --
      LagrangianDS* d = static_cast<LagrangianDS*>(ds);

      // --- RESfree calculus ---
      //
      // Get state i (previous time step)
      SimpleVector* qold, *vold;
      qold = static_cast<SimpleVector*>(d->getQMemoryPtr()->getSiconosVector(0));
      vold = static_cast<SimpleVector*>(d->getVelocityMemoryPtr()->getSiconosVector(0));
      // Computation of the external forces
      d->computeFExt(told);
      SimpleVector FExt0 = d->getFExt();
      d->computeFExt(t);
      SimpleVector FExt1 = d->getFExt();
      // RESfree ...
      SimpleVector *v = d->getVelocityPtr();
      SimpleVector *RESfree = new SimpleVector(FExt1.size());
      // Velocity free
      SimpleVector *vfree = d->getVelocityFreePtr();

      // --- Compute Velocity Free ---
      // For general Lagrangian system LNLDS:
      if (dstyp == LNLDS)
      {
        // Get Mass (remark: M is computed for present state during computeW(t) )
        SiconosMatrix *M = d -> getMassPtr();
        // Compute Qint and Fint
        // for state i
        // warning: get values and not pointers
        d->computeNNL(qold, vold);
        d->computeFInt(told, qold, vold);
        SimpleVector QNL0 = d->getNNL();
        SimpleVector FInt0 = d->getFInt();
        // for present state
        // warning: get values and not pointers
        d->computeNNL();
        d->computeFInt(t);
        SimpleVector QNL1 = d->getNNL();
        SimpleVector FInt1 = d->getFInt();
        // Compute ResFree and vfree solution of Wk(v-vfree)=RESfree
        *RESfree = *M * (*v - *vold) + h * ((1.0 - theta) * (QNL0 + FInt0 - FExt0) + theta * (QNL1 + FInt1 - FExt1));
        *vfree = *v - *W * *RESfree;
      }
      // --- For linear Lagrangian LTIDS:
      else
      {
        // get K, M and C mass pointers
        SiconosMatrix * K = static_cast<LagrangianLinearTIDS*>(d)->getKPtr();
        SiconosMatrix * C = static_cast<LagrangianLinearTIDS*>(d)->getCPtr();
        // Compute ResFree and vfree
        *RESfree = -h * (theta * FExt1 + (1.0 - theta) * FExt0 - (*C * *vold) - (*K * *qold) - h * theta * (*K * *vold));
        *vfree =  *vold - *W * *RESfree;
      }
      // calculate qfree (whereas it is useless for future computation)
      SimpleVector *qfree = d->getQFreePtr();
      *qfree = (*qold) + h * (theta * (*vfree) + (1.0 - theta) * (*vold));
      delete RESfree;
    }
    else if (dstyp == LDS || dstyp == LITIDS)
    {
      LinearDS *d = static_cast<LinearDS*>(ds);
      SimpleVector *xfree = static_cast<SimpleVector*>(d->getXFreePtr());
      SimpleVector *xold = static_cast<SimpleVector*>(d->getXMemoryPtr()->getSiconosVector(0));
      SimpleVector *rold = static_cast<SimpleVector*>(ds->getRMemoryPtr()->getSiconosVector(0));
      unsigned int sizeX = xfree->size();
      SimpleVector *xtmp = new SimpleVector(sizeX);

      SiconosMatrix *A = d->getAPtr();
      SiconosMatrix *I;
      // Deals with Mxdot
      if (d->getMxdotPtr() == NULL)
      {
        I = new SimpleMatrix(sizeX, sizeX);
        I->eye();
        *xtmp = ((*I + h * (1.0 - theta) * *A) * *xold) + (h * (1.0 - theta) * *rold);
        delete I;
      }
      else
      {
        I = d->getMxdotPtr();
        *xtmp = ((*I + h * (1.0 - theta) * *A) * *xold) + (h * (1.0 - theta) * *rold);
      }

      // Warning: b is supposed to be constant, not time dependent.
      SimpleVector *b = d->getBPtr();
      if (b != NULL) *xtmp += h * *b;

      // Warning: T is supposed to be constant, not time dependent.

      // Warning: u is supposed to depend only on time

      if (d->getUPtr() != NULL)
      {
        // get u at previous time step
        d->computeU(told);
        SimpleVector uOld = d->getU();

        // get current u
        d->computeU(t);
        SimpleVector uCurrent = d->getU();

        // get T
        SiconosMatrix *T = d->getTPtr();

        *xtmp += h * *T * (theta * uCurrent + (1.0 - theta) * uOld);
      }

      *xfree = *W * *xtmp;
      delete xtmp;
    }
    else RuntimeException::selfThrow("Moreau::computeFreeState - not yet implemented for Dynamical system type: " + dstyp);
  }
}


void Moreau::integrate(const double& tinit, const double& tend, double& tout, bool& iout)
{
  double h = tend - tinit; //strategy->getTimeDiscretisationPtr()->getH();
  // iout and tout not used. iout is set to true by default.
  iout = true;
  tout = tend;

  DSIterator it;
  SiconosMatrix * W;
  double theta;
  for (it = OSIDynamicalSystems.begin(); it != OSIDynamicalSystems.end(); ++it)
  {
    DynamicalSystem* ds = *it;
    W = WMap[ds];
    theta = thetaMap[ds];

    if (ds->getType() == LNLDS)
    {
      RuntimeException::selfThrow("Moreau::integrate - not yet implemented for Dynamical system type: " + ds->getType());
      // We do not use integrate() for LNDS
    }
    else if (ds->getType() == LTIDS)
    {
      // get the ds
      LagrangianLinearTIDS* d = static_cast<LagrangianLinearTIDS*>(ds);
      // get q and velocity pointers for current time step
      SimpleVector *v, *q, *vold, *qold;
      q = d->getQPtr();
      v = d->getVelocityPtr();
      // get q and velocity pointers for previous time step
      qold = static_cast<SimpleVector*>(d->getQMemoryPtr()->getSiconosVector(0));
      vold = static_cast<SimpleVector*>(d->getVelocityMemoryPtr()->getSiconosVector(0));
      // get mass, K and C pointers
      SiconosMatrix *M, *K, *C;
      M = d->getMassPtr();
      K = d->getKPtr();
      C = d->getCPtr();
      // get p pointer
      SimpleVector  *p;
      p = d->getPPtr();
      // Inline Version
      // The method computeFExt does not allow to compute directly
      // as a function.  To do that, you have to call directly the function of the plugin
      // or call the F77 function  MoreauLTIDS
      // Computation of the external forces
      d->computeFExt(tinit);
      SimpleVector FExt0 = d->getFExt();
      d->computeFExt(tend);
      SimpleVector FExt1 = d->getFExt();
      // velocity computation
      *v = *vold + *W * (h * (theta * FExt1 + (1.0 - theta) * FExt0 - (*C * *vold) - (*K * *qold) - h * theta * (*K * *vold)) + *p);
      // q computation
      *q = (*qold) + h * ((theta * (*v)) + (1.0 - theta) * (*vold));
      // Right Way  : Fortran 77 version with BLAS call
      // F77NAME(MoreauLTIDS)(tinit,tend,theta
      //                      ndof, &qold(0),&vold(0),
      //                      &W(0,0),&K(0,0),&C(0,0),fext,
      //                      &v(0),&q(0))
    }
    else RuntimeException::selfThrow("Moreau::integrate - not yet implemented for Dynamical system type :" + ds->getType());
  }
}

void Moreau::updateState()
{
  double h = strategyLink->getTimeDiscretisationPtr()->getH();

  DSIterator it;
  SiconosMatrix * W;
  double theta;
  for (it = OSIDynamicalSystems.begin(); it != OSIDynamicalSystems.end(); ++it)
  {
    DynamicalSystem* ds = *it;
    W = WMap[ds];
    theta = thetaMap[ds];
    // Get the DS type

    std::string dsType = ds->getType();

    if ((dsType == LNLDS) || (dsType == LTIDS))
    {
      // get dynamical system
      LagrangianDS* d = static_cast<LagrangianDS*>(ds);
      // get velocity free, p, velocity and q pointers
      SimpleVector *vfree = d->getVelocityFreePtr();
      SimpleVector *p = d->getPPtr();
      SimpleVector *v = d->getVelocityPtr();
      SimpleVector *q = d->getQPtr();
      // Save value of q and v in stateTmp for future convergence computation
      if (dsType == LNLDS)
        ds->addTmpWorkVector(v, "LagNLDSMoreau");
      // Compute velocity
      *v = *vfree +  *W * *p;
      // Compute q
      //  -> get previous time step state
      SimpleVector *vold = static_cast<SimpleVector*>(d->getVelocityMemoryPtr()->getSiconosVector(0));
      SimpleVector *qold = static_cast<SimpleVector*>(d->getQMemoryPtr()->getSiconosVector(0));
      *q = *qold + h * (theta * *v + (1.0 - theta)* *vold);
      // set reaction to zero
      p->zero();
      // --- Update W for general Lagrangian system
      if (dsType == LNLDS)
      {
        double t = strategyLink->getModelPtr()->getCurrentT();
        computeW(t, ds);
      }
      // Remark: for Linear system, W is already saved in object member w
    }
    else if (dsType == LDS || dsType == LITIDS)
    {
      SiconosVector* x = ds->getXPtr();
      SiconosVector* xFree = ds->getXFreePtr();

      *x = *xFree + (h * theta * *W * *(ds->getRPtr())) ;
    }
    else RuntimeException::selfThrow("Moreau::updateState - not yet implemented for Dynamical system type: " + dsType);
    // Remark: for Linear system, W is already saved in object member w
  }
}


void Moreau::display()
{
  OneStepIntegrator::display();

  cout << "====== Moreau OSI display ======" << endl;
  DSIterator it;
  for (it = OSIDynamicalSystems.begin(); it != OSIDynamicalSystems.end(); ++it)
  {
    cout << "--------------------------------" << endl;
    cout << "--> W of dynamical system number " << (*it)->getNumber() << ": " << endl;
    if (WMap[*it] != NULL) WMap[*it]->display();
    else cout << "-> NULL" << endl;
    cout << "--> and corresponding theta is: " << thetaMap[*it] << endl;
  }
  cout << "================================" << endl;
}

void Moreau::saveIntegratorToXML()
{
  //  OneStepIntegrator::saveIntegratorToXML();
  //if(integratorXml != NULL)
  //{
  //(static_cast<MoreauXML*>(integratorXml))->setTheta(theta );
  //     (static_cast<MoreauXML*>(integratorXml))->setW(W);
  //}
  //else
  RuntimeException::selfThrow("Moreau::saveIntegratorToXML - IntegratorXML not yet implemented.");
}

void Moreau::saveWToXML()
{
  //   if(integratorXml != NULL)
  //     {
  //       (static_cast<MoreauXML*>(integratorXml))->setW(W);
  //     }
  //   else RuntimeException::selfThrow("Moreau::saveIntegratorToXML - IntegratorXML object not exists");
  RuntimeException::selfThrow("Moreau::saveWToXML -  not yet implemented.");
}

Moreau* Moreau::convert(OneStepIntegrator* osi)
{
  Moreau* moreau = dynamic_cast<Moreau*>(osi);
  return moreau;
}
