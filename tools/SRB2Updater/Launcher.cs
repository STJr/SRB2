using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
//using System.Data.OleDb;
using System.Xml;
//using System.Drawing;
using System.Text;
using System.Windows.Forms;
using System.Net;
using System.IO;
using System.Threading;
using System.Security.Cryptography;
//using System.Runtime.InteropServices;
using System.Diagnostics;
using System.Drawing;
using System.Media;

namespace SRB2Updater
{
    public partial class Launcher : Form
    {
        private Settings settings = new Settings();
        private Debug debug = new Debug();
        // The thread inside which the download happens
        private Thread thrDownload;
        private Thread thrTotal;
        // The stream of data retrieved from the web server
        private Stream strResponse;
        // The stream of data that we write to the harddrive
        private Stream strLocal;
        // The request to the web server for file information
        private HttpWebRequest webRequest;
        // The response from the web server containing information about the file
        private HttpWebResponse webResponse;
        // The progress of the download in percentage
        private static int PercentProgress;
        private static int OverallPercentProgress;
        // Overall progress as a percentage
        private static Int64 OverallProgress;
        // Progress stored, for calculating overall
        private static Int64 CurrentProgress;
        // Total File Size of entire update
        private static Int64 TotalSize;
        // The delegate which we will call from the thread to update the form
        private delegate void UpdateProgessCallback(Int64 BytesRead, Int64 TotalBytes);
        private delegate void OverallProgessCallback(Int64 BytesRead);
        // When to pause
        bool goPause = false;
        // Download Details
        string downFile;
        // Updating
        bool filesGot = false;
        bool downloadStatus = false;
        bool doneCalculate = false;
        string formTitle = "Sonic Robo Blast 2 Launcher";
        bool loadedBat = false;
        ProcessStartInfo startinfo = new ProcessStartInfo();
        private ServerQuerier sq;
        private string MSFail;

        public Launcher(string[] args)
        {
            InitializeComponent();
            settings.GetSettings();
            sq = new ServerQuerier();
            ServerInfoListViewAdder silva = new ServerInfoListViewAdder(sq, this);
            try
            {
                sq.SetMasterServer(settings.msAddress, Convert.ToUInt16(settings.msPort));
            }
            catch (Exception exception)
            {
                MSFail = exception.Message;
            }
            sq.StartListening(silva);
            backgroundWorkerQueryServers.RunWorkerAsync();
            foreach (string arg in args)
            {
                if (arg == "-debug")
                {
                    debug.Show();
                    break;
                }
            }
            RandomBanner();
        }

        public string getMD5(string filename)
        {
            StringBuilder sb = new StringBuilder();
            FileInfo f = new FileInfo(filename);
            FileStream fs = f.OpenRead();
            MD5 md5 = new MD5CryptoServiceProvider();
            byte[] hash = md5.ComputeHash(fs);
            fs.Close();
            foreach (byte hex in hash)
                sb.Append(hex.ToString("x2"));
            string md5sum = sb.ToString();
            return md5sum;
        }

        public void PlayIt()
        {
            SoundPlayer player = new SoundPlayer(Properties.Resources.Kotaku);
            player.Play();
        }

        public void RandomBanner()
        {
            Random random = new Random();
            int rand = random.Next(0, 4);
            //this.bannerRandom.BackgroundImage = global::SRB2Updater.Properties.Resources.Banner;
            switch (rand)
            {
                case 0:
                    bannerRandom.BackgroundImage = global::SRB2Updater.Properties.Resources.Banner;
                    break;
                case 1:
                    bannerRandom.BackgroundImage = global::SRB2Updater.Properties.Resources.Banner2;
                    break;
                case 2:
                    bannerRandom.BackgroundImage = global::SRB2Updater.Properties.Resources.Banner3;
                    break;
                case 3:
                    bannerRandom.BackgroundImage = global::SRB2Updater.Properties.Resources.Banner4;
                    break;
                default:
                    bannerRandom.BackgroundImage = global::SRB2Updater.Properties.Resources.Banner;
                    break;
            }

            debug.strRandom = Convert.ToString(rand);
        }

        private void updateList(bool doCalculate)
        {
            if (filesGot == false)
            {

                XmlDataDocument xmlDatadoc = new XmlDataDocument();
                xmlDatadoc.DataSet.ReadXml("http://update.srb2.org/files_beta/files_beta.xml");
                DataSet ds = new DataSet("Files DataSet");
                ds = xmlDatadoc.DataSet;
                fileList.DataSource = ds.DefaultViewManager;
                fileList.DataMember = "File";
                filesGot = true;
            }
            if (downloadStatus == false)
            {
                foreach (DataGridViewRow fileRow in fileList.Rows)
                {
                    if (!File.Exists(fileRow.Cells["filename"].Value.ToString()) && fileRow.Cells["filename"].Value.ToString() != "srb2update.update")
                    {
//                        fileRow.DefaultCellStyle.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(192)))), ((int)(((byte)(255)))), ((int)(((byte)(192)))));
                        fileRow.Cells["localmd5"].Value = "not_found";
                    } else {
                        if (fileRow.Cells["filename"].Value.ToString() == "srb2update.update")
                            fileRow.Cells["localmd5"].Value = getMD5("srb2update.exe");
                        else
                            fileRow.Cells["localmd5"].Value = getMD5(fileRow.Cells["filename"].Value.ToString());
                    }
                    if (fileRow.Cells["localmd5"].Value.ToString() != fileRow.Cells["md5"].Value.ToString())
                    {
//                        if (fileRow.Cells["localmd5"].Value.ToString() != "not_found")
//                            fileRow.DefaultCellStyle.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(230)))), ((int)(((byte)(224)))), ((int)(((byte)(206)))));
                        fileRow.Cells["status"].Value = "Queued";
                            
                    }
                    else
                    {
                        fileRow.Cells["status"].Value = "Up to date";
                    }
                }
                if (doCalculate)
                {
                    thrTotal = new Thread(new ParameterizedThreadStart(CalculateTotalSize));
                    thrTotal.Start(0);
                }
                if (doneCalculate)
                {
                    foreach (DataGridViewRow fileRow in fileList.Rows)
                    {
                        if (fileRow.Cells["localmd5"].Value.ToString() != fileRow.Cells["md5"].Value.ToString())
                        {
                            if (fileRow.Cells["optional"].Value.ToString() == "1" && !update_optional.Checked)
                                fileRow.Cells["Status"].Value = "Skipped (Optional)";
                            else
                            {
                                downFile = fileRow.Cells["filename"].Value.ToString();
                                thrDownload = new Thread(new ParameterizedThreadStart(Download));
                                thrDownload.Start(0);
                                fileRow.Cells["Status"].Value = "Downloading...";
                                downloadStatus = true;
                                break;
                            }
                        }
                        else
                        {
                            fileRow.Cells["Status"].Value = "Up to date";
                        }
                        CurrentProgress = 0;
                    }
                }
            }
        }

        private void UpdateProgress(Int64 BytesRead, Int64 TotalBytes)
        {
            // Calculate the download progress in percentages
            PercentProgress = Convert.ToInt32((BytesRead * 100) / TotalBytes);
            // Make progress on the progress bar
            progress_currentFile.Value = PercentProgress;
            // Display the current progress on the form
            lblProgress.Text = downFile + " - " + (BytesRead / 1024) + "KB of " + (TotalBytes / 1024) + "KB (" + PercentProgress + "%)";
            this.Text = formTitle + " :: Downloading " + downFile + " (" + PercentProgress + "%)";
            debug.strPercent = PercentProgress.ToString() + "%";
            if (BytesRead >= TotalBytes - 1)
                    updateList(false);
        }

        private void UpdateOverallProgress(Int64 BytesRead)
        {
            // Calculate progress change and add to OverallProgress...
            OverallProgress += BytesRead - CurrentProgress;
            // Calculate the download progress in percentages
            if (TotalSize < 1)
                TotalSize = 1;
            OverallPercentProgress = Convert.ToInt32((OverallProgress * 100) / TotalSize);
            // Make progress on the progress bar
            if (OverallPercentProgress > 100)
                OverallPercentProgress = 100;
            progress_overall.Value = OverallPercentProgress;
            if (OverallProgress >= TotalSize)
            {
                lblProgress.Text = "Done";
                btnCheckFiles.Enabled = true;
            }
            CurrentProgress = BytesRead;
            debug.strCurrent = Convert.ToString(CurrentProgress) + " bytes";
            debug.strOverall = Convert.ToString(OverallProgress) + " bytes";
            debug.strOverallPercentage = Convert.ToString(OverallPercentProgress) + "%";
            debug.strRead = Convert.ToString(BytesRead) + " bytes";
            debug.strTotal = Convert.ToString(TotalSize) + " bytes";
        }

        private void CalculateTotalSize(object startpoint)
        {
            foreach (DataGridViewRow fileRow in fileList.Rows)
            {
                if ((fileRow.Cells["optional"].Value.ToString() == "0" || update_optional.Checked) && fileRow.Cells["localmd5"].Value.ToString() != fileRow.Cells["md5"].Value.ToString())
                {
                    try
                    {
                        // Create a request to the file we are downloading
                        webRequest = (HttpWebRequest)WebRequest.Create("http://update.srb2.org/files_beta/" + fileRow.Cells["filename"].Value.ToString());
                        // Set the starting point of the request
                        webRequest.AddRange(0);

                        // Set default authentication for retrieving the file
                        webRequest.Credentials = CredentialCache.DefaultCredentials;
                        // Retrieve the response from the server
                        webResponse = (HttpWebResponse)webRequest.GetResponse();
                        // Ask the server for the file size and store it
                        Int64 fileSize = webResponse.ContentLength;
                        TotalSize = TotalSize + fileSize;
                    }
                    finally
                    {
                        // When the above code has ended, close the streams
                        webResponse.Close();
                    }
                }
            }
            doneCalculate = true;
            updateList(false);
        }

        private void Download(object startpoint)
        {
            try
            {
                string filename = Convert.ToString(startpoint);
                // Create a request to the file we are downloading
                webRequest = (HttpWebRequest)WebRequest.Create("http://update.srb2.org/files_beta/" + downFile);
                // Set the starting point of the request
                webRequest.AddRange(0);

                // Set default authentication for retrieving the file
                webRequest.Credentials = CredentialCache.DefaultCredentials;
                // Retrieve the response from the server
                webResponse = (HttpWebResponse)webRequest.GetResponse();
                // Ask the server for the file size and store it
                Int64 fileSize = webResponse.ContentLength;
                
                // Open the URL for download
                strResponse = webResponse.GetResponseStream();

                // Create a new file stream where we will be saving the data (local drive)
                strLocal = new FileStream(downFile, FileMode.Create, FileAccess.Write, FileShare.None);                
                // It will store the current number of bytes we retrieved from the server
                int bytesSize = 0;
                // A buffer for storing and writing the data retrieved from the server
                byte[] downBuffer = new byte[2048];

                // Loop through the buffer until the buffer is empty
                while ((bytesSize = strResponse.Read(downBuffer, 0, downBuffer.Length)) > 0)
                {
                    // Write the data from the buffer to the local hard drive
                    strLocal.Write(downBuffer, 0, bytesSize);
                    // Invoke the method that updates the form's label and progress bar
                    this.Invoke(new UpdateProgessCallback(this.UpdateProgress), new object[] { strLocal.Length, fileSize });
                    this.Invoke(new OverallProgessCallback(this.UpdateOverallProgress), new object[] { strLocal.Length });

                    if (goPause == true)
                    {
                        break;
                    }
                }
            }
            finally
            {
                // When the above code has ended, close the streams
                strResponse.Close();
                strLocal.Close();
                // And update the row!
                downloadStatus = false;
                if (downFile == "srb2update.update" && loadedBat != true)
                {
                    MessageBox.Show("The updater will now restart to apply a patch.", "Self Update", MessageBoxButtons.OK);
                    CreateUpdaterBat();
                    startinfo.WindowStyle = ProcessWindowStyle.Hidden;
                    startinfo.FileName = "srb2update.bat";
                    System.Diagnostics.Process.Start(startinfo);
                    downloadStatus = false;
                    Environment.Exit(0);
                } else
                    updateList(false);
            }
        }

        private void CreateUpdaterBat()
        {
            File.WriteAllText("srb2update.bat", "ping 127.0.0.1\ncopy srb2update.update srb2update.exe\ndel srb2update.update\nsrb2update.exe\nexit");
        }


        private void update_Load(object sender, EventArgs e)
        {
            lblProgress.Text = "Getting File List...";
            if (File.Exists("srb2update.bat"))
                File.Delete("srb2update.bat");
            TotalSize = 0;
            CurrentProgress = 0;
            OverallPercentProgress = 0;
            OverallProgress = 0;
            btnCheckFiles.Enabled = false;
            updateList(true);
        }

        private void btnOptions_Click(object sender, EventArgs e)
        {
            new Options(settings).ShowDialog();
        }

        private class ServerInfoListViewAdder : ServerQuerier.ServerInfoReceiveHandler
        {
            private delegate ListViewItem AddToListCallback(ListViewItem lvi);
            private Launcher form1;
            private Dictionary<byte, string> dicGametypes = new Dictionary<byte, string>();
            private static Dictionary<ServerQuerier.ServerInfoVer, List<String>> dicDefaultFiles =
                new Dictionary<ServerQuerier.ServerInfoVer, List<String>>();

            static ServerInfoListViewAdder()
            {
                dicDefaultFiles.Add(
                    ServerQuerier.ServerInfoVer.SIV_PREME,
                    new List<String>(
                        new string[] {
							"srb2.srb", "sonic.plr", "tails.plr", "knux.plr",
							"auto.wpn", "bomb.wpn", "home.wpn", "rail.wpn", "infn.wpn",
							"drill.dta", "soar.dta", "music.dta"
						})
                    );

                dicDefaultFiles.Add(
                    ServerQuerier.ServerInfoVer.SIV_ME,
                    new List<String>(
                        new string[] {
							"srb2.wad", "sonic.plr", "tails.plr", "knux.plr",
							"rings.wpn", "drill.dta", "soar.dta", "music.dta"
						})
                    );
            }


            public ServerInfoListViewAdder(ServerQuerier sq, Launcher form1)
                : base(sq)
            {
                this.form1 = form1;

                // Gametypes.
                dicGametypes.Add(0, "Co-op");
                dicGametypes.Add(1, "Match");
                dicGametypes.Add(2, "Race");
                dicGametypes.Add(3, "Tag");
                dicGametypes.Add(4, "CTF");
                dicGametypes.Add(5, "Chaos");

                // Don't think these are actually used.
                dicGametypes.Add(42, "Team Match");
                dicGametypes.Add(43, "Time-Only Race");
            }

            public override void ProcessServerInfo(ServerQuerier.SRB2ServerInfo srb2si)
            {
                ListView lv = form1.listViewServers;

                // Build a list item.
                ListViewItem lvi = new ListViewItem(srb2si.strName);

                // So we can get address and whatever else we might need.
                lvi.Tag = srb2si;

                // Gametype string, or number if not recognised.
                if (dicGametypes.ContainsKey(srb2si.byGametype))
                    lvi.SubItems.Add(dicGametypes[srb2si.byGametype]);
                else
                    lvi.SubItems.Add(Convert.ToString(srb2si.byGametype));

                lvi.SubItems.Add(Convert.ToString(srb2si.uiTime));
                lvi.SubItems.Add(srb2si.byPlayers + "/" + srb2si.byMaxplayers);
                lvi.SubItems.Add(srb2si.strVersion);

                // Make the tooltip.
                BuildTooltip(lvi, form1.settings.ShowDefaultWads);

                // Is the game full?
                if (srb2si.byPlayers >= srb2si.byMaxplayers)
                    lvi.ForeColor = Color.DimGray;
                // Modified?
                else if (srb2si.bModified)
                    lvi.ForeColor = Color.Red;

                // Thread-safe goodness.
                if (lv.InvokeRequired)
                {
                    // Call ourselves in the context of the form's thread.
                    AddToListCallback addtolistcallback = new AddToListCallback(lv.Items.Add);
                    lv.Invoke(addtolistcallback, new object[] { lvi });
                }
                else
                {
                    // Add it!
                    lv.Items.Add(lvi);
                }

            }

            public override void HandleException(Exception e)
            {
                    
            }

            public static void BuildTooltip(ListViewItem lvi, bool bShowDefaultWads)
            {
                string strWads = String.Empty;
                ServerQuerier.SRB2ServerInfo srb2si = (ServerQuerier.SRB2ServerInfo)lvi.Tag;

                foreach (ServerQuerier.AddedWad aw in srb2si.listFiles)
                {
                    List<string> listDefaultFiles = dicDefaultFiles[srb2si.siv];

                    if (bShowDefaultWads || !listDefaultFiles.Contains(aw.strFilename))
                    {
                        strWads += String.Format("\n{0} ({1:f1} KB)", aw.strFilename, Convert.ToSingle(aw.uiSize) / 1024);
                        if (aw.bImportant)
                        {
                            if (aw.downloadtype == ServerQuerier.DownloadTypes.DT_TOOBIG)
                                strWads += " (too big to download)";
                            else if (aw.downloadtype == ServerQuerier.DownloadTypes.DT_DISABLED)
                                strWads += " (downloading disabled)";
                        }
                        else strWads += " (unimportant)";
                    }
                }

                lvi.ToolTipText = "Current map: " + srb2si.strMapName + "\n";
                if (strWads != String.Empty)
                    lvi.ToolTipText += "Wads added:" + strWads;
                else lvi.ToolTipText += "No wads added";
            }
        }

        private void backgroundWorkerQueryServers_DoWork(object sender, DoWorkEventArgs e)
        {
            MSClient msclient = new MSClient();

            try
            {
                List<MSServerEntry> listServers = msclient.GetServerList(settings.msAddress, Convert.ToUInt16(settings.msPort));


                // Query each of the individual servers asynchronously.
                foreach (MSServerEntry msse in listServers)
                {
                    sq.Query(msse.strAddress, msse.unPort);
                }
            }
            catch (System.Net.Sockets.SocketException sockexception)
            {
                MSFail = sockexception.Message;
            }
            catch (Exception exception)
            {
                MSFail = exception.Message;
            }
        }

        private void btnRefresh_Click(object sender, EventArgs e)
        {
            if (!backgroundWorkerQueryServers.IsBusy)
            {
                // Clear the server list.
                listViewServers.Items.Clear();

                // Disable the Connect button.
                btnConnect.Enabled = false;

                // Query the MS and the individual servers in another thread.
                backgroundWorkerQueryServers.RunWorkerAsync();
            }
        }

        private void btnConnect_Click(object sender, EventArgs e)
        {
            if (listViewServers.SelectedItems.Count > 0)
            {
                ConnectToServerFromListItem(listViewServers.SelectedItems[0]);
            }
        }

        private void ConnectToServerFromListItem(ListViewItem lvi)
        {
            ServerQuerier.SRB2ServerInfo srb2si = (ServerQuerier.SRB2ServerInfo)lvi.Tag;

            // Prompt to get a binary if we need one.
            if (!settings.HasBinaryForVersion(srb2si.strVersion) &&
                MessageBox.Show("To join this game, you must register an executable file for version " + srb2si.strVersion + ". Would you like to do so?", Text, MessageBoxButtons.YesNo, MessageBoxIcon.Exclamation) == DialogResult.Yes &&
                openFileDialog1.ShowDialog() == DialogResult.OK)
            {
                settings.SetBinary(srb2si.strVersion, openFileDialog1.FileName);
                settings.SaveSettings();
            }

            // Go!
            ConnectToServer(srb2si.strAddress, srb2si.unPort, srb2si.strVersion);
        }

        private void ConnectToServer(string strAddress, ushort unPort, string strVersion)
        {
            // Make sure we now have a binary.
            if (settings.HasBinaryForVersion(strVersion))
            {
                try
                {
                    string strBinary = settings.GetBinary(strVersion);
                    string strDirectory = System.IO.Path.GetDirectoryName(strBinary);
                    if (strDirectory != String.Empty)
                        System.IO.Directory.SetCurrentDirectory(strDirectory);
                    System.Diagnostics.Process.Start(strBinary, System.String.Format("-connect {0}:{1} {2}", strAddress, unPort, settings.Params)).Close();
                    if (settings.CloseOnStart)
                        Environment.Exit(0);
                }
                catch (Exception exception)
                {
                    MessageBox.Show("Unable to start SRB2: " + exception.Message + ".", "SRB2 MS Launcher", MessageBoxButtons.OK, MessageBoxIcon.Error);
                }
            }
        }

        private class ListViewSorter : System.Collections.IComparer
        {
            private int iColumn;
            public int Column { get { return iColumn; } }
            public SortOrder so;

            public ListViewSorter(int iColumn)
            {
                this.iColumn = iColumn;
                so = SortOrder.Ascending;
            }

            public int Compare(object x, object y)
            {
                ListViewItem lviX = (ListViewItem)x;
                ListViewItem lviY = (ListViewItem)y;

                return ((so == SortOrder.Ascending) ? 1 : -1) * String.Compare(lviX.SubItems[iColumn].Text, lviY.SubItems[iColumn].Text);
            }

            public void ToggleSortOrder()
            {
                if (so != SortOrder.Ascending)
                    so = SortOrder.Ascending;
                else
                    so = SortOrder.Descending;
            }
        }

        private void listViewServers_ColumnClick(object sender, ColumnClickEventArgs e)
        {
            if (listViewServers.ListViewItemSorter != null &&
                ((ListViewSorter)listViewServers.ListViewItemSorter).Column == e.Column)
            {
                ((ListViewSorter)listViewServers.ListViewItemSorter).ToggleSortOrder();
                listViewServers.Sort();
            }
            else
            {
                listViewServers.ListViewItemSorter = new ListViewSorter(e.Column);
            }
        }

        private void listViewServers_SelectedIndexChanged(object sender, EventArgs e)
        {
            btnConnect.Enabled = (listViewServers.SelectedItems.Count > 0);
        }

        private void listViewServers_ItemActivate(object sender, EventArgs e)
        {
            ConnectToServerFromListItem(listViewServers.SelectedItems[0]);
        }

        private void Launcher_FormClosed(object sender, FormClosedEventArgs e)
        {
            Environment.Exit(0);
        }

        private Bunny sequence = new Bunny();

        private void Launcher_KeyUp(object sender, KeyEventArgs e)
        {
            if (sequence.IsCompletedBy(e.KeyCode))
            {
                PlayIt();
            }
            debug.strKonami = Convert.ToString(sequence.Position);
        }

        private void btnStartSRB2_Click(object sender, EventArgs e)
        {
            System.Diagnostics.Process.Start("srb2win.exe", System.String.Format("{0}", settings.Params)).Close();
            if(settings.CloseOnStart)
                Environment.Exit(0);
        }
    }
}
