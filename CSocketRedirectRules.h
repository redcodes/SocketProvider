#pragma once
#include <list>

struct REDIRECT_RULE
{
	ULONG srcIP;
	USHORT srcPort;
	ULONG destIP;
	USHORT destPort;

	bool operator==(const REDIRECT_RULE& rule);
};

class CSocketRedirectRules
{
public:
	CSocketRedirectRules();
	virtual ~CSocketRedirectRules();
public:
	int AddRules(ULONG srcIP, USHORT srcPort, ULONG destIP, USHORT destPort);
	int AddRules(LPCSTR srcIP, USHORT srcPort, LPCSTR destIP, USHORT destPort);
	int RemoveRules(ULONG srcIP, USHORT srcPort);
	int FindRules(ULONG srcIP, USHORT srcPort, ULONG& destIP, USHORT& destPort);
private:
	std::list<REDIRECT_RULE> m_rules;
	int FindRules(ULONG srcIP, USHORT srcPort);
	void Clean();
};

