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

// NAUTITIA
//
// jys 8-OCT-2002
//
// SimpleWord is the implementation of the 1st module of
// Morphological Analysis. Each token from the main tokens
// path is searched into the specified dictionary.

#ifndef LIMA_MORPHOLOGICANALYSIS_APPROXSTRINGMATCHER_H
#define LIMA_MORPHOLOGICANALYSIS_APPROXSTRINGMATCHER_H

#include "linguisticProcessing/core/MorphologicAnalysis/MorphologicAnalysisExport.h"
#include "linguisticProcessing/core/LinguisticAnalysisStructure/LinguisticGraph.h"
#include "linguisticProcessing/core/FlatTokenizer/CharChart.h"
#include "common/MediaProcessors/MediaProcessUnit.h"
#include "common/AbstractFactoryPattern/SimpleFactory.h"
#include "linguisticProcessing/core/LinguisticResources/AbstractAccessResource.h"
#include "linguisticProcessing/common/annotationGraph/AnnotationData.h"
//#include "linguisticProcessing/core/Automaton/SpecificEntityAnnotation.h"


namespace Lima
{
namespace LinguisticProcessing
{
namespace MorphologicAnalysis
{

#define APPROX_STRING_MATCHER_CLASSID "ApproxStringMatcher"

// TODO  : à mettre dans le .cpp avec une implémentation private
typedef struct _Suggestion {
  // Position of candidate (number of unicode character) in search string
  int startPosition;
  // length of candidate (number of unicode character)
  int endPosition;
  // number of errors (delete/add) to match
  int nb_error;
  // id of target in lexicon ??
  // long unsigned int match_id;
} Suggestion;

// TODO  : à mettre dans le .cpp avec une implémentation private
typedef struct _Solution {
  Suggestion suggestion;
  // list of vertex in graph
  std::deque<LinguisticGraphVertex> vertices;
  // id of target in lexicon ??
  // long unsigned int match_id;
  // matched string
  LimaString form;
  // normalized string = matched element in lexicon
  LimaString normalizedForm;
  // position of matched string in text
  uint64_t startPos;
  uint64_t length;
} Solution;


class LIMA_MORPHOLOGICANALYSIS_EXPORT ApproxStringMatcher : public MediaProcessUnit
{

public:
  ApproxStringMatcher();
  virtual ~ApproxStringMatcher();

  void init(
    Common::XMLConfigurationFiles::GroupConfigurationStructure& unitConfiguration,
    Manager* manager) override;

  LimaStatusCode process(
    AnalysisContent& analysis) const override;


private:
  LimaStatusCode matchExactTokenAndFollowers(
    LinguisticGraph& g, 
    LinguisticGraphVertex vStart,
    LinguisticGraphVertex vEnd,
    std::multimap<int,Suggestion>& result) const;

  void matchApproxTokenAndFollowers(
    LinguisticGraph& g, 
    LinguisticGraphVertex vStart,
    LinguisticGraphVertex vEnd,
    Solution& result) const;

  int findApproxPattern(
    LimaString pattern, LimaString text,
    Suggestion& suggestion) const;

  void createVertex(
    LinguisticGraph& g, 
    LinguisticGraphVertex vStart,
    LinguisticGraphVertex vEnd,
    Solution& solution,
    Lima::Common::AnnotationGraphs::AnnotationData* annotationData ) const;

  Common::AbstractAccessByString *m_lexicon;
  FsaStringsPool* m_sp;
  int m_nbMaxError;
  Common::MediaticData::EntityType m_entityType;
  Common::MediaticData::EntityGroupId m_entityGroupId;
};
} // MorphologicAnalysis
} // LinguisticProcessing
} // Lima

#endif
