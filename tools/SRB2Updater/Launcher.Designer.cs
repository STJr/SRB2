namespace SRB2Updater
{
    partial class Launcher
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            System.Windows.Forms.DataGridViewCellStyle dataGridViewCellStyle1 = new System.Windows.Forms.DataGridViewCellStyle();
            System.Windows.Forms.DataGridViewCellStyle dataGridViewCellStyle2 = new System.Windows.Forms.DataGridViewCellStyle();
            System.Windows.Forms.DataGridViewCellStyle dataGridViewCellStyle4 = new System.Windows.Forms.DataGridViewCellStyle();
            System.Windows.Forms.DataGridViewCellStyle dataGridViewCellStyle5 = new System.Windows.Forms.DataGridViewCellStyle();
            System.Windows.Forms.DataGridViewCellStyle dataGridViewCellStyle6 = new System.Windows.Forms.DataGridViewCellStyle();
            System.Windows.Forms.DataGridViewCellStyle dataGridViewCellStyle3 = new System.Windows.Forms.DataGridViewCellStyle();
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(Launcher));
            this.panel1 = new System.Windows.Forms.Panel();
            this.panel3 = new System.Windows.Forms.Panel();
            this.btnOptions = new System.Windows.Forms.Button();
            this.btnCheckFiles = new System.Windows.Forms.Button();
            this.btnStartSRB2 = new System.Windows.Forms.Button();
            this.panel2 = new System.Windows.Forms.Panel();
            this.lblProgress = new System.Windows.Forms.Label();
            this.update_optional = new System.Windows.Forms.CheckBox();
            this.progress_overall = new System.Windows.Forms.ProgressBar();
            this.progress_currentFile = new System.Windows.Forms.ProgressBar();
            this.tab_web = new System.Windows.Forms.TabControl();
            this.tabPage1 = new System.Windows.Forms.TabPage();
            this.panel10 = new System.Windows.Forms.Panel();
            this.webBrowser3 = new System.Windows.Forms.WebBrowser();
            this.tabPage2 = new System.Windows.Forms.TabPage();
            this.panel9 = new System.Windows.Forms.Panel();
            this.webBrowser1 = new System.Windows.Forms.WebBrowser();
            this.tabPage3 = new System.Windows.Forms.TabPage();
            this.panel8 = new System.Windows.Forms.Panel();
            this.fileList = new System.Windows.Forms.DataGridView();
            this.name = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.filename = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.status = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.localmd5 = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.md5 = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.optional = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.calculated = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.serverTab = new System.Windows.Forms.TabPage();
            this.panel6 = new System.Windows.Forms.Panel();
            this.listViewServers = new System.Windows.Forms.ListView();
            this.colhdrName = new System.Windows.Forms.ColumnHeader("(none)");
            this.colhdrGametype = new System.Windows.Forms.ColumnHeader();
            this.colhdrPing = new System.Windows.Forms.ColumnHeader();
            this.colhdrPlayers = new System.Windows.Forms.ColumnHeader();
            this.colhdrVersion = new System.Windows.Forms.ColumnHeader();
            this.panel4 = new System.Windows.Forms.Panel();
            this.groupBox1 = new System.Windows.Forms.GroupBox();
            this.label4 = new System.Windows.Forms.Label();
            this.label3 = new System.Windows.Forms.Label();
            this.label2 = new System.Windows.Forms.Label();
            this.label1 = new System.Windows.Forms.Label();
            this.button1 = new System.Windows.Forms.Button();
            this.button3 = new System.Windows.Forms.Button();
            this.btnConnect = new System.Windows.Forms.Button();
            this.panel5 = new System.Windows.Forms.Panel();
            this.bannerRandom = new System.Windows.Forms.Panel();
            this.backgroundWorkerQueryServers = new System.ComponentModel.BackgroundWorker();
            this.openFileDialog1 = new System.Windows.Forms.OpenFileDialog();
            this.webBrowser2 = new System.Windows.Forms.WebBrowser();
            this.panel1.SuspendLayout();
            this.panel3.SuspendLayout();
            this.panel2.SuspendLayout();
            this.tab_web.SuspendLayout();
            this.tabPage1.SuspendLayout();
            this.panel10.SuspendLayout();
            this.tabPage2.SuspendLayout();
            this.panel9.SuspendLayout();
            this.tabPage3.SuspendLayout();
            this.panel8.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.fileList)).BeginInit();
            this.serverTab.SuspendLayout();
            this.panel6.SuspendLayout();
            this.panel4.SuspendLayout();
            this.groupBox1.SuspendLayout();
            this.panel5.SuspendLayout();
            this.SuspendLayout();
            // 
            // panel1
            // 
            this.panel1.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this.panel1.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(212)))), ((int)(((byte)(204)))), ((int)(((byte)(187)))));
            this.panel1.Controls.Add(this.panel3);
            this.panel1.Controls.Add(this.panel2);
            this.panel1.Location = new System.Drawing.Point(12, 415);
            this.panel1.Name = "panel1";
            this.panel1.Size = new System.Drawing.Size(772, 125);
            this.panel1.TabIndex = 0;
            // 
            // panel3
            // 
            this.panel3.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this.panel3.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(235)))), ((int)(((byte)(230)))), ((int)(((byte)(217)))));
            this.panel3.Controls.Add(this.btnOptions);
            this.panel3.Controls.Add(this.btnCheckFiles);
            this.panel3.Controls.Add(this.btnStartSRB2);
            this.panel3.Location = new System.Drawing.Point(557, 10);
            this.panel3.Name = "panel3";
            this.panel3.Size = new System.Drawing.Size(204, 104);
            this.panel3.TabIndex = 1;
            // 
            // btnOptions
            // 
            this.btnOptions.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this.btnOptions.Font = new System.Drawing.Font("Arial Rounded MT Bold", 9.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.btnOptions.Location = new System.Drawing.Point(105, 72);
            this.btnOptions.Name = "btnOptions";
            this.btnOptions.Size = new System.Drawing.Size(96, 29);
            this.btnOptions.TabIndex = 2;
            this.btnOptions.Text = "Options";
            this.btnOptions.UseVisualStyleBackColor = true;
            this.btnOptions.Click += new System.EventHandler(this.btnOptions_Click);
            // 
            // btnCheckFiles
            // 
            this.btnCheckFiles.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this.btnCheckFiles.Font = new System.Drawing.Font("Arial Rounded MT Bold", 9.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.btnCheckFiles.Location = new System.Drawing.Point(4, 72);
            this.btnCheckFiles.Name = "btnCheckFiles";
            this.btnCheckFiles.Size = new System.Drawing.Size(96, 29);
            this.btnCheckFiles.TabIndex = 1;
            this.btnCheckFiles.Text = "Check Files";
            this.btnCheckFiles.UseVisualStyleBackColor = true;
            this.btnCheckFiles.Click += new System.EventHandler(this.update_Load);
            // 
            // btnStartSRB2
            // 
            this.btnStartSRB2.BackColor = System.Drawing.Color.Transparent;
            this.btnStartSRB2.Font = new System.Drawing.Font("Arial Rounded MT Bold", 20.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.btnStartSRB2.Location = new System.Drawing.Point(4, 4);
            this.btnStartSRB2.Name = "btnStartSRB2";
            this.btnStartSRB2.Size = new System.Drawing.Size(197, 62);
            this.btnStartSRB2.TabIndex = 0;
            this.btnStartSRB2.Text = "Start";
            this.btnStartSRB2.UseVisualStyleBackColor = false;
            this.btnStartSRB2.Click += new System.EventHandler(this.btnStartSRB2_Click);
            // 
            // panel2
            // 
            this.panel2.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                        | System.Windows.Forms.AnchorStyles.Left)));
            this.panel2.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(235)))), ((int)(((byte)(230)))), ((int)(((byte)(217)))));
            this.panel2.Controls.Add(this.lblProgress);
            this.panel2.Controls.Add(this.update_optional);
            this.panel2.Controls.Add(this.progress_overall);
            this.panel2.Controls.Add(this.progress_currentFile);
            this.panel2.Location = new System.Drawing.Point(10, 10);
            this.panel2.Name = "panel2";
            this.panel2.Size = new System.Drawing.Size(541, 105);
            this.panel2.TabIndex = 0;
            // 
            // lblProgress
            // 
            this.lblProgress.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.lblProgress.AutoSize = true;
            this.lblProgress.Font = new System.Drawing.Font("Arial Rounded MT Bold", 9F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.lblProgress.ForeColor = System.Drawing.Color.Black;
            this.lblProgress.Location = new System.Drawing.Point(6, 68);
            this.lblProgress.Name = "lblProgress";
            this.lblProgress.Size = new System.Drawing.Size(299, 14);
            this.lblProgress.TabIndex = 14;
            this.lblProgress.Text = "Click \'Check Files\' to check for and apply updates.";
            // 
            // update_optional
            // 
            this.update_optional.AutoSize = true;
            this.update_optional.Font = new System.Drawing.Font("Arial", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.update_optional.Location = new System.Drawing.Point(372, 68);
            this.update_optional.Name = "update_optional";
            this.update_optional.Size = new System.Drawing.Size(160, 18);
            this.update_optional.TabIndex = 13;
            this.update_optional.Text = "Download Optional Updates";
            this.update_optional.UseVisualStyleBackColor = true;
            // 
            // progress_overall
            // 
            this.progress_overall.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this.progress_overall.Location = new System.Drawing.Point(9, 36);
            this.progress_overall.Name = "progress_overall";
            this.progress_overall.Size = new System.Drawing.Size(522, 26);
            this.progress_overall.TabIndex = 1;
            // 
            // progress_currentFile
            // 
            this.progress_currentFile.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this.progress_currentFile.Location = new System.Drawing.Point(9, 4);
            this.progress_currentFile.Name = "progress_currentFile";
            this.progress_currentFile.Size = new System.Drawing.Size(522, 26);
            this.progress_currentFile.TabIndex = 0;
            // 
            // tab_web
            // 
            this.tab_web.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                        | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this.tab_web.Controls.Add(this.tabPage1);
            this.tab_web.Controls.Add(this.tabPage2);
            this.tab_web.Controls.Add(this.tabPage3);
            this.tab_web.Controls.Add(this.serverTab);
            this.tab_web.Font = new System.Drawing.Font("Arial Rounded MT Bold", 9F);
            this.tab_web.Location = new System.Drawing.Point(0, 138);
            this.tab_web.Name = "tab_web";
            this.tab_web.SelectedIndex = 0;
            this.tab_web.Size = new System.Drawing.Size(770, 256);
            this.tab_web.TabIndex = 1;
            // 
            // tabPage1
            // 
            this.tabPage1.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(212)))), ((int)(((byte)(204)))), ((int)(((byte)(187)))));
            this.tabPage1.Controls.Add(this.panel10);
            this.tabPage1.Font = new System.Drawing.Font("Arial Rounded MT Bold", 9F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.tabPage1.Location = new System.Drawing.Point(4, 23);
            this.tabPage1.Name = "tabPage1";
            this.tabPage1.Padding = new System.Windows.Forms.Padding(3);
            this.tabPage1.Size = new System.Drawing.Size(762, 229);
            this.tabPage1.TabIndex = 0;
            this.tabPage1.Text = "News & Updates";
            // 
            // panel10
            // 
            this.panel10.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(235)))), ((int)(((byte)(230)))), ((int)(((byte)(217)))));
            this.panel10.Controls.Add(this.webBrowser3);
            this.panel10.Location = new System.Drawing.Point(4, 6);
            this.panel10.Name = "panel10";
            this.panel10.Padding = new System.Windows.Forms.Padding(5);
            this.panel10.Size = new System.Drawing.Size(748, 217);
            this.panel10.TabIndex = 14;
            // 
            // webBrowser3
            // 
            this.webBrowser3.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                        | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this.webBrowser3.Location = new System.Drawing.Point(8, 8);
            this.webBrowser3.MinimumSize = new System.Drawing.Size(20, 20);
            this.webBrowser3.Name = "webBrowser3";
            this.webBrowser3.Size = new System.Drawing.Size(732, 201);
            this.webBrowser3.TabIndex = 0;
            this.webBrowser3.Url = new System.Uri("http://update.srb2.org/files_beta/files_beta.xml", System.UriKind.Absolute);
            // 
            // tabPage2
            // 
            this.tabPage2.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(212)))), ((int)(((byte)(204)))), ((int)(((byte)(187)))));
            this.tabPage2.Controls.Add(this.panel9);
            this.tabPage2.Font = new System.Drawing.Font("Arial Rounded MT Bold", 9F);
            this.tabPage2.Location = new System.Drawing.Point(4, 23);
            this.tabPage2.Name = "tabPage2";
            this.tabPage2.Padding = new System.Windows.Forms.Padding(3);
            this.tabPage2.Size = new System.Drawing.Size(762, 229);
            this.tabPage2.TabIndex = 1;
            this.tabPage2.Text = "Change Log";
            // 
            // panel9
            // 
            this.panel9.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(235)))), ((int)(((byte)(230)))), ((int)(((byte)(217)))));
            this.panel9.Controls.Add(this.webBrowser1);
            this.panel9.Location = new System.Drawing.Point(4, 6);
            this.panel9.Name = "panel9";
            this.panel9.Padding = new System.Windows.Forms.Padding(5);
            this.panel9.Size = new System.Drawing.Size(748, 217);
            this.panel9.TabIndex = 13;
            // 
            // webBrowser1
            // 
            this.webBrowser1.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                        | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this.webBrowser1.Location = new System.Drawing.Point(8, 8);
            this.webBrowser1.MinimumSize = new System.Drawing.Size(20, 20);
            this.webBrowser1.Name = "webBrowser1";
            this.webBrowser1.Size = new System.Drawing.Size(732, 201);
            this.webBrowser1.TabIndex = 0;
            this.webBrowser1.Url = new System.Uri("http://update.srb2.org/files_beta/changelog.html", System.UriKind.Absolute);
            // 
            // tabPage3
            // 
            this.tabPage3.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(212)))), ((int)(((byte)(204)))), ((int)(((byte)(187)))));
            this.tabPage3.Controls.Add(this.panel8);
            this.tabPage3.Location = new System.Drawing.Point(4, 23);
            this.tabPage3.Name = "tabPage3";
            this.tabPage3.Padding = new System.Windows.Forms.Padding(3);
            this.tabPage3.Size = new System.Drawing.Size(762, 229);
            this.tabPage3.TabIndex = 2;
            this.tabPage3.Text = "Download List";
            // 
            // panel8
            // 
            this.panel8.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(235)))), ((int)(((byte)(230)))), ((int)(((byte)(217)))));
            this.panel8.Controls.Add(this.fileList);
            this.panel8.Location = new System.Drawing.Point(4, 6);
            this.panel8.Name = "panel8";
            this.panel8.Padding = new System.Windows.Forms.Padding(5);
            this.panel8.Size = new System.Drawing.Size(748, 217);
            this.panel8.TabIndex = 12;
            // 
            // fileList
            // 
            this.fileList.AllowUserToAddRows = false;
            this.fileList.AllowUserToDeleteRows = false;
            this.fileList.AllowUserToResizeRows = false;
            dataGridViewCellStyle1.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(212)))), ((int)(((byte)(204)))), ((int)(((byte)(187)))));
            dataGridViewCellStyle1.Font = new System.Drawing.Font("Arial Rounded MT Bold", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            dataGridViewCellStyle1.ForeColor = System.Drawing.Color.Black;
            dataGridViewCellStyle1.SelectionBackColor = System.Drawing.Color.FromArgb(((int)(((byte)(212)))), ((int)(((byte)(204)))), ((int)(((byte)(187)))));
            dataGridViewCellStyle1.SelectionForeColor = System.Drawing.Color.Black;
            this.fileList.AlternatingRowsDefaultCellStyle = dataGridViewCellStyle1;
            this.fileList.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                        | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this.fileList.AutoSizeColumnsMode = System.Windows.Forms.DataGridViewAutoSizeColumnsMode.Fill;
            this.fileList.BackgroundColor = System.Drawing.Color.FromArgb(((int)(((byte)(230)))), ((int)(((byte)(224)))), ((int)(((byte)(206)))));
            this.fileList.ColumnHeadersBorderStyle = System.Windows.Forms.DataGridViewHeaderBorderStyle.None;
            dataGridViewCellStyle2.Alignment = System.Windows.Forms.DataGridViewContentAlignment.MiddleLeft;
            dataGridViewCellStyle2.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(212)))), ((int)(((byte)(204)))), ((int)(((byte)(187)))));
            dataGridViewCellStyle2.Font = new System.Drawing.Font("Arial Rounded MT Bold", 9F);
            dataGridViewCellStyle2.ForeColor = System.Drawing.Color.Black;
            dataGridViewCellStyle2.Padding = new System.Windows.Forms.Padding(2);
            dataGridViewCellStyle2.SelectionBackColor = System.Drawing.Color.FromArgb(((int)(((byte)(212)))), ((int)(((byte)(204)))), ((int)(((byte)(187)))));
            dataGridViewCellStyle2.SelectionForeColor = System.Drawing.Color.Black;
            dataGridViewCellStyle2.WrapMode = System.Windows.Forms.DataGridViewTriState.True;
            this.fileList.ColumnHeadersDefaultCellStyle = dataGridViewCellStyle2;
            this.fileList.ColumnHeadersHeightSizeMode = System.Windows.Forms.DataGridViewColumnHeadersHeightSizeMode.AutoSize;
            this.fileList.Columns.AddRange(new System.Windows.Forms.DataGridViewColumn[] {
            this.name,
            this.filename,
            this.status,
            this.localmd5,
            this.md5,
            this.optional,
            this.calculated});
            dataGridViewCellStyle4.Alignment = System.Windows.Forms.DataGridViewContentAlignment.MiddleLeft;
            dataGridViewCellStyle4.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(230)))), ((int)(((byte)(224)))), ((int)(((byte)(206)))));
            dataGridViewCellStyle4.Font = new System.Drawing.Font("Arial Rounded MT Bold", 9F);
            dataGridViewCellStyle4.ForeColor = System.Drawing.SystemColors.ControlText;
            dataGridViewCellStyle4.SelectionBackColor = System.Drawing.Color.FromArgb(((int)(((byte)(230)))), ((int)(((byte)(224)))), ((int)(((byte)(206)))));
            dataGridViewCellStyle4.SelectionForeColor = System.Drawing.Color.Black;
            dataGridViewCellStyle4.WrapMode = System.Windows.Forms.DataGridViewTriState.False;
            this.fileList.DefaultCellStyle = dataGridViewCellStyle4;
            this.fileList.GridColor = System.Drawing.Color.FromArgb(((int)(((byte)(212)))), ((int)(((byte)(204)))), ((int)(((byte)(187)))));
            this.fileList.Location = new System.Drawing.Point(8, 8);
            this.fileList.MultiSelect = false;
            this.fileList.Name = "fileList";
            this.fileList.ReadOnly = true;
            dataGridViewCellStyle5.Alignment = System.Windows.Forms.DataGridViewContentAlignment.MiddleLeft;
            dataGridViewCellStyle5.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(230)))), ((int)(((byte)(224)))), ((int)(((byte)(206)))));
            dataGridViewCellStyle5.Font = new System.Drawing.Font("Arial Rounded MT Bold", 9F);
            dataGridViewCellStyle5.ForeColor = System.Drawing.SystemColors.WindowText;
            dataGridViewCellStyle5.SelectionBackColor = System.Drawing.Color.FromArgb(((int)(((byte)(230)))), ((int)(((byte)(224)))), ((int)(((byte)(206)))));
            dataGridViewCellStyle5.SelectionForeColor = System.Drawing.Color.Black;
            dataGridViewCellStyle5.WrapMode = System.Windows.Forms.DataGridViewTriState.True;
            this.fileList.RowHeadersDefaultCellStyle = dataGridViewCellStyle5;
            this.fileList.RowHeadersVisible = false;
            dataGridViewCellStyle6.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(230)))), ((int)(((byte)(224)))), ((int)(((byte)(206)))));
            dataGridViewCellStyle6.ForeColor = System.Drawing.Color.Black;
            dataGridViewCellStyle6.SelectionBackColor = System.Drawing.Color.FromArgb(((int)(((byte)(230)))), ((int)(((byte)(224)))), ((int)(((byte)(206)))));
            dataGridViewCellStyle6.SelectionForeColor = System.Drawing.Color.Black;
            this.fileList.RowsDefaultCellStyle = dataGridViewCellStyle6;
            this.fileList.RowTemplate.DefaultCellStyle.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(230)))), ((int)(((byte)(224)))), ((int)(((byte)(206)))));
            this.fileList.RowTemplate.DefaultCellStyle.Font = new System.Drawing.Font("Arial", 9.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.fileList.RowTemplate.DefaultCellStyle.SelectionBackColor = System.Drawing.Color.FromArgb(((int)(((byte)(230)))), ((int)(((byte)(224)))), ((int)(((byte)(206)))));
            this.fileList.RowTemplate.DefaultCellStyle.SelectionForeColor = System.Drawing.Color.Black;
            this.fileList.RowTemplate.ReadOnly = true;
            this.fileList.Size = new System.Drawing.Size(732, 201);
            this.fileList.TabIndex = 11;
            // 
            // name
            // 
            this.name.DataPropertyName = "name";
            dataGridViewCellStyle3.BackColor = System.Drawing.Color.White;
            this.name.DefaultCellStyle = dataGridViewCellStyle3;
            this.name.FillWeight = 128.7982F;
            this.name.HeaderText = "Name";
            this.name.Name = "name";
            this.name.ReadOnly = true;
            // 
            // filename
            // 
            this.filename.AutoSizeMode = System.Windows.Forms.DataGridViewAutoSizeColumnMode.None;
            this.filename.DataPropertyName = "filename";
            this.filename.HeaderText = "File";
            this.filename.Name = "filename";
            this.filename.ReadOnly = true;
            this.filename.SortMode = System.Windows.Forms.DataGridViewColumnSortMode.NotSortable;
            this.filename.Width = 150;
            // 
            // status
            // 
            this.status.DataPropertyName = "status";
            this.status.FillWeight = 128.7982F;
            this.status.HeaderText = "Status";
            this.status.Name = "status";
            this.status.ReadOnly = true;
            // 
            // localmd5
            // 
            this.localmd5.DataPropertyName = "localmd5";
            this.localmd5.FillWeight = 13.60544F;
            this.localmd5.HeaderText = "localmd5";
            this.localmd5.Name = "localmd5";
            this.localmd5.ReadOnly = true;
            this.localmd5.Visible = false;
            // 
            // md5
            // 
            this.md5.DataPropertyName = "md5";
            this.md5.FillWeight = 128.7982F;
            this.md5.HeaderText = "md5";
            this.md5.Name = "md5";
            this.md5.ReadOnly = true;
            this.md5.Visible = false;
            // 
            // optional
            // 
            this.optional.DataPropertyName = "optional";
            this.optional.HeaderText = "optional";
            this.optional.Name = "optional";
            this.optional.ReadOnly = true;
            this.optional.Visible = false;
            // 
            // calculated
            // 
            this.calculated.DataPropertyName = "calculated";
            this.calculated.HeaderText = "calculated";
            this.calculated.Name = "calculated";
            this.calculated.ReadOnly = true;
            this.calculated.Visible = false;
            // 
            // serverTab
            // 
            this.serverTab.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(212)))), ((int)(((byte)(204)))), ((int)(((byte)(187)))));
            this.serverTab.Controls.Add(this.panel6);
            this.serverTab.Controls.Add(this.panel4);
            this.serverTab.Location = new System.Drawing.Point(4, 23);
            this.serverTab.Name = "serverTab";
            this.serverTab.Padding = new System.Windows.Forms.Padding(3);
            this.serverTab.Size = new System.Drawing.Size(762, 229);
            this.serverTab.TabIndex = 3;
            this.serverTab.Text = "Master Server";
            // 
            // panel6
            // 
            this.panel6.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(235)))), ((int)(((byte)(230)))), ((int)(((byte)(217)))));
            this.panel6.Controls.Add(this.listViewServers);
            this.panel6.Location = new System.Drawing.Point(4, 6);
            this.panel6.Name = "panel6";
            this.panel6.Padding = new System.Windows.Forms.Padding(5);
            this.panel6.Size = new System.Drawing.Size(603, 217);
            this.panel6.TabIndex = 6;
            // 
            // listViewServers
            // 
            this.listViewServers.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                        | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this.listViewServers.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(230)))), ((int)(((byte)(224)))), ((int)(((byte)(206)))));
            this.listViewServers.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this.listViewServers.Columns.AddRange(new System.Windows.Forms.ColumnHeader[] {
            this.colhdrName,
            this.colhdrGametype,
            this.colhdrPing,
            this.colhdrPlayers,
            this.colhdrVersion});
            this.listViewServers.Font = new System.Drawing.Font("Arial Rounded MT Bold", 9F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.listViewServers.FullRowSelect = true;
            this.listViewServers.HideSelection = false;
            this.listViewServers.Location = new System.Drawing.Point(9, 8);
            this.listViewServers.Name = "listViewServers";
            this.listViewServers.ShowItemToolTips = true;
            this.listViewServers.Size = new System.Drawing.Size(586, 201);
            this.listViewServers.TabIndex = 1;
            this.listViewServers.UseCompatibleStateImageBehavior = false;
            this.listViewServers.View = System.Windows.Forms.View.Details;
            this.listViewServers.ItemActivate += new System.EventHandler(this.listViewServers_ItemActivate);
            this.listViewServers.SelectedIndexChanged += new System.EventHandler(this.listViewServers_SelectedIndexChanged);
            this.listViewServers.ColumnClick += new System.Windows.Forms.ColumnClickEventHandler(this.listViewServers_ColumnClick);
            // 
            // colhdrName
            // 
            this.colhdrName.Tag = "";
            this.colhdrName.Text = "Server Name";
            this.colhdrName.Width = 231;
            // 
            // colhdrGametype
            // 
            this.colhdrGametype.Text = "Gametype";
            this.colhdrGametype.Width = 92;
            // 
            // colhdrPing
            // 
            this.colhdrPing.Text = "Ping (ms)";
            this.colhdrPing.Width = 87;
            // 
            // colhdrPlayers
            // 
            this.colhdrPlayers.Text = "Players";
            this.colhdrPlayers.Width = 88;
            // 
            // colhdrVersion
            // 
            this.colhdrVersion.Text = "Version";
            this.colhdrVersion.Width = 87;
            // 
            // panel4
            // 
            this.panel4.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(235)))), ((int)(((byte)(230)))), ((int)(((byte)(217)))));
            this.panel4.Controls.Add(this.groupBox1);
            this.panel4.Controls.Add(this.button1);
            this.panel4.Controls.Add(this.button3);
            this.panel4.Controls.Add(this.btnConnect);
            this.panel4.Location = new System.Drawing.Point(613, 6);
            this.panel4.Name = "panel4";
            this.panel4.Padding = new System.Windows.Forms.Padding(5);
            this.panel4.Size = new System.Drawing.Size(139, 217);
            this.panel4.TabIndex = 5;
            // 
            // groupBox1
            // 
            this.groupBox1.Controls.Add(this.label4);
            this.groupBox1.Controls.Add(this.label3);
            this.groupBox1.Controls.Add(this.label2);
            this.groupBox1.Controls.Add(this.label1);
            this.groupBox1.Location = new System.Drawing.Point(9, 114);
            this.groupBox1.Name = "groupBox1";
            this.groupBox1.Size = new System.Drawing.Size(122, 95);
            this.groupBox1.TabIndex = 5;
            this.groupBox1.TabStop = false;
            this.groupBox1.Text = "Key";
            // 
            // label4
            // 
            this.label4.AutoSize = true;
            this.label4.Font = new System.Drawing.Font("Arial Rounded MT Bold", 9F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label4.ForeColor = System.Drawing.Color.Black;
            this.label4.Location = new System.Drawing.Point(6, 77);
            this.label4.Name = "label4";
            this.label4.Size = new System.Drawing.Size(87, 14);
            this.label4.TabIndex = 3;
            this.label4.Text = "Normal Game";
            // 
            // label3
            // 
            this.label3.AutoSize = true;
            this.label3.Font = new System.Drawing.Font("Arial Rounded MT Bold", 9F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label3.ForeColor = System.Drawing.Color.DimGray;
            this.label3.Location = new System.Drawing.Point(6, 57);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(78, 14);
            this.label3.TabIndex = 2;
            this.label3.Text = "Game is Full";
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Font = new System.Drawing.Font("Arial Rounded MT Bold", 9F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label2.ForeColor = System.Drawing.Color.Green;
            this.label2.Location = new System.Drawing.Point(6, 37);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(99, 14);
            this.label2.TabIndex = 1;
            this.label2.Text = "Cheats Enabled";
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Font = new System.Drawing.Font("Arial Rounded MT Bold", 9F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label1.ForeColor = System.Drawing.Color.Red;
            this.label1.Location = new System.Drawing.Point(6, 17);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(57, 14);
            this.label1.TabIndex = 0;
            this.label1.Text = "Modified";
            // 
            // button1
            // 
            this.button1.Font = new System.Drawing.Font("Arial Rounded MT Bold", 9.75F);
            this.button1.Location = new System.Drawing.Point(8, 8);
            this.button1.Name = "button1";
            this.button1.Size = new System.Drawing.Size(123, 29);
            this.button1.TabIndex = 2;
            this.button1.Text = "Refresh List";
            this.button1.UseVisualStyleBackColor = true;
            this.button1.Click += new System.EventHandler(this.btnRefresh_Click);
            // 
            // button3
            // 
            this.button3.Font = new System.Drawing.Font("Arial Rounded MT Bold", 9.75F);
            this.button3.Location = new System.Drawing.Point(8, 78);
            this.button3.Name = "button3";
            this.button3.Size = new System.Drawing.Size(123, 29);
            this.button3.TabIndex = 4;
            this.button3.Text = "Join Game (IP)";
            this.button3.UseVisualStyleBackColor = true;
            // 
            // btnConnect
            // 
            this.btnConnect.Enabled = false;
            this.btnConnect.Font = new System.Drawing.Font("Arial Rounded MT Bold", 9.75F);
            this.btnConnect.Location = new System.Drawing.Point(8, 43);
            this.btnConnect.Name = "btnConnect";
            this.btnConnect.Size = new System.Drawing.Size(123, 29);
            this.btnConnect.TabIndex = 3;
            this.btnConnect.Text = "Join Game (List)";
            this.btnConnect.UseVisualStyleBackColor = true;
            this.btnConnect.Click += new System.EventHandler(this.btnConnect_Click);
            // 
            // panel5
            // 
            this.panel5.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                        | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this.panel5.BorderStyle = System.Windows.Forms.BorderStyle.Fixed3D;
            this.panel5.Controls.Add(this.tab_web);
            this.panel5.Controls.Add(this.bannerRandom);
            this.panel5.Location = new System.Drawing.Point(12, 13);
            this.panel5.Name = "panel5";
            this.panel5.Size = new System.Drawing.Size(772, 396);
            this.panel5.TabIndex = 1;
            // 
            // bannerRandom
            // 
            this.bannerRandom.BackgroundImage = global::SRB2Updater.Properties.Resources.Banner4;
            this.bannerRandom.Location = new System.Drawing.Point(0, -2);
            this.bannerRandom.Name = "bannerRandom";
            this.bannerRandom.Size = new System.Drawing.Size(768, 123);
            this.bannerRandom.TabIndex = 3;
            // 
            // backgroundWorkerQueryServers
            // 
            this.backgroundWorkerQueryServers.DoWork += new System.ComponentModel.DoWorkEventHandler(this.backgroundWorkerQueryServers_DoWork);
            // 
            // openFileDialog1
            // 
            this.openFileDialog1.DefaultExt = "exe";
            this.openFileDialog1.Filter = "Executables files|*.exe|All files|*.*";
            // 
            // webBrowser2
            // 
            this.webBrowser2.Location = new System.Drawing.Point(8, 8);
            this.webBrowser2.MinimumSize = new System.Drawing.Size(20, 20);
            this.webBrowser2.Name = "webBrowser2";
            this.webBrowser2.Size = new System.Drawing.Size(567, 201);
            this.webBrowser2.TabIndex = 0;
            this.webBrowser2.Url = new System.Uri("http://update.srb2.org/files_beta/changelog.html", System.UriKind.Absolute);
            // 
            // Launcher
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(235)))), ((int)(((byte)(230)))), ((int)(((byte)(217)))));
            this.ClientSize = new System.Drawing.Size(796, 552);
            this.Controls.Add(this.panel5);
            this.Controls.Add(this.panel1);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.KeyPreview = true;
            this.Name = "Launcher";
            this.Text = "Launcher";
            this.FormClosed += new System.Windows.Forms.FormClosedEventHandler(this.Launcher_FormClosed);
            this.KeyUp += new System.Windows.Forms.KeyEventHandler(this.Launcher_KeyUp);
            this.panel1.ResumeLayout(false);
            this.panel3.ResumeLayout(false);
            this.panel2.ResumeLayout(false);
            this.panel2.PerformLayout();
            this.tab_web.ResumeLayout(false);
            this.tabPage1.ResumeLayout(false);
            this.panel10.ResumeLayout(false);
            this.tabPage2.ResumeLayout(false);
            this.panel9.ResumeLayout(false);
            this.tabPage3.ResumeLayout(false);
            this.panel8.ResumeLayout(false);
            ((System.ComponentModel.ISupportInitialize)(this.fileList)).EndInit();
            this.serverTab.ResumeLayout(false);
            this.panel6.ResumeLayout(false);
            this.panel4.ResumeLayout(false);
            this.groupBox1.ResumeLayout(false);
            this.groupBox1.PerformLayout();
            this.panel5.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.Panel panel1;
        private System.Windows.Forms.Panel panel2;
        private System.Windows.Forms.Panel panel3;
        private System.Windows.Forms.Button btnOptions;
        private System.Windows.Forms.Button btnCheckFiles;
        private System.Windows.Forms.Button btnStartSRB2;
        private System.Windows.Forms.TabControl tab_web;
        private System.Windows.Forms.TabPage tabPage1;
        private System.Windows.Forms.TabPage tabPage2;
        private System.Windows.Forms.Panel panel5;
        private System.Windows.Forms.WebBrowser webBrowser1;
        private System.Windows.Forms.Panel bannerRandom;
        private System.Windows.Forms.ProgressBar progress_overall;
        private System.Windows.Forms.ProgressBar progress_currentFile;
        private System.Windows.Forms.TabPage tabPage3;
        private System.Windows.Forms.DataGridView fileList;
        private System.Windows.Forms.CheckBox update_optional;
        private System.Windows.Forms.Label lblProgress;
        private System.Windows.Forms.DataGridViewTextBoxColumn name;
        private System.Windows.Forms.DataGridViewTextBoxColumn filename;
        private System.Windows.Forms.DataGridViewTextBoxColumn status;
        private System.Windows.Forms.DataGridViewTextBoxColumn localmd5;
        private System.Windows.Forms.DataGridViewTextBoxColumn md5;
        private System.Windows.Forms.DataGridViewTextBoxColumn optional;
        private System.Windows.Forms.DataGridViewTextBoxColumn calculated;
        private System.Windows.Forms.TabPage serverTab;
        private System.Windows.Forms.ListView listViewServers;
        private System.Windows.Forms.ColumnHeader colhdrName;
        private System.Windows.Forms.ColumnHeader colhdrGametype;
        private System.Windows.Forms.ColumnHeader colhdrPing;
        private System.Windows.Forms.ColumnHeader colhdrPlayers;
        private System.Windows.Forms.ColumnHeader colhdrVersion;
        private System.ComponentModel.BackgroundWorker backgroundWorkerQueryServers;
        private System.Windows.Forms.Panel panel4;
        private System.Windows.Forms.Button button1;
        private System.Windows.Forms.Button button3;
        private System.Windows.Forms.Button btnConnect;
        private System.Windows.Forms.OpenFileDialog openFileDialog1;
        private System.Windows.Forms.Panel panel6;
        private System.Windows.Forms.Panel panel8;
        private System.Windows.Forms.Panel panel9;
        private System.Windows.Forms.Panel panel10;
        private System.Windows.Forms.WebBrowser webBrowser3;
        private System.Windows.Forms.WebBrowser webBrowser2;
        private System.Windows.Forms.GroupBox groupBox1;
        private System.Windows.Forms.Label label3;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.Label label4;
    }
}