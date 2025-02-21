using System;
using System.Windows;
using Breath_of_the_Wild_Multiplayer.Source_files;
using System.Windows.Media;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Net;
using System.Net.Sockets;
using System.Diagnostics;
using System.Text;
using Newtonsoft.Json;
using Breath_of_the_Wild_Multiplayer.MVVM.Model.DTO;
using System.Threading.Tasks;
using System.Linq;
using System.Net.NetworkInformation;

namespace Breath_of_the_Wild_Multiplayer.MVVM.Model
{
    public class IPResolver
    {
        public string ResolveIP(string hostNameOrAddress)
        {
            string ipToUse = string.Empty;

            if (hostNameOrAddress == "localhost")
            {
                var host = Dns.GetHostEntry(Dns.GetHostName());
                foreach (NetworkInterface item in NetworkInterface.GetAllNetworkInterfaces())
                {
                    if (item.OperationalStatus == OperationalStatus.Up)
                    {
                        foreach (UnicastIPAddressInformation uip in item.GetIPProperties().UnicastAddresses)
                        {
                            if (uip.Address.AddressFamily == AddressFamily.InterNetwork && host.AddressList.Contains(uip.Address))
                            {
                                ipToUse = uip.Address.ToString();
                                break;
                            }
                        }
                        if (!string.IsNullOrEmpty(ipToUse)) break;
                    }
                }

                if (string.IsNullOrEmpty(ipToUse))
                {
                    ipToUse = "127.0.0.1";
                }
            }
            else
            {
                try
                {
                    var addresses = Dns.GetHostAddresses(hostNameOrAddress);
                    var ipv4Address = addresses.FirstOrDefault(ip => ip.AddressFamily == AddressFamily.InterNetwork);

                    if (ipv4Address != null)
                    {
                        ipToUse = ipv4Address.ToString();
                    }
                    else
                    {
                        ipToUse = addresses.FirstOrDefault()?.ToString() ?? hostNameOrAddress;
                    }
                }
                catch (Exception)
                {
                    ipToUse = "127.0.0.1";
                }
            }

            return ipToUse;
        }
    }
    public class serverDataModel : ObservableObject
    {

        private static Random random = new Random();
        public static int serversAdded = 0;

        private string _Name;

        public string Name
        {
            get { return _Name; }
            set { 
                _Name = value;
                 OnPropertyChanged();
            }
        }

        private string _description;

        public string description
        {
            get { return _description; }
            set { 
                _description = value;
                OnPropertyChanged();
            }
        }

        private string _playStyle;

        public string playStyle
        {
            get { return _playStyle; }
            set { 
                _playStyle = value;
                OnPropertyChanged();
            }
        }

        private int _capacity;

        public int capacity
        {
            get { return _capacity; }
            set { 
                _capacity = value;
                OnPropertyChanged();
            }
        }

        /// <summary>
        /// Get the formatted string for connected players
        /// </summary>
        public string connectedPlayersString { 
            get { return this.open ? String.Format("{0}/{1} players", playerList.Count, capacity) : ""; }
            set { OnPropertyChanged(); }
        }

        private string _questGiver;

        public string questGiver
        {
            get { return _questGiver; }
            set { 
                _questGiver = value;
                OnPropertyChanged();
            }
        }

        public Dictionary<byte, string> playerList = new Dictionary<byte, string>();

        public List<string> playerListWithNumber
        {
            get
            {
                List<string> playerNamesWithPlayerNumber = new List<string>();

                foreach(KeyValuePair<byte, string> player in playerList)
                {
                    playerNamesWithPlayerNumber.Add($"{player.Key + 1}. {player.Value}");
                }

                return playerNamesWithPlayerNumber;
            }
            set { OnPropertyChanged(); }
        }

        public bool shouldDisplayPlayersTooltip
        {
            get { return open && playerList.Count > 0; }
            set
            {
                OnPropertyChanged();
            }
        }

        public int TooltipColumnCount
        {
            get { return playerList.Count > 8 ? 8 : playerList.Count; }
            set
            {
                OnPropertyChanged();
            }
        }

        private int _ping;

        public int ping
        {
            get {
                return _ping; 
            }
            set { 
                _ping = value;
                OnPropertyChanged();
            }
        }

        public string pingData
        {
            get {
                if (this.status == ServerStatus.Pinging)
                    return "Pinging...";
                else if (this.status == ServerStatus.Offline)
                    return "Server closed";
                else if (this.status == ServerStatus.WrongPassword)
                    return "Wrong password";
                else
                    return $"{this.ping} ms";
            }
            set
            {
                OnPropertyChanged();
            }
        }

        public enum ServerStatus
        {
            Online,
            Offline,
            WrongPassword,
            Pinging
        }

        private ServerStatus _status;

        public ServerStatus status
        {
            get { return _status; }
            set { 
                _status = value;
                CallGetterUpdates();
            }
        }
        
        public SolidColorBrush pingColor
        {
            get {
                if (!this.open)
                    return new SolidColorBrush(Color.FromArgb(0xFF, 0x8C, 0x8C, 0x8C));
                if (this.ping >= 150)
                    return new SolidColorBrush(Color.FromArgb(0xFF, 0xE3, 0x46, 0x46));
                if (this.ping >= 100)
                    return new SolidColorBrush(Color.FromArgb(0xFF, 0xE3, 0xD1, 0x46));

                return new SolidColorBrush(Color.FromArgb(0xFF, 0x5F, 0xE3, 0x46));
            }
            set
            {
                OnPropertyChanged();
            }
        }

        private string _IP;

        public string IP
        {
            get { return _IP; }
            set { 
                _IP = value;
                OnPropertyChanged();
            }
        }

        private int _Port;

        public int Port
        {
            get { return _Port; }
            set { 
                _Port = value;
                OnPropertyChanged();
            }
        }

        private bool _selected;

        public bool selected
        {
            get { return _selected; }
            set { 
                _selected = value;
                OnPropertyChanged();
            }
        }

        private int _serverIndex;

        public int serverIndex
        {
            get { return _serverIndex; }
            set { 
                _serverIndex = value;
                OnPropertyChanged();
            }
        }

        private bool _favorite;

        public bool favorite
        {
            get { return _favorite; }
            set { 
                _favorite = value;
                OnPropertyChanged();
            }
        }

        private bool _open;

        public bool open
        {
            get { return _open; }
            set { 
                _open = value;
                OnPropertyChanged();
            }
        }

        public string connectMessage
        {
            get
            {
                if (!isCemuSetup)
                    return "BCML installation not found";
                if (!open)
                    if (status == ServerStatus.WrongPassword)
                        return "Wrong password";
                    else if (status == ServerStatus.Offline)
                        return "Server closed";
                return "Connect";
            }
            set
            {
                OnPropertyChanged();
            }
        }

        private Visibility _visible;

        public Visibility visible
        {
            get { return _visible; }
            set {
                _visible = value;
                OnPropertyChanged();
            }
        }

        private bool isCemuSetup;
        public RelayCommand changeFavoriteState { get; set; }
        public string Password { get; set; }

        public serverDataModel(bool isCemuSetup, string name, string ip, int port, bool favorite, string password, bool selected = false, bool async = true)
        {
            this.serverIndex = serversAdded;
            this.Name = name;
            this.description = "";
            this.playStyle = "";
            this.IP = ip;
            this.Port = port;
            this.favorite = favorite;
            this.selected = selected;
            this.isCemuSetup = isCemuSetup;
            this.Password = password;
            this.status = ServerStatus.Offline;

            this.ping = 0;
            this.capacity = 0;

            if (!string.IsNullOrEmpty(ip))
            {
                visible = Visibility.Visible;
 
                if (async)
                    _ = Task.Run(() => pingServer());
                else
                    pingServer();
                serversAdded++;
            }
            else
                visible = Visibility.Hidden;
            
            List<string> QuestGivers = new List<string>()
                {
                    "Mipha",
                    "Revali",
                    "Urbosa",
                    "Daruk",
                    "Old Man",
                    "Zelda",
                    "Ganondorf",
                    "Impa",
                    "Kass",
                    "optimusslime333"
                };

            this.questGiver = QuestGivers[random.Next(QuestGivers.Count)];

            changeFavoriteState = new RelayCommand(o =>
            {
                this.favorite = !this.favorite;
                SharedData.ServerBrowser.ChangeFavorite(this.serverIndex);
            });
            Password = password;
        }

        public void pingServer()
        {
            IPResolver resolver = new IPResolver();
            string ipToUse = resolver.ResolveIP(this.IP);

            IPEndPoint ip = new IPEndPoint(IPAddress.Parse(ipToUse), this.Port);
            Socket s = new Socket(AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp);

            this.status = ServerStatus.Pinging;

            var result = s.BeginConnect(ip, null, null);

            bool success = result.AsyncWaitHandle.WaitOne(500, true);

            if (!success)
            {
                s.Close();
                setAsOffline();
                this.status = ServerStatus.Offline;
                return;
            }

            Stopwatch pingTimer = Stopwatch.StartNew();

            int SIZE = 6144;

            byte[] info = new byte[SIZE];

            List<byte> data = new List<byte>() { 0x01 };

            data.AddRange(Encoding.UTF8.GetBytes(this.Password));

            while (data.Count < SIZE)
                data.Add(0);

            s.SendTimeout = 2500;
            s.ReceiveTimeout = 2500;

            try
            {
                s.Send(data.ToArray());

                s.Receive(info, 0, info.Length, 0);
            }
            catch
            {
                setAsOffline();
                this.status = ServerStatus.Offline;
                s.Close();
                return;
            }

            ServerDataDTO PingResult = JsonConvert.DeserializeObject<ServerDataDTO>(Encoding.UTF8.GetString(info));

            if(!PingResult.CorrectPassword)
            {
                setAsOffline();
                this.status = ServerStatus.WrongPassword;
                s.Close();
                return;
            }

            this.description = PingResult.Description;
            this.capacity = PingResult.PlayerLimit;
            this.playStyle = PingResult.Gamemode;
            this.playerList = PingResult.PlayerList.Names;
            this.status = ServerStatus.Online;
            this.open = true;

            CallGetterUpdates();

            s.Close();

            ping = (int)pingTimer.ElapsedMilliseconds;
        }

        public void setAsOffline()
        {
            open = false;
            description = "";
            capacity = 0;
            playStyle = "";
            this.playerList = new Dictionary<byte, string>();
            CallGetterUpdates();
        }

        public void CallGetterUpdates()
        {
            connectedPlayersString = "";
            playerListWithNumber = new List<string>();
            shouldDisplayPlayersTooltip = false;
            TooltipColumnCount = 0;
            pingData = "";
            pingColor = new SolidColorBrush();
            connectMessage = "";
        }
    }
}
