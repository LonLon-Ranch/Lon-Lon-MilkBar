#include "dllmain_Functions.h"
#include "BigEndian.h"
#include "QuaternionBE.h"
#include "Serialization.h"
#include "SpawningVariables.h"
#include "Vec3fBE.h"
#include "Vec3f_Operations.h"
#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"


std::vector<float> pingMult = {0, 0, 0, 0};
std::vector<uint64_t> activatedAddr;
uint64_t notPaused;
bool isCharacterSpawn = false;
std::vector<bool> WeaponChanged = {false, false, false, false};

bool Main::connectToServer(std::string serverMessage)
{
    int startingPosition = 0;
    int count = 1;

    std::string IP;
    std::string PORT;
    std::string PASSWORD;
    std::string NAME;
    std::string MODELTYPE;
    std::string MODELDATA;

    for (int i = 0; i < serverMessage.size(); i++)
    {
        if (serverMessage[i] == ';')
        {
            if (serverMessage[i + 1] == '[' && count != 7)
            {
                break;
            }

            if (startingPosition == 0)
            {
                startingPosition = i + 1;
                continue;
            }

            std::string substring;

            substring = serverMessage.substr(startingPosition, i - startingPosition);

            if (count == 1)
            {
                IP = substring;
                count++;
            } else if (count == 2)
            {
                PORT = substring;
                count++;
            } else if (count == 3)
            {
                PASSWORD = substring;
                count++;
            } else if (count == 4)
            {
                NAME = substring;
                count++;
            } else if (count == 5)
            {
                Main::serverName = substring;
                count++;
            } else if (count == 6)
            {
                MODELTYPE = substring;
                count++;
            } else
            {
                MODELDATA = substring;
            }

            startingPosition = i + 1;
        }
    }

    client->connectToServer(IP, PORT);

    byte ConnectBytes[7168];
    Serialization::Serializer::SerializeConnectData(&ConnectBytes[0], NAME, PASSWORD, MODELTYPE, MODELDATA);

    // Serialization::Serializer::CopyToArray(&ConnectBytes[0]);

    client->sendBytes(ConnectBytes);

    serverData = client->receive();

    std::cout << serverData << std::endl;

    rapidjson::Document serverDoc = Connectivity::deserializeServerData(serverData);

    if (serverDoc["Response"].GetInt() == 1)
    {
        // int pN = serverData[2] - '0';
        int pN = serverDoc["PlayerNumber"].GetInt();

        memcpy(&playerNumber, &pN, sizeof(int));

        rapidjson::Value ServerSettings(rapidjson::kArrayType);
        ServerSettings = serverDoc["Settings"];

        if (ServerSettings["EnemySync"].GetBool())
        {
            isEnemySync = true;
        }

        if (ServerSettings["GameMode"].GetInt() == 1)
        {
            isHvsSR = true;
        }

        if (ServerSettings["GameMode"].GetInt() == 2)
        {
            isDeathSwap = true;
        }

        std::map<std::string, std::string> QuestOptions = {{"Vanilla", "V"}, {"Koroks", "K"}, {"Towers", "T"}, {"Shrines", "S"}, {"Locations", "L"}, {"DivineBeast", "D"}};

        rapidjson::Value QuestSettings(rapidjson::kArrayType);
        QuestSettings = ServerSettings["QuestSyncSettings"];

        for (std::map<std::string, std::string>::iterator it = QuestOptions.begin(); it != QuestOptions.end(); ++it)
        {
            if (QuestSettings[it->first].GetBool())
            {
                if (it->first == "Shrines")
                {
                    questServerSettings.push_back("O");
                    questServerSettings.push_back("C");
                    Logging::LoggerService::LogInformation("Quest sync setting: Shrines");
                } else
                {
                    questServerSettings.push_back(it->second);
                    Logging::LoggerService::LogInformation("Quest sync settings: " + it->second);
                }

                isQuestSync = true;
            }
        }
        return true;
    }

    return false;
}

void Main::disconnectFromServer(std::string reason)
{
    byte DisconnectBytes[7168];
    Serialization::Serializer::SerializeDisconnectData(&DisconnectBytes[0], reason);

    client->sendBytes(DisconnectBytes);

    exit(1);
}

void Main::playerQueueUpdate()
{
    std::vector<std::string> axes = {"x", "y", "z"};
    float coords[] = {0, 0, 0};
    float neededUpdates;
    float correctedPing;

    float sleepTime = 1000 / float(targetFPS);
    float timePassed = 0;
    int QueueSize;

    while (true)
    {

        rapidjson::Document serverDocCopy = Connectivity::deserializeServerData(serverData);
        rapidjson::Value playerDataCopy(rapidjson::kArrayType);

        if (!serverDocCopy.IsObject())
        {
            continue;
        }

        playerDataCopy = serverDocCopy["PD"].GetArray();

        /*if (1 / ping > (serializationRate / 1000))
        {
            correctedPing = (1 / serializationRate);
        }
        else
        {
            correctedPing = ping / 1000;
        }*/

        correctedPing = ping / 1000 * pingMult[playerNumber];

        QueueSize = 0;

        for (int i = 0; i < 4; i++)
        {
            QueueSize = Main::JugadoresQueues[i][0].size() > QueueSize ? Main::JugadoresQueues[i][0].size() : QueueSize;
        }

        float correction = QueueSize < 4 ? QueueSize : QueueSize / 2;

        if (correction != 0)
        {

            neededUpdates = ((targetFPS - (1 / correctedPing)) / (1 / correctedPing)) / (correction);

            for (int k = 0; k < neededUpdates + 1; k++)
            {

                t1 = GetTickCount();

                for (int i = 0; i < 4; i++)
                {

                    if (i == playerNumber || !playerDataCopy[i]["Con"].GetBool())
                    {
                        continue;
                    }

                    for (int j = 0; j < 3; j++)
                    {

                        if (Main::JugadoresQueues[i][j].size() > 1)
                        {

                            if (Main::oldLocations[i][j] == 0)
                            {
                                Main::oldLocations[i][j] = Main::JugadoresQueues[i][j].back();
                            }

                            float newValue = (Main::JugadoresQueues[i][j].back() - Main::oldLocations[i][j]) * (k / neededUpdates) + Main::oldLocations[i][j];
                            coords[j] = newValue;
                        }
                    }

                    Jugadores[i]->changeData(playerDataCopy[i], 1, coords);
                }

                timePassed = float(GetTickCount() - t1);

                if (timePassed < sleepTime)
                    Sleep(sleepTime - timePassed);
            }

            for (int i = 0; i < 4; i++)
            {
                for (int j = 0; j < 3; j++)
                {
                    if (Main::JugadoresQueues[i][j].size() > 1)
                    {
                        Main::oldLocations[i][j] = Main::JugadoresQueues[i][j].back();
                        Main::JugadoresQueues[i][j].pop_back();
                    }
                }
            }

        } else
        {
            Sleep(1);
        }
    }
}

void Main::HelperThread()
{
    Memory::MessagerService::StartMessagerService();

    while (true)
    {
        Memory::MessagerService::DisplayMessage();

        Game::GameInstance->ProcessCreationRequests();

        Sleep(50);
    }
}

void Main::glyphScan()
{
    std::vector<int> sig = {0x45, 0x0F, 0x38, 0xF0, 0x74, 0x2D, 0x18, 0x66, 0x41, 0x0F, 0x6E, 0xC6, -1, 0x7C, 0x24, 0x08};
    uint64_t glyphFunction = Memory::PatternScan(sig, 0);

    std::cout << std::hex << glyphFunction << std::endl;

    Memory::write_bytes(glyphFunction, Memory::getNop(7), __FUNCTION__);
}

// void Main::characterSpawner()
//{
//
//     bool isPrint = false;
//     std::vector<int> lastSpawn = { 0, 0, 0, 0 };
//
//     if (isPrint) system("CLS");
//
//     Sleep(1000);
//
//     std::string RIGHT_DEFAULT = "RightHandWeaponLongName";
//     std::string LEFT_DEFAULT = "LeftHandWeaponLongName";
//     std::vector<uint64_t> WeaponAddrs;
//
//     while (true)
//     {
//
//         if (!isCharacterSpawn)
//         {
//             Sleep(1000);
//             continue;
//         }
//
//         //Memory::write_byte(notPaused, 0x0);
//
//         //Sleep(500);
//
//         //if (Memory::read_bytes(notPaused, 1)[0] == 0)
//         //{
//
//         //    Memory::MultiplayerQuest::startMQuest(activatedAddr);
//
//         //    for (int i = 0; i < 4; i++)
//         //    {
//         //        Jugadores[i]->restartAnimations();
//         //    }
//
//         //    continue;
//
//         //}
//
//         if (Main::isPaused) continue;
//
//         rapidjson::Document serverDocCopy = Connectivity::deserializeServerData(serverData);
//
//         if (!serverDocCopy.IsObject())
//         {
//             continue;
//         }
//
//         rapidjson::Value playerDataCopy(rapidjson::kArrayType);
//         playerDataCopy = serverDocCopy["PD"].GetArray();
//
//         std::vector<std::string> localLoc = { Link->Map, Link->Section };
//
//         Sleep(100);
//
//         for (int i = 0; i < 4; i++)
//         {
//
//             int action = 0;
//
//             std::string svMap = "None";
//             std::string svSection = "None";
//
//             if (playerDataCopy[i].HasMember("L"))
//             {
//
//                 svMap = playerDataCopy[i]["L"].GetArray()[0].GetString();
//                 svSection = playerDataCopy[i]["L"].GetArray()[1].GetString();
//
//             }
//
//             std::vector<std::string> serverInfo = { svMap, svSection, playerDataCopy[i]["Con"].GetBool() == true ? "True" : "False", std::to_string(playerNumber == i) };
//
//             bool sameMap = localLoc[0] == serverInfo[0];
//             bool sameSection = localLoc[1] == serverInfo[1];
//
//             bool shouldExist = false;
//
//             if (sameMap && serverInfo[2] != "False" && serverInfo[3] == "0")
//             {
//                 if (localLoc[0] == "MainField" || sameSection)
//                 {
//                     shouldExist = true;
//                 }
//             }
//
//             Memory::write_byte(Jugadores[i]->existsAddr, 0, __FUNCTION__);
//
//             Sleep(50);
//
//             bool exists = Memory::read_bytes(Jugadores[i]->existsAddr, 1, __FUNCTION__)[0] == 0x01 ? true : false;
//
//             if (exists && !shouldExist)
//             {
//                 action = -1;
//             }
//             else if ((!exists && shouldExist) || Jugadores[i]->WeaponChanged)
//             {
//                 action = 1;
//             }
//
//             if (isPrint) std::cout << exists << " " << shouldExist << " " << action << " | ";
//
//             /*action = Jugadores[i]->spawnAction(localLoc, serverInfo, isPrint);*/
//
//             if (action == -1)
//             {
//                 Jugadores[i]->deleteActor();
//                 while (Jugadores[i]->BaseAddress != 0u)
//                 {
//                 }
//                 lastSpawn[i] = 6;
//             }
//             else if (action == 1)
//             {
//                 if (lastSpawn[i] == 0 || lastSpawn[i] >= 6)
//                 {
//
//                     if (std::sqrt(pow(std::abs(Link->Position[0] - Jugadores[i]->Pos[0]), 2) + pow(std::abs(Link->Position[2] - Jugadores[i]->Pos[2]), 2)) > 100)
//                     {
//                         continue;
//                     }
//
//                     while (CheckIfPaused())
//                     {
//
//                     }
//
//                     std::cout << "Spawned " << i + 1 << " Reason: " << shouldExist << " " << exists << " " << WeaponChanged[i] << std::endl;
//
//                     //WeaponAddrs = Jugadores[i]->findWeaponAddr();
//
//                     //for (int j = 0; j < WeaponAddrs.size(); j++)
//                     //{
//
//                     //    Memory::write_string(WeaponAddrs[j], Jugadores[i]->Equipment["W"], RIGHT_DEFAULT.size() + 8);
//
//                     //    Memory::write_string(WeaponAddrs[j] + 0x20, Jugadores[i]->Equipment["S"], LEFT_DEFAULT.size() + 8);
//
//                     //}
//
//                     //while (CheckIfPaused())
//                     //{
//
//                     //}
//
//                     //Jugadores[i]->deleteActor(100);
//
//                     //Sleep(100);
//
//                     //while (CheckIfPaused())
//                     //{
//
//                     //}
//
//                     //queue_mutex.lock();
//
//                     //if (queuedActors.size() > 0)
//                     //{
//                     //    for (int j = queuedActors.size() - 1; j >= 0; j--)
//                     //    {
//                     //        if (queuedActors[j].Name == "Jugador" + std::to_string(i + 1))
//                     //        {
//                     //            queuedActors.erase(queuedActors.begin() + j);
//                     //        }
//                     //    }
//                     //}
//                     //
//                     ////std::cout << "Spawned " << i + 1 << " Reason: " << shouldExist << " " << exists << " " << WeaponChanged[i] << std::endl;
//
//                     //queueActor(i + 1, Jugadores[i]->Pos);
//                     //queue_mutex.unlock();
//
//                     //Sleep(250);
//
//                     //while (CheckIfPaused())
//                     //{
//
//                     //}
//
//                     //for (int j = 0; j < WeaponAddrs.size(); j++)
//                     //{
//
//                     //    Memory::write_string(WeaponAddrs[j], "Jugador" + std::to_string(i + 1) + RIGHT_DEFAULT, RIGHT_DEFAULT.size() + 8);
//
//                     //    Memory::write_string(WeaponAddrs[j] + 0x20, "Jugador" + std::to_string(i + 1) + LEFT_DEFAULT, LEFT_DEFAULT.size() + 8);
//
//                     //}
//
//                     //WeaponChangeMutex.lock();
//
//                     //WeaponChanged[i] = false;
//
//                     //WeaponChangeMutex.unlock();
//
//                     //Memory::write_byte(Jugadores[i]->existsAddr, 0x00);
//
//                     //lastSpawn[i] = 1;
//                     //Sleep(1000);
//
//
//
//
//
//
//                     //// TODO Despawn and respawn to make sure this data is in memory
//                     //Jugadores[i]->FindWeapons();
//
//                     //WeaponChangeMutex.lock();
//
//                     //Memory::write_string(Jugadores[i]->RightHandWeapon, Jugadores[i]->Equipment["W"], RIGHT_DEFAULT.size() + 8);
//                     //Memory::write_string(Jugadores[i]->LeftHandWeapon, Jugadores[i]->Equipment["S"], LEFT_DEFAULT.size() + 8);
//
//                     //WeaponChanged[i] = false;
//
//                     //WeaponChangeMutex.unlock();
//
//                     //while (CheckIfPaused())
//                     //{
//                     //
//                     //}
//
//                     //// TODO Implement actor despawning
//
//                     //Jugadores[i]->deleteActor(100);
//
//                     //Sleep(100);
//
//                     //while (CheckIfPaused())
//                     //{
//
//                     //}
//
//                     //queue_mutex.lock();
//
//                     //if (queuedActors.size() > 0)
//                     //{
//                     //    for (int j = queuedActors.size() - 1; j >= 0; j--)
//                     //    {
//                     //        if (queuedActors[j].Name == "Jugador" + std::to_string(i + 1))
//                     //        {
//                     //            queuedActors.erase(queuedActors.begin() + j);
//                     //        }
//                     //    }
//                     //}
//
//                     ////std::cout << "Spawned " << i + 1 << " Reason: " << shouldExist << " " << exists << " " << WeaponChanged[i] << std::endl;
//
//                     //queueActor(i + 1, Jugadores[i]->Pos);
//                     //queue_mutex.unlock();
//
//                     //while (Jugadores[i]->BaseAddress == 0)
//                     //{
//                     //}
//
//                     //while (CheckIfPaused())
//                     //{
//                     //}
//
//                     //Memory::write_string(Jugadores[i]->RightHandWeapon, "Jugador" + std::to_string(i + 1) + RIGHT_DEFAULT, RIGHT_DEFAULT.size() + 8);
//
//                     //Memory::write_string(Jugadores[i]->LeftHandWeapon, "Jugador" + std::to_string(i + 1) + LEFT_DEFAULT, LEFT_DEFAULT.size() + 8);
//
//                     //lastSpawn[i] = 1;
//
//
//
//
//
//                     while (CheckIfPaused())
//                     {
//
//                     }
//
//                     // TODO Implement actor despawning
//
//                     Jugadores[i]->deleteActor();
//
//                     while (Jugadores[i]->BaseAddress != 0)
//                     {
//
//                         if (!Jugadores[i]->checkExists())
//                         {
//
//                             Jugadores[i]->ClearPointers();
//                             break;
//
//                         }
//
//                     }
//
//                     Jugadores[i]->ClearPointers();
//
//                     //Jugadores[i]->ClearPointers();
//
//                     //Sleep(100);
//
//                     while (CheckIfPaused())
//                     {
//                     }
//
//                     queue_mutex.lock();
//
//                     Jugadores[i]->SetArmor();
//
//                     queueActor(i + 1, Jugadores[i]->Pos);
//                     Logging::LoggerService::LogDebug("Queued actor " + std::to_string(i + 1) + " for spawn.", __FUNCTION__);
//                     queue_mutex.unlock();
//
//                     //while (Jugadores[i]->BaseAddress == 0)
//                     //{
//                     //    Sleep(5);
//                     //}
//
//                     lastSpawn[i] = 1;
//                     Jugadores[i]->FindArmor();
//                     Sleep(300);
//
//                 }
//                 else
//                 {
//
//                     lastSpawn[i]++;
//
//                 }
//             }
//             else
//             {
//                 lastSpawn[i] = 6;
//             }
//
//         }
//
//         Sleep(1000);
//         if (isPrint) std::cout << std::endl;
//
//     }
//
// }

void Main::QueueBomb(std::string bombType, float position[3])
{

    queue_mutex.lock();

    queueActor(bombType, position);

    queue_mutex.unlock();
}

float Main::GetDistance(int playerID, bool includeZAxis)
{
    return std::sqrt(pow(std::abs(Link->Position[0] - Jugadores[playerID]->Pos[0]), 2) + includeZAxis ? 1 : 0 * pow(std::abs(Link->Position[1] - Jugadores[playerID]->Pos[1]), 2) + pow(std::abs(Link->Position[2] - Jugadores[playerID]->Pos[2]), 2));
}

bool Main::ExternIsPaused()
{
    return Game::GameInstance->IsPaused();
}

void Main::QuestSync()
{
    Game::GameInstance->QuestService->QuestSyncer->setup(questServerSettings, ExternIsPaused);

    Sleep(5000);

    Game::GameInstance->QuestService->QuestSyncer->readQuests();

    Main::QuestSyncReady = true;

    Memory::MessagerService::AddMessage("Quest sync service started");

    Game::GameInstance->QuestService->QuestSyncer->changeFlag();
}

std::string Main::getSendString()
{

    std::string result;

    // std::vector<std::string> dataTitle = { "WD", "PD", "ED", "QD", "BD" };

    // std::vector<std::vector<std::string>> dataNames = { { "T", "D", "W" },
    //     { "P", "R", "A", "L", "H", "E", "At", "IE" },
    //     { "H" },
    //     { "C" },
    // };

    // std::vector<std::vector<std::any>> variablesToSend = { { World->GetTime(), World->GetDay(), std::to_string((uint32_t)targetWeather) },
    //     { Link->GetPosition(), Link->GetRotation(), Link->GetAnimation(), Link->GetLocation(), Link->
    //  (), Link->GetEquipment(), Link->GetAtkUp(), Link->GetIsEquipped() },
    //{ EnemyClass->getEnemyHealthMap() } ,
    //{ Quests->getChangedQuests() } };

    // std::vector<std::vector<std::string>> dataTypes = { { "float", "int", "int" },
    //     { "float", "float", "int", "string", "int", "string", "float", "int" },
    //     { "int" },
    //     { "string" } };

    // BombSync->BombMutex.lock();

    // std::tuple<std::vector<std::string>, std::vector<std::any>, std::vector<std::string>> BombData = BombSync->GetBombPositions();

    // BombSync->BombMutex.unlock();

    // if (std::get<0>(BombData).size() == 0)
    //{
    //     dataNames.push_back({ "NA" });
    //     std::map<std::string, std::string> emptyData = { {"x", "0" }, {"y", "0" }, {"z", "0" } };
    //     variablesToSend.push_back({ emptyData });
    //     dataTypes.push_back({ "float" });
    // }
    // else
    //{
    //     dataNames.push_back(std::get<0>(BombData));
    //     variablesToSend.push_back(std::get<1>(BombData));
    //     dataTypes.push_back(std::get<2>(BombData));
    // }

    ///*for (int i = 0; i < BombData.size(); i++)
    //{
    //    dataNames.push_back(std::get<0>(BombData[i]));
    //    variablesToSend.push_back(std::get<1>(BombData[i]));
    //    dataTypes.push_back(std::get<2>(BombData[i]));
    //}*/

    // rapidjson::Document doc;
    // doc.SetObject();

    // for (int i = 0; i < variablesToSend.size(); i++)
    //{

    //    rapidjson::Value val;
    //    rapidjson::Value data(rapidjson::kObjectType);

    //    for (int j = 0; j < variablesToSend[i].size(); j++)
    //    {

    //        std::string varType = variablesToSend[i][j].type().name();

    //        if (varType.substr(0, 14) == "class std::map")
    //        {
    //            val = Connectivity::addMapToJsonDocument(doc.GetAllocator(), std::any_cast<std::map<std::string, std::string>>(variablesToSend[i][j]), dataTypes[i][j]);
    //        }
    //        else if (varType.substr(0, 14) == "class std::vec")
    //        {
    //            val = Connectivity::addVectorToJsonDocument(doc.GetAllocator(), std::any_cast<std::vector<std::string>>(variablesToSend[i][j]), dataTypes[i][j]);
    //        }
    //        else
    //        {
    //            val = Connectivity::addValueToJsonDocument(doc.GetAllocator(), std::any_cast<std::string>(variablesToSend[i][j]), dataTypes[i][j]);
    //        }

    //        data.AddMember(rapidjson::StringRef(dataNames[i][j]), val, doc.GetAllocator());

    //    }

    //    doc.AddMember(rapidjson::StringRef(dataTitle[i]), data, doc.GetAllocator());

    //}

    // rapidjson::StringBuffer buffer;
    // rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    // writer.SetMaxDecimalPlaces(6);
    // doc.Accept(writer);

    // result = buffer.GetString();

    return result;
}

void Main::PauseChecker()
{
    DWORD lastPaused = GetTickCount();

    while (true)
    {
        Game::GameInstance->PauseMutex.lock();

        bool paused = CheckIfPaused();

        if (paused)
        {
            if (!Game::GameInstance->IsGamePaused)
                Logging::LoggerService::LogInformation("Game paused.", __FUNCTION__);
            Main::isPaused = true;
            Game::GameInstance->IsGamePaused = true;
            lastPaused = GetTickCount();
        } else
        {
            if (float(GetTickCount() - lastPaused) > 1000)
            {
                if (Game::GameInstance->IsGamePaused)
                    Logging::LoggerService::LogInformation("Game unpaused.", __FUNCTION__);
                Main::isPaused = false;
                Game::GameInstance->IsGamePaused = false;
            }
        }

        Game::GameInstance->PauseMutex.unlock();

        Sleep(50);
    }
}

void Main::oldServerLoop()
{
    // Logging::LoggerService::LogInformation("Start of Breath of the Wild Multiplayer", __FUNCTION__);

    // Game::GameInstance = new MemoryAccess::LocalInstance();

    // std::vector<byte> test = { 0x00, 0x00, 0xA0, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x16, 0x54, 0x61, 0x63, 0x61, 0x5F, 0x41, 0x5F, 0x58, 0x65, 0x72, 0x65, 0x63, 0x61, 0x5F, 0x50, 0x72, 0x61, 0x5F, 0x4D, 0x69, 0x6D, 0x30, 0x01, 0x16, 0x54, 0x61, 0x63, 0x61, 0x5F, 0x41, 0x5F, 0x58, 0x65, 0x72, 0x65, 0x63, 0x61, 0x5F, 0x50, 0x72, 0x61, 0x5F, 0x4D, 0x69, 0x6D, 0x31, 0x02, 0x16, 0x54, 0x61, 0x63, 0x61, 0x5F, 0x41, 0x5F, 0x58, 0x65, 0x72, 0x65, 0x63, 0x61, 0x5F, 0x50, 0x72, 0x61, 0x5F, 0x4D, 0x69, 0x6D, 0x32, 0x03, 0x16, 0x54, 0x61, 0x63, 0x61, 0x5F, 0x41, 0x5F, 0x58, 0x65, 0x72, 0x65, 0x63, 0x61, 0x5F, 0x50, 0x72, 0x61, 0x5F, 0x4D, 0x69, 0x6D, 0x33, 0x04, 0x16, 0x54, 0x61, 0x63, 0x61, 0x5F, 0x41, 0x5F, 0x58, 0x65, 0x72, 0x65, 0x63, 0x61, 0x5F, 0x50, 0x72, 0x61, 0x5F, 0x4D, 0x69, 0x6D, 0x34, 0x05, 0x16, 0x54, 0x61, 0x63, 0x61, 0x5F, 0x41, 0x5F, 0x58, 0x65, 0x72, 0x65, 0x63, 0x61, 0x5F, 0x50, 0x72, 0x61, 0x5F, 0x4D, 0x69, 0x6D, 0x35, 0x06, 0x16, 0x54, 0x61, 0x63, 0x61, 0x5F, 0x41, 0x5F, 0x58, 0x65, 0x72, 0x65, 0x63, 0x61, 0x5F, 0x50, 0x72, 0x61, 0x5F, 0x4D, 0x69, 0x6D, 0x36, 0x07, 0x16, 0x54, 0x61, 0x63, 0x61, 0x5F, 0x41, 0x5F, 0x58, 0x65, 0x72, 0x65, 0x63, 0x61, 0x5F, 0x50, 0x72, 0x61, 0x5F, 0x4D, 0x69, 0x6D, 0x37, 0x08, 0x16, 0x54, 0x61, 0x63, 0x61, 0x5F, 0x41, 0x5F, 0x58, 0x65, 0x72, 0x65, 0x63, 0x61, 0x5F, 0x50, 0x72, 0x61, 0x5F, 0x4D, 0x69, 0x6D, 0x38, 0x09, 0x16, 0x54, 0x61, 0x63, 0x61, 0x5F, 0x41, 0x5F, 0x58, 0x65, 0x72, 0x65, 0x63, 0x61, 0x5F, 0x50, 0x72, 0x61, 0x5F, 0x4D, 0x69, 0x6D, 0x39, 0x0A, 0x17, 0x54, 0x61, 0x63, 0x61, 0x5F, 0x41, 0x5F, 0x58, 0x65, 0x72, 0x65, 0x63, 0x61, 0x5F, 0x50, 0x72, 0x61, 0x5F, 0x4D, 0x69, 0x6D, 0x31, 0x30, 0x0B, 0x17, 0x54, 0x61, 0x63, 0x61, 0x5F, 0x41, 0x5F, 0x58, 0x65, 0x72, 0x65, 0x63, 0x61, 0x5F, 0x50, 0x72, 0x61, 0x5F, 0x4D, 0x69, 0x6D, 0x31, 0x31, 0x0C, 0x17, 0x54, 0x61, 0x63, 0x61, 0x5F, 0x41, 0x5F, 0x58, 0x65, 0x72, 0x65, 0x63, 0x61, 0x5F, 0x50, 0x72, 0x61, 0x5F, 0x4D, 0x69, 0x6D, 0x31, 0x32, 0x0D, 0x17, 0x54, 0x61, 0x63, 0x61, 0x5F, 0x41, 0x5F, 0x58, 0x65, 0x72, 0x65, 0x63, 0x61, 0x5F, 0x50, 0x72, 0x61, 0x5F, 0x4D, 0x69, 0x6D, 0x31, 0x33, 0x0E, 0x17, 0x54, 0x61, 0x63, 0x61, 0x5F, 0x41, 0x5F, 0x58, 0x65, 0x72, 0x65, 0x63, 0x61, 0x5F, 0x50, 0x72, 0x61, 0x5F, 0x4D, 0x69, 0x6D, 0x31, 0x34, 0x0F, 0x17, 0x54, 0x61, 0x63, 0x61, 0x5F, 0x41, 0x5F, 0x58, 0x65, 0x72, 0x65, 0x63, 0x61, 0x5F, 0x50, 0x72, 0x61, 0x5F, 0x4D, 0x69, 0x6D, 0x31, 0x35, 0x10, 0x17, 0x54, 0x61, 0x63, 0x61, 0x5F, 0x41, 0x5F, 0x58, 0x65, 0x72, 0x65, 0x63, 0x61, 0x5F, 0x50, 0x72, 0x61, 0x5F, 0x4D, 0x69, 0x6D, 0x31, 0x36, 0x11, 0x17, 0x54, 0x61, 0x63, 0x61, 0x5F, 0x41, 0x5F, 0x58, 0x65, 0x72, 0x65, 0x63, 0x61, 0x5F, 0x50, 0x72, 0x61, 0x5F, 0x4D, 0x69, 0x6D, 0x31, 0x37, 0x12, 0x17, 0x54, 0x61, 0x63, 0x61, 0x5F, 0x41, 0x5F, 0x58, 0x65, 0x72, 0x65, 0x63, 0x61, 0x5F, 0x50, 0x72, 0x61, 0x5F, 0x4D, 0x69, 0x6D, 0x31, 0x38, 0x13, 0x17, 0x54, 0x61, 0x63, 0x61, 0x5F, 0x41, 0x5F, 0x58, 0x65, 0x72, 0x65, 0x63, 0x61, 0x5F, 0x50, 0x72, 0x61, 0x5F, 0x4D, 0x69, 0x6D, 0x31, 0x39, 0x14, 0x17, 0x54, 0x61, 0x63, 0x61, 0x5F, 0x41, 0x5F, 0x58, 0x65, 0x72, 0x65, 0x63, 0x61, 0x5F, 0x50, 0x72, 0x61, 0x5F, 0x4D, 0x69, 0x6D, 0x32, 0x30, 0x15, 0x17, 0x54, 0x61, 0x63, 0x61, 0x5F, 0x41, 0x5F, 0x58, 0x65, 0x72, 0x65, 0x63, 0x61, 0x5F, 0x50, 0x72, 0x61, 0x5F, 0x4D, 0x69, 0x6D, 0x32, 0x31, 0x16, 0x17, 0x54, 0x61, 0x63, 0x61, 0x5F, 0x41, 0x5F, 0x58, 0x65, 0x72, 0x65, 0x63, 0x61, 0x5F, 0x50, 0x72, 0x61, 0x5F, 0x4D, 0x69, 0x6D, 0x32, 0x32, 0x17, 0x17, 0x54, 0x61, 0x63, 0x61, 0x5F, 0x41, 0x5F, 0x58, 0x65, 0x72, 0x65, 0x63, 0x61, 0x5F, 0x50, 0x72, 0x61, 0x5F, 0x4D, 0x69, 0x6D, 0x32, 0x33, 0x18, 0x17, 0x54, 0x61, 0x63, 0x61, 0x5F, 0x41, 0x5F, 0x58, 0x65, 0x72, 0x65, 0x63, 0x61, 0x5F, 0x50, 0x72, 0x61, 0x5F, 0x4D, 0x69, 0x6D, 0x32, 0x34, 0x19, 0x17, 0x54, 0x61, 0x63, 0x61, 0x5F, 0x41, 0x5F, 0x58, 0x65, 0x72, 0x65, 0x63, 0x61, 0x5F, 0x50, 0x72, 0x61, 0x5F, 0x4D, 0x69, 0x6D, 0x32, 0x35, 0x1A, 0x17, 0x54, 0x61, 0x63, 0x61, 0x5F, 0x41, 0x5F, 0x58, 0x65, 0x72, 0x65, 0x63, 0x61, 0x5F, 0x50, 0x72, 0x61, 0x5F, 0x4D, 0x69, 0x6D, 0x32, 0x36, 0x1B, 0x17, 0x54, 0x61, 0x63, 0x61, 0x5F, 0x41, 0x5F, 0x58, 0x65, 0x72, 0x65, 0x63, 0x61, 0x5F, 0x50, 0x72, 0x61, 0x5F, 0x4D, 0x69, 0x6D, 0x32, 0x37, 0x1C, 0x17, 0x54, 0x61, 0x63, 0x61, 0x5F, 0x41, 0x5F, 0x58, 0x65, 0x72, 0x65, 0x63, 0x61, 0x5F, 0x50, 0x72, 0x61, 0x5F, 0x4D, 0x69, 0x6D, 0x32, 0x38, 0x1D, 0x17, 0x54, 0x61, 0x63, 0x61, 0x5F, 0x41, 0x5F, 0x58, 0x65, 0x72, 0x65, 0x63, 0x61, 0x5F, 0x50, 0x72, 0x61, 0x5F, 0x4D, 0x69, 0x6D, 0x32, 0x39, 0x1E, 0x17, 0x54, 0x61, 0x63, 0x61, 0x5F, 0x41, 0x5F, 0x58, 0x65, 0x72, 0x65, 0x63, 0x61, 0x5F, 0x50, 0x72, 0x61, 0x5F, 0x4D, 0x69, 0x6D, 0x33, 0x30, 0x1F, 0x17, 0x54, 0x61, 0x63, 0x61, 0x5F, 0x41, 0x5F, 0x58, 0x65, 0x72, 0x65, 0x63, 0x61, 0x5F, 0x50, 0x72, 0x61, 0x5F, 0x4D, 0x69, 0x6D, 0x33, 0x31, 0x1F, 0x00, 0x01, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80, 0x40, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80, 0x40, 0x03, 0x00, 0x00, 0x00, 0x1E, 0x00, 0x00, 0x00, 0x00, 0x00, 0xA0, 0x40, 0x01, 0x1E, 0x00, 0x64, 0x00, 0x01, 0x00, 0x03, 0x00, 0x32, 0x00, 0x1E, 0x00, 0x00, 0x1D, 0x00, 0x00, 0x7A, 0x44, 0x00, 0x00, 0x48, 0x43, 0x00, 0x00, 0xF0, 0x41, 0x00, 0x00, 0x48, 0x42, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80, 0x3F, 0x01, 0x01, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80, 0x40, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80, 0x40, 0x03, 0x00, 0x00, 0x00, 0x1E, 0x00, 0x00, 0x00, 0x00, 0x00, 0xA0, 0x40, 0x01, 0x1E, 0x00, 0x64, 0x00, 0x01, 0x00, 0x03, 0x00, 0x32, 0x00, 0x1E, 0x00, 0x00, 0x1D, 0x00, 0x00, 0x7A, 0x44, 0x00, 0x00, 0x48, 0x43, 0x00, 0x00, 0xF0, 0x41, 0x00, 0x00, 0x48, 0x42, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80, 0x3F, 0x02, 0x01, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80, 0x40, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80, 0x40, 0x03, 0x00, 0x00, 0x00, 0x1E, 0x00, 0x00, 0x00, 0x00, 0x00, 0xA0, 0x40, 0x01, 0x1E, 0x00, 0x64, 0x00, 0x01, 0x00, 0x03, 0x00, 0x32, 0x00, 0x1E, 0x00, 0x00, 0x1D, 0x00, 0x00, 0x7A, 0x44, 0x00, 0x00, 0x48, 0x43, 0x00, 0x00, 0xF0, 0x41, 0x00, 0x00, 0x48, 0x42, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80, 0x3F, 0x03, 0x01, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80, 0x40, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80, 0x40, 0x03, 0x00, 0x00, 0x00, 0x1E, 0x00, 0x00, 0x00, 0x00, 0x00, 0xA0, 0x40, 0x01, 0x1E, 0x00, 0x64, 0x00, 0x01, 0x00, 0x03, 0x00, 0x32, 0x00, 0x1E, 0x00, 0x00, 0x1D, 0x00, 0x00, 0x7A, 0x44, 0x00, 0x00, 0x48, 0x43, 0x00, 0x00, 0xF0, 0x41, 0x00, 0x00, 0x48, 0x42, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80, 0x3F, 0x04, 0x01, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80, 0x40, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80, 0x40, 0x03, 0x00, 0x00, 0x00, 0x1E, 0x00, 0x00, 0x00, 0x00, 0x00, 0xA0, 0x40, 0x01, 0x1E, 0x00, 0x64, 0x00, 0x01, 0x00, 0x03, 0x00, 0x32, 0x00, 0x1E, 0x00, 0x00, 0x1D, 0x00, 0x00, 0x7A, 0x44, 0x00, 0x00, 0x48, 0x43, 0x00, 0x00, 0xF0, 0x41, 0x00, 0x00, 0x48, 0x42, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80, 0x3F, 0x05, 0x01, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80, 0x40, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80, 0x40, 0x03, 0x00, 0x00, 0x00, 0x1E, 0x00, 0x00, 0x00, 0x00, 0x00, 0xA0, 0x40, 0x01, 0x1E, 0x00, 0x64, 0x00, 0x01, 0x00, 0x03, 0x00, 0x32, 0x00, 0x1E, 0x00, 0x00, 0x1D, 0x00, 0x00, 0x7A, 0x44, 0x00, 0x00, 0x48, 0x43, 0x00, 0x00, 0xF0, 0x41, 0x00, 0x00, 0x48, 0x42, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80, 0x3F, 0x06, 0x01, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80, 0x40, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80, 0x40, 0x03, 0x00, 0x00, 0x00, 0x1E, 0x00, 0x00, 0x00, 0x00, 0x00, 0xA0, 0x40, 0x01, 0x1E, 0x00, 0x64, 0x00, 0x01, 0x00, 0x03, 0x00, 0x32, 0x00, 0x1E, 0x00, 0x00, 0x1D, 0x00, 0x00, 0x7A, 0x44, 0x00, 0x00, 0x48, 0x43, 0x00, 0x00, 0xF0, 0x41, 0x00, 0x00, 0x48, 0x42, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80, 0x3F, 0x07, 0x01, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80, 0x40, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80, 0x40, 0x03, 0x00, 0x00, 0x00, 0x1E, 0x00, 0x00, 0x00, 0x00, 0x00, 0xA0, 0x40, 0x01, 0x1E, 0x00, 0x64, 0x00, 0x01, 0x00, 0x03, 0x00, 0x32, 0x00, 0x1E, 0x00, 0x00, 0x1D, 0x00, 0x00, 0x7A, 0x44, 0x00, 0x00, 0x48, 0x43, 0x00, 0x00, 0xF0, 0x41, 0x00, 0x00, 0x48, 0x42, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80, 0x3F, 0x08, 0x01, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80, 0x40, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80, 0x40, 0x03, 0x00, 0x00, 0x00, 0x1E, 0x00, 0x00, 0x00, 0x00, 0x00, 0xA0, 0x40, 0x01, 0x1E, 0x00, 0x64, 0x00, 0x01, 0x00, 0x03, 0x00, 0x32, 0x00, 0x1E, 0x00, 0x00, 0x1D, 0x00, 0x00, 0x7A, 0x44, 0x00, 0x00, 0x48, 0x43, 0x00, 0x00, 0xF0, 0x41, 0x00, 0x00, 0x48, 0x42, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80, 0x3F, 0x09, 0x01, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80, 0x40, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80, 0x40, 0x03, 0x00, 0x00, 0x00, 0x1E, 0x00, 0x00, 0x00, 0x00, 0x00, 0xA0, 0x40, 0x01, 0x1E, 0x00, 0x64, 0x00, 0x01, 0x00, 0x03, 0x00, 0x32, 0x00, 0x1E, 0x00, 0x00, 0x1D, 0x00, 0x00, 0x7A, 0x44, 0x00, 0x00, 0x48, 0x43, 0x00, 0x00, 0xF0, 0x41, 0x00, 0x00, 0x48, 0x42, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80, 0x3F, 0x0A, 0x01, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80, 0x40, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80, 0x40, 0x03, 0x00, 0x00, 0x00, 0x1E, 0x00, 0x00, 0x00, 0x00, 0x00, 0xA0, 0x40, 0x01, 0x1E, 0x00, 0x64, 0x00, 0x01, 0x00, 0x03, 0x00, 0x32, 0x00, 0x1E, 0x00, 0x00, 0x1D, 0x00, 0x00, 0x7A, 0x44, 0x00, 0x00, 0x48, 0x43, 0x00, 0x00, 0xF0, 0x41, 0x00, 0x00, 0x48, 0x42, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80, 0x3F, 0x0B, 0x01, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80, 0x40, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80, 0x40, 0x03, 0x00, 0x00, 0x00, 0x1E, 0x00, 0x00, 0x00, 0x00, 0x00, 0xA0, 0x40, 0x01, 0x1E, 0x00, 0x64, 0x00, 0x01, 0x00, 0x03, 0x00, 0x32, 0x00, 0x1E, 0x00, 0x00, 0x1D, 0x00, 0x00, 0x7A, 0x44, 0x00, 0x00, 0x48, 0x43, 0x00, 0x00, 0xF0, 0x41, 0x00, 0x00, 0x48, 0x42, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80, 0x3F, 0x0C, 0x01, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80, 0x40, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80, 0x40, 0x03, 0x00, 0x00, 0x00, 0x1E, 0x00, 0x00, 0x00, 0x00, 0x00, 0xA0, 0x40, 0x01, 0x1E, 0x00, 0x64, 0x00, 0x01, 0x00, 0x03, 0x00, 0x32, 0x00, 0x1E, 0x00, 0x00, 0x1D, 0x00, 0x00, 0x7A, 0x44, 0x00, 0x00, 0x48, 0x43, 0x00, 0x00, 0xF0, 0x41, 0x00, 0x00, 0x48, 0x42, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80, 0x3F, 0x0D, 0x01, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80, 0x40, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80, 0x40, 0x03, 0x00, 0x00, 0x00, 0x1E, 0x00, 0x00, 0x00, 0x00, 0x00, 0xA0, 0x40, 0x01, 0x1E, 0x00, 0x64, 0x00, 0x01, 0x00, 0x03, 0x00, 0x32, 0x00, 0x1E, 0x00, 0x00, 0x1D, 0x00, 0x00, 0x7A, 0x44, 0x00, 0x00, 0x48, 0x43, 0x00, 0x00, 0xF0, 0x41, 0x00, 0x00, 0x48, 0x42, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80, 0x3F, 0x0E, 0x01, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80, 0x40, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80, 0x40, 0x03, 0x00, 0x00, 0x00, 0x1E, 0x00, 0x00, 0x00, 0x00, 0x00, 0xA0, 0x40, 0x01, 0x1E, 0x00, 0x64, 0x00, 0x01, 0x00, 0x03, 0x00, 0x32, 0x00, 0x1E, 0x00, 0x00, 0x1D, 0x00, 0x00, 0x7A, 0x44, 0x00, 0x00, 0x48, 0x43, 0x00, 0x00, 0xF0, 0x41, 0x00, 0x00, 0x48, 0x42, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80, 0x3F, 0x0F, 0x01, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80, 0x40, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80, 0x40, 0x03, 0x00, 0x00, 0x00, 0x1E, 0x00, 0x00, 0x00, 0x00, 0x00, 0xA0, 0x40, 0x01, 0x1E, 0x00, 0x64, 0x00, 0x01, 0x00, 0x03, 0x00, 0x32, 0x00, 0x1E, 0x00, 0x00, 0x1D, 0x00, 0x00, 0x7A, 0x44, 0x00, 0x00, 0x48, 0x43, 0x00, 0x00, 0xF0, 0x41, 0x00, 0x00, 0x48, 0x42, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80, 0x3F, 0x10, 0x01, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80, 0x40, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80, 0x40, 0x03, 0x00, 0x00, 0x00, 0x1E, 0x00, 0x00, 0x00, 0x00, 0x00, 0xA0, 0x40, 0x01, 0x1E, 0x00, 0x64, 0x00, 0x01, 0x00, 0x03, 0x00, 0x32, 0x00, 0x1E, 0x00, 0x00, 0x1D, 0x00, 0x00, 0x7A, 0x44, 0x00, 0x00, 0x48, 0x43, 0x00, 0x00, 0xF0, 0x41, 0x00, 0x00, 0x48, 0x42, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80, 0x3F, 0x11, 0x01, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80, 0x40, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80, 0x40, 0x03, 0x00, 0x00, 0x00, 0x1E, 0x00, 0x00, 0x00, 0x00, 0x00, 0xA0, 0x40, 0x01, 0x1E, 0x00, 0x64, 0x00, 0x01, 0x00, 0x03, 0x00, 0x32, 0x00, 0x1E, 0x00, 0x00, 0x1D, 0x00, 0x00, 0x7A, 0x44, 0x00, 0x00, 0x48, 0x43, 0x00, 0x00, 0xF0, 0x41, 0x00, 0x00, 0x48, 0x42, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80, 0x3F, 0x12, 0x01, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80, 0x40, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80, 0x40, 0x03, 0x00, 0x00, 0x00, 0x1E, 0x00, 0x00, 0x00, 0x00, 0x00, 0xA0, 0x40, 0x01, 0x1E, 0x00, 0x64, 0x00, 0x01, 0x00, 0x03, 0x00, 0x32, 0x00, 0x1E, 0x00, 0x00, 0x1D, 0x00, 0x00, 0x7A, 0x44, 0x00, 0x00, 0x48, 0x43, 0x00, 0x00, 0xF0, 0x41, 0x00, 0x00, 0x48, 0x42, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80, 0x3F, 0x13, 0x01, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80, 0x40, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80, 0x40, 0x03, 0x00, 0x00, 0x00, 0x1E, 0x00, 0x00, 0x00, 0x00, 0x00, 0xA0, 0x40, 0x01, 0x1E, 0x00, 0x64, 0x00, 0x01, 0x00, 0x03, 0x00, 0x32, 0x00, 0x1E, 0x00, 0x00, 0x1D, 0x00, 0x00, 0x7A, 0x44, 0x00, 0x00, 0x48, 0x43, 0x00, 0x00, 0xF0, 0x41, 0x00, 0x00, 0x48, 0x42, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80, 0x3F, 0x14, 0x01, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80, 0x40, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80, 0x40, 0x03, 0x00, 0x00, 0x00, 0x1E, 0x00, 0x00, 0x00, 0x00, 0x00, 0xA0, 0x40, 0x01, 0x1E, 0x00, 0x64, 0x00, 0x01, 0x00, 0x03, 0x00, 0x32, 0x00, 0x1E, 0x00, 0x00, 0x1D, 0x00, 0x00, 0x7A, 0x44, 0x00, 0x00, 0x48, 0x43, 0x00, 0x00, 0xF0, 0x41, 0x00, 0x00, 0x48, 0x42, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80, 0x3F, 0x15, 0x01, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80, 0x40, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80, 0x40, 0x03, 0x00, 0x00, 0x00, 0x1E, 0x00, 0x00, 0x00, 0x00, 0x00, 0xA0, 0x40, 0x01, 0x1E, 0x00, 0x64, 0x00, 0x01, 0x00, 0x03, 0x00, 0x32, 0x00, 0x1E, 0x00, 0x00, 0x1D, 0x00, 0x00, 0x7A, 0x44, 0x00, 0x00, 0x48, 0x43, 0x00, 0x00, 0xF0, 0x41, 0x00, 0x00, 0x48, 0x42, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80, 0x3F, 0x16, 0x01, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80, 0x40, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80, 0x40, 0x03, 0x00, 0x00, 0x00, 0x1E, 0x00, 0x00, 0x00, 0x00, 0x00, 0xA0, 0x40, 0x01, 0x1E, 0x00, 0x64, 0x00, 0x01, 0x00, 0x03, 0x00, 0x32, 0x00, 0x1E, 0x00, 0x00, 0x1D, 0x00, 0x00, 0x7A, 0x44, 0x00, 0x00, 0x48, 0x43, 0x00, 0x00, 0xF0, 0x41, 0x00, 0x00, 0x48, 0x42, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80, 0x3F, 0x17, 0x01, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80, 0x40, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80, 0x40, 0x03, 0x00, 0x00, 0x00, 0x1E, 0x00, 0x00, 0x00, 0x00, 0x00, 0xA0, 0x40, 0x01, 0x1E, 0x00, 0x64, 0x00, 0x01, 0x00, 0x03, 0x00, 0x32, 0x00, 0x1E, 0x00, 0x00, 0x1D, 0x00, 0x00, 0x7A, 0x44, 0x00, 0x00, 0x48, 0x43, 0x00, 0x00, 0xF0, 0x41, 0x00, 0x00, 0x48, 0x42, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80, 0x3F, 0x18, 0x01, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80, 0x40, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80, 0x40, 0x03, 0x00, 0x00, 0x00, 0x1E, 0x00, 0x00, 0x00, 0x00, 0x00, 0xA0, 0x40, 0x01, 0x1E, 0x00, 0x64, 0x00, 0x01, 0x00, 0x03, 0x00, 0x32, 0x00, 0x1E, 0x00, 0x00, 0x1D, 0x00, 0x00, 0x7A, 0x44, 0x00, 0x00, 0x48, 0x43, 0x00, 0x00, 0xF0, 0x41, 0x00, 0x00, 0x48, 0x42, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80, 0x3F, 0x19, 0x01, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80, 0x40, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80, 0x40, 0x03, 0x00, 0x00, 0x00, 0x1E, 0x00, 0x00, 0x00, 0x00, 0x00, 0xA0, 0x40, 0x01, 0x1E, 0x00, 0x64, 0x00, 0x01, 0x00, 0x03, 0x00, 0x32, 0x00, 0x1E, 0x00, 0x00, 0x1D, 0x00, 0x00, 0x7A, 0x44, 0x00, 0x00, 0x48, 0x43, 0x00, 0x00, 0xF0, 0x41, 0x00, 0x00, 0x48, 0x42, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80, 0x3F, 0x1A, 0x01, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80, 0x40, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80, 0x40, 0x03, 0x00, 0x00, 0x00, 0x1E, 0x00, 0x00, 0x00, 0x00, 0x00, 0xA0, 0x40, 0x01, 0x1E, 0x00, 0x64, 0x00, 0x01, 0x00, 0x03, 0x00, 0x32, 0x00, 0x1E, 0x00, 0x00, 0x1D, 0x00, 0x00, 0x7A, 0x44, 0x00, 0x00, 0x48, 0x43, 0x00, 0x00, 0xF0, 0x41, 0x00, 0x00, 0x48, 0x42, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80, 0x3F, 0x1B, 0x01, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80, 0x40, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80, 0x40, 0x03, 0x00, 0x00, 0x00, 0x1E, 0x00, 0x00, 0x00, 0x00, 0x00, 0xA0, 0x40, 0x01, 0x1E, 0x00, 0x64, 0x00, 0x01, 0x00, 0x03, 0x00, 0x32, 0x00, 0x1E, 0x00, 0x00, 0x1D, 0x00, 0x00, 0x7A, 0x44, 0x00, 0x00, 0x48, 0x43, 0x00, 0x00, 0xF0, 0x41, 0x00, 0x00, 0x48, 0x42, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80, 0x3F, 0x1C, 0x01, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80, 0x40, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80, 0x40, 0x03, 0x00, 0x00, 0x00, 0x1E, 0x00, 0x00, 0x00, 0x00, 0x00, 0xA0, 0x40, 0x01, 0x1E, 0x00, 0x64, 0x00, 0x01, 0x00, 0x03, 0x00, 0x32, 0x00, 0x1E, 0x00, 0x00, 0x1D, 0x00, 0x00, 0x7A, 0x44, 0x00, 0x00, 0x48, 0x43, 0x00, 0x00, 0xF0, 0x41, 0x00, 0x00, 0x48, 0x42, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80, 0x3F, 0x1D, 0x01, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80, 0x40, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80, 0x40, 0x03, 0x00, 0x00, 0x00, 0x1E, 0x00, 0x00, 0x00, 0x00, 0x00, 0xA0, 0x40, 0x01, 0x1E, 0x00, 0x64, 0x00, 0x01, 0x00, 0x03, 0x00, 0x32, 0x00, 0x1E, 0x00, 0x00, 0x1D, 0x00, 0x00, 0x7A, 0x44, 0x00, 0x00, 0x48, 0x43, 0x00, 0x00, 0xF0, 0x41, 0x00, 0x00, 0x48, 0x42, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80, 0x3F, 0x1E, 0x01, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80, 0x40, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80, 0x40, 0x03, 0x00, 0x00, 0x00, 0x1E, 0x00, 0x00, 0x00, 0x00, 0x00, 0xA0, 0x40, 0x01, 0x1E, 0x00, 0x64, 0x00, 0x01, 0x00, 0x03, 0x00, 0x32, 0x00, 0x1E, 0x00, 0x00, 0x1D, 0x00, 0x00, 0x7A, 0x44, 0x00, 0x00, 0x48, 0x43, 0x00, 0x00, 0xF0, 0x41, 0x00, 0x00, 0x48, 0x42, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x14, 0x32, 0x00, 0x00, 0x00, 0x64, 0x00, 0x00, 0x00, 0x33, 0x00, 0x00, 0x00, 0x64, 0x00, 0x00, 0x00, 0x34, 0x00, 0x00, 0x00, 0x64, 0x00, 0x00, 0x00, 0x35, 0x00, 0x00, 0x00, 0x64, 0x00, 0x00, 0x00, 0x36, 0x00, 0x00, 0x00, 0x64, 0x00, 0x00, 0x00, 0x37, 0x00, 0x00, 0x00, 0x64, 0x00, 0x00, 0x00, 0x38, 0x00, 0x00, 0x00, 0x64, 0x00, 0x00, 0x00, 0x39, 0x00, 0x00, 0x00, 0x64, 0x00, 0x00, 0x00, 0x3A, 0x00, 0x00, 0x00, 0x64, 0x00, 0x00, 0x00, 0x3B, 0x00, 0x00, 0x00, 0x64, 0x00, 0x00, 0x00, 0x3C, 0x00, 0x00, 0x00, 0x64, 0x00, 0x00, 0x00, 0x3D, 0x00, 0x00, 0x00, 0x64, 0x00, 0x00, 0x00, 0x3E, 0x00, 0x00, 0x00, 0x64, 0x00, 0x00, 0x00, 0x3F, 0x00, 0x00, 0x00, 0x64, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x64, 0x00, 0x00, 0x00, 0x41, 0x00, 0x00, 0x00, 0x64, 0x00, 0x00, 0x00, 0x42, 0x00, 0x00, 0x00, 0x64, 0x00, 0x00, 0x00, 0x43, 0x00, 0x00, 0x00, 0x64, 0x00, 0x00, 0x00, 0x44, 0x00, 0x00, 0x00, 0x64, 0x00, 0x00, 0x00, 0x45, 0x00, 0x00, 0x00, 0x64, 0x00, 0x00, 0x00, 0x01, 0x05, 0x54, 0x65, 0x73, 0x74, 0x30, 0x3C, 0x00, 0x3C, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0xFA, 0x00, 0x3C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    //
    // DWORD time0 = GetTickCount();

    // DTO::ServerDTO* servertest = Serialization::Serializer::DeserializeServerData(test);

    // Logging::LoggerService::LogDebug(std::to_string(float(GetTickCount() - time0)));

    // time0 = GetTickCount();

    // DTO::ClientDTO* clienttest = new DTO::ClientDTO();

    // clienttest->WorldData = servertest->WorldData;

    // byte serialized[5120];
    // Serialization::Serializer::SerializeClientData(&serialized[0], clienttest);

    // Logging::LoggerService::LogDebug(std::to_string(float(GetTickCount() - time0)));

    // int playerNumberTemp = 0;

    // memInstance = new MemoryInstance(GetModuleHandleA(NULL));

    //// This one is important - sets stuff up so that we can be called by the asm patch
    // init();

    //// And set up ActorData
    // ActorData::InitDefaultValues();

    // Game::GameInstance->scan();

    // Game::GameInstance->setActorSpawning(&queue_mutex, queueActor);

    // std::map<int, MemoryAccess::LocalInstance::FlagAddresses> Flags = Game::GameInstance->scanPlayerFlags();

    // for (int i = 1; i < 32; i++)
    //     Instances::PlayerList.insert({ i, new MemoryAccess::Player(i, Game::GameInstance, Flags[i]) });

    ////client->sendMessage("!update", "Initial connection");
    ////client->receive();

    // std::vector<uint64_t> WorldData = Link->ScanData();
    ////std::stringstream stream;
    ////stream << std::hex << Link->PosAddr << std::endl;
    ////Logging::LoggerService::LogDebug(stream.str());
    ////Vec3fBE* Testing = new Vec3fBE(Link->PosAddr, __FUNCTION__);
    ////Testing->set(Vec3f(100, 5000, 100));

    // if (isQuestSync)
    //     CreateThread(0, 0, (LPTHREAD_START_ROUTINE)QuestSync, 0, 0, 0);

    // World->dayAddr = WorldData[0];
    // World->timeAddr = WorldData[1];

    // if (isGlyphSync)
    //     CreateThread(0, 0, (LPTHREAD_START_ROUTINE)glyphScan, 0, 0, 0);

    // std::cout << "Enemy sync: " << isEnemySync << std::endl;
    // std::cout << "Glyph sync: " << isGlyphSync << std::endl;

    // uint64_t offset = 0;

    // for (int i = 0; i < 4; i++)
    //{
    //     offset = Jugadores[i]->ScanData(offset);
    // }

    // std::vector<int> sig;

    // std::vector<std::string> axes = { "x", "y", "z" };
    // rapidjson::Document serverDoc;

    // if (isGlyphSync)
    //{
    //     sig = { 0x10, 0x21, 0xB2, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3F, 0x80, 0x00, 0x00, 0x42, 0xB4, 0x00, -1, 0x3F, 0x80 };
    //     uint64_t glyphAddr = Memory::PatternScan(sig, Memory::getBaseAddress(), 8, 0x42000000) - 0x24;

    //    for (int i = 0; i < 4; i++)
    //    {

    //        int glyph;

    //        switch (i)
    //        {
    //        case 0:
    //            glyph = 3;
    //            break;
    //        case 1:
    //            glyph = 0;
    //            break;
    //        case 2:
    //            glyph = 1;
    //            break;
    //        case 3:
    //            glyph = 4;
    //            break;
    //        }

    //        Jugadores[i]->setGlyphAddr(glyphAddr + 0x70 * glyph);
    //        Jugadores[i]->glyphSync = TRUE;
    //    }
    //}

    // Instances::EnemyScanner->LastClear = GetTickCount();

    // bool firstTime = true;

    // Memory::MultiplayerQuest::changeMQuestSvName(serverName, offset - 0x50000);

    // Memory::MultiplayerQuest::findMQuestPingAddress(offset - 0x50000);

    // Instances::QuestSyncer->findAddingAddresses(offset - 0x50000);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////////////////// LOCAL TEST ////////////////////////////////////////////////////////////////////////////////////////////////

    ////bool isLocalTest = true;
    // playerNumberTemp = playerNumber;

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // DWORD t2 = GetTickCount();
    // bool doCharacterSpawner = true;

    // int damageAnimations[12] = { 1694655571, 1651482698, 2193246548, 2025157687, 1690308196, 1672333949, 2205688675, 2037849600, 1920499742, 1964327943, 2512432409, 1875873914 };

    // std::vector<std::vector<float>> lastValues = { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 } };

    // DWORD LastDeathSwap = GetTickCount();

    ////activatedAddr = Memory::MultiplayerQuest::findMQuests(offset - 0x50000);
    ////Memory::MultiplayerQuest::startMQuest(activatedAddr);

    // sig = { 0x00, 0x00, -1, 0x00, 0x10, 0x29, 0x84, 0x10, 0x00, 0x01, 0x00, -1, 0x93, 0x9B, 0xE2, 0xB9 };
    // notPaused = Memory::PatternScan(sig, Memory::getBaseAddress(), 8, offset) + 0xB;

    // CreateThread(0, 0, (LPTHREAD_START_ROUTINE)PauseChecker, 0, 0, 0);

    // bool NICharacterSpawn = false;
    // bool AnCharacterSpawn = false;

    // std::cout << Memory::getBaseAddress() << std::endl;

    // while (true)
    //{

    //    if (Link->UpdateData() == 1)
    //    {

    //        //for (int i = 0; i < 4; i++)
    //        //{
    //        //    Jugadores[i]->restartAnimations();
    //        //}

    //    }
    //
    //    Logging::LoggerService::LogDebug(std::to_string(Game::GameInstance->Position->get_x(__FUNCTION__)));

    //    for (int i = 0; i < 12; i++)
    //    {
    //        if (Link->Animation == damageAnimations[i])
    //        {

    //            if (float(GetTickCount() - Link->lastHealthUpdate) > 500)
    //            {

    //                CalculatePVPDamage();

    //            }

    //            break;
    //        }
    //    }

    //    if (Main::QuestSyncReady) Instances::QuestSyncer->readQuests();

    //    t0 = GetTickCount();

    // client->sendMessage("!update", getSendString());

    //    std::string serverDataBuffer = client->receive();

    //    //while (Main::isPaused)
    //    //{
    //    //    Sleep(10);
    //    //}

    //    ping = (float(GetTickCount() - t0));

    //    if (serverDataBuffer == "")
    //    {
    //        continue;
    //    }

    //    if (serverDataBuffer.find("FW") != std::string::npos)
    //    {
    //        std::cout << "Server: Package lost" << std::endl;
    //        Sleep(2000);
    //        continue;
    //    }

    //    if (std::count(serverDataBuffer.begin(), serverDataBuffer.end(), '{') != std::count(serverDataBuffer.begin(), serverDataBuffer.end(), '}') and std::count(serverDataBuffer.begin(), serverDataBuffer.end(), '{') > 1)
    //    {
    //        Logging::LoggerService::LogDebug("Local: Package lost");
    //        Sleep(2000);
    //        continue;
    //    }

    //    serverData = serverDataBuffer;

    //    serverDoc = Connectivity::deserializeServerData(serverData);

    //    rapidjson::Value WorldData(rapidjson::kArrayType);
    //    WorldData = serverDoc["WD"];

    //    //while (Main::isPaused)
    //    //{
    //    //    Sleep(10);
    //    //}

    //    World->UpdateTime();

    //    //while (Main::isPaused)
    //    //{
    //    //    Sleep(10);
    //    //}

    //    World->SetWorldTime(WorldData["T"].GetFloat(), WorldData["D"].GetInt());

    //    newWeather = (Weather)WorldData["W"].GetInt();

    //    rapidjson::Value UpdateData(rapidjson::kArrayType);
    //    UpdateData = serverDoc["UD"].GetArray();

    //    rapidjson::Value NetworkInfo(rapidjson::kArrayType);
    //    NetworkInfo = serverDoc["ND"];

    //    serializationRate = NetworkInfo["SR"].GetInt();
    //    int SleepMultiplier = NetworkInfo["SM"].GetInt();
    //    targetFPS = (float)NetworkInfo["TFPS"].GetInt();
    //    bool isLocalTest = NetworkInfo["LT"].GetInt() == 0 ? false : true;
    //    bool NICharacterSpawn = NetworkInfo["CS"].GetInt() == 0 ? false : true;
    //    GlyphUpdateTime = NetworkInfo["GT"].GetInt();
    //    GlyphDistance = NetworkInfo["GD"].GetInt();
    //    isEnemySync = NetworkInfo["ES"].GetInt() == 0 ? false : true;
    //    isQuestSync = NetworkInfo["QS"].GetInt() == 0 ? false : true;

    //    if (isLocalTest)
    //    {
    //        playerNumber = -1;
    //    }
    //    else
    //    {
    //        playerNumber = playerNumberTemp;
    //    }

    //    Memory::MultiplayerQuest::changeMQuestPing(static_cast<int>(ping));

    //    if (1 / ping > (serializationRate / 1000))
    //    {
    //        int timeToSleepSR = ((1 / serializationRate) - (ping / 1000)) * 1000 * SleepMultiplier;
    //        if (timeToSleepSR > 0)
    //        {
    //            Sleep(timeToSleepSR);
    //            ping = ping + timeToSleepSR;
    //        }
    //    }

    //    rapidjson::Value playerData(rapidjson::kArrayType);
    //    playerData = serverDoc["PD"].GetArray();

    //    rapidjson::Value bombData(rapidjson::kArrayType);
    //    bombData = serverDoc["BD"].GetArray();

    //    AnCharacterSpawn = playerData[playerNumber == -1 ? 0 : playerNumber]["Schd"].GetInt() == 0 || playerData[playerNumber == -1 ? 0 : playerNumber]["Anim"].GetInt() == 0 ? false : true;

    //    if (AnCharacterSpawn && NICharacterSpawn)
    //    {
    //        isCharacterSpawn = true;
    //    }
    //    else
    //    {
    //        isCharacterSpawn = false;
    //    }

    //    if (isEnemySync)
    //    {
    //        float clearTime = float(GetTickCount() - Instances::EnemyScanner->LastClear) / 1000 / 60;

    //        if (clearTime > Instances::EnemyScanner->CLEARMINUTES)
    //        {
    //            Instances::EnemyScanner->EnemyMutex.lock();

    //            Instances::EnemyScanner->ClearData();

    //            Instances::EnemyScanner->EnemyMutex.unlock();

    //            Instances::EnemyScanner->LastClear = GetTickCount();
    //        }

    //        //while (Main::isPaused)
    //        //{
    //        //    Sleep(10);
    //        //}

    //        Instances::EnemyScanner->UpdateHealth(serverDoc["ED"]);
    //    }
    //    else
    //    {
    //        Instances::EnemyScanner->EnemyMutex.lock();

    //        Instances::EnemyScanner->ClearData();

    //        Instances::EnemyScanner->EnemyMutex.unlock();

    //        Instances::EnemyScanner->LastClear = GetTickCount();
    //    }

    //    rapidjson::Value questData(rapidjson::kArrayType);
    //    questData = serverDoc["QD"].GetArray();

    //    if (isQuestSync)
    //    {
    //        for (int i = 0; i < questData.Size(); i++)
    //        {

    //            if (std::find(Instances::QuestSyncer->questsToChange.begin(), Instances::QuestSyncer->questsToChange.end(), questData[i].GetString()) == Instances::QuestSyncer->questsToChange.end())
    //            {

    //                Instances::QuestSyncer->questsToChange.push_back(questData[i].GetString());

    //            }

    //        }

    //        if (Main::QuestSyncReady) questSyncUsingEvent = Instances::QuestSyncer->updateQuests(eventStatus);

    //        if (!isPaused)
    //        {
    //            Instances::QuestSyncer->resyncQuests();
    //        }

    //        eventStatus = questSyncUsingEvent;

    //    }
    //    else
    //    {

    //        Instances::QuestSyncer->QuestMutex.lock();

    //        Instances::QuestSyncer->serverQuests.clear();
    //        Instances::QuestSyncer->koroksToAdd = 0;
    //        Instances::QuestSyncer->boolsToChange.clear();
    //        Instances::QuestSyncer->itemsToAdd.clear();
    //        Instances::QuestSyncer->intsToChange.clear();

    //        for (auto const& pair : Instances::QuestSyncer->numberOfQuests)
    //        {

    //            std::string QType = pair.first;
    //            int QNumber = pair.second;

    //            for (int i = 0; i < QNumber; i++)
    //            {

    //                Instances::QuestSyncer->QuestList[QType + std::to_string(i)].Value = 0;
    //                Instances::QuestSyncer->QuestList[QType + std::to_string(i)].beingChanged = false;

    //            }

    //        }

    //        Instances::QuestSyncer->changedQuests.clear();

    //        Instances::QuestSyncer->QuestMutex.unlock();

    //    }

    //    if (firstTime)
    //    {
    //        Memory::MessagerService::AddMessage("Joined server.");
    //        CreateThread(0, 0, (LPTHREAD_START_ROUTINE)HelperThread, 0, 0, 0);
    //        CreateThread(0, 0, (LPTHREAD_START_ROUTINE)PlayerUpdater, 0, 0, 0);
    //        firstTime = false;
    //        started = true;
    //    }

    //    if (float(GetTickCount() - t2) / 1000 > 10 && doCharacterSpawner)
    //    {
    //        CreateThread(0, 0, (LPTHREAD_START_ROUTINE)characterSpawner, 0, 0, 0);
    //        doCharacterSpawner = false;
    //    }

    //    if (serverDoc["SWAP"]["Phase"].GetInt() == 1)
    //    {

    //        if (playerNumber == 0 || playerNumber == 1)
    //        {

    //            if (float(GetTickCount() - LastDeathSwap) / 1000 > 15)
    //            {

    //                LastDeathSwap = GetTickCount();
    //                Memory::MessagerService::AddMessage("Swapping positions...");

    //            }

    //        }

    //    }

    //    if (serverDoc["SWAP"]["Phase"].GetInt() == 2)
    //    {

    //        if (playerNumber == 0 || playerNumber == 1)
    //        {

    //            rapidjson::Value PosArray(rapidjson::kArrayType);
    //            PosArray = serverDoc["SWAP"]["Position"].GetArray();

    //            float newPos[3] = { PosArray[0].GetFloat(), PosArray[1].GetFloat() + 5, PosArray[2].GetFloat() };

    //            Link->Teleport(newPos);

    //        }

    //    }

    //    for (int i = 0; i < 4; i++)
    //    {

    //        if (ConnectedPlayers[i] != playerData[i]["Con"].GetBool() && i != playerNumber)
    //        {

    //            ConnectedPlayers[i] = playerData[i]["Con"].GetBool();
    //
    //            if(ConnectedPlayers[i])
    //                Jugadores[i]->Name = playerData[i]["Name"].GetString();

    //            std::string JOINED = " joined.";
    //            std::string LEFT = " left.";

    //            std::string Status = ConnectedPlayers[i] ? playerData[i]["Name"].GetString() + JOINED : Jugadores[i]->Name + LEFT;

    //            Logging::LoggerService::LogInformation("Player " + Status, __FUNCTION__);

    //            Memory::MessagerService::AddMessage(Status);

    //        }

    //        if (i == playerNumber || !playerData[i]["Con"].GetBool())
    //        {
    //            pingMult[i] = 0;
    //            continue;
    //        }

    //        if (UpdateData[i].GetInt() == 0)
    //        {
    //            pingMult[i]++;
    //            continue;
    //        }

    //        std::map<std::string, std::string> BombChoices = { {"RB", "RemoteBomb"}, {"RB2", "RemoteBomb2"}, {"RBC", "RemoteBombCube"}, {"RBC2", "RemoteBombCube2"} };

    //        for (rapidjson::Value::ConstMemberIterator iter = bombData[i].MemberBegin(); iter != bombData[i].MemberEnd(); ++iter)
    //        {

    //            std::string BombName = iter->name.GetString();
    //            float BombPositions[3] = { 0, 0, 0 };

    //            float LocalExtra = 0;

    //            if (isLocalTest && i == 0 && iter->value.GetArray()[0].GetDouble() != 0 && iter->value.GetArray()[0].GetDouble() != -1)
    //            {
    //                LocalExtra += 10;
    //            }

    //            BombPositions[0] = iter->value.GetArray()[0].GetDouble() + LocalExtra;
    //            BombPositions[1] = iter->value.GetArray()[1].GetDouble() + LocalExtra / 2;
    //            BombPositions[2] = iter->value.GetArray()[2].GetDouble();

    //            if (!Jugadores[i]->UserBombs[BombChoices[BombName]]->Exists)
    //            {
    //                if (iter->value.GetArray()[0].GetDouble() == 0 || iter->value.GetArray()[0].GetDouble() == -1) continue;

    //                if (std::sqrt(pow(std::abs(Link->Position[0] - BombPositions[0]), 2) + pow(std::abs(Link->Position[2] - BombPositions[2]), 2)) < 100)
    //                {
    //                    Jugadores[i]->UserBombs[BombChoices[BombName]]->SetData(BombChoices[BombName], BombPositions);
    //                }

    //            }
    //            else
    //            {
    //                Jugadores[i]->UserBombs[BombChoices[BombName]]->UpdatePos(BombPositions);
    //            }

    //        }

    //        Jugadores[i]->Health = playerData[i]["H"].GetInt();
    //        Jugadores[i]->changeName(playerData[i]["Name"].GetString(), NetworkInfo["DN"].GetInt());
    //        Jugadores[i]->Name = playerData[i]["Name"].GetString();

    //        //for (int m = 0; m < 3; m++)
    //        //{
    //        //    float newValue = (float)playerData[i]["P"].GetArray()[m].GetDouble();

    //        //    if (isLocalTest && m == 0)
    //        //        newValue += 2;
    //        //
    //        //    Main::JugadoresQueues[i][m].insert(Main::JugadoresQueues[i][m].begin(), newValue);

    //        //    lastValues[i][m] = newValue;
    //        //    pingMult[playerNumber] = 1;
    //        //}

    //        float numberOfUpdates = ((ping * pingMult[i]) * (targetFPS / 1000)) - Main::JugadoresQueues[i][0].size();  // (50 ms * 3) * (60/1000) updates / ms
    //        Jugadores[i]->setRotation((float)playerData[i]["R"].GetDouble());
    //        Jugadores[i]->setAnimation(playerData[i]["Schd"].GetInt(), playerData[i]["Anim"].GetInt());
    //
    //        if (numberOfUpdates > 60) numberOfUpdates = 60;

    //        for (int m = 0; m < 3; m++)
    //        {

    //            float newValue = (float)playerData[i]["P"].GetArray()[m].GetDouble();

    //            if (m == 0 && isLocalTest && i == 0)
    //            {
    //                newValue += 2.5;
    //            }

    //            if (numberOfUpdates < 1)
    //            {
    //                if (Main::JugadoresQueues[i][m].size() > 60)
    //                    continue;

    //                Main::JugadoresQueues[i][m].insert(Main::JugadoresQueues[i][m].begin(), newValue);
    //            }
    //            else
    //            {
    //                for (float update = 1; update < numberOfUpdates + 1; update++)
    //                {

    //                    float InterpolatedValue = lastValues[i][m] + (newValue - lastValues[i][m]) * (update / numberOfUpdates);
    //                    Main::JugadoresQueues[i][m].insert(Main::JugadoresQueues[i][m].begin(), InterpolatedValue);

    //                }
    //            }

    //            lastValues[i][m] = newValue;
    //            pingMult[i] = 1;

    //        }

    //        Jugadores[i]->AttkUp = playerData[i]["At"].GetFloat();

    //        Jugadores[i]->setIsEquipped(playerData[i]["IE"].GetInt());

    //        for (rapidjson::Value::ConstMemberIterator iter = playerData[i]["E"].MemberBegin(); iter != playerData[i]["E"].MemberEnd(); ++iter)
    //        {

    //            std::string EqVal = iter->value.GetString();
    //            std::string EqName = iter->name.GetString();

    //            if (Jugadores[i]->Equipment[EqName] == EqVal)
    //            {
    //                continue;
    //            }

    //            if (EqVal != ".")
    //            {
    //                Jugadores[i]->Equipment[EqName] = EqVal;

    //                if (EqName != "B")
    //                {

    //                    Jugadores[i]->WeaponChangeMutex.lock();

    //                    Jugadores[i]->WeaponChanged = true;

    //                    Jugadores[i]->WeaponChangeMutex.unlock();

    //                }
    //            }

    //        }

    //        /*for (int m = 0; m < 3; m++)
    //        {

    //            float newValue = (float)playerData[i]["P"].GetArray()[m].GetDouble();

    //            if (lastValues[i][m] != newValue)
    //            {
    //                addToQueue = true;
    //            }

    //        }

    //        if (!addToQueue)
    //        {
    //            pingMult[i]++;
    //            continue;
    //        }*/

    //    }

    //}
}

// void oldCalculatePVPDamage()
//{
//
//     float DistanceFromPlayer[4];
//     float LastAttack[4];
//     bool PotentialAttacker[4];
//
//     float HitProbability = 0;
//     int Attacker = 0;
//
//     for (int i = 0; i < 4; i++)
//     {
//         DistanceFromPlayer[i] = std::sqrt(pow(std::abs(Link->Position[0] - Jugadores[i]->Pos[0]), 2) + pow(std::abs(Link->Position[1] - Jugadores[i]->Pos[1]), 2) + pow(std::abs(Link->Position[2] - Jugadores[i]->Pos[2]), 2));
//         LastAttack[i] = float(GetTickCount() - Jugadores[i]->attackTimer);
//
//         if (DistanceFromPlayer[i] > 5 || LastAttack[i] > 500)
//         {
//             continue;
//         }
//
//         // The highest probability is where you are on Distance = 0 and LastAttack = 0 | 50% of the probability depends on the distance and 50% of the probability depends on the time | If your distance is higher than 3.5 or your LastAttack is higher than 500, you are not considered
//
//         float hitProb = (1 - (DistanceFromPlayer[i] / 4.5)) * 0.5 + (1 - (LastAttack[i] / 500)) * 0.5;
//
//         if (hitProb > HitProbability)
//         {
//             HitProbability = hitProb;
//             Attacker = i + 1;
//         }
//
//     }
//
//     if (Attacker == 0)
//     {
//         return;
//     }
//
//     Logging::LoggerService::LogInformation("Calculating pvp damage...", __FUNCTION__);
//
//     float WpnDamage = 0;
//
//     for (rapidjson::Value::ConstMemberIterator iter = Link->WeaponDamages.MemberBegin(); iter != Link->WeaponDamages.MemberEnd(); ++iter)
//     {
//         std::string Weapon = iter->name.GetString();
//         int damage = iter->value.GetInt();
//
//         if (Weapon == Jugadores[Attacker - 1]->Equipment["W"])
//         {
//             WpnDamage = damage;
//             break;
//         }
//     }
//
//     Logging::LoggerService::LogDebug("Attacker: " + std::to_string(Attacker) + " | Weapon damage: " + Link->to_string_precision(WpnDamage, 4) + " | Attack atk up: " + Link->to_string_precision(Jugadores[Attacker - 1]->AttkUp, 4) + " | Player defense: " + std::to_string(Link->def), __FUNCTION__);
//
//     WpnDamage = (WpnDamage * Jugadores[Attacker - 1]->AttkUp - Link->def) < 1 ? 1 : WpnDamage * Jugadores[Attacker - 1]->AttkUp - Link->def;
//
//     Link->reduceHealth(WpnDamage);
//
//     Logging::LoggerService::LogInformation("Reduced health by " + std::to_string(WpnDamage), __FUNCTION__);
// }

bool Main::CheckIfPaused()
{
    // Memory::write_byte(notPaused, 0x00, __FUNCTION__);
    Game::GameInstance->NotPaused->set(0, __FUNCTION__);

    Sleep(300);

    return !Game::GameInstance->NotPaused->get(__FUNCTION__);

    // return Memory::read_bytes(notPaused, 1, __FUNCTION__)[0] == 0x0 ? true : false;
}

void Memory::Bomb::SetData(std::string bombType, float initialPos[3])
{

    std::vector<std::string> BombChoices = {"CustomRemoteBomb", "CustomRemoteBomb2", "CustomRemoteBombCube", "CustomRemoteBombCube2"};

    this->BombType = bombType;
    this->Exploded = false;
    this->Pos[0] = initialPos[0];
    this->Pos[1] = initialPos[1];
    this->Pos[2] = initialPos[2];

    for (int i = 0; i < 4; i++)
    {
        if (BombType == BombChoices[i])
        {
            this->BombIndex = i;
            break;
        }
    }

    this->Exists = this->RequestCreate();
}

bool Memory::Bomb::RequestCreate()
{

    int Attempts = 0;

    while (Main::CheckIfPaused())
    {
        Sleep(5);
    }

    Main::QueueBomb("Custom" + this->BombType, this->Pos);

    Logging::LoggerService::LogDebug("Requested " + this->BombType + " creation.", __FUNCTION__);

    while (Main::BombSync->BombAvailableAddresses[this->BombIndex].size() == 0)
    {
        Sleep(5);
        Attempts++;

        if (Attempts >= 100)
        {
            Logging::LoggerService::LogWarning("Could not assign bomb. Retrying...", __FUNCTION__);
            return false;
        }
    }

    Main::BombSync->OtherPlayerBombsMutex.lock();
    this->BaseAddress = Main::BombSync->BombAvailableAddresses[this->BombIndex].back();
    Main::BombSync->BombAvailableAddresses[this->BombIndex].pop_back();
    Main::BombSync->OtherPlayerBombsMutex.unlock();

    Logging::LoggerService::LogDebug("Assigned " + std::to_string(this->BaseAddress), __FUNCTION__);

    return true;
}

void Memory::Bomb::UpdatePos(float newPosition[3])
{

    if (newPosition[0] == 0 && newPosition[1] == 0 && newPosition[2] == 0)
    {

        Main::BombSync->BombExplodeMutex.lock();

        Main::BombSync->BombsToExplode.push_back(this->BaseAddress);

        // while (std::find(Main::BombSync->BombsToExplode.begin(), Main::BombSync->BombsToExplode.end(), this->BaseAddress) != Main::BombSync->BombsToExplode.end())
        //{
        //     Sleep(5);
        // }

        this->Exploded = true;
        this->Exists = false;

        this->BaseAddress = 0;

        Main::BombSync->BombExplodeMutex.unlock();

        return;

    } else if (newPosition[0] == -1 && newPosition[1] == -1 && newPosition[2] == -1)
    {

        Main::BombSync->BombExplodeMutex.lock();

        Main::BombSync->BombsToClear.push_back(this->BaseAddress);

        this->Exploded = true;
        this->Exists = false;

        this->BaseAddress = 0;

        Main::BombSync->BombExplodeMutex.unlock();

        return;
    }

    uint64_t PosAddr = FindPosAddr();

    if (PosAddr == 0)
        return;

    this->Pos[0] = newPosition[0];
    this->Pos[1] = newPosition[1];
    this->Pos[2] = newPosition[2];

    for (int i = 0; i < 3; i++)
    {

        Memory::write_bigEndianFloat(PosAddr + (i * 0x4), Pos[i], __FUNCTION__);
    }
}

uint64_t Memory::Bomb::FindPosAddr()
{

    // uint64_t PosAddr = Memory::read_bigEndian4BytesOffset(this->BaseAddress + 0x3A0);
    // if (PosAddr == 0) return 0;
    // PosAddr = Memory::read_bigEndian4BytesOffset(PosAddr + 0x50 - 0x4);
    // if (PosAddr == 0) return 0;
    // PosAddr = Memory::read_bigEndian4BytesOffset(PosAddr + 0x4 + 0x8);
    // if (PosAddr == 0) return 0;
    // PosAddr = Memory::read_bigEndian4BytesOffset(PosAddr + 0x80);
    // if (PosAddr == 0) return 0;
    // PosAddr = Memory::read_bigEndian4BytesOffset(PosAddr);
    // if (PosAddr == 0) return 0;
    // PosAddr = Memory::read_bigEndian4BytesOffset(PosAddr + 0x5C);
    // if (PosAddr == 0) return 0;
    // PosAddr = Memory::read_bigEndian4BytesOffset(PosAddr + 0x18) + Main::baseAddr;
    // if (PosAddr == Main::baseAddr) return 0;

    uint64_t PosAddr = Memory::ReadPointers(this->BaseAddress, {0x3A0, 0x50 - 0x4, 0x4 + 0x8, 0x80, 0x0, 0x5C, 0x18}, true);

    if (PosAddr == 0)
        return 0;

    return PosAddr + 0x50;
}

void Main::mainServerLoop()
{
    Setup();

    /* Variable initialization */

    int LocalTestPlayerNumber = playerNumber; // TODO: Is it needed?
    DWORD LastDeathSwapNotif = GetTickCount();

    bool FirstCycle = true;

    while (true)
    {
        if (FirstCycle && !Game::GameInstance->IsPaused())
        {
            Memory::MessagerService::AddMessage("Joined server.");
            CreateThread(0, 0, (LPTHREAD_START_ROUTINE)HelperThread, 0, 0, 0);
            FirstCycle = false;
            started = true;
        }

        byte serverData[7168];
        Serialization::Serializer::SerializeClientData(&serverData[0], Game::GameInstance->get(started && QuestSyncReady));

        DWORD pingTimer = GetTickCount();

        client->sendBytes(&serverData[0]);

        memset(&serverData[0], 0, 7168);

        client->receiveBytes(&serverData[0]);

        DTO::ServerDTO* serverResponse;

        try
        {
            serverResponse = Serialization::Serializer::DeserializeServerData(&serverData[0]);
        } catch (LPCWSTR ex)
        {
            MessageBoxW(NULL, ex, L"", MB_OK);
            Main::disconnectFromServer("Could not deserialize server data. Make sure that both the client and server versions match.");
        }

        ping = float(GetTickCount() - pingTimer) + 1;

        if (!Game::GameInstance->IsGamePaused)
            Memory::MultiplayerQuest::changeMQuestPing(static_cast<int>(ping));

        int timeToSleep = (1000 / serverResponse->NetworkData->SerializationRate) - ping;

        if (timeToSleep > 0)
        {
            Sleep(timeToSleep);
            ping += timeToSleep;
        }

        if (serverResponse->pvp)
        {
            CalculatePVPDamage();
        }


        if (!Helper::Vec3f_Operations::Equals(serverResponse->TeleportData->Destination, Vec3f(0, 0, 0)))
            Game::GameInstance->Teleport(serverResponse->TeleportData->Destination);

        // TODO: Add set for world dto
        if (!Game::GameInstance->IsGamePaused && Game::GameInstance->Location->LastKnown.Map != "CDungeon")
        {
            if (serverResponse->NetworkData->AlwaysDay)
            {
                Game::GameInstance->World->setAlwaysDay(180.000000);
            }
            else
            {
                Game::GameInstance->World->set(serverResponse->WorldData);
            }
        }
            

        // Death swap

        if ((serverResponse->DeathSwapData->Phase == 1) &&
            (playerNumber == 0 || playerNumber == 1) &&
            (float(GetTickCount() - LastDeathSwapNotif) / 1000 > 15))
        {
            LastDeathSwapNotif = GetTickCount();
            Memory::MessagerService::AddMessage("Swapping positions...");
        }

        if ((serverResponse->DeathSwapData->Phase == 2) && (playerNumber == 0 || playerNumber == 1))
            Game::GameInstance->Teleport(serverResponse->DeathSwapData->Position);

        // Prop hunt

        switch (serverResponse->PropHuntData->Phase)
        {
        case 0:
            if (!Main::IsPropHuntStopped)
            {
                Game::GameInstance->propHuntFlags.ShowPlayer->set(true, __FUNCTION__);
                Game::GameInstance->propHuntFlags.HidePlayer->set(false, __FUNCTION__);
                Game::GameInstance->propHuntFlags.HidingPhase->set(false, __FUNCTION__);
                Game::GameInstance->propHuntFlags.RunningPhase->set(false, __FUNCTION__);
                Game::GameInstance->propHuntFlags.Countdown->set(false, __FUNCTION__);
                Main::IsProp = false;
                Main::IsPropHuntStopped = true;
                Main::HidePlayer32 = true;
                Main::FoundPlayers = {};

                for (int i = 1; i < 33; i++)
                {
                    Instances::PlayerList[i]->HideFromMap = false;
                    Instances::PlayerList[i]->DispName->set(true, __FUNCTION__);
                }

                SendTimerMessage(false, "", 0, 0);
                AddBigMessage("Game Over");
            }

            break;
        case 1:
            if (Game::GameInstance->propHuntFlags.HidingPhase->LastKnown == false)
            {
                Game::GameInstance->propHuntFlags.ShowPlayer->set(serverResponse->PropHuntData->IsHunter ? true : false, __FUNCTION__);
                Game::GameInstance->propHuntFlags.HidePlayer->set(serverResponse->PropHuntData->IsHunter ? false : true, __FUNCTION__);
                Game::GameInstance->propHuntFlags.Countdown->set(false, __FUNCTION__);
                Game::GameInstance->propHuntFlags.HidingPhase->set(true, __FUNCTION__);
                Main::IsProp = !serverResponse->PropHuntData->IsHunter;
                Main::IsPropHuntStopped = false;
                Main::HidePlayer32 = false;
                Main::FoundPlayers = {};
                Game::GameInstance->Teleport(serverResponse->PropHuntData->StartingPosition);
                SendTimerMessage(true, "down", 60, 0);

                if (serverResponse->PropHuntData->IsHunter)
                {
                    AddBigMessage("You are a hunter!");
                } else
                {
                    AddBigMessage("You are a prop!");
                }

                for (int i = 1; i < 33; i++)
                {
                    Instances::PlayerList[i]->HideFromMap = true;
                    Instances::PlayerList[i]->DispName->set(false, __FUNCTION__);
                }
            }

            break;
        case 2:
            if (Game::GameInstance->propHuntFlags.RunningPhase->LastKnown == false)
            {
                Game::GameInstance->propHuntFlags.ShowPlayer->set(serverResponse->PropHuntData->IsHunter ? true : false, __FUNCTION__);
                Game::GameInstance->propHuntFlags.HidePlayer->set(serverResponse->PropHuntData->IsHunter ? false : true, __FUNCTION__);
                Game::GameInstance->propHuntFlags.Countdown->set(false, __FUNCTION__);
                Game::GameInstance->propHuntFlags.RunningPhase->set(true, __FUNCTION__);
                Game::GameInstance->propHuntFlags.HidingPhase->set(false, __FUNCTION__);
                Main::IsProp = !serverResponse->PropHuntData->IsHunter;
                Main::IsPropHuntStopped = false;
                Main::HidePlayer32 = false;
                Main::FoundPlayers = {};

                AddBigMessage("Go get em Hunters!");
                SendTimerMessage(true, "up", 0, 0);
            }

            break;
        case 3:
            if (Game::GameInstance->propHuntFlags.Countdown->LastKnown == false)
            {
                Game::GameInstance->propHuntFlags.Countdown->set(true, __FUNCTION__);
                Game::GameInstance->propHuntFlags.ShowPlayer->set(serverResponse->PropHuntData->IsHunter ? true : false, __FUNCTION__);
                Game::GameInstance->propHuntFlags.HidePlayer->set(serverResponse->PropHuntData->IsHunter ? false : true, __FUNCTION__);
                SendTimerMessage(false, "", 0, 0);
                Main::IsProp = false;
                Main::IsPropHuntStopped = false;
                Main::HidePlayer32 = false;
                Main::FoundPlayers = {};
            }
            break;
        }

        for (auto const& pair : serverResponse->NameData->Names)
            Instances::PlayerList[pair.first + 1]->Name = pair.second;

        for (auto const& pair : serverResponse->ModelData->Models)
        {
            if (pair.first == Main::playerNumber)
            {
                Instances::PlayerList[32]->Model = pair.second;
                Instances::PlayerList[32]->Equipment->Changed = true;

                if (pair.second.ModelType != 2)
                {
                    Instances::PlayerList[32]->Model.Bumii.ffsd.no_use_ffsd = 1;
                }

                Instances::PlayerList[32]->Bumii->Initialize(Instances::PlayerList[32]->Model.Bumii);
            } else
            {
                Instances::PlayerList[pair.first + 1]->Model = pair.second;
                Instances::PlayerList[pair.first + 1]->Equipment->Changed = true;

                if (pair.second.ModelType != 2)
                {
                    Instances::PlayerList[pair.first + 1]->Model.Bumii.ffsd.no_use_ffsd = 1;
                }

                Instances::PlayerList[pair.first + 1]->Bumii->Initialize(Instances::PlayerList[pair.first + 1]->Model.Bumii);
            }
        }

        std::vector<int> ConnectedPlayers = {};

        for (auto const& player : serverResponse->ClosePlayers)
        {
            if (player->PlayerNumber + 1 == 32 && Main::HidePlayer32)
                continue;

            Instances::PlayerList[player->PlayerNumber + 1]->set(player, serverResponse->NetworkData->DisplayNames, Game::GameInstance->IsGamePaused);
            ConnectedPlayers.push_back(player->PlayerNumber + 1);

            if (serverResponse->PropHuntData->Phase == 2 && player->Health == 0)
            {
                if (std::find(Main::FoundPlayers.begin(), Main::FoundPlayers.end(), player->PlayerNumber + 1) != Main::FoundPlayers.end())
                {
                    continue;
                }

                Logging::LoggerService::LogInformation(Instances::PlayerList[player->PlayerNumber + 1]->Name + " was found.", __FUNCTION__);
                Memory::MessagerService::AddMessage(Instances::PlayerList[player->PlayerNumber + 1]->Name + " was found.");
                Main::FoundPlayers.push_back(player->PlayerNumber + 1);
            }
        }

        for (auto const& player : serverResponse->FarPlayers)
        {
            Instances::PlayerList[player->PlayerNumber + 1]->set(player, serverResponse->NetworkData->DisplayNames, Game::GameInstance->IsGamePaused);
            ConnectedPlayers.push_back(player->PlayerNumber + 1);
        }

        for (int i = 1; i < 33; i++)
            if (Instances::PlayerList[i]->connected && std::find(ConnectedPlayers.begin(), ConnectedPlayers.end(), i) == ConnectedPlayers.end())
                Instances::PlayerList[i]->Disconnect();

        Game::GameInstance->EnemyService->SetServerData(serverResponse->EnemyData);
        Game::GameInstance->QuestService->SetServerData(serverResponse->QuestData->Completed, Game::GameInstance->IsGamePaused, QuestSyncReady);
    }
}

void Main::Setup()
{
    Logging::LoggerService::LogInformation("Start of Breath of the Wild Multiplayer");

    while (Main::baseAddr == 0)
        Main::baseAddr = Memory::getBaseAddress();

    /* Local instance startup */

    Game::GameInstance = new MemoryAccess::LocalInstance();

    Game::GameInstance->playerNumber = playerNumber;

    Logging::LoggerService::LogInformation("Assigned to player " + std::to_string(playerNumber));

    while (Game::GameInstance->baseAddr == 0)
    {
        Sleep(100);
    }

    try
    {
        Game::GameInstance->scan();
    } catch (LPCWSTR ex)
    {
        MessageBoxW(NULL, ex, L"", MB_OK);
        Main::disconnectFromServer("Incorrect mod installation. Validate graphic packs and BCML.");
    }

    Game::GameInstance->setActorSpawning(&queue_mutex, queueActor, queueActor);

    /* Services startup */
    if (isQuestSync)
        CreateThread(0, 0, (LPTHREAD_START_ROUTINE)Main::QuestSync, 0, 0, 0); // Going to main to avoid creating the thread over here as it is not static
    else
        QuestSyncReady = true;

    std::map<int, MemoryAccess::LocalInstance::FlagAddresses> Flags;

    try
    {
        Flags = Game::GameInstance->scanPlayerFlags();
    } catch (LPCWSTR ex)
    {
        MessageBoxW(NULL, ex, L"", MB_OK);
        Main::disconnectFromServer("Incorrect mod installation. Validate graphic packs and BCML.");
    }

    for (int i = 1; i < 33; i++)
        Instances::PlayerList.insert({i, new MemoryAccess::Player(i, Game::GameInstance, Flags[i])});

    Flags.clear();

    Game::GameInstance->QuestService->Startup(isQuestSync, questServerSettings);

    Memory::MultiplayerQuest::changeMQuestSvName(serverName, 0);
    Memory::MultiplayerQuest::findMQuestPingAddress(0);

    CreateThread(0, 0, (LPTHREAD_START_ROUTINE)PauseChecker, 0, 0, 0);
}

void Main::SetupAssemblyPatches()
{
    /* Assembly patch startup */
    memInstance = new MemoryInstance(GetModuleHandleA(NULL));
    init();
    ActorData::InitDefaultValues();
}

void Main::CalculatePVPDamage()
{
    std::vector<int> damageAnimations = {1694655571, 1651482698, -2101720748, 2025157687, 1690308196, 1672333949, -2089278621, 2037849600, 1920499742, 1964327943, -1782534887, 1875873914}; // Animations that we know represent getting hit, staggered, etc.

    if (std::find(damageAnimations.begin(), damageAnimations.end(), Game::GameInstance->Animation->LastKnown) == damageAnimations.end() || float(GetTickCount() - Game::GameInstance->LastHealthUpdate) < 1000)
        return;

    float HitProbability = 0;
    int Attacker = 0;

    const float DISTANCE_LIMIT = 4.5;
    const float LAST_ATTACK_LIMIT = 500;

    for (int i = 1; i < 33; i++) // TODO: Check if it is a good idea to be running it like this or maybe optimize it. This will run everytime the local player looses health
    {
        if (!Instances::PlayerList[i]->connected)
            continue;

        float DistanceFromPlayer = Helper::Vec3f_Operations::GetDistance(Game::GameInstance->Position->LastKnown, Instances::PlayerList[i]->Position->LastKnown);
        float TimeSinceLastAttack = float(GetTickCount() - Instances::PlayerList[i]->LastAttack);

        if (DistanceFromPlayer > DISTANCE_LIMIT || TimeSinceLastAttack > 500)
            continue;

        /* PVP Considerations
        - The highest probability is where you are on Distance = 0 and LastAttack = 0
        - 50 % of the probability depends on the distance and 50 % of the probability depends on the time
        - If your distance is higher than 3.5 or your LastAttack is higher than 500, you are not considered
        */

        float hitProb = (1 - (DistanceFromPlayer / DISTANCE_LIMIT)) * 0.5 + (1 - (TimeSinceLastAttack / LAST_ATTACK_LIMIT)) * 0.5;

        if (hitProb > HitProbability)
        {
            HitProbability = hitProb;
            Attacker = i;
        }
    }

    if (Attacker == 0)
        return;

    float WpnDamage = 0;

    std::string WpnName = "Weapon_Null_";

    switch (Instances::PlayerList[Attacker]->Equipment->LastKnown->WType)
    {
    case 1:
        WpnName.replace(WpnName.find("Null"), 4, "Sword");
        break;
    case 2:
        WpnName.replace(WpnName.find("Null"), 4, "Lsword");
        break;
    case 3:
        WpnName.replace(WpnName.find("Null"), 4, "Spear");
        break;
    }

    std::string WpnNumber = std::to_string(Instances::PlayerList[Attacker]->Equipment->LastKnown->Sword);

    int numberOfZeros = 3 - WpnNumber.size();

    for (int i = 0; i < numberOfZeros; i++)
        WpnNumber = "0" + WpnNumber;

    WpnName = WpnName + WpnNumber;

    if (Game::GameInstance->WeaponDamages.find(WpnName) != Game::GameInstance->WeaponDamages.end())
        WpnDamage = Game::GameInstance->WeaponDamages[WpnName];
    else
        Logging::LoggerService::LogWarning(WpnName + " does not exist in database. Assigning damage 0.", __FUNCTION__);

    int defense = Game::GameInstance->Defense->get(__FUNCTION__);

    if (defense > 100)
    {
        defense = 0;
        std::stringstream stream;
        stream << "Incorrect reading for defense, defense set to 0. Value: " << defense;
        Logging::LoggerService::LogWarning(stream.str(), __FUNCTION__);
    }

    std::stringstream stream;
    stream << "Weapon: " << WpnName << " Weapon base damage: " << WpnDamage << " AtkUp: " << Instances::PlayerList[Attacker]->AtkUp << " Local Defense: " << defense;

    float CalcDamage = WpnDamage * Instances::PlayerList[Attacker]->AtkUp - defense;

    WpnDamage = CalcDamage < 1 ? 1 : CalcDamage;

    Logging::LoggerService::LogDebug(stream.str(), __FUNCTION__);
    Logging::LoggerService::LogInformation("Player " + std::to_string(Attacker) + " damaged the local player. Calculated damage: " + std::to_string(WpnDamage), __FUNCTION__);

    int newHealth = Game::GameInstance->Health->LastKnown - WpnDamage;

    if (IsProp)
    {
        newHealth = 0;

        Game::GameInstance->propHuntFlags.ShowPlayer->set(true, __FUNCTION__);
        Game::GameInstance->propHuntFlags.HidePlayer->set(false, __FUNCTION__);

        Instances::PlayerList[32]->Disconnect();

        Main::IsProp = false;
        Main::HidePlayer32 = true;
    }

    Game::GameInstance->Health->set(newHealth < 0 ? 0 : newHealth, __FUNCTION__);

    Game::GameInstance->LastHealthUpdate = GetTickCount();
}

void Main::AddBigMessage(std::string Message)
{
    Message = "Notification|" + Message;

    SendMessageToOverlay(Message);
}

void Main::SendTimerMessage(bool start, std::string countMode, int startTime, int maxTime)
{
    DWORD read;
    std::stringstream Stream;
    Stream << "Timer|" << (start ? "start" : "end") << " " << countMode << " " << startTime << " " << maxTime;

    SendMessageToOverlay(Stream.str());
}

void Main::SendMessageToOverlay(std::string Message)
{
    DWORD read;
    WriteFile(Main::namedPipe->hPipe, Message.c_str(), Message.size(), &read, nullptr);
    Logging::LoggerService::LogInformation("Sent \"" + Message + "\" " + "to client.", __FUNCTION__);
}