#include "CIPC.h"
#include "CNamedPipe.h"

CIPC* CIPC::GetInstance()
{
	return new CNamedPipe;
}
