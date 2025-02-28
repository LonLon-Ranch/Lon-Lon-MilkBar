using Breath_of_the_Wild_Multiplayer.Source_files;
using Breath_of_the_Wild_Multiplayer.MVVM.Model;
using System;
using System.Collections.ObjectModel;
using System.Collections.Generic;
using System.IO;
using Newtonsoft.Json;
using System.Diagnostics;
using Breath_of_the_Wild_Multiplayer.MVVM.Model.DTO;
using System.Linq;
using System.Threading.Tasks;
using System.Text;
using DiscordRPC;
using System.Threading;
using System.Windows;
using System.Windows.Controls;
using Breath_of_the_Wild_Multiplayer;
using System.Net.NetworkInformation;
using System.Net.Sockets;
using System.Net;
using System.Xml.Linq;

namespace Breath_of_the_Wild_Multiplayer.MVVM.ViewModel
{
    public class ServerBrowserModel : ObservableObject
    {
        private ObservableCollection<serverDataModel> _serversToShow = new ObservableCollection<serverDataModel>();

        public ObservableCollection<serverDataModel> serversToShow
        {
            get { return _serversToShow; }
            set
            {
                _serversToShow = value;
                OnPropertyChanged();
            }
        }

        private bool _isMaxOffset;

        public bool isMaxOffset
        {
            get { return _isMaxOffset; }
            set
            {
                _isMaxOffset = value;
                OnPropertyChanged();
            }
        }

        private int _scrollState;

        public int scrollState
        {
            get { return _scrollState; }
            set { 
                _scrollState = value;
                OnPropertyChanged();
            }
        }

        private ObservableCollection<serverDataModel> _selectedServer;

        public ObservableCollection<serverDataModel> selectedServer
        {
            get { return _selectedServer; }
            set { 
                _selectedServer = value;
                OnPropertyChanged();
            }
        }

        private Dictionary<int, LocalServerDTO> ServerMapping;

        private string GameDir;
        private string CemuDir;
        public List<LocalServerDTO> ServerList;

        public RelayCommand serverButtonClick { get; set; }
        public AsyncRelayCommand connectClick { get; set; }
        public RelayCommand refreshClick { get; set; }
        public RelayCommand addServerClick { get; set; }
        public RelayCommand directConnectClick { get; set; }

        public int lastSelected = -1;

        public ServerBrowserModel()
        {
            SharedData.ServerBrowser = this;

            findCemuData();
            isMaxOffset = true;
            scrollState = 0;

            LoadServers();

            serverButtonClick = new RelayCommand(o => this.changeSelected(o));
            connectClick = new AsyncRelayCommand(o => this.connectToServer(null));
            refreshClick = new RelayCommand(o =>
            {
                foreach (var server in this._serversToShow)
                    _ = Task.Run(() => server.pingServer());
            });

            addServerClick = new RelayCommand(o =>
            {
                SharedData.MainView.updateTopView(new ServerEditorModel());
            });

            directConnectClick = new RelayCommand(o =>
            {
                var DirectConnectEditor = new ServerEditorModel();
                DirectConnectEditor.Setup(-2);
                SharedData.MainView.updateTopView(DirectConnectEditor);
            });
        }

        public static bool CemuSettingsOGRPCState;

        public void GetOGCemuRPC()
        {
            string CemuDir;

            string CemuSettings = File.ReadAllText(Properties.Settings.Default.bcmlLocation);
            Dictionary<string, string> settings = JsonConvert.DeserializeObject<Dictionary<string, string>>(CemuSettings)!;
            CemuDir = settings["cemu_dir"];
            string CemuSetting = $"{CemuDir}/settings.xml";
            try
            {
                XDocument xmlDoc = XDocument.Load(CemuSetting);

                XElement discordPresence = xmlDoc.Descendants("use_discord_presence").FirstOrDefault();

                if (discordPresence != null)
                {
                    CemuSettingsOGRPCState = bool.Parse(discordPresence.Value);
                }
            }
            catch (Exception ex)
            {
                SharedData.LoadErrorMessage(ex.Message);
                return;
            }
        }

        public void ModifyRPCInCemu(string state)
        {
            string CemuDir;

            string CemuSettings = File.ReadAllText(Properties.Settings.Default.bcmlLocation);
            Dictionary<string, string> settings = JsonConvert.DeserializeObject<Dictionary<string, string>>(CemuSettings)!;
            CemuDir = settings["cemu_dir"];
            string CemuSetting = $"{CemuDir}/settings.xml";
            try
            {
                XDocument xmlDoc = XDocument.Load(CemuSetting);

                XElement discordPresence = xmlDoc.Descendants("use_discord_presence").FirstOrDefault();

                if (discordPresence != null)
                {
                    discordPresence.Value = state;

                    xmlDoc.Save(CemuSetting);
                }
            }
            catch (Exception ex)
            {
                SharedData.LoadErrorMessage(ex.Message);
                return;
            }
        }


        void DiscrodUpdate()
        {
            serverDataModel server = _selectedServer[0];
            int NumberOfPlayers = -1; // impossible number
            int OldNumber = -2; // impossible number
            string actualPlayer = Properties.Settings.Default.playerName;
            bool DisplayServer = Properties.Settings.Default.RPCDisplayServer;
            Thread.Sleep(1000);
            while (true)
            {
                server.pingServer();
                NumberOfPlayers = server.playerList.Count;

                if (server.playerList.ContainsValue(actualPlayer) && server.open)  // we verify if the player is in the server, if the server is open and if the number of players has changed.
                {
                    if (OldNumber != NumberOfPlayers)
                    {
                        if (DisplayServer)
                        {
                            DiscordRichPresence.updateRPC($"Playing in {server.Name}, Gamemode: {server.playStyle}", $"Players connected: {NumberOfPlayers}/{server.capacity}");
                        }
                        else
                        {
                            DiscordRichPresence.updateRPC($"Gamemode: {server.playStyle}", $"Players connected: {NumberOfPlayers}/{server.capacity}");
                        }
                    }
                    Thread.Sleep(10000);
                }
                else
                {
                    if (CemuSettingsOGRPCState == true)
                    {
                        ModifyRPCInCemu("true");
                    }

                    break;
                }
            }
        }


        void changeSelected(object newSelection)
        {
            if(lastSelected != -1)
            {
                _serversToShow[lastSelected].selected = false;
            }

            _serversToShow[(int)newSelection].selected = true;
            _selectedServer[0] = _serversToShow[(int)newSelection];
            lastSelected = (int)newSelection;
        }

        void LoadServers()
        {
            serverDataModel.serversAdded = 0;
            serversToShow.Clear();

            this.ServerList = JsonConvert.DeserializeObject<List<LocalServerDTO>>(Properties.Settings.Default.serversAdded)!;

            ServerMapping = new Dictionary<int, LocalServerDTO>();

            foreach (var server in this.ServerList.Where(sv => sv.Favorite))
            {
                serversToShow.Add(new serverDataModel(!string.IsNullOrEmpty(GameDir) && !string.IsNullOrEmpty(CemuDir), name: server.Name, ip: server.IP, port: server.Port, favorite: server.Favorite, password: server.Password));
                ServerMapping[serverDataModel.serversAdded - 1] = server;
            }

            foreach (var server in this.ServerList.Where(sv => !sv.Favorite))
            {
                serversToShow.Add(new serverDataModel(!string.IsNullOrEmpty(GameDir) && !string.IsNullOrEmpty(CemuDir), name: server.Name, ip: server.IP, port: server.Port, favorite: server.Favorite, password: server.Password));
                ServerMapping[serverDataModel.serversAdded - 1] = server;
            }

            selectedServer = new ObservableCollection<serverDataModel>();

            if (_serversToShow.Count == 0)
            {
                selectedServer.Add(new serverDataModel(!string.IsNullOrEmpty(GameDir) && !string.IsNullOrEmpty(CemuDir), name: "", ip: "", port: 0, favorite: false, password: ""));
            }
            else
            {
                selectedServer.Add(_serversToShow[0]);
                serversToShow[0].selected = true;
                lastSelected = 0;
            }
        }

        void findCemuData()
        {
            try
            {
                string CemuSettings = File.ReadAllText(Properties.Settings.Default.bcmlLocation);
                Dictionary<string, string> settings = JsonConvert.DeserializeObject<Dictionary<string, string>>(CemuSettings)!;
                CemuDir = settings["cemu_dir"];
                GameDir = settings["game_dir"];
            }
            catch(Exception ex)
            {
                CemuDir = "";
                GameDir = "";
            }
        }

        async Task connectToServer(serverDataModel serverToJoin = null)
        {
            bool directConnection = serverToJoin != null;

            if (serverToJoin == null)
                serverToJoin = _selectedServer[0];

            if (string.IsNullOrEmpty(GameDir) || string.IsNullOrEmpty(CemuDir))
                throw new ApplicationException("Bcml not setup.");

            if (string.IsNullOrEmpty(serverToJoin.IP) || !serverToJoin.open)
                return;

            serverToJoin.pingServer();

            IPResolver resolver = new IPResolver();
            string ipToUse = resolver.ResolveIP(serverToJoin.IP);

            if (!serverToJoin.open)
            {
                if (directConnection)
                    throw new Exception("Could not connect to server. The server may not be open.");
                else
                    return;
            }

            CharacterModel charModel = JsonConvert.DeserializeObject<CharacterModel>(Properties.Settings.Default.playerModel);


            SharedData.SetLoadingMessage("Loading player data...");

            await Task.Run(() => GameFilesModifier.ChangeAttentionForJugadores(serverToJoin.playStyle != "Prop hunt"));

            await Task.Run(() => GameFilesModifier.CleanAnimations());

            if (!Properties.Settings.Default.playAsModel)
            {
                await Task.Run(() => GameFilesModifier.ModifyGameROMPlayerModel("Link", "Link", false));
            }
            else
            {
                switch (charModel.Type)
                {
                    case CharacterModel.ModelType.ArmorSync:
                        // TODO: Add implementation for other armors with armor sync
                        await Task.Run(() => GameFilesModifier.ModifyGameROMPlayerModel(charModel.Name, charModel.Name, false));
                        break;
                    case CharacterModel.ModelType.CustomModel:
                        // We found that the game is only capable of having NPCs instead of Link. Therefore, we only change Link's model if it is an NPC
                        if (charModel.Model.StartsWith("Npc_"))
                            await Task.Run(() => GameFilesModifier.ModifyGameROMPlayerModel(charModel.Model, charModel.Model, true));
                        else
                            await Task.Run(() => GameFilesModifier.ModifyGameROMPlayerModel("Link", "Link", false));
                        break;
                    case CharacterModel.ModelType.Mii:
                        // TODO: Miis are not implemented. We found that trying to change Link to a Mii, you get issues when trying to find certain things in memory.
                        // If you want to work on this, go to the InjectDLL project, into the file MemoryAccess/LocalInstance.h
                        // There you can find the scan method. There we are looking for all the necessary addresses in memory. The specific memories that we found have issues are Animation and IsEquipped.
                        await Task.Run(() => GameFilesModifier.ModifyGameROMPlayerModel("Link", "Link", false));
                        break;
                }
            }

            await Task.Delay(2000);

            List<Process> ProcessesToFilter = Injector.GetProcesses("Cemu");

            if (DiscordRichPresence.client != null)
            {
                GetOGCemuRPC();
                ModifyRPCInCemu("false");
            }

            SharedData.SetLoadingMessage("Starting Cemu...");

            await Task.Run(() => Process.Start($"{CemuDir}/cemu.exe", $"-g \"{GameDir.Replace("content", "code")}/U-King.rpx\""));

            await Task.Delay(500);

            SharedData.SetLoadingMessage("Injecting Cemu...");

            Process CemuProcess = null;

            await Task.Run(() => {
                CemuProcess = Injector.Inject("Cemu", Directory.GetCurrentDirectory() + "\\Resources\\InjectDLL.dll", ProcessesToFilter);
            });

            SharedData.SetLoadingMessage("Starting pipe...");

            try
            {
                await Task.Run(NamedPipes.StartServer);
            }
            catch (Exception ex)
            {
                CemuProcess.Kill();
                throw ex;
            }

            SharedData.SetLoadingMessage("Connecting to server...");

            List<byte> instruction = Encoding.UTF8.GetBytes($"!connect;{ipToUse};{serverToJoin.Port};{serverToJoin.Password};{Properties.Settings.Default.playerName};{serverToJoin.Name};{(int)charModel.Type};").ToList();

            if ((int)charModel.Type < 2)
            {
                if(charModel.Model.StartsWith("Npc_"))
                {
                    instruction.AddRange(Encoding.UTF8.GetBytes($"{charModel.Model}:{charModel.Model}"));
                } 
                else
                {
                    instruction.AddRange(Encoding.UTF8.GetBytes($"{charModel.Model}:{charModel.Name}"));
                }
            }
            else
                instruction.AddRange(Encoding.UTF8.GetBytes(JsonConvert.SerializeObject(charModel.BumiiData)));

            instruction.AddRange(Encoding.UTF8.GetBytes(";[END]"));

            await Task.Run(() => {
                Func<string, string> GetResponseErrorFromJson = sresponse => {
                    if (sresponse.Contains(":;"))
                    {
                        string[] simplify = sresponse.Split(":;{")[1].Split("\"Response\":")[1].Split(",");
                        int iresponse = -1;

                        if (!Int32.TryParse(simplify[0], out iresponse))
                        {
                            throw new ApplicationException("Could not connect to server. GetResponseFromJsonSimplify[0] contained non number text.");
                        }

                        return BOTWM.Common.ServerError.GetErrorMessage(iresponse);
                    }
                    else
                    {
                        return "Couldn't grab anymore detail. Either something internal went wrong or you are joining a server which uses the original server code.";
                    }
                };

                string si1 = NamedPipes.sendInstruction(instruction.ToArray());
                if (!si1.Contains("Succeeded"))
                {
                    CemuProcess.Kill();
                    throw new ApplicationException("Could not connect to server. " + GetResponseErrorFromJson(si1));
                }

                Task.Delay(50);
                string si2 = NamedPipes.sendInstruction($"!startServerLoop");
                if (!si2.Contains("Succeeded"))
                {
                    CemuProcess.Kill();
                    throw new ApplicationException("Could not start server loop. " + GetResponseErrorFromJson(si2));
                }
            });

            SharedData.SetLoadingMessage("Starting overlay...");

            await Task.Delay(2000);
            MainWindow mainWindow = (MainWindow)Application.Current.MainWindow; // hide the version when you run the game
            mainWindow.HideTextVersion();
            SharedData.MainView.currentView = SharedData.MainView.IngameMenuVM;
            SharedData.MainView.disableBackground = true;
            SharedData.MainView.Window.Topmost = true;
            CemuFollower.Setup(CemuProcess);

            ////////////////////////////////////  rich presence part
            
            if (DiscordRichPresence.client != null)
            {
                Thread nouveauThread = new Thread(DiscrodUpdate);
                nouveauThread.Start();
            }

            ////////////////////////////////////

            NamedPipes.StartListenThread();

            SharedData.SetLoadingMessage();
        }

        void SaveServerSettings()
        {
            Properties.Settings.Default.serversAdded = JsonConvert.SerializeObject(this.ServerList);
            Properties.Settings.Default.Save();
        }

        public void ModifyServer(ServerInfoModel serverinfo)
        {
            if(serverinfo.serverIndex == -1)
            {
                var serverToAdd = new LocalServerDTO()
                {
                    Name = serverinfo.name,
                    IP = serverinfo.ip,
                    Port = Int32.Parse(serverinfo.port),
                    Password = serverinfo.password,
                    Favorite = false
                };

                this.ServerList.Add(serverToAdd);

                this.serversToShow.Add(new serverDataModel(!string.IsNullOrEmpty(GameDir) && !string.IsNullOrEmpty(CemuDir), name: serverinfo.name, ip: serverinfo.ip, port: Int32.Parse(serverinfo.port), favorite: false, password: serverinfo.password));
                this.ServerMapping[serverDataModel.serversAdded - 1] = serverToAdd;
                changeSelected(this._serversToShow.Count - 1);

                SaveServerSettings();
            }
            else if(serverinfo.serverIndex == -2)
            {
                serverDataModel serverToconnect = new serverDataModel(!string.IsNullOrEmpty(GameDir) && !string.IsNullOrEmpty(CemuDir), name: serverinfo.name, ip: serverinfo.ip, port: Int32.Parse(serverinfo.port), favorite: false, password: serverinfo.password, async: false);

                if (!serverToconnect.open)
                    throw new Exception("Could not connect to server. Check if the server is open.");

                this.connectToServer(serverToconnect);
            }
            else
            {
                this.ServerMapping[serverinfo.serverIndex].Name = serverinfo.name;
                this.ServerMapping[serverinfo.serverIndex].IP = serverinfo.ip;
                this.ServerMapping[serverinfo.serverIndex].Port = Int32.Parse(serverinfo.port);
                this.ServerMapping[serverinfo.serverIndex].Password = serverinfo.password;

                this.serversToShow[serverinfo.serverIndex].Name = serverinfo.name;
                this.serversToShow[serverinfo.serverIndex].IP = serverinfo.ip;
                this.serversToShow[serverinfo.serverIndex].Port = Int32.Parse(serverinfo.port);
                this.serversToShow[serverinfo.serverIndex].Password = serverinfo.password;

                SaveServerSettings();

                this.selectedServer[0].pingServer();
            }
        }

        public void EditServer()
        {
            var DirectConnectEditor = new ServerEditorModel();
            DirectConnectEditor.Setup(this.selectedServer[0].serverIndex, this.selectedServer[0].Name, this.selectedServer[0].IP, this.selectedServer[0].Port.ToString(), this.selectedServer[0].Password.ToString());
            SharedData.MainView.updateTopView(DirectConnectEditor);
        }

        public void RemoveServer()
        {
            this.ServerList.Remove(this.ServerMapping[this.selectedServer[0].serverIndex]);
            SaveServerSettings();
            LoadServers();
        }

        public void ChangeFavorite(int serverIndex)
        {
            this.ServerMapping[serverIndex].Favorite = this.serversToShow[serverIndex].favorite;

            SaveServerSettings();
        }
    }
}
