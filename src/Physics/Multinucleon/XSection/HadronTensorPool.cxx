//____________________________________________________________________________
/*
 Copyright (c) 2003-2018, The GENIE Collaboration
 For the full text of the license visit http://copyright.genie-mc.org
 or see $GENIE/LICENSE

 Author: Steven Gardiner <gardiner \at fnal.gov>
         Fermi National Accelerator Laboratory

 For the class documentation see the corresponding header file.

*/
//____________________________________________________________________________

// standard library includes
#include <cstdlib>
#include <fstream>
#include <string>

// libxml2 includes
#include "libxml/parser.h"
#include "libxml/xmlmemory.h"

// ROOT includes
#include <TSystem.h>

// GENIE includes
#include "Framework/Messenger/Messenger.h"
#include "Physics/Multinucleon/XSection/HadronTensorPool.h"
#include "Physics/Multinucleon/XSection/TabulatedValenciaHadronTensor.h"
#include "Physics/Multinucleon/XSection/ValenciaHadronTensorI.h"
#include "Framework/Utils/StringUtils.h"
#include "Framework/Utils/XmlParserUtils.h"

namespace {

  /// Helper function that retrieves an attribute of an xmlNodePtr and trims
  /// whitespace before returning it
  std::string get_trimmed_attribute(const xmlNodePtr& node,
    const std::string& name)
  {
    return genie::utils::str::TrimSpaces(
      genie::utils::xml::GetAttribute(node, name.c_str()));
  }

  /// Converts a string to a bool value. If the string is not "true" or
  /// "false", returns false and sets the ok flag to false
  bool string_to_bool(const std::string& str, bool& ok) {
    if (str == "true") return true;
    else if (str == "false") return false;

    ok = false;
    return false;
  }


  /// Converts a string to a genie::HadronTensorType_t value. If the string
  /// does not correspond to a valid tensor type, kHT_Undefined
  /// is returned, and the ok flag is set to false
  genie::HadronTensorType_t string_to_tensor_type(const std::string& str,
    bool& ok)
  {
    if (str == "MEC_FullAll") return genie::HadronTensorType::kHT_MEC_FullAll;
    else if (str == "MEC_Fullpn")
      return genie::HadronTensorType::kHT_MEC_Fullpn;
    else if (str == "MEC_DeltaAll")
      return genie::HadronTensorType::kHT_MEC_DeltaAll;
    else if (str == "MEC_Deltapn")
      return genie::HadronTensorType::kHT_MEC_Deltapn;
    else {
      ok = false;
      return genie::HadronTensorType::kHT_Undefined;
    }
  }

  /// Returns true if a given file exists and is accessible, or false otherwise
  bool file_exists(const std::string& file_name) {
    return std::ifstream(file_name).good();
  }

  /// Helper function for comparing xmlNode names and string literals. It helps
  /// to reduce the number of ugly casts needed in the XML parsing code.
  bool name_equal(const xmlNodePtr& np, const std::string& str) {
    return xmlStrEqual( np->name,
      reinterpret_cast<const xmlChar*>(str.c_str()) );
  }

  /// Helper function that converts the content of an xmlNode to a std::string
  std::string get_node_content(const xmlNodePtr& np) {
    return reinterpret_cast<const char*>( xmlNodeGetContent(np) );
  }
}

//____________________________________________________________________________
genie::HadronTensorPool::HadronTensorPool()
{
  bool init_ok = LoadConfig();
  if ( !init_ok ) LOG("HadronTensorPool", pERROR) << "Failed to initialize"
    " the HadronTensorPool";
}

//____________________________________________________________________________
genie::HadronTensorPool::~HadronTensorPool()
{
  std::map< std::pair<int, genie::HadronTensorType_t>, ValenciaHadronTensorI* >
    ::iterator it;
  for(it = fTensors.begin(); it != fTensors.end(); ++it) {
    ValenciaHadronTensorI* t = it->second;
    if (t) delete t;
  }
  fTensors.clear();
}
//____________________________________________________________________________
genie::HadronTensorPool& genie::HadronTensorPool::Instance()
{
  static HadronTensorPool singleton_instance;
  return singleton_instance;
}

//____________________________________________________________________________
const genie::ValenciaHadronTensorI* genie::HadronTensorPool::GetTensor(
  int tensor_pdg, genie::HadronTensorType_t type)
{
  std::pair<int, genie::HadronTensorType_t> temp_pair(tensor_pdg, type);
  if ( !fTensors.count(temp_pair) ) return NULL;
  else return fTensors.at(temp_pair);
}

//____________________________________________________________________________
bool genie::HadronTensorPool::LoadConfig(void)
{
  bool ok = true;

  // Find the XML configuration file
  std::string filename = genie::utils::xml::GetXMLFilePath("HadronTensors.xml");

  LOG("HadronTensorPool", pINFO)  << "Loading hadron tensors from the file "
    << filename;

  if ( file_exists(filename) ) {
    XmlParserStatus_t status = this->ParseXMLConfig(filename);
    if (status != kXmlOK) {
      LOG("HadronTensorPool", pWARN) << "Error encountered while"
        << " attempting to parse the XMl file \"" << filename << "\"."
        << " XML parser status: " << XmlParserStatus::AsString(status);
      ok = false;
    }
  }
  else {
    LOG("HadronTensorPool", pWARN) << "Could not read from the file: "
      << filename;
    ok = false;
  }
  return ok;
};

//____________________________________________________________________________
std::string genie::HadronTensorPool::FindTensorTableFile(
  const std::string& basename, bool& ok) const
{
  for (size_t p = 0; p < fDataPaths.size(); ++p) {
    const std::string& path = fDataPaths.at(p);
    std::string full_name = path + '/' + basename;
    if ( file_exists(full_name) ) return full_name;
  }

  // A matching file could not be found
  ok = false;
  return std::string();
}

//____________________________________________________________________________
genie::XmlParserStatus_t genie::HadronTensorPool::ParseXMLConfig(
  const std::string& filename, const std::string& table_to_use)
{
  LOG("HadronTensorPool", pDEBUG) << "Reading XML file: " << filename;

  xmlDocPtr xml_doc = xmlParseFile( filename.c_str() );
  if ( !xml_doc ) return kXmlNotParsed;

  xmlNodePtr xml_root = xmlDocGetRootElement( xml_doc );
  if ( !xml_root )
  {
    xmlFreeDoc(xml_doc);
    return kXmlEmpty;
  }

  if ( !name_equal(xml_root, "hadron_tensor_config") )
  {
    LOG("HadronTensorPool", pERROR) << "Missing <hadron_tensor_config>"
      << " tag in the configuration file " << filename;
    xmlFreeDoc(xml_doc);
    return kXmlInvalidRoot;
  }

  xmlNodePtr xml_tensor_table = xml_root->xmlChildrenNode;

  // Flag that indicates whether the requested table of hadron tensors
  // could be found
  bool found_table = false;

  // loop over <tensor_table> and <data_paths> nodes
  while (xml_tensor_table) {

    if ( name_equal(xml_tensor_table, "tensor_table") ) {

      // Load only the requested tensor table (this allows for multiple tables
      // with different names to be placed in the same XML configuration file)
      std::string table_name = get_trimmed_attribute(xml_tensor_table, "name");
      if ( table_name != table_to_use ) {
        xml_tensor_table = xml_tensor_table->next;
        continue;
      }
      else found_table = true;

      // loop over the <data_paths> entries in the tensor table
      xmlNodePtr xml_data_paths = xml_tensor_table->xmlChildrenNode;
      while ( xml_data_paths ) {
        if ( name_equal(xml_data_paths, "data_paths") ) {
          xmlNodePtr xml_path = xml_data_paths->xmlChildrenNode;

          while ( xml_path ) {
            if ( name_equal(xml_path, "path") ) {
              std::string path = genie::utils::str::TrimSpaces(
                get_node_content(xml_path));

              std::string path_type = get_trimmed_attribute(xml_path, "type");
              if (path_type == "relative") {
                // paths may be specified relative to the $GENIE folder
                path = std::string( gSystem->Getenv("GENIE") ) + '/' + path;
              }

              LOG("HadronTensorPool", pINFO) << "The HadronTensorPool will"
                " search for data files in " << path;
              fDataPaths.push_back( path );
            } // <x> == <path>
            xml_path = xml_path->next;
          } // <path> loop
        } // <x> == <data_paths>
        xml_data_paths = xml_data_paths->next;
      } // <data_paths> loop

      xmlNodePtr xml_nuclide = xml_tensor_table->xmlChildrenNode;

      // loop over the <nuclide> entries in the tensor table
      while ( xml_nuclide ) {
        if ( name_equal(xml_nuclide, "nuclide") ) {

          std::string pdg_str = get_trimmed_attribute(xml_nuclide, "pdg");

          LOG("HadronTensorPool", pDEBUG) << "Reading hadron tensor"
            << " configuration for nuclide " << pdg_str;

          int pdg = std::atoi( pdg_str.c_str() );

          xmlNodePtr xml_tensor = xml_nuclide->xmlChildrenNode;

          std::string type_str("unknown");

          while (xml_tensor) {
            if ( name_equal(xml_tensor, "tensor") ) {

              // Flag to indicate if there was a problem processing
              // the record for the current tensor
              bool tensor_ok = true;

              type_str = get_trimmed_attribute(xml_tensor, "type");

              genie::HadronTensorType_t type
                = string_to_tensor_type(type_str, tensor_ok);

              std::string calc_str
                = get_trimmed_attribute(xml_tensor, "calc");

              if (tensor_ok && calc_str == "table") {

                // Tensor values are represented using a 2D grid that is
                // stored in a data file
                xmlNodePtr xml_file = xml_tensor->xmlChildrenNode;

                tensor_ok = false;

                while (xml_file) {
                  if ( name_equal(xml_file, "file") ) {
                    std::string file_name = genie::utils::str::TrimSpaces(
                      get_node_content(xml_file));
                    // Set the ok flag to true. This will be reversed if
                    // it turns out that the file does not exist
                    tensor_ok = true;

                    std::string full_file_name
                      = FindTensorTableFile(file_name, tensor_ok);

                    std::pair<int, genie::HadronTensorType_t>
                      tensor_id(pdg, type);

                    // Do things this way rather than calling std::map::insert()
                    // to avoid allocating space for a new tensor object
                    // when one is not needed.
                    if ( !this->fTensors.count(tensor_id) ) {
                      LOG("HadronTensorPool", pDEBUG) << "Loading the hadron"
                        << " tensor data file " << full_file_name;
                      fTensors[tensor_id]
                        = new TabulatedValenciaHadronTensor(full_file_name);
                    }
                    else {
                      tensor_ok = false;
                      LOG("HadronTensorPool", pWARN) << "A hadron tensor for"
                        << " nuclide " << pdg_str << " and type " << type_str
                        << " has already been defined.";
                    }
                  } // <x> == <file>
                  xml_file = xml_file->next;
                } // <file> loop
              } // calc == "table"
              else tensor_ok = false;

              if ( !tensor_ok ) {
                LOG("HadronTensorPool", pWARN) << "Ignoring hadron tensor"
                  " for nuclide " << pdg_str << " of type " << type_str << '\n';
              }
            } // <x> == <tensor>
            xml_tensor = xml_tensor->next;
          } // <tensor> loop
        } // <x> == <nuclide>
        xml_nuclide = xml_nuclide->next;
      } // <nuclide> loop
    } // <x> == <tensor_table>
    xml_tensor_table = xml_tensor_table->next;
  } // <tensor_table> loop

  if ( !found_table ) LOG("HadronTensorPool", pERROR) << "Could not find"
    " a hadron tensor table named \"" << table_to_use << "\" in the XML"
    " configuration file " << filename;

  //xmlFreeNode(xml_tensor_table);
  xmlFreeDoc(xml_doc);

  return kXmlOK;
}
