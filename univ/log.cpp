/************************************************************************/
/* 				StrandedError.cpp --- Error Message                     */
/************************************************************************/
#include "stdafx.h"
#include "univ.h"
#include "log.h"
#include "threadpool.h"
using namespace Sloong;

const string g_strStart = "---------------------------------Start---------------------------------";
const string g_strEnd = "----------------------------------End----------------------------------";

#ifndef _WINDOWS
#include <errno.h>
#define PATH_SEPARATOR '/'
#else
#define PATH_SEPARATOR '\\'
#endif // !_WINDOWS

void Sloong::Universal::CLog::Log(const string &strErrorText, const string &strTitle, DWORD dwCode /* = 0 */, bool bFormatSysMsg /* = false */)
{
	WriteLine(Helper::Format("[%s]:[%s]", strTitle.c_str(), strErrorText.c_str()));

	if (bFormatSysMsg)
	{
		DWORD dwSysCode;
		string errMsg;
#ifdef _WINDOWS
		dwSysCode = GetLastError();
		errMsg = CUniversal::toansi(CUniversal::FormatWindowsErrorMessage(dwSysCode));
		errMsg = errMsg.substr(0, errMsg.length() - 2);
#else
		dwSysCode = errno;
		errMsg = strerror(dwSysCode);
#endif
		if (0 != dwSysCode)
		{
			string str = Helper::Format("[SYS CODE]:[%d];[SYSTEM MESSAGE]:[%s]", dwSysCode, errMsg.c_str());
			WriteLine(str);
		}
	}
}

void Sloong::Universal::CLog::Log(const string &strErrorText, LOGLEVEL level)
{
	switch (level)
	{
	case LOGLEVEL::Assert:
		Assert(strErrorText);
		break;
	case LOGLEVEL::Debug:
		Debug(strErrorText);
		break;
	case LOGLEVEL::Error:
		Error(strErrorText);
		break;
	case LOGLEVEL::Fatal:
		Fatal(strErrorText);
		break;
	case LOGLEVEL::Info:
		Info(strErrorText);
		break;
	case LOGLEVEL::Verbos:
		Verbos(strErrorText);
		break;
	case LOGLEVEL::Warn:
		Warn(strErrorText);
		break;
	default:
		break;
	}
}

void Sloong::Universal::CLog::Write(const string &szMessage)
{
	lock_guard<mutex> list_lock(m_mutexLogCache);
	m_listLogCache.push(szMessage);
	m_CV.notify_all();
}

void CLog::ProcessLogList()
{
	lock_guard<mutex> list_lock(m_mutexLogPool);
	while (!m_listLogPool.empty())
	{
		// get log message from queue.
		string str = m_listLogPool.front();
		m_listLogPool.pop();
		if (m_emOperation & LOGOPT::WriteToSTDOut)
			cout << str;
		if ((m_emOperation & LOGOPT::WriteToFile) && m_pFile != nullptr)
			fputs(str.c_str(), m_pFile);
		if (m_emOperation & LOGOPT::ImmediatelyFlush)
			Flush();
		if ((m_emOperation & LOGOPT::WriteToCustomFunction) && m_pCustomFunction != nullptr)
			m_pCustomFunction(str);
	}
}

void CLog::LogSystemWorkLoop()
{
	while (m_emStatus != RUN_STATUS::Exit)
	{
		if (m_emStatus == RUN_STATUS::Created)
			continue;

		if (!IsOpen())
		{
			m_CV.wait_for(lck, chrono::milliseconds(500));
			continue;
		}

		if (m_listLogCache.empty() && m_listLogPool.empty())
		{
			m_CV.wait_for(lck, chrono::milliseconds(10));
			continue;
		}

		ProcessWaitList();
		ProcessLogList();
	}
}

void CLog::ProcessWaitList()
{
	if (!m_listLogCache.empty())
	{
		lock_guard<mutex> list_lock(m_mutexLogCache);
		lock_guard<mutex> list_lock2(m_mutexLogPool);
		while (!m_listLogCache.empty())
		{
			m_listLogPool.push(m_listLogCache.front());
			m_listLogCache.pop();
		}
	}
}

bool Sloong::Universal::CLog::OpenFile()
{
	if (!m_bInit)
		Initialize();
	if (!(m_emOperation & LOGOPT::WriteToFile))
		return true;
	if (m_pFile != nullptr)
		return true;
	if (m_szFileName.empty())
		m_szFileName = BuildFileName();
		
	cout << "Open log file. Path>>" << m_szFileName << endl;

	CUniversal::CheckFileDirectory(m_szFileName);
	auto flag = "a+";
	if (m_emOperation & LOGOPT::AlwaysCreate)
		flag = "w+";

	int err_code;
#ifdef _WINDOWS
	err_code = fopen_s(&m_pFile, m_szFileName.c_str(), flag);
#else
	m_pFile = fopen(m_szFileName.c_str(), flag);
	err_code = errno;
#endif
	if (m_pFile == nullptr)
	{
		cerr << "Open file error. error no " << err_code << endl;
	}
	return m_pFile != nullptr;
}

void Sloong::Universal::CLog::End()
{
	if (!m_bInit || m_emStatus == RUN_STATUS::Exit)
		return;
	m_emStatus = RUN_STATUS::Exit;
	WriteLine(g_strEnd);
	if (IsOpen())
	{
		ProcessWaitList();
		ProcessLogList();
		Close();
	}
}

void CLog::SetConfiguration(const string &szFileName, const string &strExtendName, LOGLEVEL *pLevel, LOGOPT *pOpt)
{
	if (!szFileName.empty())
	{
		if (m_emType != LOGTYPE::ONEFILE)
		{
			m_szFilePath = Helper::Replace(szFileName, "/", Helper::ntos(PATH_SEPARATOR));
			m_szFilePath = Helper::Replace(szFileName, "\\", Helper::ntos(PATH_SEPARATOR));
			char pLast = m_szFilePath.c_str()[m_szFilePath.length() - 1];
			if (pLast != PATH_SEPARATOR)
			{
				m_szFilePath += PATH_SEPARATOR;
			}
		}
		else
		{
			assert(szFileName.c_str());
			Close();
			m_szFileName = szFileName;
		}
	}

	if (strExtendName.size() > 0)
	{
		m_strExtendName = strExtendName;
		WriteLine(Helper::Format("[Info]:[Set extend name to %d]", m_strExtendName));
	}

	if (pLevel)
	{
		m_emLevel = *pLevel;
		WriteLine(Helper::Format("[Info]:[Set log level to %d]", m_emLevel));
	}

	if (pOpt)
	{
		m_emOperation = *pOpt;
		WriteLine(Helper::Format("[Info]:[Set Operation to %d]", m_emOperation));
	}
}

void Sloong::Universal::CLog::Initialize(const string &szPathName, const string &strExtendName /*= ""*/, LOGOPT emOpt /*= WriteToFile */, LOGLEVEL emLevel /*= LOGLEVEL::All*/, LOGTYPE emType /*= LOGTYPE::ONEFILE*/)
{
	// All value init
	m_bInit = true;
	m_szFilePath.clear();
	m_szFileName.clear();
	m_emType = emType;

	SetConfiguration(szPathName, strExtendName, &emType, &emLevel, &emOpt);

	Start();
}

void Sloong::Universal::CLog::Start()
{
	if (m_emStatus == RUN_STATUS::Running)
		return;

	m_emStatus = RUN_STATUS::Running;
	CThreadPool::AddWorkThread(std::bind(&CLog::LogSystemWorkLoop, this));
	if (m_emType != LOGTYPE::ONEFILE)
		CThreadPool::AddWorkThread(std::bind(&CLog::FileNameUpdateWorkLoop, this));

	WriteLine(g_strStart);
}

string Sloong::Universal::CLog::BuildFileName()
{
	if( m_emType == LOGTYPE::ONEFILE )
		return "log.log";
	time_t now;
	struct tm *tmNow;
	time(&now);
	tmNow = localtime(&now);

	char szCurrentDate[10];
	static const char format[3][10] = {("%Y"), ("%Y-%m"), ("%Y%m%d")};
	strftime(szCurrentDate, 9, format[m_emType], tmNow);
	return Helper::Format("%s%s%s.log", m_szFilePath.c_str(), szCurrentDate, m_strExtendName.c_str());
}

void Sloong::Universal::CLog::FileNameUpdateWorkLoop()
{
	while (m_emStatus != RUN_STATUS::Exit)
	{
		time_t now;
		struct tm *tmNow;
		time(&now);
		tmNow = localtime(&now);

		if ((m_nLastDate != tmNow->tm_mday) || m_nLastDate == 0)
		{
			m_szFileName = BuildFileName();
			m_nLastDate = tmNow->tm_mday;
			Close();
		}

		if (tmNow->tm_hour != 23)
			this_thread::sleep_for(chrono::hours(23 - tmNow->tm_hour));

		if (tmNow->tm_min != 59)
			this_thread::sleep_for(chrono::minutes(59 - tmNow->tm_min));

		if (tmNow->tm_sec != 59)
			this_thread::sleep_for(chrono::seconds(59 - tmNow->tm_min));

		if (m_nLastDate == tmNow->tm_mday)
			this_thread::sleep_for(chrono::milliseconds(100));
	}
}