using Breath_of_the_Wild_Multiplayer.MVVM.ViewModel;
using DiscordRPC;
using System;
using System.Collections.Generic;
using System.Linq;
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

namespace Breath_of_the_Wild_Multiplayer.MVVM.View
{
    /// <summary>
    /// Lógica de interacción para ModelSelectView.xaml
    /// </summary>
    public partial class ModelSelectView : UserControl
    {
        public ModelSelectView()
        {
            InitializeComponent();
            ModelSelectModel dataContext = (ModelSelectModel)this.DataContext;
            dataContext.BodyImage = BodyPicture;
            dataContext.UpdateBodyImage();
            if (DiscordRichPresence.client != null)
            {
                DiscordRichPresence.updateRPC("Model Selection", "");
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
    }
}
