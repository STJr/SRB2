using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

namespace SRB2Updater
{
    public partial class Debug : Form
    {
        public Debug()
        {
            InitializeComponent();
        }

        public String strOverall
        {
            get { return this.lblOverall.Text; }
            set { this.lblOverall.Text = value; }
        }

        public String strKonami
        {
            get { return this.lblKonami.Text; }
            set { this.lblKonami.Text = value; }
        }

        public String strRandom
        {
            get { return this.lblRandom.Text; }
            set { this.lblRandom.Text = value; }
        }

        public String strOverallPercentage
        {
            get { return this.lblOverallPercentage.Text; }
            set { this.lblOverallPercentage.Text = value; }
        }

        public String strCurrent
        {
            get { return this.lblCurrent.Text; }
            set { this.lblCurrent.Text = value; }
        }

        public String strPercent
        {
            get { return this.lblPercent.Text; }
            set { this.lblPercent.Text = value; }
        }

        public String strRead
        {
            get { return this.lblRead.Text; }
            set { this.lblRead.Text = value; }
        }

        public String strTotal
        {
            get { return this.lblTotal.Text; }
            set { this.lblTotal.Text = value; }
        }
    }
}
