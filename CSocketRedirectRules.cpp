#include "pch.h"
#include "CSocketRedirectRules.h"
#include "winsock.h"

CSocketRedirectRules::CSocketRedirectRules()
{

}

CSocketRedirectRules::~CSocketRedirectRules()
{
	Clean();
}

int CSocketRedirectRules::AddRules(ULONG srcIP, USHORT srcPort, ULONG destIP, USHORT destPort)
{
	if (0 == FindRules(srcIP, srcPort))
		return 0;

	REDIRECT_RULE rule = { 0 };
	rule.srcIP = srcIP;
	rule.srcPort = srcPort;
	rule.destIP = destIP;
	rule.destPort = destPort;
	m_rules.push_back(rule);

	return 0;
}

int CSocketRedirectRules::AddRules(LPCSTR srcIP, USHORT srcPort, LPCSTR destIP, USHORT destPort)
{
	ULONG sIP = inet_addr(srcIP);
	ULONG dIP = inet_addr(destIP);

	return AddRules(sIP, srcPort, dIP, destPort);
}

int CSocketRedirectRules::RemoveRules(ULONG srcIP, USHORT srcPort)
{
	ULONG destIP = 0;
	USHORT destPort = 0;

	if (0 != FindRules(srcIP, srcPort, destIP, destPort))
		return -1;

	REDIRECT_RULE rule = { srcIP,srcPort,destIP,destPort };

	std::list<REDIRECT_RULE>::const_iterator cit = std::find(m_rules.begin(), m_rules.end(), rule);
	if (cit != m_rules.end())
	{
		m_rules.erase(cit);
		return 0;
	}

	return -2;
}

int CSocketRedirectRules::FindRules(ULONG srcIP, USHORT srcPort, ULONG& destIP, USHORT& destPort)
{
	/*if (ntohl(srcIP) == INADDR_LOOPBACK && ntohs(srcPort) == 1234)
	{
		destIP = htonl(INADDR_LOOPBACK);
		destPort = htons(8888);
	}*/

	for (std::list<REDIRECT_RULE>::const_iterator cit = m_rules.begin(); cit != m_rules.end(); ++cit)
	{
		REDIRECT_RULE rule = *cit;
		if (rule.srcIP == srcIP && rule.srcPort == srcPort)
		{
			destIP = rule.destIP;
			destPort = rule.destPort;
			break;
		}
	}

	return -1;
}

int CSocketRedirectRules::FindRules(ULONG srcIP, USHORT srcPort)
{
	ULONG destIP = 0;
	USHORT destPort = 0;

	return FindRules(srcIP, srcPort, destIP, destPort);
}

void CSocketRedirectRules::Clean()
{
	m_rules.clear();
}

bool REDIRECT_RULE::operator==(const REDIRECT_RULE& rule)
{
	if (this->srcIP == rule.srcIP &&
		this->srcPort == rule.srcPort &&
		this->destIP == rule.destPort &&
		this->destPort == rule.destPort
		)
		return true;

	return false;
}
