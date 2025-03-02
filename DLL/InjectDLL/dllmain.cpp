#include <cstddef>
#define _WINSOCKAPI_
#include "Connectivity.h"
#include "LoggerService.h"
#include "Memory.h"
#include "dllmain_Functions.h"
#include "dllmain_Variables.h"
#include <Windows.h>
#include <iostream>
#include <string>
#include <vector>

using namespace Main;

////////////////// Players definitions //////////////////

Memory::Link_class *Main::Link = new Memory::Link_class();
Memory::World_class *Main::World = new Memory::World_class();

Memory::OtherPlayer_class *Main::Jugador1 = new Memory::OtherPlayer_class(1);
Memory::OtherPlayer_class *Main::Jugador2 = new Memory::OtherPlayer_class(2);
Memory::OtherPlayer_class *Main::Jugador3 = new Memory::OtherPlayer_class(3);
Memory::OtherPlayer_class *Main::Jugador4 = new Memory::OtherPlayer_class(4);

Memory::OtherPlayer_class *Main::Jugadores[] = {Jugador1, Jugador2, Jugador3,
                                                Jugador4};

Memory::BombSyncer *Main::BombSync = new Memory::BombSyncer();

////////////////// Parameter initialization //////////////////

uint64_t Main::baseAddr = Memory::getBaseAddress();

std::vector<std::vector<float>> Main::Jugador1Queue = {{}, {}, {}};
std::vector<std::vector<float>> Main::Jugador2Queue = {{}, {}, {}};
std::vector<std::vector<float>> Main::Jugador3Queue = {{}, {}, {}};
std::vector<std::vector<float>> Main::Jugador4Queue = {{}, {}, {}};
std::vector<std::vector<float>> Main::JugadoresQueues[] = {
    Jugador1Queue, Jugador2Queue, Jugador3Queue, Jugador4Queue};

Connectivity::namedPipeClass *Main::namedPipe =
    new Connectivity::namedPipeClass();
Connectivity::Client *Main::client = new Connectivity::Client();

std::shared_mutex WeaponChangeMutex;
std::shared_mutex BombExplodeMutex;

float Main::targetFPS = 1000;
float Main::serializationRate = 20;
float Main::ping = 0;

int Main::playerNumber = 0;
bool Main::IsProp = false;
bool Main::IsPropHuntStopped = true;
bool Main::HidePlayer32 = true;
std::vector<byte> Main::FoundPlayers = {};
std::string Main::serverName = "";
std::vector<bool> Main::ConnectedPlayers = {false, false, false, false};

DWORD Main::t0 = GetTickCount();
DWORD Main::t1 = GetTickCount();

std::string Main::serverData = "";

bool Main::isEnemySync = false;
bool Main::pvp = true;
bool Main::AlwaysDay = false;
bool Main::isGlyphSync = false;
bool Main::isQuestSync = false;
bool Main::isHvsSR = false;
bool Main::isDeathSwap = false;

int Main::GlyphUpdateTime = 60;
int Main::GlyphDistance = 250;

std::vector<std::string> Main::questServerSettings;

bool Main::isPaused = true;

std::vector<float> Main::oldLocations[] = {
    {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}};

HMODULE myhModule;

bool Main::QuestSyncReady = false;

DWORD __stdcall EjectThread(LPVOID lpParameter) {
  Sleep(100);
  FreeLibraryAndExitThread(myhModule, 0);
}

bool startServerLoop() {

  FILE *fp;
  freopen_s(&fp, "CONOUT$", "w", stdout); // output only
  std::cout << "Start of the console" << std::endl;

  CreateThread(0, 0, (LPTHREAD_START_ROUTINE)Main::mainServerLoop, 0, 0, 0);
  return true;
}

void readInstruction() {

  bool success = false;
  DWORD read;
  bool started = false;

  while (!started) {
    TCHAR chBuff[BUFF_SIZE];
    const int responsePositiveActualLength = 11;
    TCHAR responsePositive[BUFF_SIZE] = "Succeeded:;";
    const int responseNegativeActualLength = 8;
    TCHAR responseNegative[BUFF_SIZE] = "Failed:;";
    bool response = false;

    do {
      success = ReadFile(namedPipe->hPipe, chBuff, BUFF_SIZE * sizeof(TCHAR),
                         &read, nullptr);

      if (strstr(chBuff, "!connect")) {
        response = Main::connectToServer(chBuff);
        memset(chBuff, 0, sizeof(chBuff));

        Logging::LoggerService::LogInformation(
            "Connected to server successfully");
      } else if (strstr(chBuff, "!startServerLoop")) {
        Logging::LoggerService::LogInformation(
            "Start server loop requested...");

        if (!started) {
          response = true;
          started = true;
        } else {
          exit(1);
        }
      }

      if (response) {
        // we might as well send over then data for success even though we don't
        // need it *yet*
        for (size_t i = 0; i < sizeof(serverData) / sizeof(TCHAR); ++i) {
          responsePositive[i + responsePositiveActualLength] = serverData[i];
        }
        success = WriteFile(namedPipe->hPipe, responsePositive,
                            BUFF_SIZE * sizeof(TCHAR), &read, nullptr);
      } else {
        for (size_t i = 0; i < sizeof(serverData) / sizeof(TCHAR); ++i) {
          responseNegative[i + responseNegativeActualLength] = serverData[i];
        }
        success = WriteFile(namedPipe->hPipe, responseNegative,
                            BUFF_SIZE * sizeof(TCHAR), &read, nullptr);
      }
    } while (!success);
  }

  startServerLoop();
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call,
                      LPVOID lpReserved) {
  switch (ul_reason_for_call) {
  case DLL_PROCESS_ATTACH:
    // AllocConsole();
    Main::SetupAssemblyPatches();
    Logging::LoggerService::StartLoggerService();
    namedPipe->createServer();
    CreateThread(0, 0, (LPTHREAD_START_ROUTINE)readInstruction, 0, 0, 0);
  case DLL_THREAD_ATTACH:
  case DLL_THREAD_DETACH:
  case DLL_PROCESS_DETACH:
    break;
  }
  return TRUE;
}