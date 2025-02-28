using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Runtime.ConstrainedExecution;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using Breath_of_the_Wild_Multiplayer.MVVM.Model;
using Breath_of_the_Wild_Multiplayer.MVVM.ViewModel;
using DiscordRPC;

namespace Breath_of_the_Wild_Multiplayer.MVVM.View
{
    /// <summary>
    /// Interaction logic for SettingsPanelView.xaml
    /// </summary>
    public partial class SettingsPanelView : UserControl
    {
        public string VersionInText => $"V{MainWindow.VERSION}";
        public SettingsPanelView()
        {
            InitializeComponent();
            if (DiscordRichPresence.client != null)
            {
                DiscordRichPresence.updateRPC(false, "Model Selection", "", 0, 0);
            }
        }

        private void Hyperlink_RequestNavigate(object sender, RequestNavigateEventArgs e)
        {
            //Process.Start(new ProcessStartInfo(e.Uri.AbsoluteUri));
            Process.Start(new ProcessStartInfo(e.Uri.AbsoluteUri) { UseShellExecute = true });
            e.Handled = true;
        }

        private void TextBox_TextChanged(object sender, TextChangedEventArgs e)
        {
            if (string.IsNullOrWhiteSpace(((TextBox)sender).Text))
            {
                Properties.Settings.Default.playerName = "link";
                Properties.Settings.Default.Save();
                return;
            }

            bool isNameValid = ((TextBox)sender).Text.All(c => c <= 127 && c != ';');

            if (isNameValid)
            {
                Properties.Settings.Default.playerName = ((TextBox)sender).Text;
                Properties.Settings.Default.Save();
            }
            else
            {
                var ErrorMessage = new ErrorMessageModel();
                ErrorMessage.Message = $"Player name contains non-ascii characters";
                SharedData.MainView.updateTopView(ErrorMessage);
                Properties.Settings.Default.playerName = "link";
                Properties.Settings.Default.Save();
            }
        }
    }
}
