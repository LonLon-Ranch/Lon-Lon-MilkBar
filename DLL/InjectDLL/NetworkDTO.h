#pragma once

namespace DTO
{

	class NetworkDTO
	{
	public:
		short SerializationRate;
		short TargetFPS;
		short SleepMultiplier;
		bool isLocalTest;
		bool isCharacterSpawn;
		bool DisplayNames;
		short GlyphDistance;
		short GlyphTime;
		bool isQuestSync;
		bool isEnemySync;
        bool AlwaysDay;
        bool pvp;
	};

}