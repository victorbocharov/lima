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
/************************************************************************
 *
 * @file       DisambiguatedGraphXmlDumper.h
 * @author     Olivier Mesnard (olivier.mesnard@cea.fr)
 * @date       Mon Jul  28 2016
 * copyright   Copyright (C) 2016 by CEA LIST
 * Project     Aymara/Lima
 * 
 * @brief      a dumper that outputs disambiguated graph
 * 
 ***********************************************************************/
#ifndef LIMA_LINGUISTICPROCESSING_DISAMBIGUATEDGRAPHXMLDUMPER_H
#define LIMA_LINGUISTICPROCESSING_DISAMBIGUATEDGRAPHXMLDUMPER_H

#include "linguisticProcessing/core/LinguisticProcessors/AbstractTextualAnalysisDumper.h"
#include "linguisticProcessing/core/LinguisticAnalysisStructure/LinguisticGraph.h"
#include "PosTaggerExport.h"

namespace Lima {
namespace LinguisticProcessing {
namespace PosTagger {

#define DISAMBIGUATEDGRAPHXMLDUMPER_CLASSID "DisambiguatedGraphXmlDumper"
class DisambiguatedGraphXmlFormatter;

class LIMA_POSTAGGER_EXPORT DisambiguatedGraphXmlDumper : public AbstractTextualAnalysisDumper
{
public:
  DisambiguatedGraphXmlDumper();
  virtual ~DisambiguatedGraphXmlDumper();

  void init(Common::XMLConfigurationFiles::GroupConfigurationStructure& unitConfiguration,
            Manager* manager)
  ;

  LimaStatusCode process(AnalysisContent& analysis) const;
    
private:
  DisambiguatedGraphXmlFormatter* m_formatter;
};


}
}
}

#endif
