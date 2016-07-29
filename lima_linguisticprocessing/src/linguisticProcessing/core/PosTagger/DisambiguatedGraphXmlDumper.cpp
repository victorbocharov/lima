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
 *   Copyright (C) 2004-2016 by CEA LIST                              *
 *                                                                         *
 ***************************************************************************/
#include "DisambiguatedGraphXmlDumper.h"
#include "DisambiguatedGraphXmlFormatter.h"
#include "common/time/traceUtils.h"
#include "common/MediaticData/mediaticData.h"
#include "common/XMLConfigurationFiles/xmlConfigurationFileExceptions.h"
#include "common/AbstractFactoryPattern/SimpleFactory.h"
#include "linguisticProcessing/LinguisticProcessingCommon.h"
#include "linguisticProcessing/core/LinguisticProcessors/LinguisticMetaData.h"

#include <boost/algorithm/string/replace.hpp>

using namespace std;
using namespace Lima::Common::MediaticData;
using namespace Lima::Common::XMLConfigurationFiles;

namespace Lima {
namespace LinguisticProcessing {
namespace PosTagger {

SimpleFactory<MediaProcessUnit,DisambiguatedGraphXmlDumper> DisambiguatedGraphXmlDumperFactory(DISAMBIGUATEDGRAPHXMLDUMPER_CLASSID);

DisambiguatedGraphXmlDumper::DisambiguatedGraphXmlDumper():
AbstractTextualAnalysisDumper()
{}


DisambiguatedGraphXmlDumper::~DisambiguatedGraphXmlDumper()
{}

void DisambiguatedGraphXmlDumper::init(Common::XMLConfigurationFiles::GroupConfigurationStructure& unitConfiguration,
                      Manager* manager)

{
  AbstractTextualAnalysisDumper::init(unitConfiguration,manager);

  m_language=manager->getInitializationParameters().media;
  
  m_formatter = new DisambiguatedGraphXmlFormatter( m_language );
  
}

LimaStatusCode DisambiguatedGraphXmlDumper::process(
  AnalysisContent& analysis) const
{
  PTLOGINIT;
  LDEBUG << "DisambiguatedGraphXmlDumper::process";

  LinguisticMetaData* metadata=static_cast<LinguisticMetaData*>(analysis.getData("LinguisticMetaData"));
  if (metadata == 0) {
      LERROR << "no LinguisticMetaData ! abort";
      return MISSING_DATA;
  }

  DumperStream* dstream=initialize(analysis);
  ostream& out=dstream->out();
  
  LimaStatusCode status = m_formatter->process(analysis, out);

  delete dstream;
  return status;
}

} // end namespace
} // end namespace
} // end namespace
