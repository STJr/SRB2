using System;
using System.Collections.Generic;
using System.Collections;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using System.IO;

namespace SRB2Updater
{
    public partial class Options : Form
    {
        private Settings settings;
        public Options(Settings settings)
        {
            InitializeComponent();
            this.settings = settings;
            SetOptions();
        }

        private void SetOptions()
        {
            chkDisplayWindowed.Checked = settings.displayWindowed;
            chkCustomResolution.Checked = settings.displayCustom;
            txtHeight.Text = settings.displayHeight.ToString();
            txtWidth.Text = settings.displayWidth.ToString();
            txtMSPort.Text = settings.msPort.ToString();
            settings.AddBinariesToListView(listviewBinaries);
            txtMSAddress.Text = settings.msAddress.ToString();
            txtParams.Text = settings.Params.ToString();
            chkCloseOnStart.Checked = settings.CloseOnStart;
            chkShowDefaultWads.Checked = settings.ShowDefaultWads;
            if (settings.displayCustom)
            {
                txtHeight.Enabled = true;
                txtWidth.Enabled = true;
            }
            else
            {
                txtHeight.Enabled = false;
                txtWidth.Enabled = false;
            }
        }

        private void chkCustomResolution_CheckedChanged(object sender, EventArgs e)
        {
            if (chkCustomResolution.Checked)
            {
                txtHeight.Enabled = true;
                txtWidth.Enabled = true;
            }
            else
            {
                txtHeight.Enabled = false;
                txtWidth.Enabled = false;
            }
        }

        private void btnSave_Click(object sender, EventArgs e)
        {
            settings.displayCustom = chkCustomResolution.Checked;
            settings.displayHeight = Convert.ToInt32(txtHeight.Text);
            settings.displayWidth = Convert.ToInt32(txtWidth.Text);
            settings.displayWindowed = chkDisplayWindowed.Checked;
            settings.msAddress = txtMSAddress.Text;
            settings.ShowDefaultWads = chkShowDefaultWads.Checked;
            settings.Params = txtParams.Text;
            settings.msPort = Convert.ToInt32(txtMSPort.Text);
            settings.CloseOnStart = chkCloseOnStart.Checked;
            settings.SaveSettings();
            settings.SetBinariesFromListView(listviewBinaries);
            Close();
        }

        private void btnCancel_Click(object sender, EventArgs e)
        {
            Close();
        }

        private void btnAdd_Click(object sender, EventArgs e)
        {
            listviewBinaries.Items.Add(new ListViewItem(new string[] { "[New Version]", "" }));
        }

        private void btnDel_Click(object sender, EventArgs e)
        {
            if (listviewBinaries.SelectedItems.Count > 0)
                listviewBinaries.Items.Remove(listviewBinaries.SelectedItems[0]);
        }

        private void btnBrowse_Click(object sender, EventArgs e)
        {
            if (listviewBinaries.SelectedItems.Count > 0 &&
                openFileDialog1.ShowDialog() == DialogResult.OK)
                textboxBinary.Text = openFileDialog1.FileName;
        }

        private void textboxVersion_TextChanged(object sender, EventArgs e)
        {
            if (listviewBinaries.SelectedItems.Count > 0)
                listviewBinaries.SelectedItems[0].Text = textboxVersion.Text;
        }

        private void textboxBinary_TextChanged(object sender, EventArgs e)
        {
            if (listviewBinaries.SelectedItems.Count > 0)
                listviewBinaries.SelectedItems[0].SubItems[1].Text = textboxBinary.Text;
        }

        private void listviewBinaries_SelectedIndexChanged(object sender, EventArgs e)
        {
            if (listviewBinaries.SelectedItems.Count > 0)
            {
                btnDel.Enabled = true;
                btnBrowse.Enabled = true;
                textboxVersion.Text = listviewBinaries.SelectedItems[0].Text;
                textboxBinary.Text = listviewBinaries.SelectedItems[0].SubItems[1].Text;
                textboxVersion.Enabled = true;
                textboxBinary.Enabled = true;
            }
            else
            {
                btnDel.Enabled = false;
                btnBrowse.Enabled = false;
                textboxVersion.Text = "";
                textboxBinary.Text = "";
                textboxVersion.Enabled = false;
                textboxBinary.Enabled = false;
            }
        }
    }
}
