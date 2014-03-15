VERSION 5.00
Begin VB.Form frmMaincfg 
   Caption         =   "Global Game Settings"
   ClientHeight    =   5295
   ClientLeft      =   60
   ClientTop       =   345
   ClientWidth     =   9360
   Icon            =   "frmMaincfg.frx":0000
   LinkTopic       =   "Form1"
   MaxButton       =   0   'False
   ScaleHeight     =   5295
   ScaleWidth      =   9360
   StartUpPosition =   3  'Windows Default
   Begin VB.Frame frmReset 
      Caption         =   "Reset Data (Be sure this is at the TOP of your SOC)"
      Height          =   975
      Left            =   4800
      TabIndex        =   43
      Top             =   4200
      Width           =   4455
      Begin VB.CheckBox chkReset 
         Caption         =   "Thing Properties"
         Height          =   255
         Index           =   2
         Left            =   1680
         TabIndex        =   46
         Tag             =   "4"
         Top             =   240
         Width           =   1575
      End
      Begin VB.CheckBox chkReset 
         Caption         =   "States"
         Height          =   255
         Index           =   1
         Left            =   240
         TabIndex        =   45
         Tag             =   "2"
         Top             =   600
         Width           =   1335
      End
      Begin VB.CheckBox chkReset 
         Caption         =   "Sprite Names"
         Height          =   255
         Index           =   0
         Left            =   240
         TabIndex        =   44
         Tag             =   "1"
         Top             =   240
         Width           =   1575
      End
   End
   Begin VB.CheckBox chkDisableSpeedAdjust 
      Caption         =   "Disable speed adjustment of player animations depending on how fast they are moving."
      Height          =   375
      Left            =   1080
      TabIndex        =   42
      Top             =   4200
      Width           =   3615
   End
   Begin VB.TextBox txtTitleScrollSpeed 
      Height          =   285
      Left            =   4080
      TabIndex        =   41
      Top             =   1920
      Width           =   495
   End
   Begin VB.CheckBox chkLoopTitle 
      Caption         =   "Loop the title screen music?"
      Height          =   195
      Left            =   1080
      TabIndex        =   39
      Top             =   3840
      Width           =   2415
   End
   Begin VB.TextBox txtCreditsCutscene 
      Height          =   285
      Left            =   4080
      TabIndex        =   37
      Top             =   1560
      Width           =   495
   End
   Begin VB.CommandButton cmdSave 
      Caption         =   "&Save"
      Height          =   495
      Left            =   120
      TabIndex        =   36
      Top             =   3120
      Width           =   735
   End
   Begin VB.CommandButton cmdReload 
      Caption         =   "&Reload"
      Height          =   495
      Left            =   120
      TabIndex        =   35
      Top             =   2520
      Width           =   735
   End
   Begin VB.TextBox txtNumemblems 
      Height          =   285
      Left            =   4080
      MaxLength       =   2
      TabIndex        =   33
      Top             =   3360
      Width           =   495
   End
   Begin VB.TextBox txtGamedata 
      Height          =   285
      Left            =   3240
      MaxLength       =   64
      TabIndex        =   31
      Top             =   2880
      Width           =   1335
   End
   Begin VB.TextBox txtExeccfg 
      Height          =   285
      Left            =   3240
      TabIndex        =   9
      Top             =   2400
      Width           =   1335
   End
   Begin VB.Frame frmTimers 
      Caption         =   "Timers (35 = 1 second)"
      Height          =   3975
      Left            =   4800
      TabIndex        =   8
      Top             =   120
      Width           =   4455
      Begin VB.TextBox txtGameovertics 
         Height          =   285
         Left            =   3000
         TabIndex        =   29
         Top             =   3480
         Width           =   1335
      End
      Begin VB.TextBox txtHelpertics 
         Height          =   285
         Left            =   3000
         TabIndex        =   27
         Top             =   3120
         Width           =   1335
      End
      Begin VB.TextBox txtParalooptics 
         Height          =   285
         Left            =   3000
         TabIndex        =   25
         Top             =   2760
         Width           =   1335
      End
      Begin VB.TextBox txtExtralifetics 
         Height          =   285
         Left            =   3000
         TabIndex        =   23
         Top             =   2400
         Width           =   1335
      End
      Begin VB.TextBox txtSpacetimetics 
         Height          =   285
         Left            =   3000
         TabIndex        =   21
         Top             =   2040
         Width           =   1335
      End
      Begin VB.TextBox txtUnderwatertics 
         Height          =   285
         Left            =   3000
         TabIndex        =   19
         Top             =   1680
         Width           =   1335
      End
      Begin VB.TextBox txtTailsflytics 
         Height          =   285
         Left            =   3000
         TabIndex        =   17
         Top             =   1320
         Width           =   1335
      End
      Begin VB.TextBox txtFlashingtics 
         Height          =   285
         Left            =   3000
         TabIndex        =   15
         Top             =   960
         Width           =   1335
      End
      Begin VB.TextBox txtSneakertics 
         Height          =   285
         Left            =   3000
         TabIndex        =   13
         Top             =   600
         Width           =   1335
      End
      Begin VB.TextBox txtInvulntics 
         Height          =   285
         Left            =   3000
         TabIndex        =   11
         Top             =   240
         Width           =   1335
      End
      Begin VB.Label lblGameovertics 
         Alignment       =   1  'Right Justify
         Caption         =   "Game Over Screen Time:"
         Height          =   255
         Left            =   960
         TabIndex        =   30
         Top             =   3480
         Width           =   1935
      End
      Begin VB.Label lblHelpertics 
         Alignment       =   1  'Right Justify
         Caption         =   "NiGHTS Nightopian Helper Time:"
         Height          =   255
         Left            =   240
         TabIndex        =   28
         Top             =   3120
         Width           =   2655
      End
      Begin VB.Label lblParalooptics 
         Alignment       =   1  'Right Justify
         Caption         =   "NiGHTS Paraloop Powerup Time:"
         Height          =   255
         Left            =   360
         TabIndex        =   26
         Top             =   2760
         Width           =   2535
      End
      Begin VB.Label lblExtralifetics 
         Alignment       =   1  'Right Justify
         Caption         =   "Extra Life Music Duration:"
         Height          =   255
         Left            =   960
         TabIndex        =   24
         Top             =   2400
         Width           =   1935
      End
      Begin VB.Label lblSpacetimetics 
         Alignment       =   1  'Right Justify
         Caption         =   "Space Breath Timeout:"
         Height          =   255
         Left            =   1200
         TabIndex        =   22
         Top             =   2040
         Width           =   1695
      End
      Begin VB.Label lblUnderwatertics 
         Alignment       =   1  'Right Justify
         Caption         =   "Underwater Breath Timeout:"
         Height          =   255
         Left            =   840
         TabIndex        =   20
         Top             =   1680
         Width           =   2055
      End
      Begin VB.Label lblTailsflytics 
         Alignment       =   1  'Right Justify
         Caption         =   "Tails Flying Time:"
         Height          =   255
         Left            =   1440
         TabIndex        =   18
         Top             =   1320
         Width           =   1455
      End
      Begin VB.Label lblFlashingtics 
         Alignment       =   1  'Right Justify
         Caption         =   "Flashing Time After Being Hit:"
         Height          =   255
         Left            =   360
         TabIndex        =   16
         Top             =   960
         Width           =   2535
      End
      Begin VB.Label lblSneakertics 
         Alignment       =   1  'Right Justify
         Caption         =   "Super Sneakers Time:"
         Height          =   255
         Left            =   240
         TabIndex        =   14
         Top             =   600
         Width           =   2655
      End
      Begin VB.Label lblInvulntics 
         Alignment       =   1  'Right Justify
         Caption         =   "Invincibility Time:"
         Height          =   255
         Left            =   360
         TabIndex        =   12
         Top             =   240
         Width           =   2535
      End
   End
   Begin VB.TextBox txtIntrotoplay 
      Height          =   285
      Left            =   4080
      TabIndex        =   6
      Top             =   1200
      Width           =   495
   End
   Begin VB.TextBox txtRacestage_start 
      Height          =   285
      Left            =   4080
      MaxLength       =   4
      TabIndex        =   4
      Top             =   840
      Width           =   495
   End
   Begin VB.TextBox txtSpstage_start 
      Height          =   285
      Left            =   4080
      MaxLength       =   4
      TabIndex        =   2
      Top             =   480
      Width           =   495
   End
   Begin VB.TextBox txtSstage_start 
      Height          =   285
      Left            =   4080
      MaxLength       =   4
      TabIndex        =   0
      Top             =   120
      Width           =   495
   End
   Begin VB.Label lblTitleScrollSpeed 
      Alignment       =   1  'Right Justify
      Caption         =   "Scroll speed of title background:"
      Height          =   255
      Left            =   1560
      TabIndex        =   40
      Top             =   1920
      Width           =   2415
   End
   Begin VB.Label lblCreditsCutscene 
      Alignment       =   1  'Right Justify
      Caption         =   "Cutscene # to replace credits with:"
      Height          =   255
      Left            =   1080
      TabIndex        =   38
      Top             =   1560
      Width           =   2895
   End
   Begin VB.Label lblNumemblems 
      Alignment       =   1  'Right Justify
      Caption         =   "# of LEVEL Emblems (Gamedata field must also be filled out):"
      Height          =   375
      Left            =   1440
      TabIndex        =   34
      Top             =   3240
      Width           =   2535
   End
   Begin VB.Label lblGamedata 
      Alignment       =   1  'Right Justify
      Caption         =   "Gamedata file (to save mod emblems and time data):"
      Height          =   375
      Left            =   960
      TabIndex        =   32
      Top             =   2760
      Width           =   2175
   End
   Begin VB.Label lblExeccfg 
      Alignment       =   1  'Right Justify
      Caption         =   "CFG file to instantly execute upon loading this SOC:"
      Height          =   495
      Left            =   960
      TabIndex        =   10
      Top             =   2280
      Width           =   2175
   End
   Begin VB.Label lblIntrotoplay 
      Alignment       =   1  'Right Justify
      Caption         =   "Cutscene # to use for introduction:"
      Height          =   255
      Left            =   1440
      TabIndex        =   7
      Top             =   1200
      Width           =   2535
   End
   Begin VB.Label lblRacestage_start 
      Alignment       =   1  'Right Justify
      Caption         =   "Racing mode starts/loops back to this map #:"
      Height          =   255
      Left            =   720
      TabIndex        =   5
      Top             =   840
      Width           =   3255
   End
   Begin VB.Label lblSpstage_start 
      Alignment       =   1  'Right Justify
      Caption         =   "Single Player Game Starts on this map #:"
      Height          =   255
      Left            =   1080
      TabIndex        =   3
      Top             =   480
      Width           =   2895
   End
   Begin VB.Label lblSstage_start 
      Alignment       =   1  'Right Justify
      Caption         =   "First Special Stage Map #:"
      Height          =   255
      Left            =   2040
      TabIndex        =   1
      Top             =   120
      Width           =   1935
   End
End
Attribute VB_Name = "frmMaincfg"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Private Sub cmdReload_Click()
    Call Reload
End Sub

Private Sub cmdSave_Click()
    Call WriteSettings
End Sub

Private Sub Form_Load()
    Call Reload
End Sub

Private Sub ClearForm()
    Dim i As Integer
    
    txtSstage_start.Text = ""
    txtSpstage_start.Text = ""
    txtRacestage_start.Text = ""
    txtIntrotoplay.Text = ""
    txtExeccfg.Text = ""
    txtGamedata.Text = ""
    txtNumemblems.Text = ""
    txtInvulntics.Text = ""
    txtSneakertics.Text = ""
    txtFlashingtics.Text = ""
    txtTailsflytics.Text = ""
    txtUnderwatertics.Text = ""
    txtSpacetimetics.Text = ""
    txtExtralifetics.Text = ""
    txtParalooptics.Text = ""
    txtHelpertics.Text = ""
    txtGameovertics.Text = ""
    txtCreditsCutscene.Text = ""
    txtTitleScrollSpeed.Text = ""
    chkLoopTitle.Value = 0
    chkDisableSpeedAdjust.Value = 0
    
    For i = 0 To 2
        chkReset(i).Value = 0
    Next i
End Sub

Private Sub Reload()
    Call ClearForm
    Call ReadSOCMaincfg
End Sub

Private Sub ReadSOCMaincfg()
    Dim myFSO As New Scripting.FileSystemObject
    Dim ts As TextStream
    Dim line As String
    Dim word As String
    Dim word2 As String
    
    Set ts = myFSO.OpenTextFile(SOCFile, ForReading, False)
    
SOCLoad:
    Do While Not ts.AtEndOfStream
        line = ts.ReadLine
        
        If Left(line, 1) = "#" Then GoTo SOCLoad
        
        If Left(line, 1) = vbCrLf Then GoTo SOCLoad
        
        If Len(line) < 1 Then GoTo SOCLoad
        
        word = FirstToken(line)
        word2 = SecondToken(line)
        
        If UCase(word) = "MAINCFG" Then
            Do While Len(line) > 0 And Not ts.AtEndOfStream
                line = ts.ReadLine
                word = UCase(FirstToken(line))
                word2 = UCase(SecondTokenEqual(line))
                    
                If word = "SSTAGE_START" Then
                    txtSstage_start.Text = Val(word2)
                ElseIf word = "SPSTAGE_START" Then
                    txtSpstage_start.Text = Val(word2)
                ElseIf word = "RACESTAGE_START" Then
                    txtRacestage_start.Text = Val(word2)
                ElseIf word = "INVULNTICS" Then
                    txtInvulntics.Text = Val(word2)
                ElseIf word = "SNEAKERTICS" Then
                    txtSneakertics.Text = Val(word2)
                ElseIf word = "FLASHINGTICS" Then
                    txtFlashingtics.Text = Val(word2)
                ElseIf word = "TAILSFLYTICS" Then
                    txtTailsflytics.Text = Val(word2)
                ElseIf word = "UNDERWATERTICS" Then
                    txtUnderwatertics.Text = Val(word2)
                ElseIf word = "SPACETIMETICS" Then
                    txtSpacetimetics.Text = Val(word2)
                ElseIf word = "EXTRALIFETICS" Then
                    txtExtralifetics.Text = Val(word2)
                ElseIf word = "PARALOOPTICS" Then
                    txtParalooptics.Text = Val(word2)
                ElseIf word = "HELPERTICS" Then
                    txtHelpertics.Text = Val(word2)
                ElseIf word = "GAMEOVERTICS" Then
                    txtGameovertics.Text = Val(word2)
                ElseIf word = "INTROTOPLAY" Then
                    txtIntrotoplay.Text = Val(word2)
                ElseIf word = "CREDITSCUTSCENE" Then
                    txtCreditsCutscene.Text = Val(word2)
                ElseIf word = "TITLESCROLLSPEED" Then
                    txtTitleScrollSpeed.Text = Val(word2)
                ElseIf word = "LOOPTITLE" Then
                    chkLoopTitle.Value = Val(word2)
                ElseIf word = "DISABLESPEEDADJUST" Then
                    chkDisableSpeedAdjust.Value = Val(word2)
                ElseIf word = "GAMEDATA" Then
                    txtGamedata.Text = word2
                ElseIf word = "NUMEMBLEMS" Then
                    txtNumemblems.Text = Val(word2)
                ElseIf word = "RESETDATA" Then
                    Dim resetflags As Integer
                    Dim z As Integer
                    
                    resetflags = Val(word2)
                    
                    For z = 0 To 2
                        If resetflags And chkReset(z).Tag Then
                            chkReset(z).Value = 1
                        Else
                            chkReset(z).Value = 0
                        End If
                    Next z
                ElseIf Len(line) > 0 And Left(line, 1) <> "#" Then
                    MsgBox "Error in SOC!" & vbCrLf & "Unknown line: " & line
                End If
            Loop
            Exit Do
        End If
    Loop
    
    ts.Close
    Set myFSO = Nothing
End Sub

Private Sub WriteSettings()
    Dim myFSOSource As New Scripting.FileSystemObject
    Dim tsSource As TextStream
    Dim myFSOTarget As New Scripting.FileSystemObject
    Dim tsTarget As TextStream
    Dim line As String
    Dim word As String
    Dim word2 As String
    Dim flags As Long
    Dim i As Integer
    
    Set tsSource = myFSOSource.OpenTextFile(SOCFile, ForReading, False)
    Set tsTarget = myFSOTarget.OpenTextFile(SOCTemp, ForWriting, True)
    
    Do While Not tsSource.AtEndOfStream
        line = tsSource.ReadLine
        word = UCase(FirstToken(line))
        word2 = UCase(SecondToken(line))
        
        'If the category exists in the SOC, delete it.
        If word = "MAINCFG" Then
            Do While Len(TrimComplete(tsSource.ReadLine)) > 0 And Not (tsSource.AtEndOfStream)
            Loop
        Else
            tsTarget.WriteLine line
        End If
    Loop
    
    tsSource.Close
    Set myFSOSource = Nothing
    
    If line <> "" Then tsTarget.WriteLine ""
    
    tsTarget.WriteLine "MAINCFG CATEGORY"
    txtSstage_start.Text = TrimComplete(txtSstage_start.Text)
    txtSpstage_start.Text = TrimComplete(txtSpstage_start.Text)
    txtRacestage_start.Text = TrimComplete(txtRacestage_start.Text)
    txtIntrotoplay.Text = TrimComplete(txtIntrotoplay.Text)
    txtCreditsCutscene.Text = TrimComplete(txtCreditsCutscene.Text)
    txtExeccfg.Text = TrimComplete(txtExeccfg.Text)
    txtGamedata.Text = TrimComplete(txtGamedata.Text)
    txtNumemblems.Text = TrimComplete(txtNumemblems.Text)
    txtInvulntics.Text = TrimComplete(txtInvulntics.Text)
    txtSneakertics.Text = TrimComplete(txtSneakertics.Text)
    txtFlashingtics.Text = TrimComplete(txtFlashingtics.Text)
    txtTailsflytics.Text = TrimComplete(txtTailsflytics.Text)
    txtUnderwatertics.Text = TrimComplete(txtUnderwatertics.Text)
    txtSpacetimetics.Text = TrimComplete(txtSpacetimetics.Text)
    txtExtralifetics.Text = TrimComplete(txtExtralifetics.Text)
    txtParalooptics.Text = TrimComplete(txtParalooptics.Text)
    txtHelpertics.Text = TrimComplete(txtHelpertics.Text)
    txtGameovertics.Text = TrimComplete(txtGameovertics.Text)
    txtTitleScrollSpeed.Text = TrimComplete(txtTitleScrollSpeed.Text)
      
    If txtSstage_start.Text <> "" Then tsTarget.WriteLine "SSTAGE_START = " & Val(txtSstage_start.Text)
    If txtSpstage_start.Text <> "" Then tsTarget.WriteLine "SPSTAGE_START = " & Val(txtSpstage_start.Text)
    If txtRacestage_start.Text <> "" Then tsTarget.WriteLine "RACESTAGE_START = " & Val(txtRacestage_start.Text)
    If txtIntrotoplay.Text <> "" Then tsTarget.WriteLine "INTROTOPLAY = " & Val(txtIntrotoplay.Text)
    If txtCreditsCutscene.Text <> "" Then tsTarget.WriteLine "CREDITSCUTSCENE = " & Val(txtCreditsCutscene.Text)
    If txtExeccfg.Text <> "" Then tsTarget.WriteLine "EXECCFG = " & txtExeccfg.Text
    If txtGamedata.Text <> "" Then tsTarget.WriteLine "GAMEDATA = " & txtGamedata.Text
    If txtNumemblems.Text <> "" Then
        tsTarget.WriteLine "NUMEMBLEMS = " & Val(txtNumemblems.Text)
        EditedNumemblems = True
    End If
    If txtInvulntics.Text <> "" Then tsTarget.WriteLine "INVULNTICS = " & Val(txtInvulntics.Text)
    If txtSneakertics.Text <> "" Then tsTarget.WriteLine "SNEAKERTICS = " & Val(txtSneakertics.Text)
    If txtFlashingtics.Text <> "" Then tsTarget.WriteLine "FLASHINGTICS = " & Val(txtFlashingtics.Text)
    If txtTailsflytics.Text <> "" Then tsTarget.WriteLine "TAILSFLYTICS = " & Val(txtTailsflytics.Text)
    If txtUnderwatertics.Text <> "" Then tsTarget.WriteLine "UNDERWATERTICS = " & Val(txtUnderwatertics.Text)
    If txtSpacetimetics.Text <> "" Then tsTarget.WriteLine "SPACETIMETICS = " & Val(txtSpacetimetics.Text)
    If txtExtralifetics.Text <> "" Then tsTarget.WriteLine "EXTRALIFETICS = " & Val(txtExtralifetics.Text)
    If txtParalooptics.Text <> "" Then tsTarget.WriteLine "PARALOOPTICS = " & Val(txtParalooptics.Text)
    If txtHelpertics.Text <> "" Then tsTarget.WriteLine "HELPERTICS = " & Val(txtHelpertics.Text)
    If txtGameovertics.Text <> "" Then tsTarget.WriteLine "GAMEOVERTICS = " & Val(txtGameovertics.Text)
    If txtTitleScrollSpeed.Text <> "" Then tsTarget.WriteLine "TITLESCROLLSPEED = " & Val(txtTitleScrollSpeed.Text)
    If chkLoopTitle.Value = 1 Then tsTarget.WriteLine "LOOPTITLE = " & chkLoopTitle.Value
    If chkDisableSpeedAdjust.Value = 1 Then tsTarget.WriteLine "DISABLESPEEDADJUST = " & chkDisableSpeedAdjust.Value
    
    flags = 0
    For i = 0 To 2
        If chkReset(i).Value = 1 Then
            flags = flags + Val(chkReset(i).Tag)
        End If
    Next
        
    If flags > 0 Then tsTarget.WriteLine "RESETDATA = " & flags
    
    tsTarget.Close
    Set myFSOTarget = Nothing
    
    FileCopy SOCTemp, SOCFile
    
    Kill SOCTemp
    
    MsgBox "Settings Saved."
End Sub

