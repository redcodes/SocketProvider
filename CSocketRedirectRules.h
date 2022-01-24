#pragma once
class CSocketRedirectRules
{
public:
	CSocketRedirectRules();
	virtual ~CSocketRedirectRules();
public:
	int AddRules(ULONG srcIP, USHORT srcPort, ULONG destIP, USHORT destPort);
	int RemoveRules(ULONG srcIP, USHORT srcPort);
	int FindRules(ULONG srcIP,USHORT srcPort,ULONG& destIP,USHORT& destPort);
};

