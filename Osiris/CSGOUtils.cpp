#include "CSGOUtils.h"

#include "Memory.h"
#include "SDK/ClientState.h"
#include "SDK/SplitScreen.h"

ClientState* CSGOUtils::getClientState() noexcept
{
	return reinterpret_cast<ClientState*>(std::uintptr_t(&memory->splitScreen->splitScreenPlayers[0]->client) - 8);
}