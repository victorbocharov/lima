// Copyright (c) 2010, Razvan Petru
// All rights reserved.

// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:

// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright notice, this
//   list of conditions and the following disclaimer in the documentation and/or other
//   materials provided with the distribution.
// * The name of the contributors may not be used to endorse or promote products
//   derived from this software without specific prior written permission.

// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
// IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
// INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
// BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
// OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
// OF THE POSSIBILITY OF SUCH DAMAGE.

#include "QsLog.h"
#include "QsLogDest.h"
#include <QMutex>
#include <QList>
#include <QDateTime>
#include <QtGlobal>
#include <QThread>
#include <cassert>
#include <cstdlib>
#include <stdexcept>

#ifdef USE_LOG4CPP
#include "log4cpp/Category.hh"
#include "log4cpp/Priority.hh"
#endif

LIMA_COMMONQSLOG_EXPORT QDebug&  operator<< (QDebug&  qd, const std::string& str )
{
  qd << str.c_str();
  return qd;
}

namespace QsLogging
{
typedef QList<Destination*> DestinationList;

static const char TraceString[] = "TRACE";
static const char DebugString[] = "DEBUG";
static const char InfoString[]  = "INFO";
static const char WarnString[]  = "WARN";
static const char ErrorString[] = "ERROR";
static const char FatalString[] = "FATAL";

// not using Qt::ISODate because we need the milliseconds too
static const QString fmtDateTime("yyyy-MM-ddThh:mm:ss.zzz");

#ifdef USE_LOG4CPP
static log4cpp::Priority::PriorityLevel levelToPriority[] = {
  log4cpp::Priority::NOTSET, // TraceLevel
  log4cpp::Priority::DEBUG, // DebugLevel
  log4cpp::Priority::INFO, // InfoLevel
  log4cpp::Priority::WARN, // WarnLevel
  log4cpp::Priority::ERROR, // ErrorLevel
  log4cpp::Priority::FATAL // FatalLevel
};
#endif

static const char* LevelToText(Level theLevel)
{
   switch( theLevel )
   {
   case TraceLevel:
      return TraceString;
   case DebugLevel:
      return DebugString;
   case InfoLevel:
      return InfoString;
   case WarnLevel:
      return WarnString;
   case ErrorLevel:
      return ErrorString;
   case FatalLevel:
      return FatalString;
   default:
      {
         assert(!"bad log level");
         return InfoString;
      }
   }
}

class LoggerImpl
{
public:
   LoggerImpl(const QString& aZone) :
      level(InfoLevel), zone(aZone)
   {

   }
   ~LoggerImpl()
   {
     for (auto it=destList.constBegin(); it!=destList.constEnd();++it)
       delete (*it);
   }
   QMutex logMutex;
   Level level;
   DestinationList destList;
   QString zone;
};

Logger::Logger(const QString& aZone) :
   d(new LoggerImpl(aZone))
{
}

Logger::~Logger()
{
   delete d;
}

Logger& Logger::instance(const QString& zone)
{
  static QMap<QString, ::std::shared_ptr<Logger>> staticLog;
  auto it = staticLog.find(zone);
  if (it == staticLog.end())
  {
    std::shared_ptr<Logger> logger(new Logger(zone));
#ifndef USE_LOG4CPP
    logger->addDestination(new DebugOutputDestination());
#endif
    return **staticLog.insert(zone, logger);
  }
  return **it;
}

void Logger::addDestination(Destination* destination)
{
   assert(destination);
   d->destList.push_back(destination);
}

void Logger::setLoggingLevel(Level newLevel)
{
   d->level = newLevel;
#ifdef USE_LOG4CPP
  log4cpp::Category & cat = log4cpp::Category::getInstance(d->zone.toStdString());
  cat.setPriority(levelToPriority[newLevel]);
#endif
}

Level Logger::loggingLevel() const
{
   return d->level;
}

bool Logger::isLoggingEnabled() const
{
  return d->level <= QsLogging::FatalLevel;
}

//! Returns true if logging level is lower than or equal to TraceLevel
bool Logger::isTraceEnabled() const
{
  return d->level <= QsLogging::TraceLevel;
}
//! Returns true if logging level is lower than or equal to DebugLevel
bool Logger::isDebugEnabled() const
{
  return d->level <= QsLogging::DebugLevel;
}
//! Returns true if logging level is lower than or equal to InfoLevel
bool Logger::isInfoEnabled() const
{
  return d->level <= QsLogging::InfoLevel;
}
//! Returns true if logging level is lower than or equal to WarnLevel
bool Logger::isWarnEnabled() const
{
  return d->level <= QsLogging::WarnLevel;
}
//! Returns true if logging level is lower than or equal to ErrorLevel
bool Logger::isErrorEnabled() const
{
  return d->level <= QsLogging::ErrorLevel;
}
//! Returns true if logging level is lower than or equal to FatalLevel
bool Logger::isFatalEnabled() const
{
  return d->level <= QsLogging::FatalLevel;
}


const QString& Logger::zone() const
{
  return d->zone;
}

//! creates the complete log message and passes it to the logger
void Logger::Helper::writeToLog()
{
#ifdef USE_LOG4CPP
  log4cpp::Category & cat = log4cpp::Category::getInstance(zone.toStdString());
  cat.log(levelToPriority[level], buffer.toStdString());
#else
   const char* const levelName = LevelToText(level);
   QString s;
   QTextStream ts(&s);
   ts << QThread::currentThread();
   const QString completeMessage(QString("%1 %2 %3 %4")
      .arg(QDateTime::currentDateTime().toString(fmtDateTime))
      .arg(levelName, 5)
      .arg(s)
      .arg(buffer)
      );

   Logger& logger = Logger::instance(zone);
   QMutexLocker lock(&logger.d->logMutex);
   logger.write(completeMessage);
#endif
}

Logger::Helper::~Helper()
{
   try
   {
      writeToLog();
   }
   catch(std::exception& e)
   {
      // you shouldn't throw exceptions from a sink
      Q_UNUSED(e);
      assert(!"exception in logger helper destructor");
      throw;
   }
}

//! sends the message to all the destinations
void Logger::write(const QString& message)
{
  if (message.isNull())
  {
    std::cerr << "Logger::write null message for " << d->zone.toUtf8().constData() << std::endl;
    return;
  }
   for(DestinationList::iterator it = d->destList.begin(),
       endIt = d->destList.end();it != endIt;++it)
   {
      if( !(*it) )
      {
         assert(!"null log destination");
         continue;
      }
      (*it)->write(message, d->zone);
   }
}

} // end namespace

