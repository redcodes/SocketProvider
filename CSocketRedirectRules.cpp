#include "pch.h"
#include "CSocketRedirectRules.h"
#include "winsock.h"

CSocketRedirectRules::CSocketRedirectRules()
{

}

CSocketRedirectRules::~CSocketRedirectRules()
{

}

int CSocketRedirectRules::AddRules(ULONG srcIP, USHORT srcPort, ULONG destIP, USHORT destPort)
{
	return 0;
}

int CSocketRedirectRules::RemoveRules(ULONG srcIP, USHORT srcPort)
{
	return 0;
}

int CSocketRedirectRules::FindRules(ULONG srcIP, USHORT srcPort, ULONG& destIP, USHORT& destPort)
{
	if (ntohl(srcIP) == INADDR_LOOPBACK && ntohs(srcPort) == 1234)
	{
		destIP = htonl(INADDR_LOOPBACK);
		destPort = htons(8888);
	}

	return 0;
}
