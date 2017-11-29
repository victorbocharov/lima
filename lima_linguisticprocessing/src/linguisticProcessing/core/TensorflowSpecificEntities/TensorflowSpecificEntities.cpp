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

#include "TensorflowSpecificEntities.h"

#include "linguisticProcessing/LinguisticProcessingCommon.h"
#include "common/XMLConfigurationFiles/xmlConfigurationFileExceptions.h"
#include "common/MediaticData/mediaticData.h"
#include "common/tools/FileUtils.h"
#include "common/time/traceUtils.h"
#include "common/time/timeUtilsController.h"
#include "common/AbstractFactoryPattern/SimpleFactory.h"
#include "linguisticProcessing/core/LinguisticAnalysisStructure/LinguisticGraph.h"
#include "linguisticProcessing/core/LinguisticResources/LinguisticResources.h"
#include "linguisticProcessing/common/annotationGraph/AnnotationData.h"
#include "linguisticProcessing/core/LinguisticAnalysisStructure/AnalysisGraph.h"
#include "linguisticProcessing/core/LinguisticAnalysisStructure/Token.h"
#include "linguisticProcessing/core/TextSegmentation/SegmentationData.h"

#include <QString>
#include <QDir>
#include <QRegularExpression>
#include <QStringList>
#include <memory>
#include <queue>
#include <map>
#include <externaltensorflowspecificentities/nerUtils.h>

using namespace Lima::Common::AnnotationGraphs;
using namespace Lima::Common::MediaticData;
using namespace Lima::Common::XMLConfigurationFiles;
using namespace Lima::LinguisticProcessing::LinguisticAnalysisStructure;
using namespace tensorflow;

namespace Lima
{
namespace LinguisticProcessing
{
namespace TensorflowSpecificEntities
{   
  SimpleFactory<MediaProcessUnit,TensorflowSpecificEntities> tensorflowSpecificEntitiesFactory(TENSORFLOWSPECIFICENTITIES_CLASSID);
  
  class TensorflowSpecificEntitiesPrivate
  {
    friend class TensorflowSpecificEntities;
    TensorflowSpecificEntitiesPrivate(){}
    TensorflowSpecificEntitiesPrivate(
      const std::string& graph,
      const QString& fileChars,
      const QString& fileWords,
      const QString& fileTags);
    
    std::string m_graph;
    QString m_fileChars;
    QString m_fileWords;
    QString m_fileTags;
  };
  
  TensorflowSpecificEntitiesPrivate::TensorflowSpecificEntitiesPrivate(
      const std::string& graph,
      const QString& fileChars,
      const QString& fileWords,
      const QString& fileTags) :
    m_graph(graph),
    m_fileChars(fileChars),
    m_fileWords(fileWords),
    m_fileTags(fileTags)
    {}

  TensorflowSpecificEntities::TensorflowSpecificEntities()
  :m_d(new TensorflowSpecificEntitiesPrivate())
  {
  }
  
  TensorflowSpecificEntities::~TensorflowSpecificEntities()
  {
    delete m_d;
  }
  
  void TensorflowSpecificEntities::init(
    Common::XMLConfigurationFiles::GroupConfigurationStructure& unitConfiguration,
    Manager* manager)
  {
    TFSELOGINIT;
    MediaId language = manager->getInitializationParameters().media;
    try
    {
      m_d->m_graph=
      Common::Misc::findFileInPaths(
        Common::MediaticData::MediaticData::single().getResourcesPath().c_str(),
        unitConfiguration.getParamsValueAtKey("graphOutputFile").c_str()).toStdString();
    }
    catch (NoSuchParam& )
    {
      LERROR << "no param 'graphOutputFile' in TensorflowSpecificEntities group for language " << (int) language;
      throw InvalidConfiguration();
    }
    
    try
    {
      m_d->m_fileChars=Common::Misc::findFileInPaths(
        Common::MediaticData::MediaticData::single().getResourcesPath().c_str(),
        unitConfiguration.getParamsValueAtKey("charValuesFile").c_str());
    }
    catch (NoSuchParam& )
    {
      LERROR << "no param 'charValuesFile' in TensorflowSpecificEntities group for language " << (int) language;
      throw InvalidConfiguration();
    }
    
    try
    {
       m_d->m_fileWords=
       Common::Misc::findFileInPaths(
        Common::MediaticData::MediaticData::single().getResourcesPath().c_str(),
        unitConfiguration.getParamsValueAtKey("wordValuesFile").c_str());
    }
    catch (NoSuchParam& )
    {
      LERROR << "no param 'wordValuesFile' in TensorflowSpecificEntities group for language " << (int) language;
      throw InvalidConfiguration();
    }
    
    try
    {
       m_d->m_fileTags=Common::Misc::findFileInPaths(
        Common::MediaticData::MediaticData::single().getResourcesPath().c_str(),
        unitConfiguration.getParamsValueAtKey("tagValuesFile").c_str());
    }
    catch (NoSuchParam& )
    {
      LERROR << "no param 'tagValuesFile' in TensorflowSpecificEntities group for language " << (int) language;
      throw InvalidConfiguration();
    }
  }
  
  LimaStatusCode TensorflowSpecificEntities::process(
    AnalysisContent& analysis) const
    {
      Lima::TimeUtilsController timer("TensorflowSpecificEntities");
      // start named-entity recognition here !
      TFSELOGINIT;
      LINFO << "start TensorflowSpecificEntities";
      
      // Initialize a tensorflow session
      Session* session = nullptr;
      std::shared_ptr<Status> status(new Status(NewSession(SessionOptions(), &session)));
      if (!status->ok()) {
        LERROR << status->ToString();
        return UNKNOWN_ERROR;
      }
      
      // Read in the protobuf graph we have exported
      GraphDef graphDef;
      *status = ReadBinaryProto(Env::Default(),m_d->m_graph, &graphDef);
      if (!status->ok()) {
        LERROR<< status->ToString();
        return UNKNOWN_ERROR;
      }

      // Add the graph to the session
      *status = session->Create(graphDef);
      if (!status->ok()) {
        LERROR << status->ToString();                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      
        return UNKNOWN_ERROR;
      }
      
      //Load vocabulary 
      std::map<QString,int> vocabWords;
      std::map<QChar,int> vocabChars;
      std::map<unsigned int,QString> vocabTags;
 
      try{
        vocabWords= loadFileWords(m_d->m_fileWords);
        if(vocabWords.empty()){
          return MISSING_DATA;
        }    
        vocabChars= loadFileChars(m_d->m_fileChars);
        if(vocabChars.empty()){
          return MISSING_DATA;
        }    
        vocabTags = loadFileTags(m_d->m_fileTags);
        if(vocabTags.empty()){
          return MISSING_DATA;
        }    
      }
      catch(const BadFileException& e){
        LERROR<<e.what();
        return CANNOT_OPEN_FILE_ERROR;
      }
            
      AnalysisGraph* tokenList=static_cast<AnalysisGraph*>(analysis.getData("AnalysisGraph"));
      LinguisticGraph* g=tokenList->getGraph();
      VertexTokenPropertyMap tokenMap=get(vertex_token,*g);

      // get sentence bounds
      SegmentationData* sb=static_cast<SegmentationData*>(analysis.getData("SentenceBoundariesForSE"));
      if(sb==0){
        TFSELOGINIT;
        LERROR << "TensorflowSpecificEntities::process: no sentence bounds defined ! abort";
        return MISSING_DATA;
      }
      if (sb->getGraphId() != "AnalysisGraph") {
        TFSELOGINIT;
        LERROR << "TensorflowSpecificEntities::process: SentenceBounds are computed on graph '" << sb->getGraphId() << "'";
        LERROR << "can't compute TensorflowSpecificEntities on graph AnalysisGraph !";
        return INVALID_CONFIGURATION;
      }
      std::vector<Segment>::const_iterator boundItr=(sb->getSegments()).cbegin();

      
      //Minibatching (group of max 20 sentences of different size) is used in order to amortize the cost of loading the network weights from CPU/GPU memory across many inputs.
      //and to take advantage from parallelism.
  
      int batchSizeMax = 20;
      std::vector<Eigen::MatrixXi> result;
      std::deque<LinguisticGraphVertex> visited;
      LinguisticGraphVertex endPrecedentSentence=boundItr->getFirstVertex();
      
      while(boundItr!=(sb->getSegments()).cend()){
        std::vector<std::vector<std::pair<std::vector<int>,int>>> textConverted(batchSizeMax);
        std::vector<std::vector<int>> wordIds(batchSizeMax);
        std::vector<std::vector<std::vector<int>>> charIds(batchSizeMax);
        std::vector<QStringList> sentencesByBatch;
        sentencesByBatch.reserve(batchSizeMax);
        int batchSize =0;
        
        while(batchSize<batchSizeMax && boundItr!=(sb->getSegments()).cend()){
          QStringList wordsRaw;
          LinguisticGraphVertex beginSentence=boundItr->getFirstVertex();
          LinguisticGraphVertex endSentence=boundItr->getLastVertex();
          std::queue<LinguisticGraphVertex> toVisit;
          toVisit.push(beginSentence);
          
          while(!toVisit.empty()){
            LinguisticGraphVertex currentVertex=toVisit.front();
            toVisit.pop();
            if(currentVertex!=tokenList->lastVertex()){
              if(currentVertex!=tokenList->firstVertex() && currentVertex!=endPrecedentSentence)
              {
                Token* currentToken=tokenMap[currentVertex];
                if(currentToken!=0)
                {
                  visited.push_back(currentVertex);
                  wordsRaw<<currentToken->stringForm();
                }
              }
              
              if(currentVertex!=endSentence){
                LinguisticGraphOutEdgeIt outEdge,outEdge_end;
                boost::tie(outEdge,outEdge_end)=boost::out_edges(currentVertex, *g); toVisit.push(boost::target(*outEdge,*g));
                ++outEdge;
                if(outEdge!= outEdge_end){
                  TFSELOGINIT;
                  LERROR << "TensorflowSpecificEntities::process: at this step, it is supposed that each vertex has only one neighbourhood. abort";
                  return OUT_OF_RANGE_ERROR;
                }
              }
              else
              {
                endPrecedentSentence=currentVertex;
              }
            }
          }
          
          
          //2. Transform words into ids and split all the characters and identify them
          textConverted[batchSize].reserve(wordsRaw.size());
        
          for(auto it=wordsRaw.cbegin();it!=wordsRaw.cend();++it){
            try{
              textConverted[batchSize].push_back(getProcessingWord(*it, vocabWords, vocabChars, true, true));
              if(std::get<0>(textConverted[batchSize].back()).empty()){
                return MISSING_DATA;
              }
            }
            catch(const UnknownWordClassException& e){
              LERROR<<e.what();
            }
          }
        
          //3. Gather ids of words and ids of sequences of characters according to the order of words

          wordIds[batchSize].resize(wordsRaw.size());
          charIds[batchSize].resize(wordsRaw.size());
          for(auto i=0;i<textConverted[batchSize].size();++i){
            charIds[batchSize][i].resize(textConverted[batchSize][i].first.size());
            charIds[batchSize][i]=textConverted[batchSize][i].first;
            wordIds[batchSize][i]=textConverted[batchSize][i].second;
          }
          ++batchSize;
          ++boundItr;
        }
      
        
        //4.Resize data if current batch size is fewer than batchSizeMax
    
        if(batchSize<batchSizeMax){
          wordIds.resize(batchSize);
          charIds.resize(batchSize);
          sentencesByBatch.resize(batchSize);
        }   
        
        //5. Predict tags
        result.resize(result.size()+batchSize);
        if(predictBatch(status, session, sentencesByBatch, batchSize, charIds, wordIds, result)==NERStatusCode::MISSING_DATA){
          return MISSING_DATA;
        }
        //Continue to the next batch 
      }
      std::map<LinguisticGraphVertex,QString> matchingVertextoEntity;
      std::deque<LinguisticGraphVertex>::const_iterator itVisited=visited.cbegin();
            
      for(auto i=0;i<result.size();++i){
        for(auto j=0;j<result[i].size();++j){
          matchingVertextoEntity[*itVisited]=vocabTags[result[i](j)];
          ++itVisited;
        }
      }

        
      //6. Free any resources used by the session
      *status=session->Close();
      if (!status->ok()) {
        LERROR << status->ToString();
        return UNKNOWN_ERROR;
      }
      return SUCCESS_ID;
    }
  
} // TensorflowSpecificEntities
} // LinguisticProcessing
} // Lima
