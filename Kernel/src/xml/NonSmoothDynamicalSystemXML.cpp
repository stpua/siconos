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
#include "NonSmoothDynamicalSystemXML.h"

//---  Following includes to be suppressed thanks to factory (?) ---

// DS
#include "LagrangianDSXML.h"
#include "LagrangianLinearTIDSXML.h"
#include "LinearDSXML.h"
// EC
#include "LinearECXML.h"
#include "LinearTIECXML.h"
#include "LagrangianECXML.h"
#include "LagrangianLinearECXML.h"
// DSIO
#include "LinearDSIOXML.h"
#include "LagrangianDSIOXML.h"
#include "LagrangianLinearDSIOXML.h"

using namespace std;


NonSmoothDynamicalSystemXML::NonSmoothDynamicalSystemXML(): rootNode(NULL)
{}

NonSmoothDynamicalSystemXML::NonSmoothDynamicalSystemXML(xmlNodePtr  rootNSDSNode): rootNode(rootNSDSNode)
{
  if (rootNode != NULL)
  {
    xmlNodePtr node;

    if ((node = SiconosDOMTreeTools::findNodeChild(rootNode, LMGC90_NSDS_TAG)) == NULL)
    {
      // at first, we load the DSInputOutputs because we need them to load properly the DynamicalSystemXML
      if ((node = SiconosDOMTreeTools::findNodeChild(rootNode, DSINPUTOUTPUT_DEFINITION_TAG)) != NULL)
        loadDSInputOutputXML(node);

      if ((node = SiconosDOMTreeTools::findNodeChild(rootNode, DYNAMICAL_SYSTEM_DEFINITION_TAG)) != NULL)
        loadDynamicalSystemXML(node);
      else
        XMLException::selfThrow("NonSmoothDynamicalSystemXML - loadNonSmoothDynamicalSystem error : tag " + DYNAMICAL_SYSTEM_DEFINITION_TAG + " not found.");

      // === Interactions ===
      if ((node = SiconosDOMTreeTools::findNodeChild(rootNode, INTERACTION_DEFINITION_TAG)) != NULL)
      {
        xmlNodePtr interNode = SiconosDOMTreeTools::findNodeChild(node, INTERACTION_TAG);
        // We look for all Interaction tag, and for each of them add an InteractionXML pointer in the interactionXMLSet
        while (interNode != NULL) // scan all the "Interaction" tags, and for each of them insert an InteractionXML object in the set
        {
          interactionsXMLSet.insert(new InteractionXML(interNode));
          interNode = SiconosDOMTreeTools::findFollowNode(interNode, INTERACTION_TAG);
        }
      }

      // Uncomment the following lines when Equality Constraints will be well-implemented.
      //if ((node=SiconosDOMTreeTools::findNodeChild(rootNode, EQUALITYCONSTRAINT_DEFINITION_TAG)) !=NULL)
      // loadEqualityConstraintXML(node);
    }
    else cout << "NonSmoothDynamicalSystemXML -Constructor: the Non Smooth Dynamical System is not defined -> the LMGC90 tag is used." << endl;
  }
}

NonSmoothDynamicalSystemXML::~NonSmoothDynamicalSystemXML()
{
  // Delete DSXML set ...
  SetOfDSXMLIt it;
  for (it = DSXMLSet.begin(); it != DSXMLSet.end(); ++it)
    if ((*it) != NULL) delete(*it);
  DSXMLSet.clear();

  // Delete InteractionXML set ...
  SetOfInteractionsXMLIt it2;
  for (it2 = interactionsXMLSet.begin(); it2 != interactionsXMLSet.end(); ++it2)
    if ((*it2) != NULL) delete(*it2);
  interactionsXMLSet.clear();
}

void NonSmoothDynamicalSystemXML::loadDynamicalSystemXML(xmlNodePtr  rootDSNode)
{
  xmlNodePtr node;

  string type; //Type of DS
  bool isbvp = isBVP();

  // rootDSNode = "DS_Definition". We look for its children node (DynamicalSystem and derived classes) and for
  // each of them add a new DynamicalSystemXML in the set of DSXML.
  node = SiconosDOMTreeTools::findNodeChild((const xmlNodePtr)rootDSNode);
  if (node == NULL) // At least one DynamicalSystem must be described in the xml file.
    XMLException::selfThrow("NonSmoothDynamicalSystemXML - loadDynamicalSystemXML error : at least one " + DYNAMICAL_SYSTEM_TAG + " must be declared.");

  while (node != NULL)
  {
    type = (char*)node->name; // get the type of DS
    if (type == LAGRANGIAN_NON_LINEARDS_TAG)
      DSXMLSet.insert(new LagrangianDSXML(node, isbvp));
    else if (type == LAGRANGIAN_TIDS_TAG)
      DSXMLSet.insert(new LagrangianLinearTIDSXML(node, isbvp));
    else if (type == LINEAR_DS_TAG || type == LINEAR_TIDS_TAG)
      DSXMLSet.insert(new LinearDSXML(node, isbvp));
    else if (type == NON_LINEAR_DS_TAG)
      DSXMLSet.insert(new DynamicalSystemXML(node, isbvp));
    else
      XMLException::selfThrow("NonSmoothDynamicalSystemXML - loadDynamicalSystemXML error : undefined DS type: " + type);
    // go to next node ...
    node = SiconosDOMTreeTools::findFollowNode(node);
  }
}

// WARNING : FOLLOWING FUNCTIONS ARE OBSOLETE, USELESS OR AT LEAST TO BE REVIEWED WHEN DSIO AND EQUALITY CONSTRAINTS
// WILL BE WELL IMPLEMENTED IN MODELING PACKAGE

EqualityConstraintXML* NonSmoothDynamicalSystemXML::getEqualityConstraintXML(int number)
{
  map<int, EqualityConstraintXML*>::iterator it;

  it = equalityConstraintXMLMap.find(number);
  if (it == equalityConstraintXMLMap.end())
  {
    cout << "NonSmoothDynamicalSystemXML::getEqualityConstraintXML - Error : the EqualityConstraintXML number " << number << " does not exist!" << endl;
    return NULL;
  }
  return equalityConstraintXMLMap[number];
}

void NonSmoothDynamicalSystemXML::loadNonSmoothDynamicalSystem(NonSmoothDynamicalSystem* nsds)
{
  XMLException::selfThrow("NonSmoothDynamicalSystemXML - loadDynamicalSystemXML not implemented.");
  //   string type;
  //   string tmp;
  //   xmlNodePtr  node, ecDsioNode;
  //   xmlNodePtr  dsDefinitionNode;
  //   xmlNodePtr  interactionDefinitionNode, ecDefinitionNode;
  //   DynamicalSystemXML* dsxml;
  //   InteractionXML* interactionXML;
  //   EqualityConstraintXML *ecXML;
  //   int number;
  //   unsigned int i;
  //   char num[32];
  //   map<int, DynamicalSystemXML*>::iterator it;
  //   map<int, InteractionXML*>::iterator itinter;
  //   map<int, EqualityConstraintXML*>::iterator itec;

  //   if( rootNode != NULL )
  //     {
  //       setBVP( nsds->isBVP() );

  //       // at first, we check whether we the tag is LMGC90 tag
  //       if( SiconosDOMTreeTools::findNodeChild((const xmlNodePtr )rootNode, LMGC90_NSDS_TAG) == NULL )
  //  {
  //    // creation of the DS_Definition node if necessary
  //    dsDefinitionNode = SiconosDOMTreeTools::findNodeChild((const xmlNodePtr )rootNode, DYNAMICAL_SYSTEM_DEFINITION_TAG);
  //    if( dsDefinitionNode == NULL )
  //      dsDefinitionNode = xmlNewChild(rootNode, NULL, (xmlChar*)DYNAMICAL_SYSTEM_DEFINITION_TAG.c_str(), NULL);

  //    /*
  //     * now, creation of the DynamicalSystemXML objects
  //     */
  //    for(i=0; i<nsds->getNumberOfDS(); i++)
  //      {
  //        if( nsds->getDynamicalSystemPtr(i)->getDynamicalSystemXMLPtr() == NULL )
  //    {
  //      type = nsds->getDynamicalSystemPtr(i)->getType();
  //      number = nsds->getDynamicalSystemPtr(i)->getNumber();
  //      sprintf(num, "%d", number);
  //      definedDSNumbers.push_back( number );


  //      // verifies if this Dynamical System has a number which not used
  //      it = DSXMLMap.find(number);
  //      if( it == DSXMLMap.end() )
  //        {
  //          //node = xmlNewChild( dsDefinitionNode, NULL, (xmlChar*)NSDS_DS.c_str(), NULL );
  //          if (type == LNLDS)
  //      {
  //        node = xmlNewChild( dsDefinitionNode, NULL, (xmlChar*)LAGRANGIAN_NON_LINEARDS_TAG.c_str(), NULL );
  //        xmlNewProp( node, (xmlChar*)NUMBER_ATTRIBUTE.c_str(), (xmlChar*)num );
  //        dsxml = new LagrangianDSXML();

  //        // linkage between the DynamicalSystem and his DynamicalSystemXML
  //        nsds->getDynamicalSystemPtr(i)->setDynamicalSystemXMLPtr( dsxml );

  //        // creation of the DynamicalSystemXML
  //        static_cast<LagrangianDSXML*>(dsxml)->updateDynamicalSystemXML( node, nsds->getDynamicalSystemPtr(i) );

  //        DSXMLMap[number] = dsxml;
  //      }
  //          else if (type == LTIDS)
  //      {
  //        node = xmlNewChild( dsDefinitionNode, NULL, (xmlChar*)LAGRANGIAN_TIDS_TAG.c_str(), NULL );
  //        xmlNewProp( node, (xmlChar*)NUMBER_ATTRIBUTE.c_str(), (xmlChar*)num );
  //        dsxml = new LagrangianLinearTIDSXML();

  //        // linkage between the DynamicalSystem and his DynamicalSystemXML
  //        nsds->getDynamicalSystemPtr(i)->setDynamicalSystemXMLPtr( dsxml );

  //        // creation of the DynamicalSystemXML
  //        static_cast<LagrangianLinearTIDSXML*>(dsxml)->updateDynamicalSystemXML( node, nsds->getDynamicalSystemPtr(i) );

  //        DSXMLMap[number] = dsxml;
  //      }
  //          else if (type == LDS)
  //      {
  //        node = xmlNewChild( dsDefinitionNode, NULL, (xmlChar*)LINEAR_DS_TAG.c_str(), NULL );
  //        xmlNewProp( node, (xmlChar*)NUMBER_ATTRIBUTE.c_str(), (xmlChar*)num );
  //        dsxml = new LinearDSXML();

  //        // linkage between the DynamicalSystem and his DynamicalSystemXML
  //        nsds->getDynamicalSystemPtr(i)->setDynamicalSystemXMLPtr( dsxml );

  //        // creation of the DynamicalSystemXML
  //        static_cast<LinearDSXML*>(dsxml)->updateDynamicalSystemXML( node, nsds->getDynamicalSystemPtr(i) );

  //        DSXMLMap[number] = dsxml;

  //      }
  //          else if (type == NLDS)
  //      {
  //        node = xmlNewChild( dsDefinitionNode, NULL, (xmlChar*)NON_LINEAR_DS_TAG.c_str(), NULL );
  //        xmlNewProp( node, (xmlChar*)NUMBER_ATTRIBUTE.c_str(), (xmlChar*)num );
  //        dsxml = new DynamicalSystemXML();

  //        // linkage between the DynamicalSystem and his DynamicalSystemXML
  //        nsds->getDynamicalSystemPtr(i)->setDynamicalSystemXMLPtr( dsxml );

  //        // creation of the DynamicalSystemXML
  //        dsxml->updateDynamicalSystemXML( node, nsds->getDynamicalSystemPtr(i) );

  //        DSXMLMap[number] = dsxml;
  //      }
  //          else
  //      {
  //        XMLException::selfThrow("NonSmoothDynamicalSystemXML - loadNonSmoothDynamicalSystem error : undefined DS type : " + type + " (have you forgotten to verify the xml files with the Siconos Schema file or update it!?).");
  //      }
  //        }
  //      else
  //        {
  //          tmp = num;
  //          XMLException::selfThrow("NonSmoothDynamicalSystemXML - loadNonSmoothDynamicalSystem | Error : the Dynamical System number : " + tmp + " already exists!");
  //        }
  //    }
  //        else
  //    cout<<"## /!\\ the DynamicalSystem : "<<nsds->getDynamicalSystemPtr(i)->getType()<<" number "<<nsds->getDynamicalSystemPtr(i)->getNumber()<<
  //      ", has already an XML object."<<endl;
  //      }
  //  }
  //       else
  //  {
  //    // the LMGC90 tag for DS definition is in the XML file
  //    // \todo !!!!!!  => specific treatments todo
  //  }


  //       // creation of the EqualityConstraint_Defintion if necessary
  //       if( nsds->getEqualityConstraints().size() > 0 )
  //  {
  //    ecDefinitionNode = SiconosDOMTreeTools::findNodeChild((const xmlNodePtr )rootNode, EQUALITYCONSTRAINT_DEFINITION_TAG);
  //    if( ecDefinitionNode == NULL )
  //      ecDefinitionNode = xmlNewChild(rootNode, NULL, (xmlChar*)EQUALITYCONSTRAINT_DEFINITION_TAG.c_str(), NULL);

  //    for(i=0; i<nsds->getEqualityConstraints().size(); i++)
  //      {
  //        if( nsds->getEqualityConstraintPtr(i)->getEqualityConstraintXML() == NULL )
  //    {
  //      number = nsds->getEqualityConstraintPtr(i)->getNumber();
  //      sprintf(num, "%d", number);
  //      definedEqualityConstraintNumbers.push_back( number );

  //      //verifies if the EqualityConstraint has been defined before
  //      itec = equalityConstraintXMLMap.find(number);
  //      if (itec == equalityConstraintXMLMap.end())
  //        {
  //          if( nsds->getEqualityConstraintPtr(i)->getType() == LINEAREC )
  //      {
  //        node = xmlNewChild( ecDefinitionNode, NULL, (xmlChar*)LINEAR_EC_TAG.c_str(), NULL );
  //        xmlNewProp( node, (xmlChar*)NUMBER_ATTRIBUTE.c_str(), (xmlChar*)num );
  //        ecXML = new LinearECXML();

  //        // linkage between the DynamicalSystem and his DynamicalSystemXML
  //        nsds->getEqualityConstraintPtr(i)->setEqualityConstraintXML( ecXML );

  //        // creation of the DynamicalSystemXML
  //        static_cast<LinearECXML*>(ecXML)->updateEqualityConstraintXML( node, nsds->getEqualityConstraintPtr(i) );

  //        equalityConstraintXMLMap[number] = ecXML;
  //      }
  //          else if( nsds->getEqualityConstraintPtr(i)->getType() == LINEARTIEC )
  //      {
  //        node = xmlNewChild( ecDefinitionNode, NULL, (xmlChar*)LINEAR_TIME_INVARIANT_EC_TAG.c_str(), NULL );
  //        xmlNewProp( node, (xmlChar*)NUMBER_ATTRIBUTE.c_str(), (xmlChar*)num );
  //        ecXML = new LinearTIECXML();

  //        // linkage between the DynamicalSystem and his DynamicalSystemXML
  //        nsds->getEqualityConstraintPtr(i)->setEqualityConstraintXML( ecXML );

  //        // creation of the DynamicalSystemXML
  //        static_cast<LinearTIECXML*>(ecXML)->updateEqualityConstraintXML( node, nsds->getEqualityConstraintPtr(i) );

  //        equalityConstraintXMLMap[number] = ecXML;
  //      }
  //          else if( nsds->getEqualityConstraintPtr(i)->getType() == LAGRANGIANEC )
  //      {
  //        node = xmlNewChild( ecDefinitionNode, NULL, (xmlChar*)LAGRANGIAN_EC_TAG.c_str(), NULL );
  //        xmlNewProp( node, (xmlChar*)NUMBER_ATTRIBUTE.c_str(), (xmlChar*)num );
  //        ecXML = new LagrangianECXML();

  //        // linkage between the DynamicalSystem and his DynamicalSystemXML
  //        nsds->getEqualityConstraintPtr(i)->setEqualityConstraintXML( ecXML );

  //        // creation of the DynamicalSystemXML
  //        static_cast<LagrangianECXML*>(ecXML)->updateEqualityConstraintXML( node, nsds->getEqualityConstraintPtr(i) );

  //        equalityConstraintXMLMap[number] = ecXML;
  //      }
  //          else if( nsds->getEqualityConstraintPtr(i)->getType() == LAGRANGIANLINEAREC )
  //      {
  //        node = xmlNewChild( ecDefinitionNode, NULL, (xmlChar*)LAGRANGIAN_LINEAR_EC_TAG.c_str(), NULL );
  //        xmlNewProp( node, (xmlChar*)NUMBER_ATTRIBUTE.c_str(), (xmlChar*)num );
  //        ecXML = new LagrangianECXML();

  //        // linkage between the DynamicalSystem and his DynamicalSystemXML
  //        nsds->getEqualityConstraintPtr(i)->setEqualityConstraintXML( ecXML );

  //        // creation of the DynamicalSystemXML
  //        static_cast<LagrangianLinearECXML*>(ecXML)->updateEqualityConstraintXML( node, nsds->getEqualityConstraintPtr(i) );

  //        equalityConstraintXMLMap[number] = ecXML;
  //      }
  //          else if( nsds->getEqualityConstraintPtr(i)->getType() == NLINEAREC )
  //      {
  //        node = xmlNewChild( ecDefinitionNode, NULL, (xmlChar*)NON_LINEAR_EC_TAG.c_str(), NULL );
  //        xmlNewProp( node, (xmlChar*)NUMBER_ATTRIBUTE.c_str(), (xmlChar*)num );
  //        ecXML = new EqualityConstraintXML();

  //        // linkage between the DynamicalSystem and his DynamicalSystemXML
  //        nsds->getEqualityConstraintPtr(i)->setEqualityConstraintXML( ecXML );

  //        // creation of the DynamicalSystemXML
  //        ecXML->updateEqualityConstraintXML( node, nsds->getEqualityConstraintPtr(i) );

  //        equalityConstraintXMLMap[number] = ecXML;
  //      }
  //          else XMLException::selfThrow("NonSmoothDynamicalSystemXML - loadNonSmoothDynamicalSystem | Error : the EqualityConstraint type : " + nsds->getEqualityConstraintPtr(i)->getType() + " doesn't exist!");

  //          /*  end of the save : saving the DynamicalSystem linked to this DSInputOutput */
  //          ecDsioNode = xmlNewChild( node, NULL, (xmlChar*)DSIO_CONCERNED.c_str(), NULL );
  //          for( unsigned int j=0; j<nsds->getEqualityConstraintPtr(i)->getDSInputOutputs().size(); j++)
  //      {
  //        node = xmlNewChild( ecDsioNode, NULL, (xmlChar*)DSINPUTOUTPUT_TAG.c_str(), NULL );
  //        number = nsds->getEqualityConstraintPtr(i)->getDSInputOutput(j)->getNumber();
  //        sprintf(num, "%d", number);
  //        xmlNewProp( node, (xmlChar*)NUMBER_ATTRIBUTE.c_str(), (xmlChar*)num );
  //      }
  //        }
  //      else
  //        {
  //          tmp = num;
  //          XMLException::selfThrow("NonSmoothDynamicalSystemXML - loadNonSmoothDynamicalSystem | Error : the EqualityConstraint number : " + tmp + " already exists!");
  //        }
  //    }
  //      }
  //  }


  //       // creation of the Interaction_Defintion if necessary
  //       if( nsds->getInteractionVectorSize() > 0 )
  //  {
  //    interactionDefinitionNode = SiconosDOMTreeTools::findNodeChild((const xmlNodePtr )rootNode, INTERACTION_DEFINITION_TAG);
  //    if( interactionDefinitionNode == NULL )
  //      interactionDefinitionNode = xmlNewChild(rootNode, NULL, (xmlChar*)INTERACTION_DEFINITION_TAG.c_str(), NULL);

  //    for(i=0; int(i)<nsds->getInteractionVectorSize(); i++)
  //      {
  //        if( nsds->getInteractionPtr(i)->getInteractionXMLPtr() == NULL )
  //    {
  //      number = nsds->getInteractionPtr(i)->getNumber();
  //      sprintf(num, "%d", number);
  //      definedInteractionNumbers.push_back( number );

  //      // verifies if this Dynamical System has a number which not used
  //      itinter = interactionXMLMap.find(number);
  //      if (itinter == interactionXMLMap.end())
  //        {
  //          node = xmlNewChild( interactionDefinitionNode, NULL, (xmlChar*)INTERACTION_TAG.c_str(), NULL );
  //          xmlNewProp( node, (xmlChar*)NUMBER_ATTRIBUTE.c_str(), (xmlChar*)num );
  //          interactionXML = new InteractionXML();

  //          // linkage between the DynamicalSystem and his DynamicalSystemXML
  //          nsds->getInteractionPtr(i)->setInteractionXMLPtr( interactionXML );

  //          // creation of the DynamicalSystemXML
  //          interactionXML->updateInteractionXML( node, nsds->getInteractionPtr(i) );

  //          interactionXMLMap[number] = interactionXML;
  //        }
  //      else
  //        {
  //          tmp = num;
  //          XMLException::selfThrow("NonSmoothDynamicalSystemXML - loadNonSmoothDynamicalSystem | Error : the Interaction number : " + tmp + " already exists!");
  //        }
  //    }
  //      }
  //  }
  //     }
  //   else XMLException::selfThrow("NonSmoothDynamicalSystemXML - loadNonSmoothDynamicalSystem( NonSmoothDynamicalSystem* nsds ) Error : no rootNode is defined.");
}


map<int, DSInputOutputXML*> NonSmoothDynamicalSystemXML::getDSInputOutputXMLRelatingToDS(int number)
{
  map<int, DSInputOutputXML*> m;
  vector<int> v;

  map<int, DSInputOutputXML*>::iterator iter;
  for (iter = dsInputOutputXMLMap.begin(); iter != dsInputOutputXMLMap.end(); iter++)
  {
    v = (*iter).second->getDSConcerned();
    for (unsigned int i = 0; i < v.size(); i++)
    {
      if (v[i] == number)
        m[(*iter).first] = (*iter).second;
    }
  }

  return m;
}

void NonSmoothDynamicalSystemXML::loadEqualityConstraintXML(xmlNodePtr  rootECNode)
{
  //   xmlNodePtr node;
  //   int number; //Number of an EqualityCopnstraint
  //   map<int, EqualityConstraintXML*>::iterator i;

  //   node = SiconosDOMTreeTools::findNodeChild((const xmlNodePtr )rootECNode);

  //   while(node!=NULL)
  //     {
  //       EqualityConstraintXML *ecxml;

  //       number = SiconosDOMTreeTools::getAttributeValue<int>(node, NUMBER_ATTRIBUTE);

  //       i = equalityConstraintXMLMap.find(number);
  //       if (i == equalityConstraintXMLMap.end())
  //  {
  //    definedEqualityConstraintNumbers.push_back(number);
  //    ecxml = new EqualityConstraintXML((xmlNodePtr )node, definedDSNumbers);
  //    equalityConstraintXMLMap[number] = ecxml;
  //  }
  //       else
  //  {
  //    XMLException::selfThrow("NonSmoothDynamicalSystemXML - loadEqualityConstraintXML error : wrong EQUALITYCONSTRAINT number : already exists.");
  //  }

  //       node = SiconosDOMTreeTools::findFollowNode(node);
  //     }
}

void NonSmoothDynamicalSystemXML::loadDSInputOutputXML(xmlNodePtr  rootdsioNode)
{
  xmlNodePtr node;
  int number;
  string type;
  map<int, DSInputOutputXML*>::iterator i;
  DSInputOutputXML *dsioxml;

  node = SiconosDOMTreeTools::findNodeChild((const xmlNodePtr)rootdsioNode);


  while (node != NULL)
  {
    number = SiconosDOMTreeTools::getAttributeValue<int>(node, NUMBER_ATTRIBUTE);
    type = (char*)node->name;

    i = dsInputOutputXMLMap.find(number);
    if (i == dsInputOutputXMLMap.end())
    {
      if (type == LINEAR_DSIO_TAG)
      {
        definedDSInputOutputNumbers.push_back(number);
        dsioxml = new LinearDSIOXML((xmlNodePtr)node/*, definedDSNumbers*/);
        dsInputOutputXMLMap[number] = dsioxml;
      }
      else if (type == NON_LINEAR_DSIO_TAG)
      {
        definedDSInputOutputNumbers.push_back(number);
        dsioxml = new DSInputOutputXML((xmlNodePtr)node/*, definedDSNumbers*/);
        dsInputOutputXMLMap[number] = dsioxml;
      }
      else if (type == LAGRANGIAN_DSIO_TAG)
      {
        definedDSInputOutputNumbers.push_back(number);
        dsioxml = new LagrangianDSIOXML((xmlNodePtr)node/*, definedDSNumbers*/);
        dsInputOutputXMLMap[number] = dsioxml;
      }
      else if (type == LAGRANGIAN_LINEAR_DSIO_TAG)
      {
        definedDSInputOutputNumbers.push_back(number);
        dsioxml = new LagrangianLinearDSIOXML((xmlNodePtr)node/*, definedDSNumbers*/);
        dsInputOutputXMLMap[number] = dsioxml;
      }
      else
        XMLException::selfThrow("NonSmoothDynamicalSystemXML - loadDSInputOutputXML error : wrong DSInputOutput number : already exists.");
    }
    else
    {
      XMLException::selfThrow("NonSmoothDynamicalSystemXML - loadDSInputOutputXML error : wrong DSINPUTOUTPUT number : already exists.");
    }

    node = SiconosDOMTreeTools::findFollowNode(node);
  }
}

void NonSmoothDynamicalSystemXML::updateNonSmoothDynamicalSystemXML(xmlNodePtr  node, NonSmoothDynamicalSystem* nsds)
{
  rootNode = node;
  loadNonSmoothDynamicalSystem(nsds);
}

