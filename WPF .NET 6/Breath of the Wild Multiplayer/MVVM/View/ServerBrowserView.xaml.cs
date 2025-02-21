using Breath_of_the_Wild_Multiplayer.MVVM.ViewModel;
using DiscordRPC;
using System.Windows.Controls;

namespace Breath_of_the_Wild_Multiplayer.MVVM.View
{
    /// <summary>
    /// Interaction logic for ServerBrowser.xaml
    /// </summary>
    public partial class ServerBrowser : UserControl
    {
        public ServerBrowser()
        {
            InitializeComponent();
            if (DiscordRichPresence.client != null)
            {
                DiscordRichPresence.updateRPC("Lobby Browser", "");
            }
        }

        private void ScrollViewer_ScrollChanged(object sender, ScrollChangedEventArgs e)
        {

            ScrollViewer send = (ScrollViewer)sender;

            if (send.VerticalOffset == 0 && send.VerticalOffset == send.ScrollableHeight)
                send.Tag = 0; // Full
            else if (send.VerticalOffset == 0 && send.VerticalOffset != send.ScrollableHeight)
                send.Tag = 1; // Top
            else if (send.VerticalOffset != 0 && send.VerticalOffset != send.ScrollableHeight)
                send.Tag = 2; // Middle
            else if (send.VerticalOffset != 0 && send.VerticalOffset == send.ScrollableHeight)
                send.Tag = 3; // Bottom

        }

        private void EditClick(object sender, System.Windows.RoutedEventArgs e)
        {
            ((ServerBrowserModel)this.DataContext).EditServer();
        }

        private void RemoveClick(object sender, System.Windows.RoutedEventArgs e)
        {
            ((ServerBrowserModel)this.DataContext).RemoveServer();
        }
    }
}
