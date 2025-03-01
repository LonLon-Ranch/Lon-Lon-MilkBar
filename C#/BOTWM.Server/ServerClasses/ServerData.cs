using BOTW.Logging;
using BOTWM.Server.DataTypes;
using BOTWM.Server.DTO;
using BOTWM.Server.HelperTypes;
using Newtonsoft.Json;

namespace BOTWM.Server.ServerClasses
{
    static public class ServerData
    {
        public struct ServerConfiguration
        {
            public string IP;
            public int PORT;
            public string PASSWORD;
            public string DESCRIPTION;
            public ServerSettings Settings;

            public ServerConfiguration(string ip, int port, string password, string description, ServerSettings settings)
            {
                this.IP = ip;
                this.PORT = port;
                this.PASSWORD = password;
                this.DESCRIPTION = description;
                this.Settings = settings;
            }
        }

        public const int PLAYERLIMIT = 32;

        static Dictionary<string, string> ArmorMappings;
        static bool IsEnemySync;
        static bool IsQuestSync;
        static bool AlwaysDay;

        public static World WorldData;
        public static Names NameData;
        public static Models ModelData;
        public static List<Player> PlayerList;
        public static Enemy EnemyData;
        public static Quests QuestData;
        public static DeathSwapSettings DeathSwap;
        public static Teleport TeleportData;
        public static PropHunt PropHuntData;
        public static bool pvp;
        public static ServerConfiguration Configuration;
        static List<List<bool>> Updated = new List<List<bool>>();
        static List<DeathSwapDTO> DeathSwapQueue = new List<DeathSwapDTO>();

        static Mutex DataMutex = new Mutex();
        public static Mutex DeathSwapMutex = new Mutex();

        static public void Startup(string ip, int port, string password, string description, ServerSettings settings)
        {
            WorldData = new World();
            PlayerList = new List<Player>();

            for (int i = 0; i < PLAYERLIMIT; i++)
            {
                PlayerList.Add(new Player((byte)i));

                Updated.Add(new List<bool>());

                for (int j = 0; j < PLAYERLIMIT; j++)
                    Updated[i].Add(false);

                DeathSwapQueue.Add(new DeathSwapDTO());
            }

            EnemyData = new Enemy(PLAYERLIMIT, settings.EnemySync);
            QuestData = new Quests(PLAYERLIMIT, settings.QuestSyncSettings.AnyTrue);
            NameData = new Names(PLAYERLIMIT);
            ModelData = new Models(PLAYERLIMIT);

            IsEnemySync = settings.EnemySync;
            IsQuestSync = settings.QuestSyncSettings.AnyTrue;
            AlwaysDay = false;

            ArmorMappings = ReadArmorMappingJson();

            Configuration = new ServerConfiguration(ip, port, password, description, settings);
            DeathSwap = new DeathSwapSettings();
            TeleportData = new Teleport(PLAYERLIMIT);
            PropHuntData = new PropHunt();

            pvp = true;
        }

        #region Update

        static public void UpdateWorldData(WorldDTO userData, int playerNumber)
        {
            DataMutex.WaitOne(100);

            WorldData.UpdateTime(userData);

            if (playerNumber != -1 && WorldData.isForcedWeather)
            {
                DataMutex.ReleaseMutex();
                return;
            }

            for (int i = playerNumber - 1; i >= 0; i--)
            {
                if (PlayerList[i].Connected)
                {
                    DataMutex.ReleaseMutex();
                    return;
                }
            }

            WorldData.UpdateWeather(userData);

            DataMutex.ReleaseMutex();
        }

        static public void UpdatePlayerData(ClientPlayerDTO userData, int playerNumber)
        {

            userData.Equipment = ProcessArmors(userData.Equipment);

            //TODO: Implement animation mapping

            DataMutex.WaitOne(100);
            PlayerList[playerNumber].Update(userData);

            foreach(List<bool> UpdatedList in Updated)
                UpdatedList[playerNumber] = true;

            if(playerNumber == 0)
            {
                if (Configuration.Settings.GameMode == Gamemode.DeathSwap && playerNumber == 0)
                {
                    DeathSwapMutex.WaitOne(100);

                    if (DeathSwap.Enabled && PlayerList[1].Connected)
                    {
                        byte NewDeathSwapPhase = DeathSwap.GetSwapPhase();

                        for (int i = 0; i < PLAYERLIMIT; i++)
                            if (DeathSwapQueue[i].Phase != 2)
                                DeathSwapQueue[i].Phase = NewDeathSwapPhase;

                        if (NewDeathSwapPhase == 2)
                        {
                            DeathSwapQueue[0].Position = new Vec3f(PlayerList[1].Position.ToList());

                            DeathSwapQueue[1].Position = new Vec3f(PlayerList[0].Position.ToList());
                        }
                    }
                    else
                    {
                        DeathSwap.Running = false;
                        DeathSwap.RestartTimer();
                    }

                    DeathSwapMutex.ReleaseMutex();
                }
            }

            DataMutex.ReleaseMutex();
        }

        static public void UpdateEnemyData(EnemyDTO userData)
        {
            DataMutex.WaitOne(100);
            EnemyData.Update(userData);
            DataMutex.ReleaseMutex();
        }

        static public void UpdateQuestData(QuestsDTO userData)
        {
            DataMutex.WaitOne(100);
            QuestData.Update(userData);
            DataMutex.ReleaseMutex();
        }

        static public void SetConnection(int playerNumber, bool status)
        {
            DataMutex.WaitOne(100);
            if (status)
                PlayerList[playerNumber].Connected = true;
            else
            {
                PlayerList[playerNumber] = new Player((byte)playerNumber);
                NameData.RemoveName((byte)playerNumber);
                ModelData.RemoveModel((byte)playerNumber);
                PropHuntData.Players.Remove((byte)playerNumber);
                PropHuntData.UpdateStatus();
            }
            DataMutex.ReleaseMutex();
        }

        static public void ProcessExternalQuests(List<string> Quests)
        {
            DataMutex.WaitOne(100);
            QuestData.ProcessQuests(Quests);
            DataMutex.ReleaseMutex();
        }

        #endregion

        #region Get data

        public static ServerDTO GetData(int playerNumber)
        {
            ServerDTO serverInformation = new ServerDTO();

            DataMutex.WaitOne(100);

            serverInformation.WorldData.Time = WorldData.Time;
            serverInformation.WorldData.Day = WorldData.Day;
            serverInformation.WorldData.Weather = WorldData.Weather;
            serverInformation.NameData.Names = NameData.GetQueue((byte)playerNumber);
            serverInformation.ModelData.Models = ModelData.GetQueue((byte)playerNumber);

            foreach (Player player in PlayerList)
            {
                if (!player.Connected)
                    continue;

                if (player.PlayerNumber == playerNumber && (PropHuntData.CurrentPhase == 0 || PropHuntData.Players[(byte)playerNumber].Hunter))
                    continue;

                if (player.Position.GetDistance(PlayerList[playerNumber].Position) >= 100)
                {
                    FarPlayerDTO playerDataToAdd = new FarPlayerDTO();

                    playerDataToAdd.Map(player);

                    playerDataToAdd.Updated = Updated[playerNumber][player.PlayerNumber];
                    Updated[playerNumber][player.PlayerNumber] = false;
                    serverInformation.FarPlayers.Add(playerDataToAdd);
                }
                else
                {
                    ClosePlayerDTO playerDataToAdd = new ClosePlayerDTO();

                    playerDataToAdd.Map(player);

                    playerDataToAdd.Updated = Updated[playerNumber][player.PlayerNumber];
                    Updated[playerNumber][player.PlayerNumber] = false;
                    if (playerDataToAdd.PlayerNumber == playerNumber) playerDataToAdd.PlayerNumber = 31;
                    serverInformation.ClosePlayers.Add(playerDataToAdd);
                }
            }

            serverInformation.EnemyData.Health = EnemyData.GetQueue(playerNumber);
            serverInformation.QuestData.Completed = QuestData.GetPlayerQuests(playerNumber);

            DeathSwapMutex.WaitOne(100);
            serverInformation.DeathSwapData = DeathSwapQueue[playerNumber];
            DeathSwapMutex.ReleaseMutex();

            serverInformation.TeleportData = TeleportData.GetTp(playerNumber);

            if(((byte)PropHuntData.CurrentPhase) > 0)
            {
                PropHuntData.UpdateStatus();
            }

            serverInformation.PropHuntData = PropHuntData.GetData((byte)playerNumber);

            DataMutex.ReleaseMutex();

            return serverInformation;
        }

        public static void ClearDeathSwap(int playerNumber)
        {
            DeathSwapMutex.WaitOne(100);
            DeathSwapQueue[playerNumber].Phase = 0;
            DeathSwapMutex.ReleaseMutex();
        }

        #endregion

        #region HelperMethods

        static public ConnectResponseDTO TryAssigning(ConnectDTO UserConfiguration)
        {
            if (Configuration.PASSWORD != "" && Configuration.PASSWORD != UserConfiguration.Password)
                return new ConnectResponseDTO() { Response = 3 };

            DataMutex.WaitOne(100);

            // Having a name as nothing is a tad annoying
            if (string.IsNullOrWhiteSpace(UserConfiguration.Name) || string.IsNullOrEmpty(UserConfiguration.Name))
            {
                return new ConnectResponseDTO() { Response = 4 };
            }

            // Next we find out if the name is already registed
            foreach (Player player in PlayerList)
            {
                if (player.Name == UserConfiguration.Name)
                {
                    // GET OUT
                    DataMutex.ReleaseMutex(); // the server won't crash if others disconnect
                    return new ConnectResponseDTO() { Response = 5 };
                }
            }
            // Yay not taken

            int counter = 0;
            int playerNumber = -1;
            foreach(Player player in PlayerList)
            {
                if (string.IsNullOrEmpty(player.Name))
                {
                    playerNumber = counter;
                    player.Name = UserConfiguration.Name;
                    player.PlayerNumber = (byte)playerNumber;
                    player.ModelType = UserConfiguration.ModelData.ModelType;
                    player.Model = UserConfiguration.ModelData.Model;
                    player.MiiData = UserConfiguration.ModelData.Mii;
                    break;
                }
                counter++;  
            }

            DataMutex.ReleaseMutex();

            if (playerNumber == -1)
                return new ConnectResponseDTO() { Response = 2 };

            NameData.AddName((byte)playerNumber, UserConfiguration.Name);
            ModelData.AddModel((byte)playerNumber, UserConfiguration.ModelData);

            EnemyData.FillQueue(playerNumber);
            QuestData.FillQueue(playerNumber);
            NameData.FillQueue(playerNumber);
            ModelData.FillQueue(playerNumber);

            return new ConnectResponseDTO()
            {
                Response = 1,
                PlayerNumber = playerNumber,
                Settings = Configuration.Settings,
                QuestSync = IsQuestSync,
            };

        }

        static public Player GetPlayer(int playerNumber)
        {
            DataMutex.WaitOne(100);
            Player result = PlayerList[playerNumber];
            DataMutex.ReleaseMutex();

            return result;
        }

        static public NamesDTO GetPlayers()
        {
            return NameData.GetAllPlayers();
        }

        #endregion

        #region Private methods

        static private CharacterEquipment ProcessArmors(CharacterEquipment EquipmentData)
        {

            string HeadString = AddZeros(EquipmentData.Head.ToString(), 3);
            string UpperString = AddZeros(EquipmentData.Upper.ToString(), 3);
            string LowerString = AddZeros(EquipmentData.Lower.ToString(), 3);

            if (ArmorMappings.ContainsKey(HeadString))
                EquipmentData.Head = Int16.Parse(ArmorMappings[HeadString]);

            if (ArmorMappings.ContainsKey(UpperString))
                EquipmentData.Upper = Int16.Parse(ArmorMappings[UpperString]);

            if (ArmorMappings.ContainsKey(LowerString))
                EquipmentData.Lower = Int16.Parse(ArmorMappings[LowerString]);

            return EquipmentData;
        }

        static private string AddZeros(string original, int numberOfZeros)
        {
            int zeroesToAdd = numberOfZeros - original.Length;

            for (int i = 0; i < zeroesToAdd; i++)
            {
                original = "0" + original;
            }

            return original;
        }

        static private Dictionary<string, string> ReadArmorMappingJson()
        {
            string AppdataFolder = Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData) + "\\BOTWM";
            string ArmorMappingJson = File.ReadAllText(AppdataFolder + "\\BOTWM.DedicatedServer.AppdataFiles.ArmorMapping.txt");

            return JsonConvert.DeserializeObject<Dictionary<string, string>>(ArmorMappingJson);
        }

        #endregion
    }
}