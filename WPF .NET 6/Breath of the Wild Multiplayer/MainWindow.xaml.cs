using Breath_of_the_Wild_Multiplayer.MVVM.ViewModel;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using DiscordRPC;

namespace Breath_of_the_Wild_Multiplayer
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    
    public class DiscordRichPresence
    {
        public static DiscordRpcClient client;
    
        public static void Initialize()
        {
            client = new DiscordRpcClient("1339782150471290904");
            client.Initialize();
        }
    }

    public partial class MainWindow : Window
    {
        
        public static string VERSION = "2.2"; // the version number of the launcher

        public void HideTextVersion()
        {
            //VersionText.Visibility = Visibility.Collapsed; // hide the version text
        }

        public MainWindow()
        {
            CopyAppdataFiles();

            if(Properties.Settings.Default.bcmlLocation == "")
            {
                string Local = Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData);

                if (File.Exists($"{Local}/bcml/settings.json"))
                {
                    Properties.Settings.Default.bcmlLocation = $"{Local}/bcml/settings.json";
                    Properties.Settings.Default.Save();
                }
            }

            Asserts.Asserts_function(); // call the asserts function

            if (Properties.Settings.Default.UseRPC)
            {
                DiscordRichPresence.Initialize();
            }

            InitializeComponent();

            var viewModel = DataContext as MainViewModel;

            if(viewModel != null )
            {
                viewModel.Window = this;
            }
        }

        protected void CloseClick(object sender, RoutedEventArgs e)
        {
            if (DiscordRichPresence.client != null)
            {
                DiscordRichPresence.client.ClearPresence();
                DiscordRichPresence.client.Dispose();
            }
            Close();
        }

        protected void MinimizeClick(object sender, RoutedEventArgs e)
        {
            WindowState = WindowState.Minimized;
        }

        protected void titleBarDrag(object sender, MouseButtonEventArgs e)
        {
            if (Mouse.LeftButton == MouseButtonState.Pressed)
            {
                DragMove();
            }
        }

        private void CopyAppdataFiles()
        {
            string AppdataFolder = Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData) + "\\BOTWM";
            List<string> Resources = Assembly.GetExecutingAssembly().GetManifestResourceNames().Where(resource => resource.Contains("AppdataFiles")).ToList();

            if (!Directory.Exists(AppdataFolder))
                Directory.CreateDirectory(AppdataFolder);

            foreach (string resource in Resources)
            {
                Stream s = Assembly.GetExecutingAssembly().GetManifestResourceStream(resource);
                string output = $"{AppdataFolder}\\{resource.Replace("Breath_of_the_Wild_Multiplayer.AppdataFiles.", "")}";
                using (FileStream AppdataFile = new FileStream(output, FileMode.Create))
                {
                    byte[] b = new byte[s.Length + 1];
                    s.Read(b, 0, Convert.ToInt32(s.Length));
                    AppdataFile.Write(b, 0, Convert.ToInt32(b.Length - 1));
                }
            }
        }

        private void TextBox_TextChanged(object sender, TextChangedEventArgs e)
        {

        }
    }
}
