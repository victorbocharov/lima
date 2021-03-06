/*
    Copyright 2002-2013 CEA LIST

    This file is part of LIMA.

    LIMA is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    LIMA is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with LIMA.  If not, see <http://www.gnu.org/licenses/>
*/
/***************************************************************************
 *   Copyright (C) 2004 by CEA LIST                       *
 *                                                                         *
 ***************************************************************************/

#ifndef LIMA_COMMON_PROPERTYCODE_XMLPROPERTYHANDLER_H
#define LIMA_COMMON_PROPERTYCODE_XMLPROPERTYHANDLER_H

#include "linguisticProcessing/LinguisticProcessingCommon.h"

#include <QtXml/QXmlDefaultHandler>

#include <string>
#include <vector>
#include <map>

namespace Lima
{
namespace Common
{
namespace PropertyCode
{

class LIMA_PROPERTYCODE_EXPORT XMLPropertyHandler : public QXmlDefaultHandler
{
public:
  // -----------------------------------------------------------------------
  //  Constructors
  // -----------------------------------------------------------------------
  XMLPropertyHandler();
  virtual ~XMLPropertyHandler();


  // -----------------------------------------------------------------------
  //  Implementations of the SAX DocumentHandler interface
  // -----------------------------------------------------------------------
  bool endDocument() override;
  
  bool endElement(const QString & namespaceURI, const QString & localName, const QString & qName) override;
  
  bool startElement(const QString & namespaceURI, const QString & name, const QString & qName, const QXmlAttributes & attributes) override;
  
  
  struct PropertyDescription
  {
    std::string name;
    std::vector<std::string> values;
  };

  struct SubPropertyDescription
  {
    std::string name;
    std::string parentName;
    std::vector<std::pair<std::string,std::vector<std::string> > > values;
  };

  const std::vector<PropertyDescription>& getProperties();
  const std::vector<SubPropertyDescription>& getSubProperties();

private:

  std::vector<PropertyDescription> m_properties;
  std::vector<SubPropertyDescription> m_subproperties;

  enum  PropType {NONE,PROP,SUBPROP};
  PropType m_currentProp;

};

} // closing namespace PropertyCode
} // closing namespace Common
} // closing namespace Lima

#endif
