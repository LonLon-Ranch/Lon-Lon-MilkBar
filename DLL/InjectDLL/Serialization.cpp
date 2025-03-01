#include "Serialization.h"
#include "LoggerService.h"
#include <map>
#include <sstream>

using namespace Serialization;

#pragma region Deserialization

DTO::ServerDTO* Serializer::DeserializeServerData(byte* inputBytes)
{
    currentIndex = 0;

    std::vector<byte> input;
    std::copy(&inputBytes[0], &inputBytes[7168], std::back_inserter(input));

    DTO::ServerDTO* result = new DTO::ServerDTO();

    result->WorldData = DeserializeWorldData(input);
    result->NameData = DeserializeNameData(input);
    result->ModelData = DeserializeModelDTO(input);

    byte ClosePlayers = 0;
    copyData(&ClosePlayers, &input[0] + currentIndex, 1);

    for (int i = 0; i < ClosePlayers; i++)
        result->ClosePlayers.push_back(DeserializeCloseCharacter(input));

    byte FarPlayers = 0;
    copyData(&FarPlayers, &input[0] + currentIndex, 1);

    for (int i = 0; i < FarPlayers; i++)
        result->FarPlayers.push_back(DeserializeFarCharacter(input));

    result->EnemyData = DeserializeEnemyData(input);
    result->QuestData = DeserializeQuestData(input);
    result->NetworkData = DeserializeNetworkData(input);
    result->DeathSwapData = DeserializeDeathSwapData(input);
    result->TeleportData = DeserializeTeleportData(input);
    result->PropHuntData = DeserializePropHuntData(input);
    result->pvp = DeserializePVPData(input);

    return result;
}

DTO::NamesDTO* Serializer::DeserializeNameData(std::vector<byte> input)
{
    DTO::NamesDTO* result = new DTO::NamesDTO();

    byte DictSize = 0;
    copyData(&DictSize, &input[0] + currentIndex, 1);

    for (byte i = 0; i < DictSize; i++)
    {
        byte Player;
        copyData(&Player, &input[0] + currentIndex, 1);

        std::string stringToAdd = CopyString(input);

        result->Names.insert({Player, stringToAdd});
    }

    return result;
}

DTO::ModelsDTO* Serializer::DeserializeModelDTO(std::vector<byte> input)
{
    DTO::ModelsDTO* result = new DTO::ModelsDTO();

    byte DictSize = 0;
    copyData(&DictSize, &input[0] + currentIndex, 1);

    for (byte i = 0; i < DictSize; i++)
    {
        byte Player;
        copyData(&Player, &input[0] + currentIndex, 1);

        ModelData* modelData = DeserializeModelData(input);

        ModelData modelDataToAdd = ModelData();
        modelDataToAdd.ModelType = modelData->ModelType;
        modelDataToAdd.Model = modelData->Model;
        modelDataToAdd.Bumii = modelData->Bumii;

        result->Models.insert({Player, modelDataToAdd});
    }

    return result;
}

DataTypes::ModelData* Serializer::DeserializeModelData(std::vector<byte> input)
{
    DataTypes::ModelData* result = new DataTypes::ModelData();

    copyData(&result->ModelType, &input[0] + currentIndex, 1);

    if (result->ModelType < 2)
    {
        result->Model = CopyString(input);
    } else
    {
        BumiiData* bumiiData = DeserializeBumiiData(input);

        BumiiData bumiiDataToAdd = BumiiData();
        bumiiDataToAdd.ffsd = bumiiData->ffsd;
        bumiiDataToAdd.body = bumiiData->body;
        bumiiDataToAdd.personal = bumiiData->personal;
        bumiiDataToAdd.common = bumiiData->common;
        bumiiDataToAdd.shape = bumiiData->shape;
        bumiiDataToAdd.hair = bumiiData->hair;
        bumiiDataToAdd.eye = bumiiData->eye;
        bumiiDataToAdd.eye_ctrl = bumiiData->eye_ctrl;
        bumiiDataToAdd.eyebrow = bumiiData->eyebrow;
        bumiiDataToAdd.nose = bumiiData->nose;
        bumiiDataToAdd.mouth = bumiiData->mouth;
        bumiiDataToAdd.beard = bumiiData->beard;
        bumiiDataToAdd.glass = bumiiData->glass;
        bumiiDataToAdd.korog = bumiiData->korog;
        bumiiDataToAdd.goron = bumiiData->goron;
        bumiiDataToAdd.gerudo = bumiiData->gerudo;
        bumiiDataToAdd.rito = bumiiData->rito;
        bumiiDataToAdd.zora = bumiiData->zora;

        result->Bumii = bumiiDataToAdd;
    }

    return result;
}

DataTypes::BumiiData* Serializer::DeserializeBumiiData(std::vector<byte> input)
{
    DataTypes::BumiiData* result = new DataTypes::BumiiData();

    copyData(&result->ffsd.no_use_ffsd + 3, &input[0] + currentIndex, 1);
    result->ffsd.type = copyIntFromShort(&input[0] + currentIndex);
    result->body.race = copyIntFromShort(&input[0] + currentIndex);
    result->body.type = copyIntFromShort(&input[0] + currentIndex);
    result->body.number = copyIntFromShort(&input[0] + currentIndex);
    result->body.weight = copyIntFromShort(&input[0] + currentIndex);
    result->body.height = copyIntFromShort(&input[0] + currentIndex);
    result->personal.sex_age = copyIntFromShort(&input[0] + currentIndex);
    result->personal.fav_color = copyIntFromShort(&input[0] + currentIndex);
    result->personal.sub_color_1 = copyIntFromShort(&input[0] + currentIndex);
    result->personal.sub_color_2 = copyIntFromShort(&input[0] + currentIndex);
    result->personal.head_fav_color = copyIntFromShort(&input[0] + currentIndex);
    result->personal.shoulder_fav_color = copyIntFromShort(&input[0] + currentIndex);
    result->personal.shoulder_sub_color_1 = copyIntFromShort(&input[0] + currentIndex);
    result->common.backpack = copyIntFromShort(&input[0] + currentIndex);
    result->common.hat = copyIntFromShort(&input[0] + currentIndex);
    copyData(&result->common.no_hat_always + 3, &input[0] + currentIndex, 1);
    result->common.body_correct = copyIntFromShort(&input[0] + currentIndex);
    copyData(&result->common.is_mid_age + 3, &input[0] + currentIndex, 1);
    copyData(&result->common.rot_cravicle, &input[0] + currentIndex, 4);
    copyData(&result->common.rot_arm, &input[0] + currentIndex, 4);
    copyData(&result->common.rot_leg, &input[0] + currentIndex, 4);
    copyData(&result->common.rot_crotch, &input[0] + currentIndex, 4);
    result->shape.jaw = copyIntFromShort(&input[0] + currentIndex);
    result->shape.wrinkle = copyIntFromShort(&input[0] + currentIndex);
    result->shape.make = copyIntFromShort(&input[0] + currentIndex);
    copyData(&result->shape.trans_v, &input[0] + currentIndex, 4);
    copyData(&result->shape.scale, &input[0] + currentIndex, 4);
    result->shape.skin_color = copyIntFromShort(&input[0] + currentIndex);
    result->hair.type = copyIntFromShort(&input[0] + currentIndex);
    result->hair.color = copyIntFromShort(&input[0] + currentIndex);
    copyData(&result->hair.flip + 3, &input[0] + currentIndex, 1);
    result->eye.type = copyIntFromShort(&input[0] + currentIndex);
    result->eye.color = copyIntFromShort(&input[0] + currentIndex);
    copyData(&result->eye.trans_v, &input[0] + currentIndex, 4);
    copyData(&result->eye.trans_u, &input[0] + currentIndex, 4);
    copyData(&result->eye.rotate, &input[0] + currentIndex, 4);
    copyData(&result->eye.scale, &input[0] + currentIndex, 4);
    copyData(&result->eye.aspect, &input[0] + currentIndex, 4);
    copyData(&result->eye.eyeball_trans_u, &input[0] + currentIndex, 4);
    copyData(&result->eye.eyeball_trans_v, &input[0] + currentIndex, 4);
    copyData(&result->eye.eyeball_scale, &input[0] + currentIndex, 4);
    result->eye.highlight_bright = copyIntFromShort(&input[0] + currentIndex);
    copyData(&result->eye_ctrl.base_offset, &input[0] + currentIndex, 12);
    copyData(&result->eye_ctrl.translim_out, &input[0] + currentIndex, 4);
    copyData(&result->eye_ctrl.translim_in, &input[0] + currentIndex, 4);
    copyData(&result->eye_ctrl.translim_d, &input[0] + currentIndex, 4);
    copyData(&result->eye_ctrl.translim_u, &input[0] + currentIndex, 4);
    copyData(&result->eye_ctrl.neck_offset_ud, &input[0] + currentIndex, 4);
    result->eyebrow.type = copyIntFromShort(&input[0] + currentIndex);
    result->eyebrow.color = copyIntFromShort(&input[0] + currentIndex);
    copyData(&result->eyebrow.trans_v, &input[0] + currentIndex, 4);
    copyData(&result->eyebrow.trans_u, &input[0] + currentIndex, 4);
    copyData(&result->eyebrow.rotate, &input[0] + currentIndex, 4);
    copyData(&result->eyebrow.scale, &input[0] + currentIndex, 4);
    copyData(&result->eyebrow.aspect, &input[0] + currentIndex, 4);
    result->nose.type = copyIntFromShort(&input[0] + currentIndex);
    copyData(&result->nose.trans_v, &input[0] + currentIndex, 4);
    copyData(&result->nose.scale, &input[0] + currentIndex, 4);
    result->mouth.type = copyIntFromShort(&input[0] + currentIndex);
    result->mouth.color = copyIntFromShort(&input[0] + currentIndex);
    copyData(&result->mouth.trans_v, &input[0] + currentIndex, 4);
    copyData(&result->mouth.scale, &input[0] + currentIndex, 4);
    copyData(&result->mouth.aspect, &input[0] + currentIndex, 4);
    result->beard.mustache = copyIntFromShort(&input[0] + currentIndex);
    copyData(&result->beard.scale, &input[0] + currentIndex, 4);
    result->beard.type = copyIntFromShort(&input[0] + currentIndex);
    result->beard.color = copyIntFromShort(&input[0] + currentIndex);
    result->glass.type = copyIntFromShort(&input[0] + currentIndex);
    result->glass.color = copyIntFromShort(&input[0] + currentIndex);
    result->korog.mask = copyIntFromShort(&input[0] + currentIndex);
    result->korog.skin_color = copyIntFromShort(&input[0] + currentIndex);
    result->korog.left_plant = copyIntFromShort(&input[0] + currentIndex);
    result->korog.right_plant = copyIntFromShort(&input[0] + currentIndex);
    result->goron.skin_color = copyIntFromShort(&input[0] + currentIndex);
    result->gerudo.hair = copyIntFromShort(&input[0] + currentIndex);
    result->gerudo.hair_color = copyIntFromShort(&input[0] + currentIndex);
    result->gerudo.glass = copyIntFromShort(&input[0] + currentIndex);
    result->gerudo.glass_color = copyIntFromShort(&input[0] + currentIndex);
    result->gerudo.skin_color = copyIntFromShort(&input[0] + currentIndex);
    result->gerudo.lip_color = copyIntFromShort(&input[0] + currentIndex);
    result->rito.body_color = copyIntFromShort(&input[0] + currentIndex);
    result->rito.hair_color = copyIntFromShort(&input[0] + currentIndex);
    result->zora.body_color = copyIntFromShort(&input[0] + currentIndex);

    return result;
}

DTO::WorldDTO* Serializer::DeserializeWorldData(std::vector<byte> input)
{
    DTO::WorldDTO* result = new DTO::WorldDTO();
    copyData(&result->Time, &input[0] + currentIndex, 12);

    return result;
}

DTO::CloseCharacterDTO* Serializer::DeserializeCloseCharacter(std::vector<byte> input)
{
    DTO::CloseCharacterDTO* result = new DTO::CloseCharacterDTO();

    copyData(&result->PlayerNumber, &input[0] + currentIndex, 1);
    // copyData(&result->Status, &input[0] + currentIndex, 1);
    copyData(&result->Updated, &input[0] + currentIndex, 1);
    copyData(&result->Position, &input[0] + currentIndex, 12);
    copyData(&result->Rotation1, &input[0] + currentIndex, 16);
    copyData(&result->Rotation2, &input[0] + currentIndex, 16);
    copyData(&result->Rotation3, &input[0] + currentIndex, 16);
    copyData(&result->Rotation4, &input[0] + currentIndex, 16);
    copyData(&result->Animation, &input[0] + currentIndex, 4);
    copyData(&result->Health, &input[0] + currentIndex, 4);
    copyData(&result->AtkUp, &input[0] + currentIndex, 4);
    copyData(&result->IsEquipped, &input[0] + currentIndex, 1);
    // copyData(&result->Equipment, &input[0] + currentIndex, 13);
    copyData(&result->Equipment.WType, &input[0] + currentIndex, 1);
    copyData(&result->Equipment.Sword, &input[0] + currentIndex, 2);
    copyData(&result->Equipment.Shield, &input[0] + currentIndex, 2);
    copyData(&result->Equipment.Bow, &input[0] + currentIndex, 2);
    copyData(&result->Equipment.Head, &input[0] + currentIndex, 2);
    copyData(&result->Equipment.Upper, &input[0] + currentIndex, 2);
    copyData(&result->Equipment.Lower, &input[0] + currentIndex, 2);

    std::map<byte, std::string> MapDict = {{1, "MainField"}, {2, "MainFieldDungeon"}, {3, "CDungeon"}, {4, "AoCField"}};
    std::map<byte, std::string> DvnBstDict = {{1, "RemainsWind"}, {2, "RemainsWater"}, {3, "RemainsElectric"}, {4, "RemainsFire"}};
    std::map<int, std::string> LetterDict = {{0, "A"}, {1, "B"}, {2, "C"}, {3, "D"}, {4, "E"}, {5, "F"}, {6, "G"}, {7, "H"}, {8, "I"}, {9, "J"}};

    byte MapByte = 0;
    byte SectByte = 0;

    copyData(&MapByte, &input[0] + currentIndex, 1);
    copyData(&SectByte, &input[0] + currentIndex, 1);

    if (MapDict.count(MapByte))
        result->Location.Map = MapDict[MapByte];

    int Number;
    int SectLetter;

    switch (MapByte)
    {
    case 1:
        Number = SectByte % 8;
        SectLetter = (SectByte - (Number)) / 8;
        result->Location.Section = LetterDict[SectLetter] + "-" + std::to_string(Number);
        break;
    case 2:
        if (DvnBstDict.count(SectByte))
            result->Location.Section = DvnBstDict[SectByte];
        break;
    case 3:
        std::string NumberString = std::to_string(SectByte);

        if (NumberString.size() == 1)
            NumberString = "00" + NumberString;
        else if (NumberString.size() == 2)
            NumberString = "0" + NumberString;

        result->Location.Section = "Dungeon" + NumberString;

        break;
    }

    copyData(&result->Bomb, &input[0] + currentIndex, 48);

    return result;
}

DTO::FarCharacterDTO* Serializer::DeserializeFarCharacter(std::vector<byte> input)
{
    DTO::FarCharacterDTO* result = new DTO::FarCharacterDTO();

    copyData(&result->PlayerNumber, &input[0] + currentIndex, 1);
    // copyData(&result->Status, &input[0] + currentIndex, 1);
    copyData(&result->Updated, &input[0] + currentIndex, 1);
    copyData(&result->Position, &input[0] + currentIndex, 12);

    std::map<byte, std::string> MapDict = {{1, "MainField"}, {2, "MainFieldDungeon"}, {3, "CDungeon"}, {4, "AoCField"}};
    std::map<byte, std::string> DvnBstDict = {{1, "RemainsWind"}, {2, "RemainsWater"}, {3, "RemainsElectric"}, {4, "RemainsFire"}};
    std::map<int, std::string> LetterDict = {{0, "A"}, {1, "B"}, {2, "C"}, {3, "D"}, {4, "E"}, {5, "F"}, {6, "G"}, {7, "H"}, {8, "I"}, {9, "J"}};

    byte MapByte = 0;
    byte SectByte = 0;

    copyData(&MapByte, &input[0] + currentIndex, 1);
    copyData(&SectByte, &input[0] + currentIndex, 1);

    if (MapDict.count(MapByte))
        result->Location.Map = MapDict[MapByte];

    int Number;
    int SectLetter;

    switch (MapByte)
    {
    case 1:
        Number = SectByte % 8;
        SectLetter = (SectByte - (Number)) / 8;
        result->Location.Section = LetterDict[SectLetter] + "-" + std::to_string(Number);
        break;
    case 2:
        if (DvnBstDict.count(SectByte))
            result->Location.Section = DvnBstDict[SectByte];
        break;
    case 3:
        std::string NumberString = std::to_string(SectByte);

        if (NumberString.size() == 1)
            NumberString = "00" + NumberString;
        else if (NumberString.size() == 2)
            NumberString = "0" + NumberString;

        result->Location.Section = "Dungeon" + NumberString;

        break;
    }

    return result;
}

DTO::EnemyDTO* Serializer::DeserializeEnemyData(std::vector<byte> input)
{
    DTO::EnemyDTO* result = new DTO::EnemyDTO();

    byte ListSize = 0;
    copyData(&ListSize, &input[0] + currentIndex, 1);

    for (int i = 0; i < ListSize; i++)
    {
        EnemyData* data = new EnemyData();
        copyData(&data->Hash, &input[0] + currentIndex, 4);
        copyData(&data->Health, &input[0] + currentIndex, 4);

        EnemyData dataToAdd = EnemyData();
        dataToAdd.Hash = data->Hash;
        dataToAdd.Health = data->Health;

        result->Health.push_back(dataToAdd);
    }

    return result;
}

DTO::QuestDTO* Serializer::DeserializeQuestData(std::vector<byte> input)
{
    DTO::QuestDTO* result = new DTO::QuestDTO();

    byte ListSize = 0;
    copyData(&ListSize, &input[0] + currentIndex, 1);

    for (int i = 0; i < ListSize; i++)
    {
        std::string stringToAdd = CopyString(input);

        result->Completed.push_back(stringToAdd);
    }

    return result;
}

DTO::NetworkDTO* Serializer::DeserializeNetworkData(std::vector<byte> input)
{
    DTO::NetworkDTO* result = new DTO::NetworkDTO();

    copyData(&result->SerializationRate, &input[0] + currentIndex, 2);
    copyData(&result->TargetFPS, &input[0] + currentIndex, 2);
    copyData(&result->SleepMultiplier, &input[0] + currentIndex, 2);
    copyData(&result->isLocalTest, &input[0] + currentIndex, 1);
    copyData(&result->isCharacterSpawn, &input[0] + currentIndex, 1);
    copyData(&result->DisplayNames, &input[0] + currentIndex, 1);
    copyData(&result->GlyphDistance, &input[0] + currentIndex, 2);
    copyData(&result->GlyphTime, &input[0] + currentIndex, 2);
    copyData(&result->isQuestSync, &input[0] + currentIndex, 1);
    copyData(&result->isEnemySync, &input[0] + currentIndex, 1);
    copyData(&result->AlwaysDay, &input[0] + currentIndex, 1);
    

    return result;
}

DTO::DeathSwapDTO* Serializer::DeserializeDeathSwapData(std::vector<byte> input)
{
    DTO::DeathSwapDTO* result = new DTO::DeathSwapDTO();

    copyData(&result->Phase, &input[0] + currentIndex, 1);
    copyData(&result->Position, &input[0] + currentIndex, 12);

    return result;
}

DTO::TeleportDTO* Serializer::DeserializeTeleportData(std::vector<byte> input)
{
    DTO::TeleportDTO* result = new DTO::TeleportDTO();
    copyData(&result->Destination, &input[0] + currentIndex, 12);

    return result;
}

DTO::PropHuntDTO* Serializer::DeserializePropHuntData(std::vector<byte> input)
{
    DTO::PropHuntDTO* result = new DTO::PropHuntDTO();
    copyData(&result->IsPlaying, &input[0] + currentIndex, 1);
    copyData(&result->Phase, &input[0] + currentIndex, 1);
    copyData(&result->StartingPosition, &input[0] + currentIndex, 12);
    copyData(&result->IsHunter, &input[0] + currentIndex, 1);

    return result;
}

bool* Serializer::DeserializePVPData(std::vector<byte> input)
{
    bool* result = new bool();
    copyData(&result, &input[0] + currentIndex, 1);

    return result;
}

#pragma endregion

#pragma region Serialization

void Serializer::SerializeConnectData(byte* outputArray, std::string name, std::string password, std::string modelType, std::string modelData)
{
    memset(ClientData, 0, 7168);
    currentIndex = 0;

    byte ActionType = 2;

    copyData(&ClientData[0] + currentIndex, &ActionType, 1);

    byte StringSize = name.size();
    copyData(&ClientData[0] + currentIndex, &StringSize, 1);
    copyData(&ClientData[0] + currentIndex, &name[0], StringSize);

    StringSize = password.size();
    copyData(&ClientData[0] + currentIndex, &StringSize, 1);
    copyData(&ClientData[0] + currentIndex, &password[0], StringSize);

    StringSize = modelType.size();
    copyData(&ClientData[0] + currentIndex, &StringSize, 1);
    copyData(&ClientData[0] + currentIndex, &modelType[0], StringSize);

    short LongStringSize = modelData.size();
    copyData(&ClientData[0] + currentIndex, &LongStringSize, 2);
    copyData(&ClientData[0] + currentIndex, &modelData[0], LongStringSize);

    memcpy(outputArray, &ClientData[0], 7168);
}

void Serializer::SerializeDisconnectData(byte* outputArray, std::string reason)
{
    memset(ClientData, 0, 7168);
    currentIndex = 0;

    byte ActionType = 4;

    copyData(&ClientData[0] + currentIndex, &ActionType, 1);

    byte StringSize = reason.size();
    copyData(&ClientData[0] + currentIndex, &reason[0], StringSize);

    memcpy(outputArray, &ClientData[0], 7168);
}

void Serializer::SerializeClientData(byte* outputArray, DTO::ClientDTO* input)
{
    memset(ClientData, 0, 7168);
    currentIndex = 0;

    byte ActionType = 3;

    copyData(&ClientData[0] + currentIndex, &ActionType, 1);

    SerializeWorldData(input->WorldData);
    SerializeCharacterData(input->PlayerData);
    SerializeEnemyData(input->EnemyData);
    SerializeQuestData(input->QuestData);

    memcpy(outputArray, &ClientData[0], 7168);
}

void Serializer::SerializeWorldData(DTO::WorldDTO* input)
{
    copyData(&ClientData[0] + currentIndex, &input->Time, 12);
}

void Serializer::SerializeCharacterData(DTO::ClientCharacterDTO* input)
{
    copyData(&ClientData[0] + currentIndex, &input->Position, 12);
    copyData(&ClientData[0] + currentIndex, &input->Rotation1, 16);
    copyData(&ClientData[0] + currentIndex, &input->Rotation2, 16);
    copyData(&ClientData[0] + currentIndex, &input->Rotation3, 16);
    copyData(&ClientData[0] + currentIndex, &input->Rotation4, 16);
    copyData(&ClientData[0] + currentIndex, &input->Animation, 4);
    copyData(&ClientData[0] + currentIndex, &input->Health, 4);
    copyData(&ClientData[0] + currentIndex, &input->AtkUp, 4);
    copyData(&ClientData[0] + currentIndex, &input->IsEquipped, 1);
    // copyData(&ClientData[0] + currentIndex, &input->Equipment, 13);
    copyData(&ClientData[0] + currentIndex, &input->Equipment.WType, 1);
    copyData(&ClientData[0] + currentIndex, &input->Equipment.Sword, 2);
    copyData(&ClientData[0] + currentIndex, &input->Equipment.Shield, 2);
    copyData(&ClientData[0] + currentIndex, &input->Equipment.Bow, 2);
    copyData(&ClientData[0] + currentIndex, &input->Equipment.Head, 2);
    copyData(&ClientData[0] + currentIndex, &input->Equipment.Upper, 2);
    copyData(&ClientData[0] + currentIndex, &input->Equipment.Lower, 2);

    byte MapByte = 0;
    byte SectByte = 0;
    std::map<std::string, byte> MapDict = {{"MainField", 1}, {"MainFieldDungeon", 2}, {"CDungeon", 3}, {"AoCField", 4}};
    std::map<std::string, byte> DvnBstDict = {{"RemainsWind", 1}, {"RemainsWater", 2}, {"RemainsElectric", 3}, {"RemainsFire", 4}};
    std::map<std::string, int> LetterDict = {{"A", 0}, {"B", 1}, {"C", 2}, {"D", 3}, {"E", 4}, {"F", 5}, {"G", 6}, {"H", 7}, {"I", 8}, {"J", 9}};

    if (MapDict.count(input->Location.Map))
        MapByte = MapDict[input->Location.Map];

    int SectLetter;
    int Number;

    switch (MapByte)
    {
    case 1:
        if (input->Location.Section != "WarpIcon")
        {
            SectLetter = LetterDict[input->Location.Section.substr(0, 1)];
            Number = stoi(input->Location.Section.substr(2, 1));
            SectByte = (SectLetter * 8) + Number;
        }
        break;
    case 2:
        if (DvnBstDict.count(input->Location.Section))
            SectByte = DvnBstDict[input->Location.Section];
        break;
    case 3:
        if (input->Location.Section.size() == 10)
            SectByte = stoi(input->Location.Section.substr(7, 3));
        break;
    }

    copyData(&ClientData[0] + currentIndex, &MapByte, 1);
    copyData(&ClientData[0] + currentIndex, &SectByte, 1);

    copyData(&ClientData[0] + currentIndex, &input->Bomb, 48);
}

void Serializer::SerializeEnemyData(DTO::EnemyDTO* input)
{
    byte ListSize = input->Health.size();
    copyData(&ClientData[0] + currentIndex, &ListSize, 1);

    for (int i = 0; i < ListSize; i++)
    {
        copyData(&ClientData[0] + currentIndex, &input->Health[i].Hash, 4);
        copyData(&ClientData[0] + currentIndex, &input->Health[i].Health, 4);
    }
}

void Serializer::SerializeQuestData(DTO::QuestDTO* input)
{
    byte ListSize = input->Completed.size();
    copyData(&ClientData[0] + currentIndex, &ListSize, 1);

    for (int i = 0; i < ListSize; i++)
    {
        byte StringSize = input->Completed[i].size();
        copyData(&ClientData[0] + currentIndex, &StringSize, 1);
        copyData(&ClientData[0] + currentIndex, &input->Completed[i], StringSize);
    }
}

#pragma endregion

void Serializer::CopyToArray(byte* byteArray)
{
    memcpy(byteArray, &ClientData[0], 7168);
} // Unused for now

std::string Serializer::CopyString(std::vector<byte> input)
{
    std::string result = "";

    byte StringSize = 0;
    copyData(&StringSize, &input[0] + currentIndex, 1);

    for (int i = 0; i < StringSize; i++)
    {
        char character;
        copyData(&character, &input[0] + currentIndex, 1);

        result += character;
    }

    return result;
}