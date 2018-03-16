/*************************************************************************
 *     rMSI - R package for MSI data processing
 *     Copyright (C) 2018 Pere Rafols Soler
 * 
 *     This program is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation, either version 3 of the License, or
 *     (at your option) any later version.
 * 
 *     This program is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU General Public License for more details.
 * 
 *     You should have received a copy of the GNU General Public License
 *     along with this program.  If not, see <http://www.gnu.org/licenses/>.
 **************************************************************************/

#include "pugixml.hpp"
#include <Rcpp.h>
#include <string>
#include <cstring>
#include <algorithm> //To get the transform() function
#include <cmath>
using namespace Rcpp;
using namespace pugi;

// imzML resulting data structure:
// $UUID a String object with the UUID
// $continuous_mode a boolean which is true if spectra in continuous mode
// $compression_mz a boolean indicating wheher data is compressed or not
// $compression_int a boolean indicating wheher data is compressed or not
// $mz_dataType a String with the data type: "float", "int"... etc...
// $int_dataType a String with the data type: "float", "int"... etc...
// $pixel_size_um a double with the pixel area? check this...
// $run_data a data.frane with the columns: x, y, mzLength, mzOffset, intLength, intOffset
//
// full path to xml_path must be specified... R function path.expand() can be used 4 this
// [[Rcpp::export]]
List CimzMLParse( String xml_path )
{
  xml_document doc;
  xml_parse_result  result = doc.load_file(xml_path.get_cstring());
  
  if (!result)
  {
    Rcout << "XML [" << xml_path.get_cstring() << "] parsed with errors, attr value: [" << doc.child("node").attribute("attr").value() << "]" << std::endl;
    Rcout << "Error description: " << result.description() << std::endl;
    Rcout << "Error offset: " << result.offset << " (error at [..." << (xml_path.get_cstring() + result.offset) << "]" << std::endl;
    return( List::create(Named("Error") = "XML parse error") );
  }
  
  xml_node mzML = doc.child("mzML");
  if( mzML == NULL )
  {
    return( List::create(Named("Error") = "XML parse error: no mzML node found") );
  }
  
  xml_node fileDesc = mzML.child("fileDescription");
  if( fileDesc == NULL )
  {
    return( List::create(Named("Error") = "XML parse error: no fileDescription node found") );
  } 
  
  xml_node fileCont = fileDesc.child("fileContent");
  if( fileCont == NULL )
  {
    return( List::create(Named("Error") = "XML parse error: no fileContent node found") );
  }
  
  // Data fields
  String sUUID = "";
  String sMD5_Checksum = "";
  String sSHA_Checksum = "";
  bool bContinuous;
  bool bCompressionMz = true; //an error will be raised if this reamains true by the end of xml parsing
  bool bCompressionInt = true; //an error will be raised if this reamains true by the end of xml parsing
  String sMzDataType = "";
  String sIntDataType = "";
  double dPixelSize = 0.0;
  
  // Error control
  bool bUUID_present = false;
  bool bChecksum_present = false;
  bool bDataMode_present = false;
  
  
  std::string accession; //Accession to XML nodes...
  std::string id; //Id accession field used for every xml accession
  std::string value; //Value of item used for every xml accession
  for (xml_node cvParam = fileCont.child("cvParam"); cvParam; cvParam = cvParam.next_sibling("cvParam"))
  {
    accession = cvParam.attribute("accession").value();
    value = cvParam.attribute("value").value();
    
    if(accession == "IMS:1000080")
    {
      //Parse the UUID to get just the hex representation in a string (without dashes and {})
      std::size_t ipos = value.find('{');
      if( ipos != std::string::npos )
      {
        value.erase(ipos, 1);
      }
      do
      {
        ipos = value.find('-'); 
        if( ipos != std::string::npos )
        {
          value.erase(ipos, 1);
        } 
      } while ( ipos != std::string::npos);
      ipos = value.find('}');
      if( ipos != std::string::npos )
      {
        value.erase(ipos, 1);
      }
      for( int i=0; i < value.length(); i++)
      {
        value[i] = toupper(value[i]);
      }
      sUUID = value;
      bUUID_present = true;
    }
    if( accession == "IMS:1000091") //SHA Checksum
    {
      transform(value.begin(), value.end(), value.begin(),::toupper);
      sSHA_Checksum = value;
      bChecksum_present = true;
    }
    if( accession == "IMS:1000090") //MD5 Checksum
    {
      transform(value.begin(), value.end(), value.begin(),::toupper);
      sSHA_Checksum = value;
      bChecksum_present = true;
    }
    if( accession == "IMS:1000030")
    {
      bContinuous = true;
      bDataMode_present = true;
    }
    if( accession == "IMS:1000031")
    {
      bContinuous = false;
      bDataMode_present = true;
    }
  }
  
  //Error handling
  if( !bUUID_present )
  {
    return( List::create(Named("Error") = "imzML parse error: UUID not present") );
  }
  if( !bChecksum_present )
  {
    return( List::create(Named("Error") = "imzML parse error: Checksum not present") );
  }
  if( !bDataMode_present )
  {
    return( List::create(Named("Error") = "imzML parse error: Data mode continuous/processed not present") );
  }

  //referenceableParamGroupList
  xml_node refParamGrpLst = mzML.child("referenceableParamGroupList");
  if( refParamGrpLst == NULL )
  {
    return( List::create(Named("Error") = "XML parse error: no referenceableParamGroupList node found") );
  }
  for (xml_node refParamGrp = refParamGrpLst.child("referenceableParamGroup"); refParamGrp; refParamGrp = refParamGrp.next_sibling("referenceableParamGroup"))
  {
    id = refParamGrp.attribute("id").value();
    if( id == "mzArray" || id == "intensityArray" || id == "intensities" )
    {
      for (xml_node cvParam = refParamGrp.child("cvParam"); cvParam; cvParam = cvParam.next_sibling("cvParam"))
      {
        accession = cvParam.attribute("accession").value();
        
        //No compression
        if( accession == "MS:1000576" &&  id == "mzArray")
        {
          bCompressionMz = false;
        }
        if( accession == "MS:1000576" &&  (id == "intensityArray" || id == "intensities") )
        {
          bCompressionInt = false;
        }
        
        //Check if mass is in valid units
        if( id == "mzArray" && accession == "MS:1000514" )
        {
          if( strcmp(cvParam.attribute("unitAccession").value(), "MS:1000040") != 0 )
          {
            return( List::create(Named("Error") = "m/z Array is not in m/z format, rMSI only supports m/z units") );
          }
        }
       
        if( accession == "IMS:1000101") //External data
        {
          if( !cvParam.attribute("value").as_bool() )
          {
            return( List::create(Named("Error") = "imzML parse error: data must be external (stored in the ibd file)") );
          }
        }
        
        //8 bits integer data array
        //TODO This is not encoded in mzML CV obo file but is specified as valid in imzML spec!
                
        //16 bits integer data array
        //TODO This is not encoded in mzML CV obo file but is specified as valid in imzML spec!
        
        //32 bits integer data array
        if(accession == "IMS:1000141" &&  id == "mzArray")
        {
          sMzDataType = "int";
        }
        if(accession == "IMS:1000141" &&  (id == "intensityArray" || id == "intensities"))
        {
          sIntDataType = "int";
        }
        
        //64 bits integer data array
        if(accession == "IMS:1000142" &&  id == "mzArray")
        {
          sMzDataType = "long";
        }
        if(accession == "IMS:1000142" && (id == "intensityArray" || id == "intensities"))
        {
          sIntDataType = "long";
        }
        
        //32 bits float data array
        if(accession == "MS:1000521" &&  id == "mzArray")
        {
          sMzDataType = "float";
        }
        if(accession == "MS:1000521" &&  (id == "intensityArray" || id == "intensities"))
        {
          sIntDataType = "float";
        }
      
        //64 bits float data array
        if(accession == "MS:1000523" &&  id == "mzArray")
        {
          sMzDataType = "double";
        }
        if(accession == "MS:1000523" &&  (id == "intensityArray" || id == "intensities"))
        {
          sIntDataType = "double";
        }
      }
    }
  }
  
  //Error handling
  if( bCompressionInt || bCompressionMz )
  {
    return( List::create(Named("Error") = "imzML parse error: Data must not be compressed in order to be loaded in rMSI") );
  }
  if( strcmp(sMzDataType.get_cstring(), "") == 0)
  {
    return( List::create(Named("Error") = "imzML parse error: No mass data type found") );
  }
  if( strcmp(sIntDataType.get_cstring(), "") == 0)
  {
    return( List::create(Named("Error") = "imzML parse error: No intensity data type found") );
  }

  //Get the pixel size
  xml_node scanSetLst = mzML.child("scanSettingsList");
  if( scanSetLst == NULL )
  {
    return( List::create(Named("Error") = "XML parse error: no scanSettingsList node found") );
  }
  xml_node scanSet = scanSetLst.child("scanSettings");
  if( scanSet == NULL )
  {
    return( List::create(Named("Error") = "XML parse error: no scanSettings node found") );
  }
  for (xml_node cvParam = scanSet.child("cvParam"); cvParam; cvParam = cvParam.next_sibling("cvParam"))
  {
    accession = cvParam.attribute("accession").value();
    if( accession == "IMS:1000046")
    {
      dPixelSize = sqrt( cvParam.attribute("value").as_double());
    }
  }
  
  //Error handling for pixel size
  if( dPixelSize == 0.0)
  {
    return( List::create(Named("Error") = "imzML parse error: No pixel size found") );
  }

  //Parse binary offsets array  
  xml_node runNode = mzML.child("run");
  if( runNode == NULL )
  {
    return( List::create(Named("Error") = "XML parse error: no run node found") );
  }
  xml_node specLst = runNode.child("spectrumList");
  if( specLst == NULL )
  {
    return( List::create(Named("Error") = "XML parse error: no spectrumList node found") );
  }
  
  //Get the total number of pixels as the numbe rof items in spcLst
  const int num_of_pixels = specLst.select_nodes("spectrum").size();
  NumericVector iPos_x(num_of_pixels);
  NumericVector iPos_y(num_of_pixels);
  NumericVector imzLength(num_of_pixels);
  NumericVector imzOffset(num_of_pixels);
  NumericVector iintLength(num_of_pixels);
  NumericVector iintOffset(num_of_pixels);
  
  //Pointers to fast acces to imzLength, imzOffset, iintLength and iintOffset
  NumericVector *ptr_iLength;
  NumericVector *ptr_iOffsets;
  
  int iPixel = 0; //pixel iterator
  bool byMissing, bxMissing;
  bool bArrayPtrNotSet, bLengthMissing, bOffsetMissing;
  for (xml_node spectrum = specLst.child("spectrum"); spectrum; spectrum = spectrum.next_sibling("spectrum")) //For each pixel in the dataset
  {
    //Error handling for iPixel overflow
    if( iPixel >= num_of_pixels)
    {
      return( List::create(Named("Error") = "XML parse error: Pixel counter overflow") );
    }
    
    //Parse pixel coordinates
    bxMissing = true;
    byMissing = true;
    xml_node scan = spectrum.child("scanList").child("scan");
    for (xml_node cvParam = scan.child("cvParam"); cvParam; cvParam = cvParam.next_sibling("cvParam"))
    {
      accession = cvParam.attribute("accession").value();
      if( accession == "IMS:1000050" )
      {
        iPos_x[iPixel] = cvParam.attribute("value").as_double(); 
        bxMissing = false;
      }
      if( accession == "IMS:1000051" )
      {
        iPos_y[iPixel] = cvParam.attribute("value").as_double();
        byMissing = false;
      }
    }
    
    //Error handling for missing XY values
    if( bxMissing || byMissing )
    {
      return( List::create(Named("Error") = "XML parse error: Some pixel is missing coordinates") );
    }
    
    //Parse arrays offsets
    bArrayPtrNotSet = true;
    xml_node binArrayLst = spectrum.child("binaryDataArrayList");
    for (xml_node binArray = binArrayLst.child("binaryDataArray"); binArray; binArray = binArray.next_sibling("binaryDataArray"))
    {
      std::string sArrayType = binArray.child("referenceableParamGroupRef").attribute("ref").value();
      if( sArrayType == "mzArray" )
      {
        ptr_iLength = &imzLength;
        ptr_iOffsets = &imzOffset;
        bArrayPtrNotSet = false;
      }
      if( sArrayType == "intensityArray" || sArrayType == "intensities")
      {
        ptr_iLength = &iintLength;
        ptr_iOffsets = &iintOffset;
        bArrayPtrNotSet = false;
      }
      
      //Error handling no ptr set so no data available
      if( bArrayPtrNotSet )
      {
        return( List::create(Named("Error") = "XML parse error: Missing binary data reference") );
      }
      
      bLengthMissing = true;
      bOffsetMissing = true;
      for (xml_node cvParam = binArray.child("cvParam"); cvParam; cvParam = cvParam.next_sibling("cvParam"))
      {
        accession = cvParam.attribute("accession").value();
        if( accession == "IMS:1000103")
        {
          (*ptr_iLength)[iPixel] = cvParam.attribute("value").as_double();
          bLengthMissing = false;
        }
        if( accession == "IMS:1000102")
        {
          (*ptr_iOffsets)[iPixel] = cvParam.attribute("value").as_double();
          bOffsetMissing = false;
        }
      }
      
      //Error handling no ptr set so no data available
      if( bLengthMissing ||  bOffsetMissing)
      {
        return( List::create(Named("Error") = "XML parse error: Missing binary data assignment") );
      }
      
    }
    
    iPixel++;
  }
  
  return List::create(Named("UUID") = sUUID , 
                      Named("SHA") = sSHA_Checksum,
                      Named("MD5") = sMD5_Checksum,
                      Named("continuous_mode")= bContinuous,
                      Named("compression_mz")= bCompressionMz,
                      Named("compression_int")= bCompressionInt,
                      Named("mz_dataType") = sMzDataType,
                      Named("int_dataType") = sIntDataType,
                      Named("pixel_size_um") = dPixelSize,
                      Named("run_data") = DataFrame::create( Named("x") = iPos_x,
                                                             Named("y") = iPos_y, 
                                                             Named("mzLength") = imzLength, 
                                                             Named("mzOffset") = imzOffset, 
                                                             Named("intLength") = iintLength, 
                                                             Named("intOffset") = iintOffset)
                      );
}

// imzML imgInfo data structure:
// $UUID a String object with the UUID
// $continuous_mode a boolean which is true if spectra in continuous mode
// $compression_mz a boolean indicating wheher data is compressed or not
// $compression_int a boolean indicating wheher data is compressed or not
// $mz_dataType a String with the data type: "float", "int"... etc...
// $int_dataType a String with the data type: "float", "int"... etc...
// $pixel_size_um a double with the pixel area? check this...
// $run_data a data.frane with the columns: x, y, mzLength, mzOffset, intLength, intOffset
//
// full path to xml_path must be specified... R function path.expand() can be used 4 this
// [[Rcpp::export]]
bool CimzMLStore( String fname, List imgInfo )
{
  //Reusable pugi variables
  pugi::xml_node cvParam;
  
  // empty xml document with custom declaration node
  pugi::xml_document doc;
  pugi::xml_node decl = doc.prepend_child(pugi::node_declaration);
  decl.append_attribute("version") = "1.0";
  decl.append_attribute("encoding") = "UTF-8";
  decl.append_attribute("standalone") = "no";
  
  //mzML top level node
  pugi::xml_node node_mzML = doc.append_child("mzML");
  node_mzML.append_attribute("version") =  "1.1";
  node_mzML.append_attribute("xmlns") = "http://psi.hupo.org/ms/mzml";
  node_mzML.append_attribute("xmlns:xsi") = "http://www.w3.org/2001/XMLSchema-instance";
  node_mzML.append_attribute("xsi:schemaLocation") = "http://psi.hupo.org/ms/mzml http://psidev.info/files/ms/mzML/xsd/mzML1.1.0_idx.xsd";

  //cvList top node
  pugi::xml_node node_cvList = node_mzML.append_child("cvList");
  node_cvList.append_attribute("count") = "3";
  pugi::xml_node node_cv = node_cvList.append_child("cv");
  node_cv.append_attribute("id") = "MS";
  node_cv.append_attribute("fullName") = "Proteomics Standards Initiative Mass Spectrometry Ontology";
  node_cv.append_attribute("version") = "1.3.1";
  node_cv.append_attribute("URI") = "http://psidev.info/ms/mzML/psi-ms.obo";
  node_cv = node_cvList.append_child("cv");
  node_cv.append_attribute("id") = "UO";
  node_cv.append_attribute("fullName") = "Unit Ontology";
  node_cv.append_attribute("version") = "1.15";
  node_cv.append_attribute("URI") = "http://obo.cvs.sourceforge.net/obo/obo/ontology/phenotype/unit.obo";
  node_cv = node_cvList.append_child("cv");
  node_cv.append_attribute("id") = "IMS";
  node_cv.append_attribute("fullName") = "Imaging MS Ontology";
  node_cv.append_attribute("version") = "0.9.1";
  node_cv.append_attribute("URI") = "http://www.maldi-msi.org/download/imzml/imagingMS.obo";
  
  //fileDescription node
  pugi::xml_node node_fdesc = node_mzML.append_child("fileDescription");
  
  //fileContent
  pugi::xml_node fileContent = node_fdesc.append_child("fileContent");
  cvParam = fileContent.append_child("cvParam");
  cvParam.append_attribute("accession") = "IMS:1000080";
  cvParam.append_attribute("cvRef") = "IMS";
  cvParam.append_attribute("name") = "universally unique identifier";
  cvParam.append_attribute("value") = "{906c8753-47ae-43ee-8fa8-f00e6e9288fa}"; //TODO set UUID from input data List parsing the string to add the {} chars
  
  /*
  //TODO i'm working here...
    <cvParam accession="IMS:1000031" cvRef="IMS" name="processed"/>
    <cvParam accession="IMS:1000090" cvRef="IMS" name="ibd MD5" value="8783D5A6FA45448806D3D871B99DE3F"/>
    <cvParam accession="MS:1000294" cvRef="MS" name="mass spectrum"/>
  */

  
  // save document to file
  return(doc.save_file(fname.get_cstring(), "\t", pugi::format_default, pugi::encoding_utf8 ) );
}
