namespace BOTW.Logging
{
    public static class Logger
    {
        public enum LogLevelEnum : int
        {
            INFO = 2,
            WARNING = 3,
            DEBUG = 4
        }

        public enum LogWriteLevelEnum : int
        {
            CRT = 0,
            ERR = 1,
            INF = 2,
            WRN = 3,
            DBG = 4
        }

        private static LogLevelEnum CMDLevel;
        private static LogLevelEnum LogLevel;

        private static string CurrentDirectory = Directory.GetCurrentDirectory();
        private static string LogPath = $"{CurrentDirectory}\\LatestLog.txt";
        private static string LogsFolder = $"{CurrentDirectory}\\Logs";

        private static Mutex logMutex = new Mutex();

        public static void Start(LogLevelEnum logLevel, LogLevelEnum cmdLevel)
        {
            LogLevel = logLevel;
            CMDLevel = cmdLevel;

            SetupLogFile();
        }

        public static void LogCritical(string message, string details = "", ConsoleColor color = ConsoleColor.DarkRed) => WriteToLog(LogWriteLevelEnum.CRT, message, details, color);
        public static void LogError(string message, string details = "", ConsoleColor color = ConsoleColor.DarkRed) => WriteToLog(LogWriteLevelEnum.ERR, message, details, color);
        public static void LogInformation(string message, string details = "", ConsoleColor color = ConsoleColor.White) => WriteToLog(LogWriteLevelEnum.INF, message, details, color);
        public static void LogWarning(string message, string details = "", ConsoleColor color = ConsoleColor.DarkYellow) => WriteToLog(LogWriteLevelEnum.WRN, message, details, color);
        public static void LogDebug(string message, string details = "", ConsoleColor color = ConsoleColor.Magenta) => WriteToLog(LogWriteLevelEnum.DBG, message, details, color);

        public static string LogInput(string message)
        {
            if(!string.IsNullOrEmpty(message))
                WriteToLog(LogWriteLevelEnum.INF, message, "", ConsoleColor.White, false);

            string userInput = Console.ReadLine();

            using (StreamWriter LogFile = new StreamWriter(LogPath, true))
            {
                LogFile.WriteLine(userInput);
            }

            return userInput;
        }

        private static void SetupLogFile()
        {
            // Ensure the Logs folder exists
            if (!Directory.Exists(LogsFolder))
                Directory.CreateDirectory(LogsFolder);
        
            // Move the existing LatestLog.txt to the Logs folder with a timestamp
            if (File.Exists(LogPath))
            {
                string previousLogTime = File.GetCreationTime(LogPath).ToString("yyyy-MM-dd, HH-mm-ss");
                string previousLogPath = $"{LogsFolder}\\{previousLogTime}.txt";
        
                if (File.Exists(previousLogPath))
                    File.Delete(previousLogPath);
        
                File.Move(LogPath, previousLogPath);
            }
        
            // Always create a new log file for the current session
            string creationTime = DateTime.Now.ToString("yyyy-MM-dd, HH-mm-ss");
            string newLogPath = $"{LogsFolder}\\{creationTime}.txt";
            using (File.CreateText(newLogPath)) { }
        
            // Point LatestLog.txt to the new log file
            LogPath = newLogPath;
        
            // Get all log files and keep only the 5 most recent ones
            var logFiles = Directory.GetFiles(LogsFolder, "*.txt")
                                    .OrderByDescending(f => File.GetCreationTime(f))
                                    .ToList();
        
            while (logFiles.Count > 10)
            {
                File.Delete(logFiles.Last());
                logFiles.RemoveAt(logFiles.Count - 1);
            }
        }

        private static void WriteToLog(LogWriteLevelEnum writeLevel, string message, string details, ConsoleColor color, bool newLine = true, bool writeToCMD = true)
        {
            string MessageTime = DateTime.Now.ToString("HH:mm:ss");

            if(writeToCMD)
                WriteToConsole(writeLevel, message, MessageTime, color, newLine);

            if ((int)writeLevel > (int)LogLevel)
                return;

            logMutex.WaitOne(100);

            using (StreamWriter LogFile = new StreamWriter(LogPath, true))
            {
                LogFile.Write($"[{MessageTime}] {message}");

                if (!string.IsNullOrEmpty(details))
                    LogFile.Write($" - Details: {details}");

                if(newLine)
                    LogFile.WriteLine();
            }

            logMutex.ReleaseMutex();
        }

        private static void WriteToConsole(LogWriteLevelEnum writeLevel, string message, string datetime, ConsoleColor color, bool newLine)
        {
            if ((int)writeLevel > (int)CMDLevel)
                return;

            Console.ForegroundColor = ConsoleColor.Gray;

            Console.Write($"[{datetime}] ");

            Console.ForegroundColor = color;

            if (newLine)
                Console.WriteLine(message);
            else
                Console.Write(message);


            Console.ForegroundColor = ConsoleColor.White;
        }
    }
}
