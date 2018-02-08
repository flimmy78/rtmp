#pragma once
#include <map>
#include <string>
#include "CircularQueue.h"

using namespace std;

struct CBuffer
{
	unsigned int lenght;
	unsigned char* data;
	unsigned long long pts;
};

class CBufferInfo
{
public:
	CBufferInfo(int nQueueSize = 6);
	virtual ~CBufferInfo();
public:
	CCircularQueue<CBuffer>* usingQueue(){ return m_BufferQueue; };

public:
	CCircularQueue<CBuffer>* m_BufferQueue;
};

class CBufferManager
{
public:
	CBufferManager();
	virtual ~CBufferManager();

public:
	bool Init(unsigned int maxLength);
	bool Register(string name, CBufferInfo* info);
	bool UnRegister(string name);
	void Sync(); 
	bool Write(unsigned char* data, unsigned int length, unsigned long long pts);

private:
	map<string, CBufferInfo*> 	m_RegMap;	//user map - witch use this buffer
	unsigned char* m_pStart;	//buffer start point
	unsigned int m_Lenght;	//total buffer length
	unsigned int m_Offset;	//current position
};
