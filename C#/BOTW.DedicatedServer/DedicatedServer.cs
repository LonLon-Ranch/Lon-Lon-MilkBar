using BOTWM.Server;
using System.Reflection;
using Newtonsoft.Json;
using BOTWM.Server.ServerClasses;
using BOTWM.Server.DTO;
using BOTWM.Server.HelperTypes;
using BOTWM.Server.DataTypes;
using BOTW.Logging;
using Newtonsoft.Json.Linq;
using System.Net.NetworkInformation;

namespace BOTW.DedicatedServer
{

    public class ServerCommand : Attribute
    {
        public bool Debug;

        public ServerCommand(bool debug = false)
        {
            Debug = debug;
        }
    }

    [AttributeUsage(AttributeTargets.Method, AllowMultiple = true)]
    public class AlternateName : Attribute
    {
        public string name;

        public AlternateName(string alternateName)
        {
            this.name = alternateName;
        }
    }

    public class Description : Attribute
    {
        public string description;

        public Description(string description)
        {
            this.description = description;
        }
    }

    [AttributeUsage(AttributeTargets.Method, AllowMultiple = true)]
    public class ExtraHelp : Attribute
    {
        public string Help;

        public ExtraHelp(string help)
        {
            Help = help;
        }
    }

    public class Command
    {

        public MethodInfo Method;
        public string Name;
        public string Description;
        public List<string> AlternateNames = new List<string>();
        public List<string> LowerAlternateNames = new List<string>();

        public Command(MethodInfo method, string name, string description, List<string> alternateNames)
        {
            Method = method;
            Name = name;
            AlternateNames = alternateNames;
            Description = description;

            foreach(string altName in AlternateNames)
            {
                LowerAlternateNames.Add(altName.ToLower());
            }
        }
    }

    public class DedicatedServer
    {

        Server server = new Server();
        List<Command> CommandList = new List<Command>();
        ConsoleColor commandColors = ConsoleColor.Cyan;
        Dictionary<string, string> serverVariables = new Dictionary<string, string>();
        Dictionary<string, List<string>> QuestData = new Dictionary<string, List<string>>();

        //Dictionary<string, bool[]> Gamemodes = new Dictionary<string, bool[]>();
        List<ServerSettings> Gamemodes = new List<ServerSettings>();

        enum Weathers : int
        {
            bluesky,
            cloudy,
            rain,
            heavyrain,
            snow,
            heavysnow,
            thunderstorm,
            thunderrain,
            blueskyrain
        }

        Dictionary<string, Vec3f> LandmarkPositions = JsonConvert.DeserializeObject<Dictionary<string, Vec3f>>(File.ReadAllText(Directory.GetCurrentDirectory() + "/Landmarks.json"));
        List<ProphuntLocation> ServerProphuntLocations = JsonConvert.DeserializeObject<List<ProphuntLocation>>(File.ReadAllText(Directory.GetCurrentDirectory() + "/PropHuntLocations.json"));

        public class ServerConnection
        {
            public string IP { get; set; }
            public int Port { get; set; }
            public string Password { get; set; }
        }

        public class ServerInformation
        {
            public string Description { get; set; }
        }

        private (string ipAddress, int port, string password) GetServerConfiguration()
        {
            // Get active network adapters
            var interfaces = NetworkInterface.GetAllNetworkInterfaces()
                .Where(ni => ni.OperationalStatus == OperationalStatus.Up)
                .ToArray();

            if (interfaces.Length == 0)
            {
                throw new Exception("No active network adapters found.");
            }

            Logger.LogInformation("---Available Network Adapters---", color: ConsoleColor.Cyan);

            for (int i = 0; i < interfaces.Length; i++)
            {
                Logger.LogInformation($"({i}) {interfaces[i].Name} - {interfaces[i].Description}");
            }

            int selection = -1;
            while (selection == -1)
            {
                string input = Logger.LogInput($"Select an adapter (0 to {interfaces.Length - 1}): ");
                if (int.TryParse(input, out int selected) && selected >= 0 && selected < interfaces.Length)
                {
                    selection = selected;
                }
                else
                {
                    Logger.LogError("Invalid selection. Try again.");
                }
            }

            var selectedInterface = interfaces[selection];
            var ipAddress = selectedInterface.GetIPProperties().UnicastAddresses
                .FirstOrDefault(ip => ip.Address.AddressFamily == System.Net.Sockets.AddressFamily.InterNetwork);

            if (ipAddress == null)
            {
                throw new Exception("No IPv4 address found for the selected adapter.");
            }

            Logger.LogInformation($"Selected IP: {ipAddress.Address}");

            // Prompt for port selection
            int port;
            while (true)
            {
                string input = Logger.LogInput("Enter the port for the server (default is 5050): ");
                if (string.IsNullOrEmpty(input))
                {
                    port = 5050;
                    break;
                }
                if (int.TryParse(input, out port) && port > 0 && port <= 65535)
                {
                    break;
                }
                Logger.LogError("Invalid port. Please enter a number between 1 and 65535.");
            }

            // Prompt for password
            Logger.LogInformation("Enter a password for the server or press Enter to leave it empty:");
            string password = Logger.LogInput(string.Empty);

            return (ipAddress.Address.ToString(), port, password);
        }

        public void setup()
        {
            string configFilePath = Path.Combine(Directory.GetCurrentDirectory(), "ServerConfig.ini");

            if (!File.Exists(configFilePath))
            {
                throw new FileNotFoundException($"Configuration file not found: {configFilePath}");
            }

            // Parse INI file
            var configData = ParseIniFile(configFilePath);

            // Gather server configuration
            (string selectedIp, int port, string password) = GetServerConfiguration();

            // Read Server Information
            string description = configData["ServerInformation"]["Description"];

            // Initialize ServerConfig
            ServerConfig svConfig = new ServerConfig
            {
                Connection = new BOTWM.Server.HelperTypes.ServerConfig.ConnectionData
                {
                    IP = selectedIp,
                    Port = port,
                    Password = password
                },
                ServerInformation = new BOTWM.Server.HelperTypes.ServerConfig.ServerInformationData
                {
                    Description = description
                }
            };

            // Log selected configuration
            Logger.LogInformation($"Server will use IP: {selectedIp}", color: ConsoleColor.Cyan);
            Logger.LogInformation($"Server will use Port: {port}", color: ConsoleColor.Cyan);
            Logger.LogInformation($"Password set: {(string.IsNullOrEmpty(password) ? "None" : "****")}", color: ConsoleColor.Cyan);

            // Start the server
            server.serverStart(
                svConfig.Connection.IP,
                svConfig.Connection.Port,
                svConfig.Connection.Password,
                svConfig.ServerInformation.Description,
                GetServerSettings(svConfig)
            );

            server.startListen();

            Logger.LogInformation("Type help to see available commands");
        }

        // Utility method to parse INI file
        private Dictionary<string, Dictionary<string, string>> ParseIniFile(string filePath)
        {
            var result = new Dictionary<string, Dictionary<string, string>>(StringComparer.OrdinalIgnoreCase);
            string[] lines = File.ReadAllLines(filePath);
            string currentSection = null;

            foreach (string line in lines)
            {
                string trimmed = line.Trim();

                if (string.IsNullOrWhiteSpace(trimmed) || trimmed.StartsWith("#") || trimmed.StartsWith(";"))
                {
                    continue;
                }

                if (trimmed.StartsWith("[") && trimmed.EndsWith("]"))
                {
                    currentSection = trimmed.Substring(1, trimmed.Length - 2).Trim();
                    if (!result.ContainsKey(currentSection))
                    {
                        result[currentSection] = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
                    }
                }
                else if (!string.IsNullOrEmpty(currentSection))
                {
                    string[] keyValue = trimmed.Split('=', 2);
                    if (keyValue.Length == 2)
                    {
                        result[currentSection][keyValue[0].Trim()] = keyValue[1].Trim();
                    }
                }
            }

            return result;
        }

        private ServerSettings GetServerSettings(ServerConfig svConfig)
        {
            if (svConfig.Gamemode.DefaultGamemode)
                return new ServerSettings(svConfig.DefaultGamemode.Name,
                                          svConfig.DefaultGamemode.EnemySync,
                                          svConfig.DefaultGamemode.QuestSync,
                                          svConfig.DefaultGamemode.KorokSync,
                                          svConfig.DefaultGamemode.TowerSync,
                                          svConfig.DefaultGamemode.ShrineSync,
                                          svConfig.DefaultGamemode.LocationSync,
                                          svConfig.DefaultGamemode.DungeonSync,
                                          (Gamemode)svConfig.DefaultGamemode.Special);

            bool isGamemode = Logger.LogInput("Are you playing a gamemode? (1 for true, 0 for false): ") == "1" ? true : false;

            if (isGamemode)
            {
                Logger.LogInformation("---Available gamemodes---", color: commandColors);

                int counter = 0;

                foreach (ServerSettings Gamemode in Gamemodes)
                {
                    Logger.LogInformation($"({counter}) {Gamemode.SettingsName}");
                    counter++;
                }

                int optionSelected = -1;

                while (optionSelected == -1)
                {
                    if (!Int32.TryParse(Logger.LogInput("Type the number corresponding to the gamemode you want to play: "), out optionSelected))
                    {
                        Logger.LogError($"Invalid gamemode. Correct values go from 0 to {Gamemodes.Count() - 1}");
                        continue;
                    }
                    else
                    {
                        if (optionSelected > Gamemodes.Count() - 1 || optionSelected < 0)
                        {
                            Logger.LogError($"Invalid gamemode. Correct values go from 0 to {Gamemodes.Count() - 1}");
                            optionSelected = -1;
                            continue;
                        }

                        Logger.LogInformation($"Selected gamemode {Gamemodes[optionSelected].SettingsName}", color: commandColors);

                        return Gamemodes[optionSelected];
                    }
                }
            }

            //V | K | T | O | C | L | D

            bool enemySync = InputToBoolean("Enemy sync (1 for true, 0 for false): ");
            bool questSync = InputToBoolean("Quest sync (1 for true, 0 for false): ");
            bool korokSync = InputToBoolean("Korok sync (1 for true, 0 for false): ");
            bool towerSync = InputToBoolean("Tower sync (1 for true, 0 for false): ");
            bool shrineSync = InputToBoolean("Shrine sync (1 for true, 0 for false): ");
            bool locationSync = InputToBoolean("Location sync (1 for true, 0 for false): ");
            bool dungeonSync = InputToBoolean("Dungeon sync (1 for true, 0 for false): ");
            string GMInput = Logger.LogInput("Gamemode selection (0 for no gamemode, 1 for Hunter vs Speedrunner, 2 for DeathSwap): ");

            Gamemode GM = Gamemode.NoGamemode;

            if (Int32.TryParse(GMInput, out int value))
            {
                if (value == 1)
                    GM = Gamemode.HunterVsSpeedrunner;
                if (value == 2)
                    GM = Gamemode.DeathSwap;
            }

            ServerSettings selectedServerSettings = new ServerSettings("Custom", enemySync, questSync, korokSync, towerSync, shrineSync, locationSync, dungeonSync, GM);

            bool Match = false;

            foreach (ServerSettings gamemode in Gamemodes)
            {
                if (selectedServerSettings.CompareSettings(gamemode))
                {
                    Logger.LogWarning($"Your selected server settings match \"{gamemode.SettingsName}\" gamemode. Next time you want to play with these settings, you can select that gamemode.");
                    Match = true;
                    selectedServerSettings.SettingsName = gamemode.SettingsName;
                    break;
                }
            }

            if (!Match)
            {
                if (Logger.LogInput("Do you wish to save your selected server settings? (1 for yes, 0 for no): ") == "1")
                {
                    selectedServerSettings.SettingsName = Logger.LogInput("Select a name for your settings: ");

                    Gamemodes.Add(selectedServerSettings);

                    string GamemodeJson = JsonConvert.SerializeObject(Gamemodes);

                    File.WriteAllText(Directory.GetCurrentDirectory() + "/Gamemodes.json", GamemodeJson);

                    Logger.LogInformation($"Saved gamemode: {selectedServerSettings.SettingsName}");
                }
                else
                {
                    selectedServerSettings.SettingsName = "Custom";
                }
            }

            return selectedServerSettings;
        }

        private bool InputToBoolean(string message) => Logger.LogInput(message) == "1" ? true : false;

        public void process_commands(string input)
        {
            try
            {
                string input_command = input.Split(" ")[0];
                List<string> unprocessed_input_parameters = input.Split(" ").ToList();
                unprocessed_input_parameters.RemoveAt(0);

                List<string> input_parameters = new List<string>();

                for (int i = 0; i < unprocessed_input_parameters.Count; i++)
                {
                    if (unprocessed_input_parameters[i].First() != '\"' || (unprocessed_input_parameters[i].First() == '\"' && unprocessed_input_parameters[i].Last() == '\"'))
                    {
                        input_parameters.Add(unprocessed_input_parameters[i].Replace("\"", ""));
                        continue;
                    }

                    string new_input = unprocessed_input_parameters[i].Replace("\"", "");

                    for (int j = i + 1; j < unprocessed_input_parameters.Count; j++)
                    {
                        new_input += $" {unprocessed_input_parameters[j]}".Replace("\"", "");

                        if (unprocessed_input_parameters[j].Last() != '\"')
                            continue;

                        i = j;
                        input_parameters.Add(new_input);
                        break;

                    }
                }

                foreach (Command command in CommandList)
                {
                    if (input_command.ToLower() == command.Name.ToLower() || command.LowerAlternateNames.Contains(input_command.ToLower()))
                    {
                        if (input_parameters.Count() > 0 && input_parameters[0].ToLower() == "help")
                        {
                            string param = " ";

                            foreach (var parameter in command.Method.GetParameters())
                            {
                                param += "<" + parameter.Name + "> ";
                            }

                            param = param.Substring(0, param.Length - 1);

                            Logger.LogInformation($"{command.Name}{param}: {command.Description}", color: commandColors);

                            foreach (ExtraHelp extraHelp in command.Method.GetCustomAttributes(typeof(ExtraHelp), false))
                            {
                                Logger.LogInformation($"{extraHelp.Help}");
                            }

                            return;
                        }
                        else
                        {
                            if (input_parameters.Count() >= command.Method.GetParameters().Where(x => !x.IsOptional).Count() && input_parameters.Count() <= command.Method.GetParameters().Count())
                            {
                                List<object> parameters = new List<object>();

                                foreach (string param in input_parameters)
                                {
                                    parameters.Add(param);
                                }

                                for (int i = 0; i < command.Method.GetParameters().Count() - input_parameters.Count(); i++)
                                {
                                    parameters.Add(Type.Missing);
                                }

                                command.Method.Invoke(this, parameters.ToArray());
                                return;
                            }
                            else
                            {

                                string param = " ";

                                foreach (var parameter in command.Method.GetParameters())
                                {
                                    param += parameter.Name + " ";
                                }

                                Logger.LogError($"Correct usage: {command.Name}{param}");
                                return;

                            }
                        }
                    }
                }

                Logger.LogError($"Command {input_command} was not found. Type help to see available commands");
            }
            catch(Exception ex)
            {
                Logger.LogError($"Command failed {ex.ToString()}");
            }
        }
        
        public void setupCommands()
        {
            string AppdataFolder = Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData) + "\\BOTWM";

            string fileName = "\\BOTWM.DedicatedServer.AppdataFiles.QuestFlagsNames.txt";
            if (!File.Exists(AppdataFolder + fileName))
            {
                List<string> Resources = Assembly.GetExecutingAssembly().GetManifestResourceNames().Where(resource => resource.Contains("AppdataFiles")).ToList();

                if (!Directory.Exists(AppdataFolder))
                    Directory.CreateDirectory(AppdataFolder);

                foreach (string resource in Resources)
                {
                    Stream s = Assembly.GetExecutingAssembly().GetManifestResourceStream(resource);
                    string output = $"{AppdataFolder}\\";
                    using (FileStream AppdataFile = new FileStream(output, FileMode.Create))
                    {
                        byte[] b = new byte[s.Length + 1];
                        s.Read(b, 0, Convert.ToInt32(s.Length));
                        AppdataFile.Write(b, 0, Convert.ToInt32(b.Length - 1));
                    }
                }
            }

            string text = File.ReadAllText(AppdataFolder + fileName);

            QuestData = JsonConvert.DeserializeObject<Dictionary<string, List<string>>>(text);

            serverVariables.Add("time", "t");
            serverVariables.Add("day", "d");
            serverVariables.Add("weather", "w");

            //Gamemodes.Add("Game Completion", new bool[] { true, true, true, false, true, true, true, false, false });
            //Gamemodes.Add("Hunter VS Speedrunner", new bool[] { true, true, false, false, true, true, true, false, true });
            //Gamemodes.Add("Any% Speedrun", new bool[] { true, true, false, false, true, true, true, false, false });
            //Gamemodes.Add("Bingo???", new bool[] { true, true, false, false, true, true, true, false, false });
            //Gamemodes.Add("Hide n' Seek", new bool[] { true, true, false, false, false, false, false, false, false });

            Gamemodes = JsonConvert.DeserializeObject<List<ServerSettings>>(File.ReadAllText(Directory.GetCurrentDirectory() + "/Gamemodes.json"));

            var methods = AppDomain.CurrentDomain.GetAssemblies()
                .SelectMany(x => x.GetTypes())
                .Where(x => x.IsClass)
                .SelectMany(x => x.GetMethods())
                .Where(x => x.GetCustomAttributes(typeof(ServerCommand), false).FirstOrDefault() != null && x.GetCustomAttributes(typeof(Description), false).FirstOrDefault() != null);

            foreach (var method in methods)
            {
                List<string> alternateNames = new List<string>();

                foreach (AlternateName attribute in method.GetCustomAttributes(typeof(AlternateName), false))
                {
                    alternateNames.Add(attribute.name);
                }

                bool shouldAdd = true;

                foreach (var parameter in method.GetParameters())
                {
                    if (parameter.ParameterType != typeof(string))
                    {
                        shouldAdd = false;
                    }
                }

                if (!shouldAdd) continue;

                CommandList.Add(new Command(method, method.Name, ((Description)method.GetCustomAttribute(typeof(Description), false)).description, alternateNames));
            }
        }

        public void CopyAppdataFiles()
        {
            string AppdataFolder = Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData) + "\\BOTWM";
            List<string> Resources = Assembly.GetExecutingAssembly().GetManifestResourceNames().Where(resource => resource.Contains("AppdataFiles")).ToList();

            if (!Directory.Exists(AppdataFolder))
                Directory.CreateDirectory(AppdataFolder);

            foreach (string resource in Resources)
            {
                Stream s = Assembly.GetExecutingAssembly().GetManifestResourceStream(resource);
                string output = $"{AppdataFolder}\\{resource.Replace("BOTW.DedicatedServer.AppdataFiles.", "")}";
                using (FileStream AppdataFile = new FileStream(output, FileMode.Create))
                {
                    byte[] b = new byte[s.Length + 1];
                    s.Read(b, 0, Convert.ToInt32(s.Length));
                    AppdataFile.Write(b, 0, Convert.ToInt32(b.Length - 1));
                }
            }
        }

        [ServerCommand]
        [AlternateName("Commands")]
        [AlternateName("H")]
        [Description("Shows available commands")]
        public void Help()
        {

            Logger.LogInformation("---Showing available commands---", color: commandColors);

            Console.ForegroundColor = ConsoleColor.White;

            foreach (var command in CommandList)
            {

                if (((ServerCommand)command.Method.GetCustomAttribute(typeof(ServerCommand), false)).Debug) continue;

                string param = "";

                if (command.Method.GetParameters().Count() > 0)
                {
                    param += " ";

                    foreach (var parameter in command.Method.GetParameters())
                    {
                        param += "<" + parameter.Name + "> ";
                    }

                    param = param.Substring(0, param.Length - 1);
                }

                Logger.LogInformation($"{command.Name}{param}: {command.Description}");
            }

        }

        private Dictionary<int, string> GetLandmarks(string filter = "") => filter == "" ? LandmarkPositions.Keys
                                                                                                            .Select((key, index) => new { key, index })
                                                                                                            .ToDictionary(x => x.index + 1, x => x.key) :
                                                                                           LandmarkPositions.Keys
                                                                                                            .Select((key, index) => new { key, index })
                                                                                                            .Where(x => x.key.ToLower().Contains(filter.ToLower()))
                                                                                                            .ToDictionary(x => x.index + 1, x => x.key);

        [ServerCommand]
        [Description("Gets the available landmarks to teleport to")]
        [ExtraHelp("")]
        public void Landmarks(string filter = "")
        {
            Dictionary<int, string> FilteredLandmarks = GetLandmarks(filter);

            if(FilteredLandmarks.Count == 0)
            {
                Logger.LogError($"No landmark found with the filter {filter}");
            }

            foreach(KeyValuePair<int, string> Landmark in FilteredLandmarks)
            {
                string spaces = Landmark.Key < 10 ? "  " : Landmark.Key < 100 ? " " : "";

                Logger.LogInformation($"[{Landmark.Key}]{spaces} {Landmark.Value}", color: commandColors);
            }
        }

        [ServerCommand]
        [Description("Teleport player to position or other player")]
        [AlternateName("Tp")]
        [ExtraHelp("Usage 1: Tp <Source> <Destination>")]
        [ExtraHelp("<Source>: Player number, player name or @a for everyone")]
        [ExtraHelp("Use \"p#\" to teleport a player by its number. ")]
        [ExtraHelp("<Destination>: Player number, player name or landmark")]
        [ExtraHelp("Use \"p#\" to teleport to a player by its number. Otherwise, use \"l#\" to teleport to a landmark")]
        [ExtraHelp("Usage 2: Tp <Source> <Destination_x> <Destination_y> <Destination_z>")]
        [ExtraHelp("<Source>: Player number, player name or @a for everyone")]
        [ExtraHelp("<Destination_x>: Destination x axis")]
        [ExtraHelp("<Destination_x>: Destination y axis")]
        [ExtraHelp("<Destination_x>: Destination z axis")]
        public void Teleport(string source, string destination, string destination_y = "", string destination_z = "")
        {
            Dictionary<byte, string> PlayerList = ServerData.GetPlayers().Names;

            List<int> SourcePlayers = new List<int>();

            if(source.StartsWith("p") && source.Count() < 4)
            {
                int player = 0;
                if (Int32.TryParse(source.Replace("p", ""), out player) && PlayerList.ContainsKey((byte)(player - 1)))
                    SourcePlayers.Add(player - 1);
                else
                    SourcePlayers.AddRange(PlayerList.Where(p => p.Value == source).Select(p => (int)p.Key));
            }
            else if(source == "@a")
                SourcePlayers.AddRange(PlayerList.Select(p => (int)p.Key));
            else
                SourcePlayers.AddRange(PlayerList.Where(p => p.Value == source).Select(p => (int)p.Key));

            SourcePlayers = SourcePlayers.Where(p => ServerData.GetPlayer(p).Connected).ToList();

            if (SourcePlayers.Count == 0)
            {
                Logger.LogError($"Could not find player that matches {source}");
                return;
            }

            Vec3f Destination = new Vec3f();
            
            if(!string.IsNullOrEmpty(destination_y) && !string.IsNullOrEmpty(destination_z))
            {
                float x, y, z;

                if(!float.TryParse(destination, out x) || !float.TryParse(destination_y, out y) || !float.TryParse(destination_z, out z))
                {
                    Logger.LogError($"Destination input was not valid. Use \"tp help\" for extra information");
                    return;
                }

                Destination = new Vec3f(x, y, z);
            }
            else
            {
                if (destination.StartsWith("p") && destination.Count() < 4)
                {
                    int player = 0;
                    if (Int32.TryParse(destination.Replace("p", ""), out player))
                        Destination = ServerData.GetPlayer(player - 1).Position;
                    else
                        Destination = ServerData.GetPlayer(PlayerList.Where(p => p.Value == destination).Select(p => (int)p.Key).FirstOrDefault()).Position;
                }
                else if(destination.StartsWith("l") && destination.Count() < 5)
                {
                    int landmark = 0;
                    if (Int32.TryParse(destination.Replace("l", ""), out landmark) && GetLandmarks().ContainsKey(landmark))
                        Destination = LandmarkPositions[GetLandmarks()[landmark]];
                    else
                        Destination = ServerData.GetPlayer(PlayerList.Where(p => p.Value == destination).Select(p => (int)p.Key).FirstOrDefault()).Position;
                }
                else
                {
                    if(PlayerList.Any(p => p.Value == destination))
                        Destination = ServerData.GetPlayer(PlayerList.Where(p => p.Value == destination).Select(p => (int)p.Key).FirstOrDefault()).Position;
                }
            }

            if(Destination == null || (Destination.x == 0 && Destination.y == 0 && Destination.z == 0))
            {
                Logger.LogError($"Destination input was not valid. Use \"tp help\" for extra information");
                return;
            }

            ServerData.TeleportData.AddTp(SourcePlayers, Destination);

            Logger.LogInformation($"Requested teleport of {SourcePlayers.Count} players to {Destination.x}, {Destination.y}, {Destination.z}");
        }

        [ServerCommand]
        [Description("Stops enemy and quest sync")]
        public void Stop()
        {

            server.isEnemySync = false;
            server.isQuestSync = false;

            Logger.LogInformation("Deactivated quest and enemy sync", color: commandColors);

        }

        [ServerCommand]
        [Description("Starts enemy and quest sync")]
        public void Start()
        {

            server.isEnemySync = true;
            server.isQuestSync = true;

            Logger.LogInformation("Activated quest and enemy sync", color: commandColors);
        }

        [ServerCommand]
        [Description("Get or set limits for DeathSwap")]
        [ExtraHelp("Usage 1: DeathSwap <state>")]
        [ExtraHelp("<state>: on or off")]
        [ExtraHelp("Usage 2: DeathSwap <LowerLimit>:<UpperLimit>")]
        [ExtraHelp("<LowerLimit> and <UpperLimit> should be integers")]
        [ExtraHelp("Usage 3: DeathSwap <Value>")]
        [ExtraHelp("<Value> should be an integer")]
        [ExtraHelp("Usage 4: DeathSwap")]
        [ExtraHelp("Get current state and current limits")]
        [AlternateName("DS")]
        public void DeathSwap(string parameter = "")
        {
            if (parameter == "on")
            {
                ServerData.DeathSwapMutex.WaitOne(100);

                ServerData.DeathSwap.Enabled = true;

                Logger.LogInformation("Enabled death swap.", color: commandColors);

                ServerData.DeathSwapMutex.ReleaseMutex();

                return;
            }
            else if (parameter == "off")
            {
                ServerData.DeathSwapMutex.WaitOne(100);

                ServerData.DeathSwap.Enabled = false;
                ServerData.DeathSwap.Running = false;

                Logger.LogInformation("Disabled death swap.", color: commandColors);

                ServerData.DeathSwapMutex.ReleaseMutex();

                return;
            }
            else if (parameter.Contains(':'))
            {

                if (parameter.Split(":").Length != 2)
                {
                    Logger.LogError("Invalid parameter. Use DeathSwap Help to get information on how to use the command.");
                    return;
                }

                int Lower;
                int Upper;

                if (!int.TryParse(parameter.Split(":")[0], out Lower))
                {
                    Logger.LogError("Invalid Lower limit. Use DeathSwap Help to get information on how to use the command.");
                    return;
                }

                if (!int.TryParse(parameter.Split(":")[1], out Upper))
                {
                    Logger.LogError("Invalid Upper limit. Use DeathSwap Help to get information on how to use the command.");
                    return;
                }

                ServerData.DeathSwapMutex.WaitOne(100);

                ServerData.DeathSwap.ChangeLimits(Lower, Upper, -1, 1);
                ServerData.DeathSwap.CalculateNewLimit();

                Logger.LogInformation($"Set limits to: {Lower}:{Upper}", color: commandColors);

                ServerData.DeathSwapMutex.ReleaseMutex();

                return;

            }
            else if (parameter == "")
            {

                string ExtraMessage = "";

                if (ServerData.DeathSwap.TimerLimit.random)
                    ExtraMessage = $", Lower limit: {ServerData.DeathSwap.TimerLimit.Lower}, Upper limit: {ServerData.DeathSwap.TimerLimit.Upper}";

                Double TimeLeft = ServerData.DeathSwap.TimeLeft();

                Logger.LogInformation($"Time until next swap: {Math.Truncate(TimeLeft)} min {(int)Math.Round((TimeLeft - Math.Truncate(TimeLeft)) * 60, 0)} sec", color: commandColors);
                Logger.LogInformation($"Current DeathSwap settings => Enabled: {ServerData.DeathSwap.Enabled}, Is random: {ServerData.DeathSwap.TimerLimit.random}{ExtraMessage}", color: commandColors);
                return;

            }
            else
            {

                int NewValue;

                if (int.TryParse(parameter, out NewValue))
                {

                    ServerData.DeathSwapMutex.WaitOne(100);

                    //server.DeathSwap.TimerLimit.random = true;
                    ServerData.DeathSwap.ChangeLimits(-1, -1, NewValue, 0);

                    Logger.LogInformation($"Set death swap timer to {NewValue}", color: commandColors);

                    ServerData.DeathSwapMutex.ReleaseMutex();

                    return;

                }

                Logger.LogError("Invalid parameter. Use DeathSwap Help to get information on how to use the command.");
                return;

            }

        }


        [ServerCommand]
        [Description("Change Hunter vs Speedrunner glyph settings")]
        [ExtraHelp("Usage: Glyph <time (in seconds)> <distance>")]
        [ExtraHelp("To leave a value unchanged, set it to -1")]
        public void Glyph(string time = "", string distance = "")
        {

            short Time;
            short Distance;
            List<string> message = new List<string>();

            if(time == "" || time == "-1")
            {
                Time = server.GlyphTime;
            }else
            {
                if (!Int16.TryParse(time, out Time))
                {
                    Logger.LogError("Invalid time value. Time should be an integer");
                    return;
                }
                else
                {
                    message.Add($" time to {Time} ");
                }
            }

            if(distance == "" || distance == "-1")
            {
                Distance = server.GlyphDistance;
            }
            else
            {
                if (!Int16.TryParse(distance, out Distance))
                {
                    Logger.LogError("Invalid distance value. Distance should be an integer");
                    return;
                }
                else
                {
                    message.Add($" distance to {Distance} ");
                }
            }

            server.GlyphTime = Time;
            server.GlyphDistance = Distance;

            Logger.LogInformation($"Changed the{string.Join("and", message)}", color: commandColors);

        }

        [ServerCommand(true)]
        [Description("Shows available debug commands")]
        [AlternateName("_H")]
        public void _Help()
        {

            Logger.LogInformation("---Showing available debug commands---", color: commandColors);

            Console.ForegroundColor = ConsoleColor.White;

            foreach (var command in CommandList)
            {

                if (!((ServerCommand)command.Method.GetCustomAttribute(typeof(ServerCommand), false)).Debug) continue;

                string param = "";

                if (command.Method.GetParameters().Count() > 0)
                {
                    param += " ";

                    foreach (var parameter in command.Method.GetParameters())
                    {
                        param += "<" + parameter.Name + "> ";
                    }

                    param = param.Substring(0, param.Length - 1);
                }

                Logger.LogInformation($"{command.Name}{param}: {command.Description}");
            }

        }

        //[ServerCommand(true)]
        //[Description("Get or set next client log print")]
        //[AlternateName("_CL")]
        //[ExtraHelp("Usage: ClientLog <value>")]
        //[ExtraHelp("<value> = 0 for get current value")]
        //public void _ClientLog(string value = "0")
        //{

        //    int val;

        //    if (!Int32.TryParse(value, out val))
        //    {
        //        server.LogInfo($"The parameter <value> has to be a number. Correct usage: ClientLog <value>", ConsoleColor.DarkRed);
        //        return;
        //    }

        //    if(val == 0)
        //    {
        //        server.LogInfo($"{server.ClientLog}", commandColors);
        //    }else if(val > 0)
        //    {
        //        server.LogInfo($"Succesfuly updated client log to {val}", commandColors);
        //        server.ClientLog = val;
        //    }

        //}

        //[ServerCommand(true)]
        //[Description("Set next server log print")]
        //[AlternateName("_SL")]
        //public void _ServerLog()
        //{

        //    server.LogInfo($"Succesfuly activated server log", commandColors);
        //    server.ServerLog = true;

        //}


        [ServerCommand]
        [Description("set always day")]

        public void AlwaysDay()
        {
            server.AlwaysDay = !server.AlwaysDay;
            Logger.LogInformation(!server.AlwaysDay ? "Deactivated AlwaysDay" : "Activated AlwaysDay", color: commandColors);
        }



        [ServerCommand]
        [Description("Get or set time")]
        [ExtraHelp("Usage: Time Get or Time Set <value>")]
        [ExtraHelp("<value> format: HH:MM")]
        public void Time(string action = "", string value = "")
        {

            if(action.ToLower() == "get" || action == "")
            {

                if (value != "") Logger.LogWarning($"Ignored the value {value}. Correct usage: Time Get");

                double serverTime = ServerData.WorldData.Time;
                int serverDay = ServerData.WorldData.Day;

                if (serverTime == -1)
                {
                    Logger.LogWarning("Time has not been set yet.");
                    return;
                }

                string Hour;
                string Minute;

                if(serverTime == 0)
                {
                    Hour = "00";
                    Minute = "00";
                }
                else
                {

                    Hour = Math.Truncate((serverTime / 15)).ToString();
                    Minute = ((((serverTime / 15) - Math.Truncate(serverTime / 15)) * 60).ToString() + "0").Substring(0, 2);

                    if (Minute[1] == '.') Minute = "0" + Minute[0];
                }

                Logger.LogInformation($"Server time is {Hour}:{Minute} and the current day is {serverDay}", color: commandColors);
                return;

            }
            else if(action.ToLower() == "set")
            {

                int Hour;
                int Minute;

                if(value == "")
                {
                    Logger.LogError($"The parameter <value> cannot be empty. Correct usage: Time Set HH:MM");
                    return;
                }

                if(value.Split(":").Length != 2)
                {
                    Logger.LogError($"Invalid time format. Correct usage: Time Set HH:MM");
                    return;
                }

                if (!Int32.TryParse(value.Split(":")[0], out Hour))
                {
                    Logger.LogError($"The parameter <value> has to be a time value. Correct usage: Time Set HH:MM");
                    return;
                }

                if (!Int32.TryParse(value.Split(":")[1], out Minute))
                {
                    Logger.LogError($"The parameter <value> has to be a time value. Correct usage: Time Set HH:MM");
                    return;
                }

                if(Hour < 0 || Hour > 24)
                {
                    Logger.LogError($"Invalid Hour value. Hours can only take values from 0 to 24");
                    return;
                }

                if (Minute < 0 || Minute > 60)
                {
                    Logger.LogError($"Invalid Minute value. Minutes can only take values from 0 to 60");
                    return;
                }

                float newTime = ((float)Hour + ((float)Minute / 60)) * 360 / 24;
                double serverTime = ServerData.WorldData.Time;
                int serverDay = ServerData.WorldData.Day;
                int serverWeather = ServerData.WorldData.Weather;

                if (serverTime == -1)
                {
                    ServerData.UpdateWorldData(new WorldDTO() { Time = newTime, Day = 0, Weather = serverWeather }, -1);
                }
                else
                {
                    if(newTime > serverTime)
                    {
                        ServerData.UpdateWorldData(new WorldDTO() { Time = newTime, Day = serverDay, Weather = serverWeather }, -1);
                    }
                    else
                    {
                        ServerData.UpdateWorldData(new WorldDTO() { Time = newTime, Day = serverDay + 1, Weather = serverWeather }, -1);
                    }
                }

                Logger.LogInformation($"Time set to {value}", color: commandColors);

            }else
            {
                Logger.LogError($"Invalid action. Correct usage: Time Get or Time Set <value>");
            }

        }

        [ServerCommand]
        [Description("Get or set weather")]
        [ExtraHelp("Usage: Weather Get or Weather Set <value>")]
        [ExtraHelp("<value> options:")]
        [ExtraHelp("\t     auto")]
        [ExtraHelp("\t     BlueSky")]
        [ExtraHelp("\t     Cloudy")]
        [ExtraHelp("\t     Rain")]
        [ExtraHelp("\t     HeavyRain")]
        [ExtraHelp("\t     Snow")]
        [ExtraHelp("\t     HeavySnow")]
        [ExtraHelp("\t     Thunderstorm")]
        [ExtraHelp("\t     ThunderRain")]
        [ExtraHelp("\t     BlueSkyRain")]
        public void Weather(string action = "", string value = "")
        {
            if(action.ToLower() == "get" || action == "")
            {

                int ServerWeather = ServerData.WorldData.Weather;

                Logger.LogInformation($"Current server weather is {((Weathers)ServerWeather).ToString()}", color: commandColors);
                return;

            }else if(action.ToLower() == "set")
            {

                value = value.ToLower();

                if(value == "auto")
                {
                    ServerData.WorldData.isForcedWeather = false;
                    Logger.LogInformation("Returned weather control back to players.", color: commandColors);
                    return;
                }

                Weathers UserWeather;

                if(!Enum.TryParse<Weathers>(value, out UserWeather))
                {

                    Logger.LogError($"Invalid weather value. Type Weather Help to see the available weather types");
                    return;

                }

                double serverTime = ServerData.WorldData.Time;
                int serverDay = ServerData.WorldData.Day;
                int serverWeather = ServerData.WorldData.Weather;

                ServerData.WorldData.isForcedWeather = true;
                ServerData.UpdateWorldData(new WorldDTO() { Time = (float)serverTime, Day = serverDay, Weather = (int)UserWeather }, -1);

                Logger.LogInformation($"Weather set to {value}", color: commandColors);
                return;

            }else
            {
                Logger.LogError($"Invalid action. Correct usage: Weather Get or Weather Set <value>. Type Weather Help to see the available weather types");
            }
        }

        //[ServerCommand]
        //[Description("See current mod's version")]
        //[AlternateName("ver")]
        //public void Version()
        //{
        //    server.LogInfo($"Current mod's version is: {server.Version}", ConsoleColor.DarkMagenta);
        //}

        [ServerCommand]
        [Description("Cleans the console")]
        [AlternateName("CLS")]
        public void Clear()
        {
            Console.Clear();
        }

        [ServerCommand(true)]
        [Description("Prints the current quest data from the server")]
        public void _Quests(string search = "")
        {
            foreach (string item in ServerData.QuestData.ServerQuests)
            {
                if (string.IsNullOrEmpty(search) || (QuestData[item][1].Contains(search)))
                {
                    Logger.LogInformation($"{QuestData[item][1]}");
                }
            }
        }

        [ServerCommand(true)]
        [Description("Completes quest")]
        [AlternateName("_CQ")]
        [ExtraHelp("Usage: _CompleteQ <quest>")]
        [ExtraHelp("<quest> example: C1,C2,C3")]
        public void _CompleteQ(string quest)
        {
            quest = $"[\"{string.Join("\",\"", quest.Split(","))}\"]";

            try
            {
                ServerData.ProcessExternalQuests(JsonConvert.DeserializeObject<List<string>>(quest));

                Logger.LogInformation("Quests added to the list", color: commandColors);
            }
            catch(Exception ex)
            {
                Logger.LogError(ex.Message);
                Logger.LogError("Failed to add quests. Use _CompleteQ help to see the correct usage");
            }

        }

        [ServerCommand(true)]
        [Description("Prints a certain property from a player")]
        public void _Get(string playerNumber, string property)
        {

            int PN = 0;

            if(!Int32.TryParse(playerNumber, out PN))
            {
                Logger.LogError("Parameter playerNumber should be an integer value");
                return;
            }

            if (ServerData.PlayerList.Count < PN || !ServerData.PlayerList[PN - 1].Connected)
            {
                Logger.LogInformation($"Player {PN} is not connected");
                return;
            }

            FieldInfo[] PlayerFields = typeof(Player).GetFields();

            if (!PlayerFields.Any(Fld => Fld.Name.ToLower() == property.ToLower()))
            {
                Logger.LogError($"Player{playerNumber} doesn't contain the property {property}");
                return;
            }

            Logger.LogInformation($"---Player{playerNumber}'s {property} is---", color: commandColors);

            object playerData = PlayerFields.Where(Fld => Fld.Name.ToLower() == property.ToLower()).First().GetValue(ServerData.PlayerList[PN - 1]);

            if(playerData.GetType().ToString() == "System.Collections.Generic.Dictionary`2[System.String,System.Object]")
            {

                foreach(KeyValuePair<string, object> kvp in (Dictionary<string, object>)playerData)
                {
                    Logger.LogInformation($"{kvp.Key}: {kvp.Value}");
                }

            }
            else if(playerData.GetType().ToString() == "System.Collections.Generic.Dictionary`2[System.String,System.String]")
            {

                foreach(KeyValuePair<string, string> kvp in (Dictionary<string, string>)playerData)
                {
                    Logger.LogInformation($"{kvp.Key}: {kvp.Value}");
                }

            }
            else if(playerData.GetType().ToString().Contains("List"))
            {
                foreach(object item in (List<object>)playerData)
                {
                    Logger.LogInformation($"{item.ToString()}");
                }
            }
            else
            {
                Logger.LogInformation($"{playerData.ToString()}");
            }

        }

        [ServerCommand(true)]
        [Description("Prints all properties available players")]
        public void _Properties()
        {
            Logger.LogInformation($"---Player properties are---", color: commandColors);
            
            FieldInfo[] PlayerFields = typeof(Player).GetFields();

            foreach (var property in PlayerFields.Select(Fld => Fld.Name))
            {
                Logger.LogInformation($"{property}");
            }
        }

        //[ServerCommand(true)]
        //[Description("Enable/Disable enemy logging")]
        //public void _EnemyLog()
        //{
        //    server.EnemyLog = !server.EnemyLog;
        //    server.LogInfo(!server.EnemyLog ? "Deactivated enemy log" : "Activated enemy log", commandColors);
        //}

        //[ServerCommand(true)]
        //[AlternateName("_Test")]
        //[AlternateName("_Local")]
        //[Description("Enable/Disable local test")]
        //public void _LocalTest()
        //{
        //    server.isLocalTest = !server.isLocalTest;
        //    server.LogInfo(!server.isLocalTest ? "Deactivated local test" : "Activated local test", commandColors);
        //}

        [ServerCommand]
        [Description("Enable/Disable name tags")]
        public void NameTags()
        {
            server.DisplayNames = !server.DisplayNames;
            Logger.LogInformation(!server.DisplayNames ? "Deactivated Name Tags" : "Activated Name Tags", color: commandColors);
        }

        [ServerCommand(true)]
        [Description("Change player's model")]
        [ExtraHelp("Usage: Model <player> <ModelFolder:ModelName>")]
        public void _Model(string playerNumber, string model)
        {
            byte PN = 0;

            if (!Byte.TryParse(playerNumber, out PN))
            {
                Logger.LogError("Parameter playerNumber should be an integer value");
                return;
            }

            if (ServerData.PlayerList.Count < PN || !ServerData.PlayerList[PN - 1].Connected)
            {
                Logger.LogInformation($"Player {PN} is not connected");
                return;
            }

            ModelDataDTO playerModel = ServerData.ModelData.PlayerModels[(byte)(PN - 1)];

            if(model.ToLower() == "link")
            {
                playerModel.ModelType = 0;
                playerModel.Model = "Jugador1ModelNameLongForASpecificReason";
            }else
            {
                playerModel.ModelType = 1;
                playerModel.Model = model;
            }

            ServerData.ModelData.AddModel((byte)(PN - 1), playerModel);

            Logger.LogInformation($"Player {PN} model set to {model}", color: commandColors);
        }

        private int getDigitCount(int number)
        {
            return number.ToString().Length;
        }

        [ServerCommand]
        [Description("Return locations defined for prophunt")]
        [AlternateName("PHL")]
        public void PropHuntLocations()
        {
            for (int i = 0; i < this.ServerProphuntLocations.Count(); i++)
            {
                Logger.LogInformation($"{this.ServerProphuntLocations[i].Name}");
            }
        }

        [ServerCommand]
        [Description("Starts or stops a match of prophunt")]
        [ExtraHelp("Usage: PropHunt <state> <location>")]
        [ExtraHelp("<state>: start or stop")]
        [ExtraHelp("<location>: location where prophunt is going to be played. Use PropHuntLocations to retrieve the existing locations for prophunt")]
        [AlternateName("PH")]
        public void PropHunt(string state = "", string location = "")
        {
            try
            {
                bool iState = this.GetProphuntState(state);

                if(!iState)
                {
                    // Call prop hunt stop //
                    ServerData.PropHuntData.Stop();
                    return;
                }

                if (ServerData.PlayerList.Where(p => p.Connected).Count() < 2)
                {
                    Logger.LogError("At least two players are necessary to activate Prop Hunt");
                    return;
                }

                // Get prophunt location
                ProphuntLocation pLocation = this.GetLocation(location);

                // Call prop hunt start //
                ServerData.PropHuntData.Start(pLocation, 60);

            }
            catch (Exception ex)
            {
                Logger.LogError(ex.Message);
                Logger.LogWarning($"Usage: PropHunt <state> <location>");
                Logger.LogWarning("<state>: start or stop");
                Logger.LogWarning("<location>: location where prophunt is going to be played. Use PropHuntLocations to retrieve the existing locations for prophunt");
                return;
            }
        }
    
        private bool GetProphuntState(string state)
        {
            if (state == "" || (state != "start" && state != "stop" && state != "on" && state != "off"))
                throw new Exception("State must be set.");

            return state == "start" || state == "on";
        }

        private ProphuntLocation GetLocation(string location)
        {
            if (location == "")
                throw new Exception("Location must be set.");

            if(Int32.TryParse(location, out int result))
            {
                if (this.ServerProphuntLocations.Count < result)
                    throw new Exception("Invalid value for location");

                return this.ServerProphuntLocations[result];
            }

            if (!this.ServerProphuntLocations.Any(loc => loc.Name.ToLower() == location.ToLower()))
                throw new Exception("Location must be part of the list of locations.");

            return this.ServerProphuntLocations.Find(loc => loc.Name.ToLower() == location.ToLower());
        }
    }
}
