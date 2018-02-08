#include <iostream> 
#include <string.h>
#include "BufferManager.h"
using namespace std;

CBufferManager::CBufferManager()
{
}


CBufferManager::~CBufferManager()
{
	if (m_pStart != NULL)
	{
		delete[] m_pStart;
		m_pStart = NULL;
	}
}

bool CBufferManager::Init(unsigned int maxLength)
{
	m_pStart = new unsigned char[maxLength];
	if (m_pStart == 0)
	{
		//debug
		fprintf(stderr, "new maxLength:%d error\n", maxLength);
		return false;
	}

	m_Lenght = maxLength;
	m_Offset = 0;

	return true;
}

bool CBufferManager::Register(string name, CBufferInfo* info)
{
	pair<map<string, CBufferInfo*>::iterator, bool> ret;
	ret = m_RegMap.insert(pair<string, CBufferInfo*>(name, info));
	if (!ret.second)
	{
		fprintf(stderr, "CmdDespatch::RegisterCmdHandle L_Insert name:%s error\n", name.c_str());
		return false;
	}

	fprintf(stderr, "CmdDespatch::RegisterCmdHandle L_Insert name:%s success\n", name.c_str());

	return true;
}

bool CBufferManager::UnRegister(string name)
{
	map<string, CBufferInfo*>::iterator l_it;
	l_it = m_RegMap.find(name);

	if (l_it == m_RegMap.end())
	{
		fprintf(stderr, "UnregisterCmdHandle name:%s error\n", name.c_str());
		return false;
	}
	else
	{
		fprintf(stderr, "UnregisterCmdHandle name:%s erase\n", name.c_str());
		m_RegMap.erase(l_it);//delete 112;
		return true;
	}
}
void CBufferManager::Sync()
{
	map<string, CBufferInfo*>::iterator l_it;
	for (l_it = m_RegMap.begin(); l_it != m_RegMap.end(); l_it++)
	{
		string name = l_it->first;
		CBufferInfo* pInfo = l_it->second;
		if (pInfo != NULL)
		{
			CCircularQueue<CBuffer>* bufferQueue = pInfo->usingQueue(); 

			bufferQueue->SyncRwPoint();
		}
	}

}
bool CBufferManager::Write(unsigned char* data, unsigned int length, unsigned long long pts)
{
	if (data == NULL || length <= 0 || m_pStart == NULL)
	{
		return false;
	}

	if (length > m_Lenght) return false;

	//default: We believe that the cache is large enough, there is no cover immediately
	if (m_Offset + length > m_Lenght)
	{
		m_Offset = 0;
	}

	//fprintf(stderr, "Write data:%p length:%u pts:%llu \n", data, length, pts);
	unsigned char* pos = m_pStart + m_Offset;

	memcpy(pos, data, length);
	m_Offset += length;

	//Traverse Map, updated info
	map<string, CBufferInfo*>::iterator l_it;
	for (l_it = m_RegMap.begin(); l_it != m_RegMap.end(); l_it++)
	{
		string name = l_it->first;
		CBufferInfo* pInfo = l_it->second;
		if (pInfo != NULL)
		{
			CCircularQueue<CBuffer>* bufferQueue = pInfo->usingQueue();
			CBuffer buffer;
			buffer.data = pos;
			buffer.lenght = length;
			buffer.pts = pts;

			bufferQueue->PushBack(buffer);
		}
	}

	return true;
}

///////////////////////////////////////////////////////////////////////////////////////

CBufferInfo::CBufferInfo(int nQueueSize)
{
	m_BufferQueue = new CCircularQueue<CBuffer>(nQueueSize);
}

CBufferInfo::~CBufferInfo()
{
	if (m_BufferQueue != NULL)
	{
		delete m_BufferQueue;
		m_BufferQueue = NULL;
	}
}
