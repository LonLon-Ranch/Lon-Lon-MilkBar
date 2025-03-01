#pragma once

#include "Actor.h"

using namespace DataTypes;

namespace MemoryAccess
{
	class WorldObj
	{
	public:
		BigEndian<int>* Day;
		BigEndian<float>* WritableTime;
		BigEndian<float>* ReadableTime;
		int Weather = 0;
		int LocalWeather = 0;

		void set(DTO::WorldDTO* input)
		{
			Mutex.lock();

			int currentDay = this->Day->get(__FUNCTION__);
			float currentTime = this->ReadableTime->get(__FUNCTION__);

			if (currentTime < 350 && (input->Day > currentDay || currentDay - input->Day > 1 || (currentDay == input->Day && input->Time - currentTime > 2)))
			{
				this->Day->set(input->Day, __FUNCTION__);
				this->WritableTime->set(input->Time, __FUNCTION__);
				Sleep(100);
			}

			Weather = input->Weather;
			Mutex.unlock();
		}

		void setAlwaysDay(float time)
                {
                    Mutex.lock();

                    int currentDay = this->Day->get(__FUNCTION__);
                    float currentTime = this->ReadableTime->get(__FUNCTION__);

                    this->WritableTime->set(time, __FUNCTION__);
                    Sleep(100);

                    Mutex.unlock();
                }

		DTO::WorldDTO* get()
		{
			Mutex.lock();
			DTO::WorldDTO* result = new DTO::WorldDTO();

			result->Day = this->Day->get(__FUNCTION__);

			result->Time = this->ReadableTime->get(__FUNCTION__);

			result->Weather = this->LocalWeather;
			Mutex.unlock();

			return result;
		}
		
	private:
		std::shared_mutex Mutex;
	};

	class LocalInstance : public Actor
	{
	public:
		/* Flags object */
		struct FlagAddresses {
			uint64_t DispNameFlag;
			uint64_t ExistsFlag;
			uint64_t StatusFlag;
			uint64_t NameFlag;
			uint64_t AnimationFlag;
			uint64_t HoldFlag;
			uint64_t MapPin;
			uint64_t AttackAnimation;
		};

		/* Global flags - Flags used in events like GameRomCamera */
		struct PropHunt_Flags {
			LittleEndian<bool>* HidingPhase;
			LittleEndian<bool>* RunningPhase;
			LittleEndian<bool>* Countdown;
			LittleEndian<bool>* HidePlayer;
			LittleEndian<bool>* ShowPlayer;
			LittleEndian<bool>* StopMusic;
		};

		BigEndian<int>* Health;
		BigEndian<int>* Defense;
		BigEndian<float>* AtkUp;
		LittleEndian<bool>* IsEquipped;
		LocationAccess* Location;
		EquippedItems* Equipment;
		BigEndian<int>* Animation;
		BombAccess* Bomb = new BombAccess();
		BombAccess* Bomb2 = new BombAccess();
		BombAccess* BombCube = new BombAccess();
		BombAccess* BombCube2 = new BombAccess();
		EnemyAccess* EnemyService = new EnemyAccess();
		QuestAccess* QuestService = new QuestAccess();

		std::shared_mutex PauseMutex;

		WorldObj* World = new WorldObj();

		LittleEndian<bool>* NotPaused;

		DWORD LastQuestUpdate = 0;

		int playerNumber;
		PropHunt_Flags propHuntFlags = PropHunt_Flags();
		
		/* PVP */
		DWORD LastHealthUpdate = GetTickCount();
		std::map<std::string, int> WeaponDamages = readWeaponDamages();

		/* Actor Spawning */
		std::shared_mutex* queue_mutex;
		void (*queuePlayer)(int, float*);
		void (*queueActor)(std::string, float*);
		std::vector<std::tuple<int, Vec3f>> PlayerQueue = {};

		void setActorSpawning(std::shared_mutex* mutex, void (*method)(int, float*), void (*queueActorMethod)(std::string, float*))
		{
			queue_mutex = mutex;
			queuePlayer = method;
			queueActor = queueActorMethod;
		}

		void ChangeActorModel(std::string actorName, uint64_t actorAddr)
		{
			uint64_t addr = Memory::ReadPointers(actorAddr, { 0x39C, 0x6C });
			
			addr = Memory::ReadPointers(addr, { 0x4B0, 0x38 });
			uint64_t face = Memory::ReadPointers(addr, { 0x0, 0xC }, true);
			uint64_t folder = 0;

			int i = 0;
			std::string readString = "";

			while (readString != "Armor" && i < 50)
			{
				i++;
				readString = Memory::read_string(face - i, 5, "ChangeActorModel");
			}

			if (readString == "Armor")
			{
				folder = face - i;
			}

			std::stringstream stream;
			stream << actorName << ": " << std::hex << actorAddr << " -> Folder: " << std::hex << folder << " Model: " << std::hex << face;
			Logging::LoggerService::LogInformation(stream.str());

			Memory::write_string(folder, "Armor_002", 10, "Rewrite armor");
			Memory::write_string(face, "Armor_002", 10, "Rewrite armor");
		}

		bool IsGamePaused = false;

		void RequestCreate(int playerNumber, Vec3f pos = Vec3f(0, 0, 0))
		{
			if (IsPaused())
			{
				Logging::LoggerService::LogWarning("Create request failed: Game is paused.");
				return;
			}

			queue_mutex->lock();

			Logging::LoggerService::LogDebug("Requesting creation of player " + std::to_string(playerNumber) + "...", __FUNCTION__);

			if (pos.x() == 0 && pos.y() == 0 && pos.z() == 0)
				pos = Position->get(__FUNCTION__);

			//float SpawnPos[3] = { pos.x(), pos.y(), pos.z() };

			//queuePlayer(playerNumber, SpawnPos);
			PlayerQueue.insert(PlayerQueue.begin(), {playerNumber, pos});

			queue_mutex->unlock();

			Logging::LoggerService::LogDebug("Requested creation of player " + std::to_string(playerNumber), __FUNCTION__);
		}

		void ProcessCreationRequests()
		{
			queue_mutex->lock();

			if (PlayerQueue.size() > 0 && !IsGamePaused)
			{
				int playerToSpawn = std::get<0>(PlayerQueue.back());
				Vec3f posToSpawn = std::get<1>(PlayerQueue.back());

				PlayerQueue.pop_back();

				float SpawnPos[3] = { posToSpawn.x(), posToSpawn.y(), posToSpawn.z() };

				queuePlayer(playerToSpawn, SpawnPos);
			}

			queue_mutex->unlock();
		}

		bool RequestCreate(std::string actor, Vec3f pos = Vec3f(0, 0, 0))
		{
			if (IsPaused())
			{
				Logging::LoggerService::LogWarning("Create request failed: Game is paused.");
				return false;
			}

			queue_mutex->lock();

			if (pos.x() == 0 && pos.y() == 0 && pos.z() == 0)
				pos = Position->get(__FUNCTION__);

			float SpawnPos[3] = { pos.x(), pos.y(), pos.z() };

			std::stringstream stream;
			stream << "Requestion creation of " << actor << " at x: " << SpawnPos[0] << " y: " << SpawnPos[1] << " z: " << SpawnPos[2];

			Logging::LoggerService::LogDebug(stream.str(), __FUNCTION__);

			queueActor(actor, SpawnPos);

			queue_mutex->unlock();

			Logging::LoggerService::LogDebug("Requested creation of actor " + actor, __FUNCTION__);

			return true;
		}

		void setAddress(uint64_t addr)
		{
			Actor::setAddress(addr);
		}

		void scan()
		{
			Animation = new BigEndian<int>(this->baseAddr, { 0x448, 0x4C, 0x10, 0x19C, 0x38, 0xA0, 0x10, -0x5C }, 0x10, "Memory::LocalInstance::setAddress::Animation");
			//Animation = new BigEndian<int>({ 0x8A, 0xB9, 0x46, 0x6F, -1, 0x00, 0x00 }, 0x08, "Memory::LocalInstance::scan::Animation", 0);

			Logging::LoggerService::LogInformation("Scanning game instance...", __FUNCTION__);

			Location = new LocationAccess();
			Equipment = new EquippedItems();
			World->Day = new BigEndian<int>({ 0x7B, 0x4B, 0xE8, 0x51, 0x00, 0x00, 0x00, 0x00, 0x10, 0x29, 0x84, 0xC8 }, -0x04, "Memory::LocalInstance::scan::World->Day");
			//Health = new BigEndian<int>({ 0x6E, -1, -1, 0x38, 0x00, 0x00, 0x00, -1, 0x6E, -1, -1, -1, 0x6E, -1, -1, 0x58, -1, -1, -1, -1, 0x00, 0x00, 0x00, -1, 0x6E, -1, -1, -1, 0x6E, 0xD1 }, 0x04, __FUNCTION__);
			AtkUp = new BigEndian<float>({ 0xBF, 0x80, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, -1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, -1, 0x80, 0x00, 0x00 }, 0x10C, "Memory::LocalInstance::scan::AtkUp");
			//IsEquipped = new LittleEndian<bool>({ 0x6E, -1, -1, 0x68, 0x6E, -1, -1, 0xB4, 0x6E, -1, -1, 0x00, 0x6E }, 0x40, "Memory::LocalInstance::scan::IsEquipped");
			IsEquipped = new LittleEndian<bool>(this->baseAddr, { 0x394, 0xAC, 0xA4 }, 0x0, "Memory::LocalInstance::scan::IsEquipped");
			//Defense = new BigEndian<int>({ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, -1, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, -1, 0x3F, 0x80, 0x00, 0x00 }, 0x20, __FUNCTION__);
			Defense = new BigEndian<int>(this->baseAddr, { 0x3b4, 0x0, 0x20, 0x4, 0x28, 0xd4, 0x158, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0xb0, 0x4, 0x4, 0xac, 0x28, 0x4 }, -0xE54, "Memory::LocalInstance::scan::Defense");
			Health = new BigEndian<int>(Memory::getBaseAddress() + this->baseAddr + 0x540, "Memory::LocalInstance::scan::Health");
			//World->Time = new BigEndian<float>({ 0x10, 0x29, 0x7D, 0x40, -1, 0x2E, 0xE3, 0x38 }, -0x04, __FUNCTION__);
			NotPaused = new LittleEndian<bool>({ 0x93, 0x9B, 0xE2, 0xB9, 0x00, -1, 0x00, 0x00, 0x10, 0x29, 0x84, 0x10 }, -0x1, "Memory::LocalInstance::scan::NotPaused", 0);

			propHuntFlags.Countdown = new LittleEndian<bool>({ 0x30, 0xA9, 0xAC, 0x57, 0x00, -1, 0x00, 0x00, 0x10, 0x29, 0x84, 0x10 }, -0x1, "LocalInstance::scan::PH_Countdown", 0);

			uint64_t newOffset = propHuntFlags.Countdown->address - Memory::RegionStart;

			propHuntFlags.HidePlayer = new LittleEndian<bool>({ 0x05, 0x22, 0xFA, 0x47, 0x00, -1, 0x00, 0x00, 0x10, 0x29, 0x84, 0x10 }, -0x1, "LocalInstance::scan::PH_HideTimer", newOffset);
			propHuntFlags.HidingPhase = new LittleEndian<bool>({ 0xA8, 0xA5, 0x8F, 0x41, 0x00, -1, 0x00, 0x00, 0x10, 0x29, 0x84, 0x10 }, -0x1, "LocalInstance::scan::PH_HidingPhase", newOffset);
			propHuntFlags.RunningPhase = new LittleEndian<bool>({ 0xB6, 0x68, 0x05, 0x99, 0x00, -1, 0x00, 0x00, 0x10, 0x29, 0x84, 0x10 }, -0x1, "LocalInstance::scan::PH_RunningPhase", newOffset);
			propHuntFlags.ShowPlayer = new LittleEndian<bool>({ 0x60, 0x3C, 0x05, 0xAA, 0x00, -1, 0x00, 0x00, 0x10, 0x29, 0x84, 0x10 }, -0x1, "LocalInstance::scan::PH_ShowPhase", newOffset);
			propHuntFlags.StopMusic = new LittleEndian<bool>({ 0xBC, 0x92, 0x90, 0x4B, 0x00, -1, 0x00, 0x00, 0x10, 0x29, 0x84, 0x10 }, -0x1, "LocalInstance::scan::PH_StopMusic", newOffset);

			Logging::LoggerService::LogInformation("Scanned game instance successfully.", __FUNCTION__);
		}

		std::map<int, FlagAddresses> scanPlayerFlags()
		{
			const int BOOL_OFFSET = 0x10;
			const int STR32_OFFSET = 0xC0;
			const int INT_OFFSET = 0x20;
			const int ANIM_OFFSET = 0x220;
			const int EQUIP_OFFSET = 0x98;
			const int EQUIP_EXTRA_OFFSET = 0x8;

			std::map<int, FlagAddresses> result;
			LPCWSTR errorMessage = L"Failed to find mod address. Make sure that the mod is installed correctly in BCML and that BCML, Extended Memory and Multiplayer Utilities graphics packs are enabled.";

			Logging::LoggerService::LogInformation("Scanning game flags...", __FUNCTION__);

			for (int i = 1; i < 33; i++)
				result[i] = FlagAddresses();

			std::vector<int> sig = { 0x82, 0xCB, 0xCC, 0x60, 0x00, -1, 0x00, 0x00, 0x10, 0x29, 0x84, 0x10 }; // Jugador10_DispNameFlag
			std::vector<int> FlagOrder1 = { 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 1, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 2, 30, 31, 32, 3, 4, 5, 6, 7, 8, 9 };
			std::vector<int> FlagOrder2 = { 1, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 2, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 3, 30, 31, 32, 4, 5, 6, 7, 8, 9 };

			uint64_t addr = Memory::PatternScan(sig, Memory::getBaseAddress(), 8, 0) - 0x1;

			for (int i = 0; i < FlagOrder1.size(); i++)
			{
				result[FlagOrder1[i]].DispNameFlag = addr;

				addr += BOOL_OFFSET;

				result[FlagOrder1[i]].ExistsFlag = addr;

				addr += BOOL_OFFSET;
			}

			sig = { 0x87, 0x68, 0xFA, 0x81, 0x00, -1, 0x00, 0x00, 0x10, 0x29, 0x84, 0xC8 }; // Jugador10_Status
			
			addr = Memory::PatternScan(sig, Memory::getBaseAddress(), 8, 0) - 0x4;

			for (int i = 0; i < FlagOrder1.size(); i++)
			{
				result[FlagOrder1[i]].StatusFlag = addr;

				addr += INT_OFFSET;
			}

			sig = { 0xF2, 0xA5, 0xD4, 0xA2, 0x00, -1, 0x00, 0x00, 0x10, 0x29, 0x86, 0x38 }; // Jugador10_Name

			addr = Memory::PatternScan(sig, Memory::getBaseAddress(), 8, 0) - 0x20;

			for (int i = 0; i < FlagOrder1.size(); i++)
			{
				result[FlagOrder1[i]].NameFlag = addr;

				addr += STR32_OFFSET;
			}

			sig = { 0x0D, 0x4A, 0x75, 0x67, 0x61, 0x64, 0x6F, -1, 0x31, 0x5F, 0x48, 0x6F, 0x6C, 0x64 }; //Jugador1_Hold

			addr = Memory::PatternScan(sig, Memory::getBaseAddress(), 8) + 0x1;
			//uint64_t addr_copy = addr;

			int retries = 0;

			while (addr < 30000)
			{
				Logging::LoggerService::LogDebug("Could not find hold address.", __FUNCTION__);
				Sleep(1000);

				if (retries == 15)
				{
					Logging::LoggerService::LogError("Failed to find mod address. Make sure that the mod is installed correctly in BCML.");
					throw errorMessage;
				}

				addr = Memory::PatternScan(sig, Memory::getBaseAddress(), 8) + 0x1;
				retries++;
			}

			for (int i = 1; i < 33; i++)
			{
				result[i].HoldFlag = addr;

				addr += EQUIP_OFFSET;

				if (i > 9)
					addr += EQUIP_EXTRA_OFFSET;
			}

			addr += 0x1000;
			std::vector<BYTE> animthing = { 0x4A, 0x75, 0x67, 0x61, 0x64, 0x6F, 0x72, 0x31 };
			uint64_t offset = 0;

			while (offset < 5000)
			{
				if (Memory::read_bytes(addr + offset, 8, __FUNCTION__) == animthing)
				{
					addr = addr + offset;
					break;
				}

				offset++;
			}

			if (offset == 5000)
				addr = 0;

			if (addr < 30000)
			{
				Logging::LoggerService::LogError("Could not find animation address.", __FUNCTION__);
				throw errorMessage;
			}

			for (int i = 1; i < 34; i++)
			{
				std::stringstream stream;

				if (Memory::read_string(addr + 8, 1, __FUNCTION__) == "_")
					result[std::stoi(Memory::read_string(addr + 7, 1, __FUNCTION__))].AnimationFlag = addr;
				else
					result[std::stoi(Memory::read_string(addr + 7, 2, __FUNCTION__))].AnimationFlag = addr;
				
				uint64_t offset = 25;
				std::vector<BYTE> jugador = { 0x4A, 0x75, 0x67, 0x61, 0x64, 0x6F, 0x72 };

				if (i == 33)
					break;

				while (true)
				{
					if(Memory::read_string(addr + offset, 14, __FUNCTION__) == "animationthing")
					{
						if (Memory::read_string(addr + offset - 3, 1, __FUNCTION__) == "r")
							addr = addr + offset - 9;
						else
							addr = addr + offset - 10;

						break;
					}

					offset++;
				}
			}

			//addr += 0x4000;
			offset = 0;

			std::vector<BYTE> attack = { 0x5F, 0x41, 0x74, 0x74, 0x61, 0x63, 0x6B, 0x41, 0x6E, 0x69, 0x6D, 0x61, 0x74, 0x69, 0x6F, 0x6E };

			while (offset < 0x10000)
			{
				if (Memory::read_bytes(addr + offset, attack.size(), __FUNCTION__) == attack)
				{
					addr = addr + offset;

					break;
				}

				offset++;
			}

			if (offset == 0x10000)
			{
				Logging::LoggerService::LogError("Could not find attack animation address.", __FUNCTION__);
				throw errorMessage;
			}

			for (int i = 1; i < 33; i++)
			{
				if (Memory::read_bytes(addr - 0x9, 1, __FUNCTION__)[0] == 0x18) // Single digit
				{
					int playerNumber = std::stoi(Memory::read_string(addr - 0x1, 1, __FUNCTION__));
					result[playerNumber].AttackAnimation = addr - 0x8;
					addr += 29;
				}
				else
				{
					int playerNumber = std::stoi(Memory::read_string(addr - 0x2, 2, __FUNCTION__));
					result[playerNumber].AttackAnimation = addr - 0x9;
					addr += 28;
				}
			}

			sig = { 0x06, 0x46, 0x40, 0xD2, 0x00, 0x45, 0x12, 0x98 };
			addr = Memory::PatternScan(sig, Memory::getBaseAddress(), 8) + 0x1;

			retries = 0;

			while (addr < 30000)
			{
				Logging::LoggerService::LogDebug("Could not find map pin address.", __FUNCTION__);
				Sleep(1000);

				if (retries == 15)
				{
					throw std::exception("Failed to find mod address. Make sure that the mod is installed correctly in BCML.");
				}

				addr = Memory::PatternScan(sig, Memory::getBaseAddress(), 8) + 0x1;
				retries++;
			}

			for (int i = 0; i < 32; i++)
			{
				result[FlagOrder2[i]].MapPin = addr;

				uint64_t offset = 0x40;
				std::vector<BYTE> MapPin = { 0x46, 0x40, 0xD2, 0x00, 0x45, 0x12, 0x98, 0x00 };

				while (true)
				{
					if (Memory::read_bytes(addr + offset, 8, __FUNCTION__) == MapPin)
					{
						addr = addr + offset;
						break;
					}

					offset += 0x4;
				}
			}

			Logging::LoggerService::LogInformation("Scanned game flags successfully.", __FUNCTION__);

			return result;
		}

		DTO::ClientDTO* get(bool ServicesStarted)
		{
			DTO::ClientDTO* result = new DTO::ClientDTO();

			int LastHealth = this->Health->LastKnown;

			result->WorldData = this->World->get();
			result->PlayerData = get_characterData();
			if (ServicesStarted)
			{
				result->EnemyData = EnemyService->UpdateHealth(); //TODO: Implement enemy and quest sync

				if (float(LastQuestUpdate - GetTickCount()) > 5000)
				{
					result->QuestData = QuestService->UpdateQuests();
					LastQuestUpdate = GetTickCount();
				}
				else
				{
					result->QuestData = new DTO::QuestDTO();
				}

			}
			else
			{
				result->EnemyData = new DTO::EnemyDTO();
				result->QuestData = new DTO::QuestDTO();
			}

			if(this->Health->LastKnown != LastHealth)
				LastHealthUpdate = GetTickCount();

			return result;
		}

		bool IsPaused()
		{
			PauseMutex.lock();
			bool res = this->IsGamePaused;
			PauseMutex.unlock();

			return res;
		}

	private:
		DTO::ClientCharacterDTO* get_characterData()
		{
			DTO::ClientCharacterDTO* result = new DTO::ClientCharacterDTO();

			result->Position = this->Position->get(__FUNCTION__);
			result->Rotation1 = this->Rotation1->get(__FUNCTION__);
			result->Rotation2 = this->Rotation2->get(__FUNCTION__);
			result->Rotation3 = this->Rotation3->get(__FUNCTION__);
			result->Rotation4 = this->Rotation4->get(__FUNCTION__);
			result->Animation = this->Animation->get(__FUNCTION__);
			result->Health = this->Health->get(__FUNCTION__);
			result->AtkUp = this->AtkUp->get(__FUNCTION__);
			result->IsEquipped = this->IsEquipped->get(__FUNCTION__);
			result->Equipment = this->Equipment->getEquipment(__FUNCTION__);
			result->Location = this->Location->get(__FUNCTION__);
			result->Bomb = this->Bomb->get(__FUNCTION__);
			result->Bomb2 = this->Bomb2->get(__FUNCTION__);
			result->BombCube = this->BombCube->get(__FUNCTION__);
			result->BombCube2 = this->BombCube2->get(__FUNCTION__);

			return result;
		}

		std::map<std::string, int> readWeaponDamages()
		{
			char* appdata = nullptr;
			size_t sz = 0;

			_dupenv_s(&appdata, &sz, "APPDATA");

			std::string str(appdata);

			std::string filepath = "\\BOTWM\\WeaponDamages.txt";

			std::ifstream file(appdata + filepath);

			std::stringstream buffer;
			buffer << file.rdbuf();

			rapidjson::Document doc;
			doc.Parse(buffer.str().c_str());

			rapidjson::StringBuffer buf;
			rapidjson::Writer<rapidjson::StringBuffer> writer(buf);

			doc.Accept(writer);

			std::map<std::string, int> result = {};

			for (rapidjson::Value::ConstMemberIterator iter = doc.MemberBegin(); iter != doc.MemberEnd(); iter++)
			{
				std::string WeaponName = iter->name.GetString();
				result.insert({ WeaponName, iter->value.GetInt()});
			}

			return result;
		}
	};
}