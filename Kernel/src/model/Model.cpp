/* Siconos-Kernel, Copyright INRIA 2005-2012.
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
 * Contact: Vincent ACARY, siconos-team@lists.gforge.inria.fr
 */

#include "Model.hpp"
#include "SiconosModelXML.hpp"
#include "KernelConfig.h"
#include "SimulationXML.hpp"
#include "NonSmoothDynamicalSystem.hpp"
#include "NonSmoothDynamicalSystemXML.hpp"
#include "TimeDiscretisation.hpp"
#include "TimeStepping.hpp"
#include "EventDriven.hpp"
#include "Topology.hpp"

#include "TypeName.hpp"


// --- CONSTRUCTORS ---

// --- Default (public) constructor ---
Model::Model(): _t(0.0), _t0(0.0), _T(0.0), _title("none"), _author("nobody"), _description("none"),
  _date("none"), _xmlSchema("none")
{}

// -> xml
Model::Model(const std::string& xmlFile):
  _t(0.0), _t0(0.0), _T(-1.0),
  _title("none"), _author("nobody"), _description("none"),
  _date("none"), _xmlSchema(XML_SCHEMA)
{
  // Built DOMtree
  _modelxml.reset(new SiconosModelXML(xmlFile));

  // Load data (default value for T is -1)
  if (_modelxml->hasT()) _T = _modelxml->getT();
  _t0 = _modelxml->t0();
  if (_modelxml->hasTCurrent()) _t = _modelxml->getTCurrent();
  else _t = _t0;
  _title = _modelxml->title();
  _author = _modelxml->author();
  _description = _modelxml->description();
  _date = _modelxml->date();
  if (_modelxml->hasXMLSchema())
    _xmlSchema = _modelxml->getXMLSchema();

  // Memory allocation for _nsds and simulation
  _nsds.reset(new NonSmoothDynamicalSystem
              (_modelxml->getNonSmoothDynamicalSystemXML()));
  if (_modelxml->hasSimulation())
  {
    if (_modelxml->getSimulationXML()->
        getSimulationXMLType() == TIMESTEPPING_TAG)
      _strat.reset(new TimeStepping
                   (_modelxml->getSimulationXML(), _t0, _T,
                    setOfGraph<DynamicalSystemsSet>(_nsds->dynamicalSystems()),
                    _nsds->interactions()));
    else if (_modelxml->getSimulationXML()->getSimulationXMLType() == EVENTDRIVEN_TAG)
      _strat.reset(new EventDriven
                   (_modelxml->getSimulationXML(), _t0, _T,
                    setOfGraph<DynamicalSystemsSet>(_nsds->dynamicalSystems()),
                    _nsds->interactions()));
    else RuntimeException::selfThrow
      ("Model: xml constructor, wrong type of simulation" +
       (_modelxml->getSimulationXML()->getSimulationXMLType()));
  }
}

// --- From a minimum set of data ---
Model::Model(double newT0, double newT, const std::string& newTitle,
             const std::string& newAuthor, const std::string& newDescription,
             const std::string& newDate, const std::string& newSchema):
  _t(newT0), _t0(newT0), _T(-1), _title(newTitle),
  _author(newAuthor), _description(newDescription), _date(newDate), _xmlSchema(newSchema)
{
  if (newT > _t0) _T = newT;
  else if (newT > 0 && newT <= _t0)
    RuntimeException::selfThrow
    ("Model::constructor from min data: Warning, final T lower than t0");

  /* empty */
  _nsds.reset(new NonSmoothDynamicalSystem());
  // else no T in the model!
}

Model::~Model()
{
  if (_strat)
    _strat->clear();
}

void Model::setSimulationPtr(SP::Simulation newPtr)
{
  // Warning: this function may be used carefully because of the links
  // between Model and TimeDiscretisation The model of the simulation
  // input MUST be the current model.
  _strat = newPtr;
}

void Model::setNonSmoothDynamicalSystemPtr(SP::NonSmoothDynamicalSystem newPtr)
{
  _nsds = newPtr;
}

void Model::setSiconosModelXMLPtr(SP::SiconosModelXML newPtr)
{
  _modelxml = newPtr;
}

void Model::initialize(SP::Simulation simulation)
{
  // Connection to input simulation only if non null.
  // Two cases:
  // 1- simulation object has been created inside the model => no need
  //  to link => no input arg. for the present function (input =
  //  default = empty) - Ex: after xml constructor.
  // 2- simulation built outside of the model => input simulation
  // required
  if (simulation)
    _strat = simulation;

  assert(_strat && "Model::initialize() error - The simulation object of this model is null.");

  // === topology init (computes Interaction sets, relative degrees ...) ===
  _nsds->topology()->initialize();

  // === Simulation init ===
  _strat->initialize(shared_from_this());

  // symmetry in indexSets
  _nsds->topology()->setProperties();

}


// --- OTHER FUNCTIONS ---


// --- XML RELATED FUNCTIONS ---
void Model::saveToXMLFile(char* xmlFile)
{
  std::cout << "## Model->checkXMLPlatform()" <<std::endl;
  //   the first operation to do is to check the XML objects
  checkXMLPlatform();

  std::cout << "## Model->savePlatformToXML()" <<std::endl;
  //   copy the values of the platform to the DOM tree
  savePlatformToXML();

  std::cout << "## Model->checkXMLDOMTree()" <<std::endl;
  //   verifies that the DOM tree respects the XML schema
  checkXMLDOMTree();

  std::cout << "## Model->saveSiconosModelInXMLFile()" <<std::endl;
  //   saves in a file the DOM tree
  _modelxml->saveSiconosModelInXMLFile(xmlFile);
}

void Model::saveToDOMTree()
{
  checkXMLPlatform();
  savePlatformToXML();
  checkXMLDOMTree();
}

void Model::savePlatformToXML()
{
  // update of the data of the Model
  _modelxml->sett0(_t0);
  _modelxml->setT(_T);
  _modelxml->setTCurrent(_t);
  _modelxml->setTitle(_title);
  _modelxml->setAuthor(_author);
  _modelxml->setDescription(_description);
  _modelxml->setDate(_date);
  _modelxml->setXMLSchema(_xmlSchema);

  // save of the NonSmoothDynamicalSystem
  _nsds->saveNSDSToXML();

  // save of the Simulation

  if (_strat)
  {
    _strat->timeDiscretisation()->saveTimeDiscretisationToXML();

    if (Type::value(*_strat) == Type::TimeStepping)
      (std11::static_pointer_cast<TimeStepping>(_strat))->saveSimulationToXML();
    else if (Type::value(*_strat) == Type::EventDriven)
      (std11::static_pointer_cast<EventDriven>(_strat))->saveSimulationToXML();
    else RuntimeException::selfThrow("Model::savePlatformToXML - bad kind of Simulation");
  }
  else //RuntimeException::selfThrow("Model::saveToXML - object SimulationXML does not exist");
    std::cout << "Model::saveToXML - Warning : No Simulation is defined" <<std::endl;
}

bool Model::checkXMLDOMTree()
{
  bool res = false;
  if (_modelxml)
    res = _modelxml->checkSiconosDOMTree();

  std::cout << " # checkModelCoherency()" <<std::endl;
  checkModelCoherency();
  return res;
}

void Model::checkXMLPlatform()
{
  if (_modelxml)
  {
    if (_modelxml->getNonSmoothDynamicalSystemXML())
    {
      // we must create/update the DynamicalSystemXMLs
      _nsds->nonSmoothDynamicalSystemXML()->
      updateNonSmoothDynamicalSystemXML
      (_modelxml->getNonSmoothDynamicalSystemXML()->getRootNode(), _nsds);
    }
    else if (_nsds)
    {
      // creation of the NonSmoothDynamicalSystemXML and of all
      // the DynamicalSystemXML and InteractionXML
      _modelxml->loadModel(shared_from_this());
      // \todo to be tested !!
    }
    else RuntimeException::selfThrow
      ("Model::checkXMLPlatform - There's no NonSmoothDynamicalSystem in the Platform, the XML platform can't be built");

    if ((_strat))
    {
      if (_modelxml->getSimulationXML())
      {
        //
        // no SimulationXML already exists, so no
        // TimeDiscretisationXML, OneStepIntegratorXML and
        // OneStepNSProblemXML are existing because these
        // objects are required when a Simulation is defined in
        // the XML input file

        // we must update all the Model to do the creation of
        // the SimulationXML and of all the OneStepIntegratorXML
        // and OneStepNSProblemXML
        //
        _modelxml->loadModel(shared_from_this());
        // \todo to be tested !!
      }
      else
      {
        _strat->simulationXML()->
        saveSimulation2XML
        (_modelxml->getSimulationXML()->getRootNode(),  _strat);
      }
    }
  }
  else
  {
    // in this case, we must create all the XML objects
    // Siconos_Modelxml, NonSmoothDynamicalSystemXML, SimulationXML,
    // ...

    // to build all the XML objects, we must fold all the objects of
    // the platform

    _modelxml.reset(new SiconosModelXML());
    _modelxml->loadModel(shared_from_this());
  }
}


void Model::checkModelCoherency()
{
  // at first, checking the XML if matrix and vector are well defined
  // by example if DynamicalSystems have BoundaryConditions when the
  // NonSmoothDynamicalSystem is BVP for example

  if (_modelxml->checkSiconosDOMTreeCoherency() == true)
    std::cout << "Data of the XML DOM tree are coherent." <<std::endl;
  else
    std::cout << "Warning : Data of the XML DOM tree are not coherent." <<std::endl;
}

int Model::xmlSchemaValidated(std::string xmlFile, std::string xmlSchema)
{
  int res;
  std::cout << "int Model::xmlSchemaValidated(std::string xmlFile, std::string xmlSchema)" <<std::endl;
  res = _modelxml->validateXmlFile(xmlFile, xmlSchema);
  return res;
}


// --- OTHER FUNCTIONS ---

void Model::display() const
{
  std::cout << " =========> Model named " << _title << ", written by " << _author << " (" << _date << ")." <<std::endl;
  std::cout << " ----- Description: " << _description <<std::endl;
  std::cout << " ----- xml schema: " << _xmlSchema <<std::endl;
  std::cout <<std::endl;
  std::cout << " Time runs from " << _t0 << " to " << _T <<std::endl;
  std::cout << " Current time is " << _t <<std::endl;
  std::cout <<std::endl;
  if (!_nsds) std::cout << "No NSDS linked to the Model" <<std::endl;
  if (_strat) std::cout << "The simulation (name: " << _strat->name() << ") is a " << Type::name(*_strat) << "." <<std::endl;
  else std::cout << "No simulation attached to this model." <<std::endl;
  std::cout <<std::endl;
  std::cout << " ============================" <<std::endl;
}

void Model::setT(const double& newValue)
{
  _T = newValue;
  _strat->updateT(newValue);
}

