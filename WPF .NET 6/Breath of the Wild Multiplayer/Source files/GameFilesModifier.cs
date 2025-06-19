using CsYaz0;
using Microsoft.Win32;
using Newtonsoft.Json;
using Nintendo.Bfres;
using SarcWrapper;
using SarcWrapper.Actor.Pack.ActorLink;
using SarcWrapper.Actor.Pack.ASList;
using SarcWrapper.Actor.Pack.ModelList;
using SarcWrapper.Pack;
using SarcWrapper.SarcTypes;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Threading.Tasks;
using System.Windows.Controls;
using System.Windows.Media.Media3D;
using YamlDotNet.RepresentationModel;

namespace Breath_of_the_Wild_Multiplayer.Source_files
{
    public static class GameFilesModifier
    {
        public struct AampLibAction
        {
            public string Data;
            public string Instruction;
        }

        public static void ChangeAttentionForJugadores(bool enabled)
        {
            string ModLoaderPath = "";
            // Get bcml mod path to save data in that folder
            if (Properties.Settings.Default.UseBcml)
            {
                ModLoaderPath = Properties.Settings.Default.bcmlLocation.Replace("settings.json", @"merged\content");
            }
            else
            {
                string ukmmModLocation = "";
                using (StreamReader reader = new StreamReader(Properties.Settings.Default.ukmmSettingLocation))
                {
                    YamlStream yaml = new YamlStream();
                    yaml.Load(reader);

                    YamlMappingNode root = (YamlMappingNode)yaml.Documents[0].RootNode;

                    if (root.Children.TryGetValue(new YamlScalarNode("storage_dir"), out YamlNode storageDirNode))
                    {
                        ukmmModLocation = storageDirNode.ToString();
                    }
                }
                ModLoaderPath = $@"{ukmmModLocation}\wiiu\profiles\Default\merged\content";
            }

            List<Tuple<ISarcFile, AampLibAction>> filesToModify = new List<Tuple<ISarcFile, AampLibAction>>();

            List<ActorPack> jugadores = new List<ActorPack>();

            foreach (string path in Directory.GetFiles(@$"{ModLoaderPath}\Actor\Pack\").Where(file => Path.GetFileName(file).StartsWith("Jugador")))
            {
                ActorPack jugador = new ActorPack(path);
                ActorLink actorLinkFile = jugador.ActorLink.Files()[0];

                filesToModify.Add(new Tuple<ISarcFile, AampLibAction>(actorLinkFile, new AampLibAction()
                {
                    Data = actorLinkFile.GetByteString(),
                    Instruction = $"l(param_root).o(LinkTarget).v(AttentionUser,{(enabled ? actorLinkFile.FileName : "Dummy")})"
                }));

                jugadores.Add(jugador);
            }

            RunAampLib(filesToModify);
            jugadores.ForEach(jugador => jugador.Save(true));
        }

        public static void CreateCombinedArmors()
        {
            Stopwatch stopwatch = Stopwatch.StartNew();

            /* Obtaining the data */
            // Get game folders so that we can extract the armors and files


            string GameDir = "";
            string UpdateDir = "";

            string ModLoaderPath = "";
            // Get bcml mod path to save data in that folder
            if (Properties.Settings.Default.UseBcml)
            {
                ModLoaderPath = Properties.Settings.Default.bcmlLocation.Replace("settings.json", @"merged\content");
                string CemuSettings = File.ReadAllText(Properties.Settings.Default.bcmlLocation);
                Dictionary<string, string> settings = JsonConvert.DeserializeObject<Dictionary<string, string>>(CemuSettings)!;
                GameDir = settings["game_dir"];
                UpdateDir = settings["update_dir"];
            }
            else
            {
                string ukmmModLocation = "";
                using (StreamReader reader = new StreamReader(Properties.Settings.Default.ukmmSettingLocation))
                {
                    YamlStream yaml = new YamlStream();
                    yaml.Load(reader);

                    YamlMappingNode root = (YamlMappingNode)yaml.Documents[0].RootNode;

                    if (root.Children.TryGetValue(new YamlScalarNode("storage_dir"), out YamlNode storageDirNode))
                    {
                        ukmmModLocation = storageDirNode.ToString();
                    }

                    // move to "wiiu_config"
                    if (root.Children.TryGetValue(new YamlScalarNode("wiiu_config"), out var wiiuConfigNode) &&
                        wiiuConfigNode is YamlMappingNode wiiuConfigMapping)
                    {
                        // move to "dump" > "source"
                        if (wiiuConfigMapping.Children.TryGetValue(new YamlScalarNode("dump"), out var dumpNode) &&
                            dumpNode is YamlMappingNode dumpMapping &&
                            dumpMapping.Children.TryGetValue(new YamlScalarNode("source"), out var sourceNode) &&
                            sourceNode is YamlMappingNode sourceMapping)
                        {

                            if (sourceMapping.Children.TryGetValue(new YamlScalarNode("content_dir"), out var contentDirNode))
                            {
                                GameDir = contentDirNode.ToString();
                            }

                            if (sourceMapping.Children.TryGetValue(new YamlScalarNode("update_dir"), out var updateDirNode))
                            {
                                UpdateDir = updateDirNode.ToString();
                            }
                        }
                    }
                }

                ModLoaderPath = $@"{ukmmModLocation}\wiiu\profiles\Default\merged\content";
            }

            string titleBGPath = @$"{ModLoaderPath}\Pack\TitleBG.pack";

            /* Instantiate objects */
            // Create title bg object and load it's data
            TitleBG titleBG = new TitleBG(titleBGPath);
            titleBG.GetLoadedData(); // Load data

            List<Model> totalModels = new List<Model>();

            /* Get armor models */
            foreach (string path in Directory.GetFiles(@$"{ModLoaderPath}\Model\").Where(file => Path.GetFileName(file).StartsWith("Armor_") && !file.Contains("Tex")))
            {
                BfresFile armorFile = new BfresFile(new MemoryStream(Yaz0.Decompress(File.ReadAllBytes(path))));

                totalModels.AddRange(GetArmorModels(armorFile));
            }

            byte[] LinkDecompressed = Yaz0.Decompress(titleBG.LoadedData["Model/Link.sbfres"].ToArray());

            IEnumerable<Model> headModels = totalModels.Where(m => m.Name.Contains("Head"));
            IEnumerable<Model> upperModels = totalModels.Where(m => m.Name.Contains("Upper"));
            IEnumerable<Model> lowerModels = totalModels.Where(m => m.Name.Contains("Lower"));

            int filesSaved = 0;

            foreach (var h in headModels)
            {
                string headNumber = h.UserData["ArmorNumber"].GetValueStringArray()[0];

                Dictionary<string, BfresFile> fileToSave = new Dictionary<string, BfresFile>();

                Debug.WriteLine($"Saved {filesSaved} files in {stopwatch.ElapsedMilliseconds/1000} seconds");

                Parallel.ForEach(upperModels, u =>
                {
                    string upperNumber = u.UserData["ArmorNumber"].GetValueStringArray()[0];

                    foreach (var l in lowerModels)
                    {
                        string lowerNumber = l.UserData["ArmorNumber"].GetValueStringArray()[0];

                        BfresFile outputModel = new BfresFile(new MemoryStream(LinkDecompressed));

                        string fileName = $"MPArmor_{headNumber}_{upperNumber}_{lowerNumber}";

                        outputModel.Models.Add("Head", h);
                        outputModel.Models.Add("Upper", u);
                        outputModel.Models.Add("Lower", l);

                        outputModel.Name = fileName;

                        fileToSave.Add($@"{ModLoaderPath}/Model/Test/{fileName}.sbfres", outputModel);
                    }
                });

                foreach(var f in fileToSave)
                {
                    using MemoryStream ms = new();
                    {
                        f.Value.ToBinary(ms);
                    }

                    File.WriteAllBytes(f.Key, Yaz0.Compress(ms.ToArray()).ToArray());
                }

                filesSaved += fileToSave.Count;
            }
        }

        public static void CreateModifiedModel()
        {
            /* Obtaining the data */
            // Get game folders so that we can extract the armors and files
            string GameDir = "";
            string UpdateDir = "";

            string ModLoaderPath = "";
            // Get bcml mod path to save data in that folder
            if (Properties.Settings.Default.UseBcml)
            {
                ModLoaderPath = Properties.Settings.Default.bcmlLocation.Replace("settings.json", @"merged\content");
                string CemuSettings = File.ReadAllText(Properties.Settings.Default.bcmlLocation);
                Dictionary<string, string> settings = JsonConvert.DeserializeObject<Dictionary<string, string>>(CemuSettings)!;
                GameDir = settings["game_dir"];
                UpdateDir = settings["update_dir"];
            }
            else
            {
                string ukmmModLocation = "";
                using (StreamReader reader = new StreamReader(Properties.Settings.Default.ukmmSettingLocation))
                {
                    YamlStream yaml = new YamlStream();
                    yaml.Load(reader);

                    YamlMappingNode root = (YamlMappingNode)yaml.Documents[0].RootNode;

                    if (root.Children.TryGetValue(new YamlScalarNode("storage_dir"), out YamlNode storageDirNode))
                    {
                        ukmmModLocation = storageDirNode.ToString();
                    }

                    // move to "wiiu_config"
                    if (root.Children.TryGetValue(new YamlScalarNode("wiiu_config"), out var wiiuConfigNode) &&
                        wiiuConfigNode is YamlMappingNode wiiuConfigMapping)
                    {
                        // move to "dump" > "source"
                        if (wiiuConfigMapping.Children.TryGetValue(new YamlScalarNode("dump"), out var dumpNode) &&
                            dumpNode is YamlMappingNode dumpMapping &&
                            dumpMapping.Children.TryGetValue(new YamlScalarNode("source"), out var sourceNode) &&
                            sourceNode is YamlMappingNode sourceMapping)
                        {

                            if (sourceMapping.Children.TryGetValue(new YamlScalarNode("content_dir"), out var contentDirNode))
                            {
                                GameDir = contentDirNode.ToString();
                            }

                            if (sourceMapping.Children.TryGetValue(new YamlScalarNode("update_dir"), out var updateDirNode))
                            {
                                UpdateDir = updateDirNode.ToString();
                            }
                        }
                    }
                }
                ModLoaderPath = $@"{ukmmModLocation}\wiiu\profiles\Default\merged\content";
            }

            string titleBGPath = @$"{UpdateDir}\Pack\TitleBG.pack";

            /* Instantiate objects */
            // Create title bg object and load it's data
            TitleBG titleBG = new TitleBG(titleBGPath);
            titleBG.GetLoadedData(); // Load data

            /************ Player model ************/
            /* Modify Link models */
            // Create outputModel
            if(!File.Exists($@"{ModLoaderPath}/Model/Jugador1ModelNameLongForASpecificReason.sbfres"))
            {
                byte[] LinkDecompressed = Yaz0.Decompress(titleBG.LoadedData["Model/Link.sbfres"].ToArray());
                BfresFile outputModel = new BfresFile(new MemoryStream(LinkDecompressed));

                // Modify model for head
                outputModel.Models.Add("Jugador1ModelNameLongForASpecificReasonHead", outputModel.Models[0]);
                outputModel.Models.RemoveAt(0);
                outputModel.Models[0].Name = "Jugador1ModelNameLongForASpecificReasonHead";
                List<string> shapesToRemove = outputModel.Models[0].Shapes.Keys.Where(k => k.Contains("Skin")).ToList();
                foreach (string shape in shapesToRemove)
                    outputModel.Models[0].Shapes.RemoveKey(shape);

                // Modify model for chest
                outputModel.Models.Add("Jugador1ModelNameLongForASpecificReasonChest", new BfresFile(new MemoryStream(LinkDecompressed)).Models[0]); // Sadly we need to reopen the file to create a copy
                outputModel.Models[1].Name = "Jugador1ModelNameLongForASpecificReasonChest";
                shapesToRemove = outputModel.Models[1].Shapes.Keys.Where(k => !k.Contains("Skin") || !k.Contains("Upper")).ToList();
                foreach (string shape in shapesToRemove)
                    outputModel.Models[1].Shapes.RemoveKey(shape);

                // Modify model to be empty
                outputModel.Models.Add("EmptyModel", new BfresFile(new MemoryStream(LinkDecompressed)).Models[0]); // Sadly we need to reopen the file to create a copy
                outputModel.Models[2].Name = "EmptyModel";
                outputModel.Models[2].Shapes.Clear();

                /* Get armor models */
                foreach (string path in Directory.GetFiles(@$"{UpdateDir}\Model\").Where(file => Path.GetFileName(file).StartsWith("Armor_") && !file.Contains("Tex")))
                {
                    BfresFile armorFile = new BfresFile(new MemoryStream(Yaz0.Decompress(File.ReadAllBytes(path))));

                    List<Model> models = GetArmorModels(armorFile);

                    foreach (Model model in models)
                    {
                        outputModel.Models.Add(model.Name, model);
                    }
                }

                byte[] DefaultArmorDecompressed = Yaz0.Decompress(titleBG.LoadedData["Model/Armor_Default.sbfres"].ToArray());
                BfresFile defaultArmorFile = new BfresFile(new MemoryStream(DefaultArmorDecompressed));

                foreach (Model model in GetArmorModels(defaultArmorFile))
                {
                    outputModel.Models.Add(model.Name, model);
                }

                outputModel.Name = "Jugador1ModelNameLongForASpecificReason";

                using MemoryStream ms = new();
                {
                    outputModel.ToBinary(ms);
                }

                File.WriteAllBytes($@"{ModLoaderPath}/Model/Jugador1ModelNameLongForASpecificReason.sbfres", Yaz0.Compress(ms.ToArray()).ToArray());
            }

            /************ Player textures ************/
            /* Modify Link's tex1 file */
            if(!File.Exists($@"{ModLoaderPath}/Model/Jugador1ModelNameLongForASpecificReason.Tex1.sbfres"))
            {
                byte[] LinkDecompressed = Yaz0.Decompress(File.ReadAllBytes($@"{GameDir}\Model\Link.Tex1.sbfres"));
                BfresFile outputTexture = new BfresFile(new MemoryStream(LinkDecompressed));

                /* Get armor models from game dir*/
                foreach (string path in Directory.GetFiles(@$"{GameDir}\Model\").Where(file => Path.GetFileName(file).StartsWith("Armor_") && file.Contains("Tex1")))
                {
                    BfresFile armorFile = new BfresFile(new MemoryStream(Yaz0.Decompress(File.ReadAllBytes(path))));

                    foreach (var tex in armorFile.Textures)
                    {
                        outputTexture.Textures.Add(tex.Key, tex.Value);
                    }
                }

                /* Get armor models from update dir*/
                foreach (string path in Directory.GetFiles(@$"{UpdateDir}\Model\").Where(file => Path.GetFileName(file).StartsWith("Armor_") && file.Contains("Tex1")))
                {
                    BfresFile armorFile = new BfresFile(new MemoryStream(Yaz0.Decompress(File.ReadAllBytes(path))));

                    foreach (var tex in armorFile.Textures)
                    {
                        outputTexture.Textures.Add(tex.Key, tex.Value);
                    }
                }

                outputTexture.Name = "Jugador1ModelNameLongForASpecificReason";

                using MemoryStream ms = new();
                {
                    outputTexture.ToBinary(ms);
                }

                File.WriteAllBytes($@"{ModLoaderPath}/Model/Jugador1ModelNameLongForASpecificReason.Tex1.sbfres", Yaz0.Compress(ms.ToArray()).ToArray());
            }

            /* Modify Link's tex2 file */
            if (!File.Exists($@"{ModLoaderPath}/Model/Jugador1ModelNameLongForASpecificReason.Tex2.sbfres"))
            {
                byte[] LinkDecompressed = Yaz0.Decompress(titleBG.LoadedData["Model/Link.Tex2.sbfres"].ToArray());
                BfresFile outputTexture = new BfresFile(new MemoryStream(LinkDecompressed));

                /* Get armor models */
                foreach (string path in Directory.GetFiles(@$"{UpdateDir}\Model\").Where(file => Path.GetFileName(file).StartsWith("Armor_") && file.Contains("Tex2")))
                {
                    BfresFile armorFile = new BfresFile(new MemoryStream(Yaz0.Decompress(File.ReadAllBytes(path))));

                    foreach (var tex in armorFile.Textures)
                    {
                        outputTexture.Textures.Add(tex.Key, tex.Value);
                    }
                }

                byte[] DefaultArmorDecompressed = Yaz0.Decompress(titleBG.LoadedData["Model/Armor_Default.Tex2.sbfres"].ToArray());
                BfresFile defaultArmor = new BfresFile(new MemoryStream(DefaultArmorDecompressed));

                foreach(var tex in defaultArmor.Textures)
                {
                    outputTexture.Textures.Add(tex.Key, tex.Value);
                }

                outputTexture.Name = "Jugador1ModelNameLongForASpecificReason";

                using MemoryStream ms = new();
                {
                    outputTexture.ToBinary(ms);
                }

                File.WriteAllBytes($@"{ModLoaderPath}/Model/Jugador1ModelNameLongForASpecificReason.Tex2.sbfres", Yaz0.Compress(ms.ToArray()).ToArray());
            }
        }

        private static List<Model> GetArmorModels(BfresFile file)
        {
            List<Model> result = new List<Model>();

            // Get armor head
            if(file.Models.Keys.Any(k => k.Contains("Head")))
            {
                Model headModel = file.Models[file.Models.Keys.Where(k => k.Contains("Head")).First()];
                string armorNumber = headModel.Name.Split("_")[1];
                headModel.Name = $"MP_{headModel.Name.Replace("_A", "").Replace("_B", "")}";
                UserData userData = new UserData() { Name = "ArmorNumber" };
                userData.SetValue([armorNumber]);
                headModel.UserData.Add(userData.Name, userData);
                result.Add(headModel);
            }

            // Get armor chest
            if(file.Models.Keys.Any(k => k.Contains("Upper")))
            {
                Model upperModel = file.Models[file.Models.Keys.Where(k => k.Contains("Upper")).First()];
                string armorNumber = upperModel.Name.Split("_")[1];
                upperModel.Name = $"MP_{upperModel.Name.Replace("_A", "").Replace("_B", "")}";
                UserData userData = new UserData() { Name = "ArmorNumber" };
                userData.SetValue([armorNumber]);
                upperModel.UserData.Add(userData.Name, userData);
                result.Add(upperModel);
            }

            // Get armor leggings
            if(file.Models.Keys.Any(k => k.Contains("Lower")))
            {
                Model lowerModel = file.Models[file.Models.Keys.Where(k => k.Contains("Lower")).First()];
                string armorNumber = lowerModel.Name.Split("_")[1];
                lowerModel.Name = $"MP_{lowerModel.Name.Replace("_A", "").Replace("_B", "")}";
                UserData userData = new UserData() { Name = "ArmorNumber" };
                userData.SetValue([armorNumber]);
                lowerModel.UserData.Add(userData.Name, userData);
                result.Add(lowerModel);
            }

            return result;
        }

        public static void CleanAnimations()
        {
            /* Obtaining the data */
            // Get game folders so that we can extract the armors and files
            string GameDir = "";
            string UpdateDir = "";

            string ModLoaderPath = "";
            // Get bcml mod path to save data in that folder
            if (Properties.Settings.Default.UseBcml)
            {
                ModLoaderPath = Properties.Settings.Default.bcmlLocation.Replace("settings.json", @"merged\content");
                string CemuSettings = File.ReadAllText(Properties.Settings.Default.bcmlLocation);
                Dictionary<string, string> settings = JsonConvert.DeserializeObject<Dictionary<string, string>>(CemuSettings)!;
                GameDir = settings["game_dir"];
                UpdateDir = settings["update_dir"];
            }
            else
            {
                string ukmmModLocation = "";
                using (StreamReader reader = new StreamReader(Properties.Settings.Default.ukmmSettingLocation))
                {
                    YamlStream yaml = new YamlStream();
                    yaml.Load(reader);

                    YamlMappingNode root = (YamlMappingNode)yaml.Documents[0].RootNode;

                    if (root.Children.TryGetValue(new YamlScalarNode("storage_dir"), out YamlNode storageDirNode))
                    {
                        ukmmModLocation = storageDirNode.ToString();
                    }

                    // move to "wiiu_config"
                    if (root.Children.TryGetValue(new YamlScalarNode("wiiu_config"), out var wiiuConfigNode) &&
                        wiiuConfigNode is YamlMappingNode wiiuConfigMapping)
                    {
                        // move to "dump" > "source"
                        if (wiiuConfigMapping.Children.TryGetValue(new YamlScalarNode("dump"), out var dumpNode) &&
                            dumpNode is YamlMappingNode dumpMapping &&
                            dumpMapping.Children.TryGetValue(new YamlScalarNode("source"), out var sourceNode) &&
                            sourceNode is YamlMappingNode sourceMapping)
                        {

                            if (sourceMapping.Children.TryGetValue(new YamlScalarNode("content_dir"), out var contentDirNode))
                            {
                                GameDir = contentDirNode.ToString();
                            }

                            if (sourceMapping.Children.TryGetValue(new YamlScalarNode("update_dir"), out var updateDirNode))
                            {
                                UpdateDir = updateDirNode.ToString();
                            }
                        }
                    }
                }
                ModLoaderPath = $@"{ukmmModLocation}\wiiu\profiles\Default\merged\content";
            }

            if (!File.Exists($@"{ModLoaderPath}/Model/Player_Animation_NoFace.sbfres"))
            {
                var res = new BfresFile(new MemoryStream(Yaz0.Decompress(File.ReadAllBytes($@"{GameDir}\Model\Player_Animation.sbfres"))));
                res.Name = "Player_Animation_NoFace";

                List<string> bannedBones = new List<string>() {
                "face",
                "cheek",
                "chin",
                "lip",
                "teeth",
                "eye",
                "eyeball",
                "eyebrow",
                "eyelid",
                "lip",
                "nose"
            };

                foreach (var f in res.SkeletalAnims.Values)
                {
                    f.BoneAnims = f.BoneAnims.Where(bAnim => !bannedBones.Any(banned => bAnim.Name.ToLower().Contains(banned))).ToList();
                }

                using MemoryStream ms = new();
                {
                    res.ToBinary(ms);
                }

                File.WriteAllBytes($@"{ModLoaderPath}/Model/Player_Animation_NoFace.sbfres", Yaz0.Compress(ms.ToArray()).ToArray());
            }
        }

        public static void WrapperTests()
        {
            //string bcmlPath = Properties.Settings.Default.bcmlLocation.Replace("settings.json", "merged\\content");

            //string titleBGPath = $"{bcmlPath}\\Pack\\TitleBG.pack";

            //TitleBG titleBG = new TitleBG(titleBGPath);
            //ActorPack gameRomPlayer = titleBG.Actor.Pack.Actors.File("GameROMPlayer");

            //gameRomPlayer.ModelList.File("Player_Link").LoadedData.ModifyModel("TestNew", "Link");
            //gameRomPlayer.ASList.File("Player").LoadedData.ModifyAnimation(true);

            //titleBG.Save(true);
        }

        public static void ModifyGameROMPlayerModel(string folder, string model, bool isNotLink, string bumiiPath = null)
        {
            List<Tuple<ISarcFile, AampLibAction>> filesToModify = new List<Tuple<ISarcFile, AampLibAction>>();

            string ModLoaderPath = "";
            if (Properties.Settings.Default.UseBcml)
            {
                ModLoaderPath = Properties.Settings.Default.bcmlLocation.Replace("settings.json", @"merged\content");
            }
            else
            {
                string ukmmModLocation = "";
                using (StreamReader reader = new StreamReader(Properties.Settings.Default.ukmmSettingLocation))
                {
                    YamlStream yaml = new YamlStream();
                    yaml.Load(reader);

                    YamlMappingNode root = (YamlMappingNode)yaml.Documents[0].RootNode;

                    if (root.Children.TryGetValue(new YamlScalarNode("storage_dir"), out YamlNode storageDirNode))
                    {
                        ukmmModLocation = storageDirNode.ToString();
                    }
                }
                ModLoaderPath = $@"{ukmmModLocation}\wiiu\profiles\Default\merged\content";
            }

            string titleBGPath = @$"{ModLoaderPath}\Pack\TitleBG.pack";

            TitleBG titleBG = new TitleBG(titleBGPath);
            ActorPack gameRomPlayer = titleBG.Actor.Pack.Actors.File("GameROMPlayer");

            filesToModify.Add(gameRomPlayer.ModelList.Files()[0].ModifyModel(folder, model));
            filesToModify.Add(gameRomPlayer.ASList.Files()[0].ModifyAnimation(isNotLink));

            // LEFTOVER CODE - Used for modifying Link's bumiis files
            //if (string.IsNullOrEmpty(bumiiPath))
            //{
            //    using (var stream = Assembly.GetExecutingAssembly().GetManifestResourceStream("Breath_of_the_Wild_Multiplayer.Resources.Dummy.bumii"))
            //    {
            //        byte[] ba = new byte[stream.Length];
            //        stream.Read(ba, 0, ba.Length);

            //        gameRomPlayer.UMii.Files()[0].SetCustomData(ba);
            //    }
            //}
            //else
            //{
            //    gameRomPlayer.UMii.Files()[0].SetCustomData(File.ReadAllBytes(bumiiPath));
            //}

            string armorFolder = isNotLink ? folder : "Armor_Default";
            string armorModelBase = isNotLink ? model : "Armor_Default_Extra_##";

            filesToModify.Add(titleBG.Actor.Pack.Actors.File("Armor_Default_Extra_00").ModelList.Files()[0].ModifyModel(armorFolder, armorModelBase.Replace("##", "00")));
            filesToModify.Add(titleBG.Actor.Pack.Actors.File("Armor_Default_Extra_01").ModelList.Files()[0].ModifyModel(armorFolder, armorModelBase.Replace("##", "01")));

            ActorPack pauseMenuActor = new ActorPack(@$"{ModLoaderPath}\Actor\Pack\PauseMenuPlayer.sbactorpack");
            filesToModify.Add(pauseMenuActor.ModelList.Files()[0].ModifyModel(folder, model));
            filesToModify.Add(pauseMenuActor.ASList.Files()[0].ModifyAnimation(isNotLink));

            List<ActorPack> armors = new List<ActorPack>();

            foreach (string path in Directory.GetFiles(@$"{ModLoaderPath}\Actor\Pack\").Where(file => Path.GetFileName(file).StartsWith("Armor_")))
            {
                ActorPack armorActor = new ActorPack(path);
                ModelList modelListFile = armorActor.ModelList.Files()[0];
                string armorFullName = modelListFile.FileName;

                if (armorFullName.Contains("011"))
                    armorFullName = armorFullName.Replace("011", "030");

                string armorNumber = armorFullName.Split("_")[1];

                if (File.Exists(path.Replace(".sbactorpack", "_B.sbactorpack")))
                    armorFullName += "_A";

                filesToModify.Add(armorActor.ModelList.Files()[0].ModifyModel(isNotLink ? folder : $"Armor_{armorNumber}", isNotLink ? model : armorFullName));

                armors.Add(armorActor);
            }

            RunAampLib(filesToModify);

            titleBG.Save(true);
            pauseMenuActor.Save(true);
            armors.ForEach(armor => armor.Save(true));
        }

        private static Tuple<ISarcFile, AampLibAction> ModifyModel(this ModelList file, string folder, string model)
        {
            return new Tuple<ISarcFile, AampLibAction>(file, new AampLibAction()
            {
                Data = file.GetByteString(),
                Instruction = $"l(param_root).l(ModelData).l(ModelData_0).o(Base).v(Folder,{folder});l(param_root).l(ModelData).l(ModelData_0).l(Unit).o(Unit_0).v(UnitName,{model})"
            });
        }

        private static Tuple<ISarcFile, AampLibAction> ModifyAnimation(this ASList file, bool isNoFace)
        {
            string extra = isNoFace ? "_NoFace" : "";

            return new Tuple<ISarcFile, AampLibAction>(file, new AampLibAction()
            {
                Data = file.GetByteString(),
                Instruction = $"l(param_root).l(AddReses).o(AddRes_0).v(Anim,Player_Animation{extra}"
            });
        }

        private static void RunAampLib(List<Tuple<ISarcFile, AampLibAction>> filesToModify)
        {
            //string byteString = string.Join(" ", byteArray.Select(b => b.ToString("X2")));
            string file = Directory.GetCurrentDirectory() + "\\Resources\\aapmLib.exe";

            string AppdataFolder = Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData) + "\\BOTWM\\Temp";

            if (!Directory.Exists(AppdataFolder))
                Directory.CreateDirectory(AppdataFolder);

            File.WriteAllText(@$"{AppdataFolder}\AampTemp.txt", JsonConvert.SerializeObject(filesToModify.Select(file => file.Item2), formatting: Formatting.Indented));

            ProcessStartInfo start = new ProcessStartInfo()
            {
                FileName = file,
                UseShellExecute = false,
                RedirectStandardOutput = true,
                CreateNoWindow = true
            };

            var cmd = Process.Start(start);
            string output = cmd.StandardOutput.ReadToEnd();

            cmd.WaitForExit();

            List<string> result = JsonConvert.DeserializeObject<List<string>>(File.ReadAllText(@$"{AppdataFolder}\AampTemp.txt"));

            for(int i = 0; i < result.Count(); i++)
            {
                filesToModify[i].Item1.SetCustomData(result[i].Replace("\r\n", "").Split(" ").Select(h => Convert.ToByte(h, 16)).ToArray());
            }
        }

        //private static void ModifyPauseMenuModel(string bcmlPath, string folder, string model)
        //{
        //    string pauseMenuActorPath = $"{bcmlPath}\\Actor\\Pack\\PauseMenuPlayer.sbactorpack";

        //    Sarc pauseMenuActorPack = Sarc.FromBinary(Yaz0.Decompress(File.ReadAllBytes(pauseMenuActorPath)));
        //    AampFile modelList = AampFile.FromBinary(pauseMenuActorPack["Actor/ModelList/PauseMenuPlayer.bmodellist"]);
        //    modelList.ModifyModel(folder, model);

        //    pauseMenuActorPack["Actor/ModelList/PauseMenuPlayer.bmodellist"] = modelList.ToBinary();

        //    using MemoryStream pauseMenuActorMemoryStream = new();
        //    {
        //        pauseMenuActorPack.Write(pauseMenuActorMemoryStream);
        //    }

        //    File.WriteAllBytes(pauseMenuActorPath, Yaz0.Compress(pauseMenuActorMemoryStream.ToArray()).ToArray());
        //}

        //private static void ModifyDefaultExtraArmor(this Sarc titleBG, string folder, string model, string armorNum)
        //{
        //    Sarc armorActorPack = Sarc.FromBinary(Yaz0.Decompress(titleBG[$"Actor/Pack/Armor_Default_Extra_{armorNum}.sbactorpack"]));
        //    AampFile armorModelList = AampFile.FromBinary(armorActorPack[$"Actor/ModelList/Armor_Default_Extra_{armorNum}.bmodellist"]);

        //    armorModelList.ModifyModel(folder, model);

        //    armorActorPack[$"Actor/ModelList/Armor_Default_Extra_{armorNum}.bmodellist"] = armorModelList.ToBinary();

        //    using MemoryStream ms = new();
        //    {
        //        armorActorPack.Write(ms);
        //    }

        //    titleBG[$"Actor/Pack/Armor_Default_Extra_{armorNum}.sbactorpack"] = Yaz0.Compress(ms.ToArray()).ToArray();
        //}

        //public static void ModifyArmors(string bcmlPath, string folder, string model, bool isNotLink)
        //{
        //    //foreach(string path in Directory.GetFiles("D:\\Games\\Wii U Unpacked\\The Legend of Zelda Breath of the Wild (UPDATE DATA) (v208) (3.243 GB) (USA) (unpacked)\\content\\Actor\\Pack").Where(file => file.StartsWith("Armor_")))
        //    var test = Directory.GetFiles($"{bcmlPath}\\Actor\\Pack\\");
        //    foreach (string path in Directory.GetFiles($"{bcmlPath}\\Actor\\Pack\\").Where(file => Path.GetFileName(file).StartsWith("Armor_")))
        //    {
        //        ModifyArmor(path, folder, model, isNotLink);
        //    }

        //}

        //private static void ModifyArmor(string path, string folder, string model, bool isNotLink)
        //{
        //    Sarc armorActorPack = Sarc.FromBinary(Yaz0.Decompress(File.ReadAllBytes(path)));
        //    string modelListPath = armorActorPack.Keys.Where(key => key.Contains("ModelList")).First();
        //    string armorFullName = modelListPath.Split("/").Last().Replace(".bmodellist", "");
        //    string armorNumber = armorFullName.Split("_")[1];
        //    AampFile modelList = AampFile.FromBinary(armorActorPack[modelListPath]);

        //    modelList.ModifyModel(!isNotLink ? $"Armor_{armorNumber}" : folder, !isNotLink ? armorFullName : model);

        //    armorActorPack[modelListPath] = modelList.ToBinary();

        //    using MemoryStream armorMemoryStream = new();
        //    {
        //        armorActorPack.Write(armorMemoryStream);
        //    }

        //    //path = "D:\\BreathOfTheWildMultiplayer\\PythonProject\\Github\\BOTWMCode\\BNP Files\\BreathOfTheWildMultiplayerBase\\content\\Actor\\Pack\\" + Path.GetFileName(path);

        //    File.WriteAllBytes(path, Yaz0.Compress(armorMemoryStream.ToArray()).ToArray());
        //}
    }
}
