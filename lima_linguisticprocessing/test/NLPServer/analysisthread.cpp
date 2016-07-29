/*
    <one line to give the program's name and a brief idea of what it does.>
    Copyright (C) <year>  <name of author>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

*/

#include "analysisthread.h"

//#include "linguisticProcessing/core/AnalysisDumper/JsonResults.h"

#include "common/MediaticData/mediaticData.h"
#include "common/Data/strwstrtools.h"
#include "common/Handler/AbstractXmlDocumentHandler.h"
#include "linguisticProcessing/client/AnalysisHandlers/SimpleStreamHandler.h"
 #include "linguisticProcessing/client/AnalysisHandlers/BowTextWriter.h"
#include "linguisticProcessing/client/AnalysisHandlers/BowTextHandler.h"

// factories
#include "linguisticProcessing/client/LinguisticProcessingClientFactory.h"

#include <qhttprequest.h>
#include <qhttpresponse.h>

#include <QStringList>
#include <QPair>
#include <boost/graph/buffer_concepts.hpp>

#include <fstream>
#include <sstream>      // std::stringstream
#include <QtCore/QString>
#include <QtCore/QTemporaryFile>

using namespace Lima;
using namespace Lima::Common;
using namespace Lima::Common::XMLConfigurationFiles;


class AnalysisThreadPrivate
{
friend class AnalysisThread;
public:
  AnalysisThreadPrivate (Lima::LinguisticProcessing::AbstractLinguisticProcessingClient* m_analyzer, 
                        QHttpRequest *req, QHttpResponse *resp,
                        const std::set<std::string>& langs);
  ~AnalysisThreadPrivate() {}
  
  Lima::LinguisticProcessing::AbstractLinguisticProcessingClient* m_analyzer;
  QHttpRequest* m_request;
  QHttpResponse* m_response;
  const std::set<std::string>& m_langs;
};

AnalysisThreadPrivate::AnalysisThreadPrivate(Lima::LinguisticProcessing::AbstractLinguisticProcessingClient* analyzer, 
                    QHttpRequest *req, QHttpResponse *resp,
                    const std::set<std::string>& langs) :
    m_analyzer(analyzer),
    m_request(req), m_response(resp),
    m_langs(langs)
{
}

AnalysisThread::AnalysisThread (Lima::LinguisticProcessing::AbstractLinguisticProcessingClient* analyzer, 
                  QHttpRequest *req, QHttpResponse *resp, 
                  const std::set<std::string>& langs, QObject* parent ):
    QThread(parent),
    m_d(new AnalysisThreadPrivate(analyzer,req,resp,langs))
{
  CORECLIENTLOGINIT;
  LDEBUG << "AnalysisThread::AnalysisThread()...";
  connect(this,SIGNAL(started()),this,SLOT(slotStarted()),Qt::QueuedConnection);
}

AnalysisThread::~AnalysisThread()
{
  delete m_d;
}

void AnalysisThread::slotStarted()
{
  CORECLIENTLOGINIT;
  LDEBUG << "AnalysisThread::slotStarted";

}

void AnalysisThread::startAnalysis()
{
  CORECLIENTLOGINIT;
  LDEBUG << "AnalysisThread::startAnalysis" << m_d->m_request->methodString() << m_d->m_request->url().path();

  // Arguments to be read from the request
  QString language, pipeline, text;
  QPair<QString, QString> item;
  std::map<std::string,std::string> metaData;
  std::set<std::string> inactiveUnits;

  // First case = GET
  if (m_d->m_request->methodString() == "HTTP_GET" && m_d->m_request->url().path() == "/annot")
  {
    LDEBUG << "AnalysisThread::startAnalysis: process annot request (mode HTTP_GET)";
    // read arguments from the request
    Q_FOREACH( item, m_d->m_request->url().queryItems())
    {
      QTemporaryFile tempFile;
      metaData["FileName"]=tempFile.fileName().toUtf8().constData();
      if (item.first == "lang")
      {
        language = item.second;
        metaData["Lang"]=language.toUtf8().data();
        LDEBUG << "AnalysisThread::startAnalysis: " << "language=" << language;
      }
      if (item.first == "pipeline")
      {
        pipeline = item.second;
        LDEBUG << "AnalysisThread::startAnalysis: " << "pipeline=" << pipeline;
      }
      if (item.first == "text")
      {
        text = item.second;
        LDEBUG << "AnalysisThread::startAnalysis: " << "text='" << text << "'";
      }
    }
  }
  // second case: HTTP_POST
  else if (m_d->m_request->methodString() == "HTTP_POST" && m_d->m_request->url().path() == "/annot")
  {
    LDEBUG << "AnalysisThread::startAnalysis: process extractEN request (mode HTTP_POST)";
    std::string text_utf8;
    Q_FOREACH( item, m_d->m_request->url().queryItems())
    {
      if (item.first == "lang")
      {
        language = item.second;
        metaData["Lang"]=language.toUtf8().data();
        LDEBUG << "AnalysisThread::startAnalysis: " << "language=" << language;
      }
      if (item.first == "pipeline")
      {
        pipeline = item.second;
        LDEBUG << "AnalysisThread::startAnalysis: " << "pipeline=" << pipeline;
      }
    }
    const QByteArray& body = m_d->m_request->body();
    text_utf8 = std::string(body.data());
    text = Misc::utf8stdstring2limastring(text_utf8);
  }
  else
  {
    m_d->m_response->writeHead(405);
    m_d->m_response->setHeader("Allow","GET,POST");
    m_d->m_response->end(QByteArray("Only GET and POST search queries are currently allowed."));
    return;
  }

  // Handle bad parameters cases
  if( language.isEmpty() )
  {
    m_d->m_response->writeHead(400);
    m_d->m_response->end(QByteArray("Empty language"));
    return;
  }

  if( m_d->m_langs.find(metaData["Lang"]) == m_d->m_langs.end() )
  {
    m_d->m_response->writeHead(400);
    QString errorMessage = QString(tr("Language %1 is no initialized")).arg(language);
    m_d->m_response->end(errorMessage.toUtf8());
    return;
  }
  if( text.isEmpty() )
  {
    m_d->m_response->writeHead(400);
    QString errorMessage = QString(tr("Text is empty"));
    m_d->m_response->end(errorMessage.toUtf8());
    return;
  }

  // Create a handler and map it for every logger
  std::map<std::string, AbstractAnalysisHandler*> handlers;
  LinguisticProcessing::SimpleStreamHandler* neLogWriter = new LinguisticProcessing::SimpleStreamHandler();
  handlers.insert(std::make_pair("se", neLogWriter));
  std::ostringstream* neOss = new std::ostringstream();
  neLogWriter->setOut(neOss);
  LinguisticProcessing::SimpleStreamHandler* sentenceLogWriter = new LinguisticProcessing::SimpleStreamHandler();
  handlers.insert(std::make_pair("sentence", sentenceLogWriter));
  std::ostringstream* sentenceOss = new std::ostringstream();
  sentenceLogWriter->setOut(sentenceOss);
  LinguisticProcessing::SimpleStreamHandler* fullTokenLogWriter = new LinguisticProcessing::SimpleStreamHandler();
  handlers.insert(std::make_pair("disambiguatedTokens", fullTokenLogWriter));
  std::ostringstream* tokenOss = new std::ostringstream();
  fullTokenLogWriter->setOut(tokenOss);
   
  // analyze Text...
  
  LDEBUG << "Analyzing" << language << text;
  std::ostringstream ots;
  std::string pipe = pipeline.toUtf8().data();
  QTemporaryFile tempFile;
  metaData["FileName"]=tempFile.fileName().toUtf8().constData();
  m_d->m_analyzer->analyze(text,metaData, pipe, handlers, inactiveUnits);

  std::string resultString("<?xml version='1.0' encoding='UTF-8'?>");
  resultString.append("<NLPAnnot>");
  resultString.append("<specificEntitiesXmlLoggerForLimaserver>");
  resultString.append(neOss->str());
  resultString.append("</specificEntitiesXmlLoggerForLimaserver>");
  resultString.append("<sentenceBoundariesXmlLogger>");
  resultString.append(sentenceOss->str());
  resultString.append("</sentenceBoundariesXmlLogger>");
  resultString.append("<disambiguatedGraphXmlLogger>");
  resultString.append(tokenOss->str());
  resultString.append("<justForFun/>");
  resultString.append("</disambiguatedGraphXmlLogger>");
  resultString.append("</NLPAnnot>");
  LDEBUG << "AnalysisThread::startAnalysis: seLogger output is " << QString::fromUtf8(resultString.c_str());

  m_d->m_response->setHeader("Content-Type", "text/xml; charset=utf-8");
  m_d->m_response->writeHead(200);

// QString body = QString::fromUtf8(resultString.c_str());
// m_d->m_response->end(body.toUtf8());
  m_d->m_response->end(QByteArray(resultString.c_str()));
  m_d->m_request->deleteLater();
  LDEBUG << "AnalysisThread::startAnalysis: free log and stream... ";
  if( neLogWriter != 0 )
    delete neLogWriter;
  if( neOss != 0 )
    delete neOss;
  if( sentenceLogWriter != 0 )
    delete sentenceLogWriter;
  if( sentenceOss != 0 )
    delete sentenceOss;
  if( fullTokenLogWriter != 0 )
    delete fullTokenLogWriter;
  if( tokenOss != 0 )
    delete tokenOss;
  // ???
  // deleteLater();
  //quit();
  
}
