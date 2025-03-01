using BOTWM.Server.HelperTypes;

namespace BOTWM.Server.DTO
{
    public class ServerDTO
    {
        public ServerDTO()
        {
            WorldData = new WorldDTO();
            NameData = new NamesDTO();
            ModelData = new ModelsDTO();
            ClosePlayers = new List<ClosePlayerDTO>();
            FarPlayers = new List<FarPlayerDTO>();
            EnemyData = new EnemyDTO();
            QuestData = new QuestsDTO();
            NetworkData = new NetworkDTO();
            DeathSwapData = new DeathSwapDTO();
            TeleportData = new TeleportDTO();
            PropHuntData = new PropHuntDTO();
        }

        public WorldDTO WorldData;
        public NamesDTO NameData;
        public ModelsDTO ModelData;
        public List<ClosePlayerDTO> ClosePlayers;
        public List<FarPlayerDTO> FarPlayers;
        public EnemyDTO EnemyData;
        public QuestsDTO QuestData;
        public NetworkDTO NetworkData;
        public DeathSwapDTO DeathSwapData;
        public TeleportDTO TeleportData;
        public PropHuntDTO PropHuntData;
    }
}
