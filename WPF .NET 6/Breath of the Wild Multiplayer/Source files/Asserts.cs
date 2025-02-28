using System;
using PythonCheckerApp;
using Errorpopup;
using System.IO;
using Breath_of_the_Wild_Multiplayer.MVVM.ViewModel;
using Newtonsoft.Json;
using System.Collections.Generic;
using OneDriveApp;
using Breath_of_the_Wild_Multiplayer.MVVM.View;

namespace Breath_of_the_Wild_Multiplayer
{
    public class Asserts
    {
        private static string CemuDir;
        public static void Asserts_function()
        {
            string Local = Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData);
            string bcmlPath = Breath_of_the_Wild_Multiplayer.Properties.Settings.Default.bcmlLocation.Replace("settings.json", @"merged\content");
            var pythonChecker = new PythonChecker();
            bool isFromStore = pythonChecker.IsPythonFromMicrosoftStore();

            if (isFromStore && Breath_of_the_Wild_Multiplayer.Properties.Settings.Default.PythonStoreWarning)
            {
                var PythonWarning = new PythonWarning();
                PythonWarning.ShowDialog();
            }

            if (!File.Exists($"{Local}/bcml/settings.json"))
            {
                var errorpopup = new Error();
                Error.ShowErrorPopup("BCML is not installed, or the data directory is modified. TIP: The BNP files need to be installed on the same drive that your python installation is in (Most likely the C: Drive)");
                Environment.Exit(0);

            }

            if (!Directory.Exists($"{bcmlPath}/Model"))
            {
                Error.ShowErrorPopup("The mods are not correctly installed in bcml. TIP: The BNP files need to be installed on the same drive that your python installation is in (Most likely the C: Drive)");
                Environment.Exit(0);
            }

            // get the cemu path
            try
            {
                string CemuSettings = File.ReadAllText(Properties.Settings.Default.bcmlLocation);
                Dictionary<string, string> settings = JsonConvert.DeserializeObject<Dictionary<string, string>>(CemuSettings)!;
                CemuDir = settings["cemu_dir"];
            }
            catch (Exception ex)
            {
                CemuDir = "";
            }

            //--------------------------------

            if (CemuDir.Contains("OneDrive") && Breath_of_the_Wild_Multiplayer.Properties.Settings.Default.CemuOneDrive)
            {
                var OneDrivewarning = new OneDrive();
                OneDrivewarning.ShowDialog();
            }

            if (AppDomain.CurrentDomain.BaseDirectory.Contains("OneDrive") && Breath_of_the_Wild_Multiplayer.Properties.Settings.Default.CemuOneDrive)
            {
                var OneDrivewarning = new OneDrive();
                OneDrivewarning.ShowDialog();
            }

            string[] dirs = Directory.GetDirectories($"{Local}/bcml/mods");
            if (dirs.Length > 3 && Breath_of_the_Wild_Multiplayer.Properties.Settings.Default.ModsNumberWarning)
            {
                var ModsNumberWarning = new ModsNumberWarning();
                ModsNumberWarning.ShowDialog();
            }


        }
    }
}