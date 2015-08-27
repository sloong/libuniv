#include "stdafx.h"
#include "file.h"
#include "exception.h"
#include "univ.h"

using namespace Sloong::Universal;
// This function will load one line string from file.
// if the file is empty, return empty string.
LPCTSTR Sloong::Universal::CFile::GetLine()
{
	int index = 0;
	int length = 0;

	while (true)
	{
		if (!fgetws(m_szBuffer, 256, m_pFileStream))
		{
			return NULL;
		}

		for (length = (int)_tcslen(m_szBuffer), index = 0; isspace(m_szBuffer[index]); index++);

		if (index >= length || m_szBuffer[index] == '#')
		{
			continue;
		}

		return &m_szBuffer[index];
	}
}

Sloong::Universal::CFile::CFile()
{

}

Sloong::Universal::CFile::~CFile()
{
	if (m_pFileStream)
	{
		fclose(m_pFileStream);
		m_pFileStream = nullptr;
	}

}

HRESULT Sloong::Universal::CFile::Open(CString strFileName, OpenFileAccess emMode)
{
	if (!Access(strFileName, emMode))
		return S_FALSE;

	//CreateFile(strFileName,)
	return S_OK;
}

void Sloong::Universal::CFile::Close()
{

}

errno_t Sloong::Universal::CFile::OpenStream(CString szFileName, CString szMode)
{
	if (!Access(szFileName, Exist))
	{
		throw CException(CUniversal::FormatW(L"Can't open file stream %s, the file is not existing.", szFileName));
	}
	return _tfopen_s(&m_pFileStream, szFileName.c_str(), szMode.c_str());
}

bool Sloong::Universal::CFile::Access(CString szFileName, OpenFileAccess emMode)
{
	if (0 == _taccess_s(szFileName.c_str(), emMode))
		return true;
	else
		return false;
}
