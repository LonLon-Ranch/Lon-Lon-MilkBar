#pragma once

#include "CharacterDTO.h"
#include "DeathSwapDTO.h"
#include "EnemyDTO.h"
#include "ModelsDTO.h"
#include "NamesDTO.h"
#include "NetworkDTO.h"
#include "PropHuntDTO.h"
#include "QuestDTO.h"
#include "TeleportDTO.h"
#include "WorldDTO.h"

namespace DTO
{
class ServerDTO
{
  public:
    WorldDTO* WorldData;
    NamesDTO* NameData;
    ModelsDTO* ModelData;
    std::vector<CloseCharacterDTO*> ClosePlayers;
    std::vector<FarCharacterDTO*> FarPlayers;
    EnemyDTO* EnemyData;
    QuestDTO* QuestData;
    NetworkDTO* NetworkData;
    DeathSwapDTO* DeathSwapData;
    TeleportDTO* TeleportData;
    PropHuntDTO* PropHuntData;
    bool* pvp;
};

} // namespace DTO