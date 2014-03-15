using System;
using System.Collections.Generic;
using System.Collections;
using System.Text;
using System.Data;
using Microsoft.Win32;
using System.Windows.Forms;
using System.IO;

namespace SRB2Updater
{
    public class Settings
    {
        // Global Settings
        public bool boolDisplayWindowed;
        public bool boolDisplayCustomResolution;
        public bool boolCloseOnStart;
        public int intDisplayWidth;
        public int intDisplayHeight;
        public string strMSAddress;
        public int intMSPort;
        public Array arrWadList;
        public string strParams;

        public string Params
        {
            get { return strParams; }
            set { strParams = value; }
        }

        public Array wadList
        {
            get { return arrWadList; }
            set { arrWadList = value; }
        }

        public bool CloseOnStart
        {
            get { return boolCloseOnStart; }
            set { boolCloseOnStart = value; }
        }

        public string msAddress
        {
            get { return strMSAddress; }
            set { strMSAddress = value; }
        }

        public int msPort
        {
            get { return intMSPort; }
            set { intMSPort = value; }
        }

        public bool displayWindowed
        {
            get { return boolDisplayWindowed; }
            set { boolDisplayWindowed = value; }
        }

        public bool displayCustom
        {
            get { return boolDisplayCustomResolution; }
            set { boolDisplayCustomResolution = value; }
        }

        public int displayWidth
        {
            get { return intDisplayWidth; }
            set { intDisplayWidth = value; }
        }

        public int displayHeight
        {
            get { return intDisplayHeight; }
            set { intDisplayHeight = value; }
        }

        bool boolShowDefaultWads;
        public bool ShowDefaultWads
        {
            get { return boolShowDefaultWads; }
            set { boolShowDefaultWads = value; }
        }

        public void GetSettings()
        {
            RegistryKey rk = Registry.CurrentUser.CreateSubKey(@"Software\SonicTeamJunior\Launcher");
            RegistryKey rkDisplay = Registry.CurrentUser.CreateSubKey(@"Software\SonicTeamJunior\Launcher\Display");
            RegistryKey rkMS = Registry.CurrentUser.CreateSubKey(@"Software\SonicTeamJunior\Launcher\MasterServer");
            RegistryKey rkBinaries = rk.CreateSubKey("Binaries");
            rkBinaries.SetValue("2.0.4", Directory.GetCurrentDirectory() + "\\srb2win.exe");

            try { boolDisplayCustomResolution = Convert.ToInt32(rkDisplay.GetValue("CustomResolution", 0)) != 0; }
            catch { boolDisplayCustomResolution = false; }
            try { boolDisplayWindowed = Convert.ToInt32(rkDisplay.GetValue("Windowed", 0)) != 0; }
            catch { boolDisplayWindowed = false; }
            try { boolShowDefaultWads = Convert.ToInt32(rkDisplay.GetValue("ShowDefaultWads", 0)) != 0; }
            catch { boolShowDefaultWads = false; }
            try { boolCloseOnStart = Convert.ToInt32(rkDisplay.GetValue("CloseOnStart", 0)) != 0; }
            catch { boolCloseOnStart = false; }
            intDisplayHeight = Convert.ToInt32(rkDisplay.GetValue("Height", "480"));
            intDisplayWidth = Convert.ToInt32(rkDisplay.GetValue("Width", "640"));
            intMSPort = Convert.ToInt32(rkMS.GetValue("Port", "28900"));
            strMSAddress = Convert.ToString(rkMS.GetValue("Address", "ms.srb2.org"));
            strParams = Convert.ToString(rk.GetValue("Params"));

            dicBinaries.Clear();
            foreach (string strName in rkBinaries.GetValueNames())
                dicBinaries[strName] = (string)rkBinaries.GetValue(strName);


            rk.Close();
            rkDisplay.Close();
            rkMS.Close();
            rkBinaries.Close();
        }

        public void SaveSettings()
        {
            RegistryKey rk = Registry.CurrentUser.CreateSubKey(@"Software\SonicTeamJunior\Launcher");
            RegistryKey rkDisplay = rk.CreateSubKey("Display");
            RegistryKey rkMS = rk.CreateSubKey("MasterServer");
            rkDisplay.SetValue("CustomResolution", boolDisplayCustomResolution);
            rkDisplay.SetValue("Windowed", boolDisplayWindowed);
            rkDisplay.SetValue("Height", intDisplayHeight);
            rkDisplay.SetValue("Width", intDisplayWidth);
            rkMS.SetValue("Port", intMSPort);
            rkMS.SetValue("Address", strMSAddress);
            rkMS.SetValue("ShowDefaultWads", boolShowDefaultWads);
            rk.SetValue("Params", strParams);
            rk.SetValue("CloseOnStart", boolCloseOnStart);

            rk.DeleteSubKey("Binaries", false);
            RegistryKey rkBinaries = rk.CreateSubKey("Binaries");
            rkBinaries.SetValue("2.0.4", Directory.GetCurrentDirectory() + "\\srb2win.exe");
            foreach (string strName in dicBinaries.Keys)
                rkBinaries.SetValue(strName, dicBinaries[strName]);

            rk.Close();
        }

/*        public void WriteToRegistry()
        {
            RegistryKey rk = Registry.CurrentUser.CreateSubKey(@"Software\SRB2MSLauncher");

            rk.SetValue("Params", strParams);
            rk.SetValue("Masterserver", strMSAddress);
            rk.SetValue("MSPort", unMSPort, RegistryValueKind.DWord);
            rk.SetValue("ShowDefaultWads", Convert.ToInt32(bShowDefaultWads), RegistryValueKind.DWord);

            rk.DeleteSubKey("Binaries", false);
            RegistryKey rkBinaries = rk.CreateSubKey("Binaries");
            foreach (string strName in dicBinaries.Keys)
                rkBinaries.SetValue(strName, dicBinaries[strName]);

            rkBinaries.Close();
            rk.Close();
        }

        public void LoadFromRegistry()
        {
            RegistryKey rk = Registry.CurrentUser.CreateSubKey(@"Software\SRB2MSLauncher");
            RegistryKey rkBinaries = rk.CreateSubKey("Binaries");

            strParams = (string)rk.GetValue("Params", "");
            strMSAddress = (string)rk.GetValue("Masterserver", "ms.srb2.org");

            try { unMSPort = Convert.ToUInt16(rk.GetValue("MSPort", MS_DEFAULT_PORT)); }
            catch { unMSPort = MS_DEFAULT_PORT; }

            try { bShowDefaultWads = Convert.ToInt32(rk.GetValue("ShowDefaultWads", 0)) != 0; }
            catch { bShowDefaultWads = true; }

            dicBinaries.Clear();
            foreach (string strName in rkBinaries.GetValueNames())
                dicBinaries[strName] = (string)rkBinaries.GetValue(strName);

            rkBinaries.Close();
            rk.Close();
        }*/


        public List<string> VersionStrings
        {
            get { return new List<string>(dicBinaries.Keys); }
        }

        public bool HasBinaryForVersion(string strVersion)
        {
            return dicBinaries.ContainsKey(strVersion) && dicBinaries[strVersion] != "";
        }

        public void AddBinariesToListView(ListView lv)
        {
            foreach (string strName in dicBinaries.Keys)
                lv.Items.Add(new ListViewItem(new string[] { strName, dicBinaries[strName] }));
        }

        public void SetBinariesFromListView(ListView lv)
        {
            dicBinaries.Clear();
            foreach (ListViewItem lvi in lv.Items)
                dicBinaries[lvi.Text] = lvi.SubItems[1].Text;
        }

		Dictionary<string, string> dicBinaries = new Dictionary<string, string>();

		public void SetBinary(string strVersion, string strBinary)
		{
			dicBinaries[strVersion] = strBinary;
		}

        public string GetBinary(string strVersion)
        {
            return dicBinaries[strVersion];
        }
    }
}