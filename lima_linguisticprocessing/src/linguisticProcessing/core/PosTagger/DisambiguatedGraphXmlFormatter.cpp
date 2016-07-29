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
#include "DisambiguatedGraphXmlFormatter.h"
#include "common/MediaticData/mediaticData.h"
#include "linguisticProcessing/core/LinguisticAnalysisStructure/AnalysisGraph.h"
#include "linguisticProcessing/common/PropertyCode/PropertyManager.h"
#include "common/Data/strwstrtools.h"

using namespace std;
using namespace Lima::LinguisticProcessing::LinguisticAnalysisStructure;
using namespace Lima::Common::Misc;

namespace Lima
{

namespace LinguisticProcessing
{

namespace PosTagger
{

DisambiguatedGraphXmlFormatter::DisambiguatedGraphXmlFormatter( const MediaId& language )
: m_language(language)
{
  m_macroManager=&(static_cast<const Common::MediaticData::LanguageData&>(Common::MediaticData::MediaticData::single().mediaData(m_language)).getPropertyCodeManager().getPropertyManager("MACRO"));
  m_microManager=&(static_cast<const Common::MediaticData::LanguageData&>(Common::MediaticData::MediaticData::single().mediaData(m_language)).getPropertyCodeManager().getPropertyManager("MICRO"));
  
}

LimaStatusCode DisambiguatedGraphXmlFormatter::process(AnalysisContent& analysis, ostream& out) const
{
  AnalysisGraph* posTokenList=static_cast<AnalysisGraph*>(analysis.getData("PosGraph"));
  const LinguisticGraph* posGraph=posTokenList->getGraph();
  
  const FsaStringsPool& sp=Common::MediaticData::MediaticData::single().stringsPool(m_language);

  out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << endl;
  out << "<vertices>" << endl;

  LinguisticGraphInEdgeIt inItr,inItrEnd;
  LinguisticGraphVertexIt vxItr,vxItrEnd;
  boost::tie(vxItr,vxItrEnd) = vertices(*posGraph);
  for (;vxItr!=vxItrEnd;vxItr++)
  {
    MorphoSyntacticData* dw=get(vertex_data,*posGraph,*vxItr);
    Token* ft=get(vertex_token,*posGraph,*vxItr);

    if (ft == 0 || dw == 0)
    {
      continue;
    }

    uint64_t pos=0;
    uint64_t len=0;
    if (ft!=0)
    {
      pos=ft->position();
      len=ft->length();
    }

    LinguisticCode macro = dw->firstValue(m_macroManager->getPropertyAccessor());
    LinguisticCode micro = dw->firstValue(m_microManager->getPropertyAccessor());

    std::string smacro = m_macroManager->getPropertySymbolicValue(macro);
    std::string smicro = m_microManager->getPropertySymbolicValue(micro);

    out << "<vertex id=\"" << *vxItr << "\" position=\"" << pos << "\" length=\"" << len << "\" >" << endl;
    std::set<StringsPoolIndex> lemmas = dw->allLemma();
    std::set<StringsPoolIndex>::const_iterator lemmaIt, lemmaIt_end;
    lemmaIt = lemmas.begin(); lemmaIt_end = lemmas.end();
    for (; lemmaIt != lemmaIt_end; lemmaIt++)
    {
      out << "  <lemma>" << limastring2utf8stdstring(sp[*lemmaIt]) << "</lemma>" << endl;
    }
    out << "  <macro>" << smacro << "</macro>" << endl;
    out << "  <micro>" << smicro << "</micro>" << endl;
    boost::tie(inItr,inItrEnd) = in_edges(*vxItr,*posGraph);
    for (;inItr!=inItrEnd;inItr++)
    {
      out << "  <pred id=\"" << source(*inItr,*posGraph) << "\"/>" << endl;
    }
    out << "</vertex>" << endl;
  }
  out << "</vertices>" << endl;

  return SUCCESS_ID;
}


}

}

}
