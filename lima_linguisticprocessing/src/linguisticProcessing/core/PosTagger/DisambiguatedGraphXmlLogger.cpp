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
 *   Copyright (C) 2004-2012 by CEA LIST                              *
 *                                                                         *
 ***************************************************************************/
#include "DisambiguatedGraphXmlLogger.h"
#include "DisambiguatedGraphXmlFormatter.h"
#include "common/AbstractFactoryPattern/SimpleFactory.h"
#include "linguisticProcessing/core/LinguisticProcessors/LinguisticMetaData.h"
#include "linguisticProcessing/core/LinguisticResources/LinguisticResources.h"
#include "linguisticProcessing/core/LinguisticAnalysisStructure/AnalysisGraph.h"
// #include "linguisticProcessing/core/Graph/phoenixGraph.h"
#include "common/Data/strwstrtools.h"
#include "common/time/traceUtils.h"
#include "common/MediaticData/mediaticData.h"
#include "linguisticProcessing/common/PropertyCode/PropertyManager.h"
#include "common/misc/fsaStringsPool.h"

#include "linguisticProcessing/core/LinguisticAnalysisStructure/MorphoSyntacticData.h"
#include "linguisticProcessing/core/LinguisticAnalysisStructure/Token.h"


#include <iostream>


#include <fstream>

using namespace std;
using namespace Lima::LinguisticProcessing::LinguisticAnalysisStructure;
using namespace Lima::Common::Misc;

namespace Lima
{

namespace LinguisticProcessing
{

namespace PosTagger
{

SimpleFactory<MediaProcessUnit,DisambiguatedGraphXmlLogger> disambiguatedGraphXmlLoggerFactory("DisambiguatedGraphXmlLogger");

DisambiguatedGraphXmlLogger::DisambiguatedGraphXmlLogger()
: AbstractLinguisticLogger(".output.xml")
{}


DisambiguatedGraphXmlLogger::~DisambiguatedGraphXmlLogger()
{
  delete m_formatter;
}

void DisambiguatedGraphXmlLogger::init(
  Common::XMLConfigurationFiles::GroupConfigurationStructure& unitConfiguration,
  Manager* manager)

{
  /** @addtogroup ProcessUnitConfiguration
   * - <b>&lt;group name="..." class="DictionaryCode"&gt;</b>
   *    -  dictionaryCode : DictionaryCode resource
   */

  AbstractLinguisticLogger::init(unitConfiguration,manager);

  m_language=manager->getInitializationParameters().media;
  m_formatter = new DisambiguatedGraphXmlFormatter( m_language );

}

LimaStatusCode DisambiguatedGraphXmlLogger::process(
  AnalysisContent& analysis) const
{
  TimeUtils::updateCurrentTime();
  LinguisticMetaData* metadata=static_cast<LinguisticMetaData*>(analysis.getData("LinguisticMetaData"));
  if (metadata == 0)
  {
    PTLOGINIT;
    LERROR << "no LinguisticMetaData ! abort";
    return MISSING_DATA;
  }

  ofstream out;
  if (!openLogFile(out,metadata->getMetaData("FileName")))
  {
    PTLOGINIT;
    LERROR << "Can't open log file ";
    return CANNOT_OPEN_FILE_ERROR;
  }
  
  LimaStatusCode status = m_formatter->process(analysis, out);

  out.close();

  TimeUtils::logElapsedTime("DisambiguatedGraphXmlLogger");

  return status;
}


}

}

}
