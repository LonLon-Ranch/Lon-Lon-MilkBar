#pragma once

#include "ClientDTO.h"
#include "ServerDTO.h"
#include <Windows.h>

namespace Serialization
{
static short currentIndex = 0;
static byte ClientData[7168];

static class Serializer
{
  private:
    static void copyData(void* Dst, const void* Src, int size)
    {
        memcpy(Dst, Src, size);
        currentIndex += size;
    }

    static int copyIntFromShort(const void* Src)
    {
        short temp = 0;
        copyData(&temp, Src, 2);

        return temp;
    }

    static DTO::WorldDTO* DeserializeWorldData(std::vector<byte> input);
    static DTO::NamesDTO* DeserializeNameData(std::vector<byte> input);
    static DTO::ModelsDTO* DeserializeModelDTO(std::vector<byte> input);
    static DTO::CloseCharacterDTO* DeserializeCloseCharacter(std::vector<byte> input);
    static DTO::FarCharacterDTO* DeserializeFarCharacter(std::vector<byte> input);
    static DTO::EnemyDTO* DeserializeEnemyData(std::vector<byte> input);
    static DTO::QuestDTO* DeserializeQuestData(std::vector<byte> input);
    static DTO::NetworkDTO* DeserializeNetworkData(std::vector<byte> input);
    static DTO::DeathSwapDTO* DeserializeDeathSwapData(std::vector<byte> input);
    static DTO::TeleportDTO* DeserializeTeleportData(std::vector<byte> input);
    static DTO::PropHuntDTO* DeserializePropHuntData(std::vector<byte> input);
    static DataTypes::ModelData* DeserializeModelData(std::vector<byte> input);
    static DataTypes::BumiiData* DeserializeBumiiData(std::vector<byte> input);

    static void SerializeWorldData(DTO::WorldDTO* input);
    static void SerializeCharacterData(DTO::ClientCharacterDTO* input);
    static void SerializeEnemyData(DTO::EnemyDTO* input);
    static void SerializeQuestData(DTO::QuestDTO* input);

  public:
    static DTO::ServerDTO* DeserializeServerData(byte* inputBytes);
    static void SerializeConnectData(byte* outputArray, std::string name, std::string password, std::string modelType, std::string modelData);
    static void SerializeDisconnectData(byte* outputArray, std::string reason);
    static void SerializeClientData(byte* outputArray, DTO::ClientDTO* input);

    static void CopyToArray(byte* array);
    static std::string CopyString(std::vector<byte> input);
};

} // namespace Serialization