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
#include "linguisticProcessing/core/SyntacticAnalysis/SyntacticData.h"
#include "linguisticProcessing/core/LinguisticAnalysisStructure/Token.h"
#include "linguisticProcessing/core/TextSegmentation/SegmentationData.h"
#include "linguisticProcessing/core/Automaton/SpecificEntityAnnotation.h"
#include "linguisticProcessing/core/SpecificEntities/SpecificEntitiesMicros.h"
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
using namespace Lima::LinguisticProcessing::SyntacticAnalysis;
using namespace Lima::LinguisticProcessing::SpecificEntities;
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
      const QString& fileTags,
      MediaId language);
    
    std::string m_graph;
    QString m_fileChars;
    QString m_fileWords;
    QString m_fileTags;
    std::map<LinguisticGraphVertex,QString> m_matchingVertextoEntity;
    std::deque<LinguisticGraphVertex> m_visitedVertex;
    MediaId m_language;
    const Common::PropertyCode::PropertyAccessor* m_microAccessor;
    FsaStringsPool* m_sp;
  };
  
  TensorflowSpecificEntitiesPrivate::TensorflowSpecificEntitiesPrivate(
      const std::string& graph,
      const QString& fileChars,
      const QString& fileWords,
      const QString& fileTags,
      MediaId language) :
    m_graph(graph),
    m_fileChars(fileChars),
    m_fileWords(fileWords),
    m_fileTags(fileTags),
    m_matchingVertextoEntity(),
    m_visitedVertex(),
    m_language(language)
    {
      m_microAccessor=&(static_cast<const Common::MediaticData::LanguageData&>(Common::MediaticData::MediaticData::single().mediaData(language)).getPropertyCodeManager().getPropertyAccessor("MICRO"));
      m_sp=&(Common::MediaticData::MediaticData::changeable().stringsPool(language));
    }

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
    m_d->m_language = manager->getInitializationParameters().media;
    m_d->m_microAccessor=&(static_cast<const Common::MediaticData::LanguageData&>(Common::MediaticData::MediaticData::single().mediaData(m_d->m_language)).getPropertyCodeManager().getPropertyAccessor("MICRO"));
    m_d->m_sp=&(Common::MediaticData::MediaticData::changeable().stringsPool(m_d->m_language));
    
    try
    {
      m_d->m_graph=
      Common::Misc::findFileInPaths(
        Common::MediaticData::MediaticData::single().getResourcesPath().c_str(),
        unitConfiguration.getParamsValueAtKey("graphOutputFile").c_str()).toStdString();
    }
    catch (NoSuchParam& )
    {
      LERROR << "no param 'graphOutputFile' in TensorflowSpecificEntities group for m_d->m_language " << (int) m_d->m_language;
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
      LERROR << "no param 'charValuesFile' in TensorflowSpecificEntities group for m_d->m_language " << (int) m_d->m_language;
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
      LERROR << "no param 'wordValuesFile' in TensorflowSpecificEntities group for m_d->m_language " << (int) m_d->m_language;
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
      LERROR << "no param 'tagValuesFile' in TensorflowSpecificEntities group for m_d->m_language " << (int) m_d->m_language;
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
      
     LimaStatusCode status=runTFGraph(analysis);
     if(status!=SUCCESS_ID)
     {
       return status;
     }
     
     if(updateAnalysisData(analysis))
     {
       
       return SUCCESS_ID;
     }
     else
     {
       return UNKNOWN_ERROR;
     }
     
     
    }
  
  LimaStatusCode TensorflowSpecificEntities::runTFGraph(AnalysisContent& analysis) const
  {
    // Initialize a tensorflow session
    Session* session = nullptr;
    std::shared_ptr<Status> status(new Status(NewSession(SessionOptions(), &session)));
    if (!status->ok()) {
      TFSELOGINIT;
      LERROR <<status->ToString();
      return UNKNOWN_ERROR;
    }
    
    // Read in the protobuf graph we have exported
    GraphDef graphDef;
    *status = ReadBinaryProto(Env::Default(),m_d->m_graph, &graphDef);
    if (!status->ok()) {
      TFSELOGINIT;
      LERROR<< status->ToString();
      return UNKNOWN_ERROR;
    }

    // Add the graph to the session
    *status = session->Create(graphDef);
    if (!status->ok()) {
      TFSELOGINIT;
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
      TFSELOGINIT;
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
               m_d->m_visitedVertex.push_back(currentVertex);
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
            TFSELOGINIT;
            LERROR<<e.what();
            return UNKNOWN_ERROR;
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
    
    std::deque<LinguisticGraphVertex>::const_iterator itVisited=m_d->m_visitedVertex.cbegin();
          
    for(auto i=0;i<result.size();++i){
      for(auto j=0;j<result[i].size();++j){
        m_d->m_matchingVertextoEntity[*itVisited]=vocabTags[result[i](j)];
        ++itVisited;
      }
    }
         
    //6. Free any resources used by the session
    *status=session->Close();
    if (!status->ok()) {
      TFSELOGINIT;
      LERROR << status->ToString();
      return UNKNOWN_ERROR;
    }
    
    return SUCCESS_ID;
  }

  bool TensorflowSpecificEntities::updateAnalysisData(AnalysisContent& analysis) const
  {
    AnalysisGraph* analysisGraph=static_cast<AnalysisGraph*>(analysis.getData("AnalysisGraph"));
        
    std::deque<LinguisticGraphVertex>::const_iterator itVisited=m_d->m_visitedVertex.cbegin();
    while(itVisited!=m_d->m_visitedVertex.cend())
    {
      if(m_d->m_matchingVertextoEntity[*itVisited]!="O")
      {
        //look if there is a group of words of the same entity
//         std::queue<LinguisticGraphVertex> entityFound;
//         entityFound.push(*itVisited);
//         while(itVisited!=m_d->m_visitedVertex.cend() && m_d->m_matchingVertextoEntity[*itVisited]==m_d->m_matchingVertextoEntity[entityFound.front()])
//         {
//           entityFound.push(*itVisited);
//           itVisited++;
//         }
        Automaton::RecognizerMatch entityFound(analysisGraph,*itVisited,true);
        EntityType seType=Common::MediaticData::MediaticData::single().getEntityType(m_d->m_matchingVertextoEntity[*itVisited].remove(QRegularExpression("^[BI]-")));
        entityFound.setType(seType);
        itVisited++;
        while(itVisited!=m_d->m_visitedVertex.cend() && m_d->m_matchingVertextoEntity[*itVisited]==m_d->m_matchingVertextoEntity[entityFound.getBegin()])
        {
          entityFound.addBackVertex(*itVisited);
          itVisited++;
        }
        if(!createSpecificEntity(entityFound,analysis))
        {
          return false;
        }
      }
      else
      {
        itVisited++;
      }
    }
    return true;
  }
  
  bool TensorflowSpecificEntities::createSpecificEntity(
    Automaton::RecognizerMatch& entityFound,
    AnalysisContent& analysis) const
  {
      AnalysisGraph* analysisGraph=static_cast<AnalysisGraph*>(analysis.getData("AnalysisGraph"));    
    
      SyntacticData* syntacticData=static_cast<SyntacticData*>(analysis.getData("SyntacticData"));

      AnnotationData* annotationData = static_cast< AnnotationData* >(analysis.getData("AnnotationData"));
      
      LinguisticGraph* lingGraph = const_cast<LinguisticGraph*>(analysisGraph->getGraph());
      VertexTokenPropertyMap tokenMap = get(vertex_token, *lingGraph);
      VertexDataPropertyMap dataMap = get(vertex_data, *lingGraph);
      
      if (annotationData==0)
      {
        return false;
      }
      //create a specif annotation
      if (annotationData->dumpFunction("TensorflowSpecificEntities") == 0)
      {
        annotationData->dumpFunction("TensorflowSpecificEntities", new DumpSpecificEntityAnnotation());
      }
      
      SpecificEntityAnnotation annot(entityFound,*m_d->m_sp);
      std::ostringstream oss;
      annot.dump(oss);
      LinguisticGraphVertex head = annot.getHead();
      const MorphoSyntacticData* dataHead = dataMap[head];
      
      // Preparer le Token et le MorphoSyntacticData pour le nouveau noeud. Construits
      // a partir des infos de l'entitee nommee
      StringsPoolIndex seFlex = annot.getString();
      StringsPoolIndex seLemma = annot.getNormalizedString();
      //No features with this method
      StringsPoolIndex seNorm = annot.getNormalizedForm();

      // creer un MorphoSyntacticData
      MorphoSyntacticData* newMorphData = new MorphoSyntacticData();

      // all linguisticElements of this morphosyntacticData share common SE information
      LinguisticElement elem;
      elem.inflectedForm = seFlex; // StringsPoolIndex
      elem.lemma = seLemma; // StringsPoolIndex
      elem.normalizedForm = seNorm; // StringsPoolIndex
      elem.type = TENSORFLOW_SPECIFIC_ENTITY; // MorphoSyntacticType
     
      EntityType seType=entityFound.getType();
      const LimaString& resourceName =
      Common::MediaticData::MediaticData::single().getEntityGroupName(seType.getGroupId())+"Micros";
      AbstractResource* res=LinguisticResources::single().getResource(m_d->m_language,resourceName.toUtf8().constData());
      
      if (res!=0) {
        SpecificEntities::SpecificEntitiesMicros* entityMicros=static_cast<SpecificEntities::SpecificEntitiesMicros*>(res);
        const std::set<LinguisticCode>* micros=entityMicros->getMicros(seType);
        addMicrosToMorphoSyntacticData(newMorphData,dataHead,*micros,elem);
      }
      else {
        // cannot find micros for this type: error
        TFSELOGINIT;
        LERROR << "TensorflowSpecificEntities::createSpecificEntity: Missing resource " << resourceName ;
        delete newMorphData;
        return false;
      }
      const FsaStringsPool& sp=*m_d->m_sp;
      Token* newToken = new Token(
          seFlex,
          sp[seFlex],
          entityFound.positionBegin(),
          entityFound.length());
      TStatus tStatus = tokenMap[head]->status();
      newToken->setStatus(tStatus);
      
      if (newMorphData->empty())
      {
        TFSELOGINIT;
        LERROR << "TensorflowSpecificEntities::createSpecificEntity : Found no morphosyntactic  data for new vertex. Abort.";
        delete newToken;
        delete newMorphData;
        assert(false);
        return false;
      }
      return true;
  }
  
  void TensorflowSpecificEntities::addMicrosToMorphoSyntacticData(
     LinguisticAnalysisStructure::MorphoSyntacticData* newMorphData,
     const LinguisticAnalysisStructure::MorphoSyntacticData* oldMorphData,
     const std::set<LinguisticCode>& micros,
     LinguisticAnalysisStructure::LinguisticElement& elem) const
     {
      // try to filter existing microcategories
      for (MorphoSyntacticData::const_iterator it=oldMorphData->begin(), 
            it_end=oldMorphData->end(); it!=it_end; it++) {
        
        if (micros.find(m_d->m_microAccessor->readValue((*it).properties)) !=
            micros.end()) {
          elem.properties=(*it).properties;
          newMorphData->push_back(elem);
        }
      }
      // if no categories kept : assign all micros to keep
      if (newMorphData->empty()) {
        for (std::set<LinguisticCode>::const_iterator it=micros.begin(),
              it_end=micros.end(); it!=it_end; it++) {
          elem.properties=*it;
          newMorphData->push_back(elem);
        }
      }
    }
       
} // TensorflowSpecificEntities
} // LinguisticProcessing
} // Lima
