using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace BOTWM.Server.DTO
{
    public class NetworkDTO
    {
        public short SerializationRate;
        public short TargetFPS;
        public short SleepMultiplier;
        public bool isLocalTest;
        public bool isCharacterSpawn;
        public bool DisplayNames;
        public short GlyphDistance;
        public short GlyphTime;
        public bool isQuestSync;
        public bool isEnemySync;
        public bool AlwaysDay;
    }
}
