#include "Logger.h"
#include <stdarg.h>
#include <cxxabi.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <vector>
#include <regex>
#include "spdlog/spdlog.h"
#include "spdlog/async_logger.h"
#include "Assert.h"
#include "Exception.h"
#include "TraceListener.h"
#include "Process.h"
#include "Enviorment.h"

using namespace std;
using namespace core;
using namespace spdlog;
using namespace spdlog::sinks;
using namespace spdlog::level;

namespace core
{
	Logger& Logger::Instance()
	{
		static Logger logger;
		return logger;
	}

	Logger::~Logger()
	{
	}

	Logger::Logger(): m_severity(TraceSeverity::NoneWorking)
	{
		static_assert((int)TraceSeverity::Verbose == (int)level_enum::debug, "TraceSeverity::Verbose dosn't match spdlog debug");
		static_assert((int)TraceSeverity::Info == (int)level_enum::info, "TraceSeverity::Info dosn't match spdlog info");
		static_assert((int)TraceSeverity::Fatal == (int)level_enum::err, "TraceSeverity::Error dosn't match spdlog err");

	}

	tuple<Logger::FileName, Logger::Line, Logger::FunctionName> Logger::GetFunctionAndLine(char* mangledSymbol){
		int status;
        static regex functionManglingPattern("\\((.*)\\+(0x[0-9a-f]*)\\)\\s*\\[(0x[0-9a-f]*)\\]");
		cmatch functionMangaledMatch;
		if(regex_search(mangledSymbol, functionMangaledMatch, functionManglingPattern)) {
			unique_ptr<char, void (*)(void *)> unMangledName(
					abi::__cxa_demangle(functionMangaledMatch[1].str().c_str(), nullptr, 0, &status), &std::free);

			//Get file and line
			ChildProcess childProcess = Process::SpawnChildProcess("addr2line", "addr2line", (string("--exe=") + Enviorment::Instance().GetProcessPath().c_str()
                 + Enviorment::Instance().GetProcessName().c_str()).c_str(), functionMangaledMatch[3].str().c_str(), (const char*)NULL);

            char buffer[1024] = {0};
            LINUX_VERIFY(read(childProcess.GetStdOutPipe().GetReadDescriptor(),
                                          buffer, 1024) != -1);
			static regex fileNameLinePattern("([a-zA-Z0-9]*.[a-zA-Z0-9]*):([0-9]*)");
			cmatch fileNameLineMatch;
			ASSERT(regex_search(buffer, fileNameLineMatch, fileNameLinePattern));

			static regex functionPattern("([a-zA-Z]*::[a-zA-Z]*)");
			cmatch functionMatch;
			if(unMangledName.get() != nullptr)
				ASSERT(regex_search(unMangledName.get(), functionMatch, functionPattern));

			return make_tuple(fileNameLineMatch[1].str(), fileNameLineMatch[2].str(), functionMatch.size() >= 1 ?
                      functionMatch[1].str() : "???????");

		}
        return make_tuple("???????", "???????", "???????");
	};

	void Logger::Start(TraceSeverity severity)
	{
		ASSERT(!m_running.exchange(true));
		vector<shared_ptr<sink>> sinks;
		lock_guard<mutex> lock(m_mut);	
		{
			for(const shared_ptr<TraceListener>& listener : m_listeners)
			{
				sinks.emplace_back(listener->GetSink());
			}
		}
		m_logger = make_shared<async_logger>("Logger", sinks.begin(), sinks.end(), 4096, 
			async_overflow_policy::discard_log_msg);
		m_logger->set_level(level_enum(severity));
		m_severity = severity;
	}
	void Logger::Log(TraceSeverity severity, const string& message)
	{
		m_logger->log((level_enum)severity, message.c_str());
        Flush();
	}

	void Logger::Flush()
	{
		m_logger->flush();
	}

	void Logger::AddListener(const shared_ptr<TraceListener>& listener)
	{
		lock_guard<mutex> lock(m_mut);
		m_listeners.emplace_back(listener);
	}

	string Logger::FormatMessage(const Source& source, const char* format, ...)
	{
		va_list arguments;
		va_start(arguments, format);
		string result;

		char buf[Local_buffer_size] = "";
		int size = snprintf(buf, Local_buffer_size, "%s:%s:%d\t", source.file, 
			source.function, source.line);
		ASSERT(size < Local_buffer_size);
		size += vsnprintf(buf + size, Local_buffer_size - size, format, arguments);
		if(size < Local_buffer_size)
			result = buf;
		else //message was trunced or operation failed
		{
			int bufferSize = max(size, 32 * 1024);
			vector<char> largerBuf;
			largerBuf.resize(bufferSize);
			int largerSize = snprintf(&largerBuf[0], bufferSize, "%s:%s:%d\t", source.file, 
				source.function, source.line);
			ASSERT(largerSize < bufferSize);
			vsnprintf(&largerBuf[largerSize], bufferSize - largerSize, format, arguments);
			result = string(largerBuf.begin(), largerBuf.end());
		}

		va_end(arguments);
		return result;
	}
}
