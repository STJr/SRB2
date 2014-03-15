VERSION 5.00
Begin VB.Form frmLevelHeader 
   Caption         =   "Level Header Info"
   ClientHeight    =   5250
   ClientLeft      =   60
   ClientTop       =   345
   ClientWidth     =   7650
   Icon            =   "frmLevelHeader.frx":0000
   LinkTopic       =   "Form1"
   MaxButton       =   0   'False
   ScaleHeight     =   5250
   ScaleWidth      =   7650
   StartUpPosition =   3  'Windows Default
   Begin VB.TextBox txtRunSOC 
      Height          =   285
      Left            =   6240
      MaxLength       =   8
      TabIndex        =   49
      Top             =   4680
      Width           =   1215
   End
   Begin VB.CheckBox chkLevelSelect 
      Caption         =   "Show on Host Game selection menu"
      Height          =   375
      Left            =   1680
      TabIndex        =   48
      Top             =   4800
      Width           =   1935
   End
   Begin VB.CheckBox chkTimeAttack 
      Caption         =   "Include in Time Attack calculations"
      Height          =   255
      Left            =   1680
      TabIndex        =   47
      Top             =   4440
      Width           =   2775
   End
   Begin VB.CheckBox chkNoReload 
      Caption         =   "Retain level state when player dies."
      Height          =   255
      Left            =   1680
      TabIndex        =   46
      Top             =   4080
      Width           =   2895
   End
   Begin VB.CommandButton cmdSave 
      Caption         =   "&Save Map"
      Height          =   735
      Left            =   120
      Style           =   1  'Graphical
      TabIndex        =   45
      Top             =   4440
      Width           =   1215
   End
   Begin VB.CommandButton cmdRename 
      Caption         =   "&Rename Map"
      Height          =   375
      Left            =   120
      TabIndex        =   44
      Top             =   3960
      Width           =   1215
   End
   Begin VB.CommandButton cmdDelete 
      Caption         =   "&Delete Map"
      Height          =   375
      Left            =   120
      TabIndex        =   43
      Top             =   3480
      Width           =   1215
   End
   Begin VB.CommandButton cmdAddMap 
      Caption         =   "&Add Map"
      Height          =   375
      Left            =   120
      TabIndex        =   42
      Top             =   3000
      Width           =   1215
   End
   Begin VB.CheckBox chkNossmusic 
      Caption         =   "Disable Super Sonic music changes"
      Height          =   255
      Left            =   4560
      TabIndex        =   29
      Top             =   1200
      Width           =   2895
   End
   Begin VB.CheckBox chkHidden 
      Caption         =   "Don't show on level selection menu"
      Height          =   255
      Left            =   4560
      TabIndex        =   28
      Top             =   480
      Width           =   2895
   End
   Begin VB.TextBox txtCountdown 
      Height          =   285
      Left            =   6360
      MaxLength       =   3
      TabIndex        =   26
      Top             =   840
      Width           =   735
   End
   Begin VB.TextBox txtCutscenenum 
      Height          =   285
      Left            =   4440
      MaxLength       =   3
      TabIndex        =   25
      Top             =   3720
      Width           =   495
   End
   Begin VB.TextBox txtPrecutscenenum 
      Height          =   285
      Left            =   2640
      MaxLength       =   3
      TabIndex        =   22
      Top             =   3720
      Width           =   495
   End
   Begin VB.CheckBox chkScriptislump 
      Caption         =   "Script is a lump in WAD, not a file"
      Height          =   255
      Left            =   1680
      TabIndex        =   21
      Top             =   3360
      Width           =   2775
   End
   Begin VB.TextBox txtScriptname 
      Height          =   285
      Left            =   2640
      MaxLength       =   191
      TabIndex        =   19
      Top             =   3000
      Width           =   1455
   End
   Begin VB.TextBox txtSkynum 
      Height          =   285
      Left            =   2640
      MaxLength       =   4
      TabIndex        =   17
      Top             =   2640
      Width           =   495
   End
   Begin VB.ComboBox cmbWeather 
      Height          =   315
      ItemData        =   "frmLevelHeader.frx":0442
      Left            =   2640
      List            =   "frmLevelHeader.frx":0458
      TabIndex        =   15
      Top             =   2280
      Width           =   2295
   End
   Begin VB.TextBox txtForcecharacter 
      Height          =   285
      Left            =   2640
      MaxLength       =   2
      TabIndex        =   13
      Top             =   1920
      Width           =   495
   End
   Begin VB.ComboBox cmbMusicslot 
      Height          =   315
      Left            =   2640
      TabIndex        =   11
      Top             =   1560
      Width           =   1815
   End
   Begin VB.TextBox txtNextlevel 
      Height          =   285
      Left            =   2640
      MaxLength       =   4
      TabIndex        =   9
      Top             =   1200
      Width           =   615
   End
   Begin VB.Frame frmTypeOfLevel 
      Caption         =   "Type of Level"
      Height          =   2775
      Left            =   5040
      TabIndex        =   8
      Top             =   1680
      Width           =   2535
      Begin VB.CheckBox chkTypeoflevel 
         Caption         =   "Christmas"
         Height          =   255
         Index           =   11
         Left            =   1440
         TabIndex        =   41
         Tag             =   "1024"
         Top             =   960
         Width           =   975
      End
      Begin VB.CheckBox chkTypeoflevel 
         Caption         =   "2D"
         Height          =   255
         Index           =   10
         Left            =   1440
         TabIndex        =   40
         Tag             =   "512"
         Top             =   720
         Width           =   735
      End
      Begin VB.CheckBox chkTypeoflevel 
         Caption         =   "Mario"
         Height          =   255
         Index           =   9
         Left            =   120
         TabIndex        =   39
         Tag             =   "256"
         Top             =   2400
         Width           =   1455
      End
      Begin VB.CheckBox chkTypeoflevel 
         Caption         =   "Sonic Adventure"
         Height          =   255
         Index           =   8
         Left            =   120
         TabIndex        =   38
         Tag             =   "128"
         Top             =   2160
         Width           =   1575
      End
      Begin VB.CheckBox chkTypeoflevel 
         Caption         =   "NiGHTS"
         Height          =   255
         Index           =   7
         Left            =   120
         TabIndex        =   37
         Tag             =   "64"
         Top             =   1920
         Width           =   1335
      End
      Begin VB.CheckBox chkTypeoflevel 
         Caption         =   "Chaos"
         Height          =   255
         Index           =   6
         Left            =   120
         TabIndex        =   36
         Tag             =   "32"
         Top             =   1680
         Width           =   1455
      End
      Begin VB.CheckBox chkTypeoflevel 
         Caption         =   "Capture the Flag"
         Height          =   255
         Index           =   5
         Left            =   120
         TabIndex        =   35
         Tag             =   "16"
         Top             =   1440
         Width           =   1695
      End
      Begin VB.CheckBox chkTypeoflevel 
         Caption         =   "Tag"
         Height          =   255
         Index           =   4
         Left            =   120
         TabIndex        =   34
         Tag             =   "8"
         Top             =   1200
         Width           =   1215
      End
      Begin VB.CheckBox chkTypeoflevel 
         Caption         =   "Match"
         Height          =   255
         Index           =   3
         Left            =   120
         TabIndex        =   33
         Tag             =   "4"
         Top             =   960
         Width           =   855
      End
      Begin VB.CheckBox chkTypeoflevel 
         Caption         =   "Race"
         Height          =   255
         Index           =   2
         Left            =   120
         TabIndex        =   32
         Tag             =   "2"
         Top             =   720
         Width           =   855
      End
      Begin VB.CheckBox chkTypeoflevel 
         Caption         =   "Cooperative"
         Height          =   255
         Index           =   1
         Left            =   120
         TabIndex        =   31
         Tag             =   "1"
         Top             =   480
         Width           =   1215
      End
      Begin VB.CheckBox chkTypeoflevel 
         Caption         =   "Single Player"
         Height          =   255
         Index           =   0
         Left            =   120
         TabIndex        =   30
         Tag             =   "4096"
         Top             =   240
         Width           =   1215
      End
   End
   Begin VB.CheckBox chkNozone 
      Caption         =   "Don't show ""ZONE"" after Level Name"
      Height          =   255
      Left            =   4560
      TabIndex        =   7
      Top             =   120
      Width           =   3015
   End
   Begin VB.TextBox txtAct 
      Height          =   285
      Left            =   2640
      MaxLength       =   2
      TabIndex        =   5
      Top             =   840
      Width           =   495
   End
   Begin VB.TextBox txtInterscreen 
      Height          =   285
      Left            =   2640
      MaxLength       =   8
      TabIndex        =   3
      Top             =   480
      Width           =   1335
   End
   Begin VB.ListBox lstMaps 
      Height          =   2790
      Left            =   120
      Sorted          =   -1  'True
      TabIndex        =   2
      Top             =   120
      Width           =   1215
   End
   Begin VB.TextBox txtLevelName 
      Height          =   285
      Left            =   2640
      MaxLength       =   32
      TabIndex        =   0
      Top             =   120
      Width           =   1815
   End
   Begin VB.Label lblRunSOC 
      Alignment       =   1  'Right Justify
      Caption         =   "Run SOC at level load (lump name):"
      Height          =   495
      Left            =   4440
      TabIndex        =   50
      Top             =   4560
      Width           =   1695
   End
   Begin VB.Label lblCountdown 
      Alignment       =   1  'Right Justify
      Caption         =   "Level Timer (seconds):"
      Height          =   255
      Left            =   4560
      TabIndex        =   27
      Top             =   840
      Width           =   1695
   End
   Begin VB.Label lblCutscenenum 
      Alignment       =   1  'Right Justify
      Caption         =   "Cutscene to play after level:"
      Height          =   495
      Left            =   3240
      TabIndex        =   24
      Top             =   3600
      Width           =   1095
   End
   Begin VB.Label lblPrecutscenenum 
      Alignment       =   1  'Right Justify
      Caption         =   "Cutscene to play before level:"
      Height          =   375
      Left            =   1320
      TabIndex        =   23
      Top             =   3600
      Width           =   1215
   End
   Begin VB.Label lblScriptName 
      Alignment       =   1  'Right Justify
      Caption         =   "Script Name:"
      Height          =   255
      Left            =   1440
      TabIndex        =   20
      Top             =   3000
      Width           =   1095
   End
   Begin VB.Label lblSkynum 
      Alignment       =   1  'Right Justify
      Caption         =   "Sky #:"
      Height          =   255
      Left            =   1800
      TabIndex        =   18
      Top             =   2640
      Width           =   735
   End
   Begin VB.Label Label1 
      Alignment       =   1  'Right Justify
      Caption         =   "Weather:"
      Height          =   255
      Left            =   1680
      TabIndex        =   16
      Top             =   2280
      Width           =   855
   End
   Begin VB.Label lblForcecharacter 
      Caption         =   "Force Character #:"
      Height          =   375
      Left            =   1440
      TabIndex        =   14
      Top             =   1800
      Width           =   1095
   End
   Begin VB.Label lblMusicslot 
      Alignment       =   1  'Right Justify
      Caption         =   "Music:"
      Height          =   255
      Left            =   1800
      TabIndex        =   12
      Top             =   1560
      Width           =   735
   End
   Begin VB.Label lblNextlevel 
      Alignment       =   1  'Right Justify
      Caption         =   "Next Level:"
      Height          =   255
      Left            =   1440
      TabIndex        =   10
      Top             =   1200
      Width           =   1095
   End
   Begin VB.Label lblAct 
      Alignment       =   1  'Right Justify
      Caption         =   "Act:"
      Height          =   255
      Left            =   2040
      TabIndex        =   6
      Top             =   840
      Width           =   495
   End
   Begin VB.Label lblInterscreen 
      Alignment       =   1  'Right Justify
      Caption         =   "Intermission BG:"
      Height          =   255
      Left            =   1320
      TabIndex        =   4
      Top             =   480
      Width           =   1215
   End
   Begin VB.Label lblLevelName 
      Alignment       =   1  'Right Justify
      Caption         =   "Level Name:"
      Height          =   255
      Left            =   1560
      TabIndex        =   1
      Top             =   120
      Width           =   975
   End
End
Attribute VB_Name = "frmLevelHeader"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Option Explicit

Private Sub cmdAddMap_Click()
    Dim Response As String
    Dim NewNum As Integer
    
    Response$ = InputBox("Enter the new level (NUMBER ONLY):")
    
    If Response = "" Then
        Exit Sub
    End If
    
    NewNum = Val(TrimComplete(Response))
    
    lstMaps.AddItem "Level " & NewNum
    lstMaps.ListIndex = lstMaps.ListCount - 1
End Sub

Private Sub cmdDelete_Click()
    Dim i As Integer
    
    If MsgBox("Delete this level header?", vbYesNo) = vbNo Then
        Exit Sub
    End If
    
    i = InStr(lstMaps.List(lstMaps.ListIndex), " ") + 1
    
    i = Mid(lstMaps.List(lstMaps.ListIndex), i, Len(lstMaps.List(lstMaps.ListIndex)) - i + 1)
    i = Val(i)
    Call WriteLevel(True, i)
    lstMaps.RemoveItem lstMaps.ListIndex
    
    If lstMaps.ListCount > 0 Then
        lstMaps.ListIndex = 0
    End If
End Sub

Private Sub cmdRename_Click()
    Dim Response As String
    Dim NewNum As Integer
    Dim i As Integer
    
    Response$ = InputBox("Rename level to (NUMBER ONLY):")
    
    If Response = "" Then
        Exit Sub
    End If
    
    NewNum = Val(TrimComplete(Response))
    
    i = InStr(lstMaps.List(lstMaps.ListIndex), " ") + 1
    
    i = Mid(lstMaps.List(lstMaps.ListIndex), i, Len(lstMaps.List(lstMaps.ListIndex)) - i + 1)
    i = Val(i)
    Call WriteLevel(True, i)
    lstMaps.List(lstMaps.ListIndex) = "Level " & NewNum
    Call cmdSave_Click
End Sub

Private Sub cmdSave_Click()
    Dim i As Integer
    
    i = InStr(lstMaps.List(lstMaps.ListIndex), " ") + 1
    
    i = Val(Mid(lstMaps.List(lstMaps.ListIndex), i, Len(lstMaps.List(lstMaps.ListIndex)) - i + 1))
    Call WriteLevel(False, i)
End Sub

Private Sub Form_Load()
    Call LoadMusic
    Call LoadSOCMaps
    If lstMaps.ListCount > 0 Then lstMaps.ListIndex = 0
End Sub

Private Sub LoadSOCMaps()
    Dim myFSO As New Scripting.FileSystemObject
    Dim ts As TextStream
    Dim line As String
    Dim word As String
    Dim word2 As String
    
    Set ts = myFSO.OpenTextFile(SOCFile, ForReading, False)
    
    lstMaps.Clear
    
SOCLoad:
    Do While Not ts.AtEndOfStream
        line = ts.ReadLine
        
        If Left(line, 1) = "#" Then GoTo SOCLoad
        
        If Left(line, 1) = vbCrLf Then GoTo SOCLoad
        
        If Len(line) < 1 Then GoTo SOCLoad
        
        word = FirstToken(line)
        word2 = SecondToken(line)
        
        If UCase(word) = "LEVEL" Then
            lstMaps.AddItem ("Level " & Val(word2))
        End If
    Loop
    
    ts.Close
    Set myFSO = Nothing
End Sub

Private Sub LoadMusic()
    Dim myFSO As New Scripting.FileSystemObject
    Dim ts As TextStream
    Dim line As String
    Dim number As Integer
    Dim startclip As Integer, endclip As Integer
    Dim addstring As String
    
    ChDir SourcePath
    Set ts = myFSO.OpenTextFile("sounds.h", ForReading, False)
    
    Do While InStr(ts.ReadLine, "Music list (don't edit this comment!)") = 0
    Loop
    
    ts.SkipLine ' typedef enum
    ts.SkipLine ' {
    
    line = ts.ReadLine
    number = 0
    
    cmbMusicslot.Clear
    
    Do While InStr(line, "NUMMUSIC") = 0
        startclip = InStr(line, "mus_")
        If InStr(line, "mus_") <> 0 Then
            endclip = InStr(line, ",")
            line = Mid(line, startclip, endclip - startclip)
            addstring = number & " - " & line
            cmbMusicslot.AddItem addstring
            number = number + 1
        End If
        line = ts.ReadLine
    Loop
    
    ts.Close
    Set myFSO = Nothing
End Sub

Private Sub ClearForm()
    Dim j As Integer
    
    txtLevelName.Text = ""
    txtInterscreen.Text = ""
    txtAct.Text = ""
    txtNextlevel.Text = ""
    cmbMusicslot.Text = ""
    txtForcecharacter.Text = ""
    cmbWeather.Text = ""
    txtSkynum.Text = ""
    txtScriptname.Text = ""
    chkScriptislump.Value = 0
    txtPrecutscenenum.Text = ""
    txtCutscenenum.Text = ""
    txtRunSOC.Text = ""
    chkNozone.Value = 0
    chkHidden.Value = 0
    txtCountdown.Text = ""
    chkNossmusic.Value = 0
    chkNoReload.Value = 0
    chkTimeAttack.Value = 0
    chkLevelSelect = 0
    
    For j = 0 To 11
        chkTypeoflevel(j).Value = 0
    Next j
End Sub

Private Sub lstMaps_Click()
    Dim startclip As Integer
    Call ClearForm
    startclip = InStr(lstMaps.List(lstMaps.ListIndex), " ")
    Call LoadSOCMapInfo(Val(Mid(lstMaps.List(lstMaps.ListIndex), startclip + 1, Len(lstMaps.List(lstMaps.ListIndex)))))
End Sub

Private Sub LoadSOCMapInfo(num As Integer)
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
        
        word = UCase(FirstToken(line))
        word2 = UCase(SecondToken(line))
        
        If word = "LEVEL" Then
            If Val(word2) = num Then
                Do While Len(line) > 0 And Not ts.AtEndOfStream
                    line = ts.ReadLine
                    word = UCase(FirstToken(line))
                    word2 = UCase(SecondTokenEqual(line))
                    
                    If word = "LEVELNAME" Then
                        txtLevelName.Text = word2
                    ElseIf word = "INTERSCREEN" Then
                        txtInterscreen.Text = word2
                    ElseIf word = "ACT" Then
                        txtAct.Text = Val(word2)
                    ElseIf word = "NOZONE" Then
                        chkNozone.Value = Val(word2)
                    ElseIf word = "TYPEOFLEVEL" Then
                        ProcessMapFlags (Val(word2))
                    ElseIf word = "NEXTLEVEL" Then
                        txtNextlevel.Text = Val(word2)
                    ElseIf word = "MUSICSLOT" Then
                        cmbMusicslot.ListIndex = Val(word2)
                    ElseIf word = "FORCECHARACTER" Then
                        txtForcecharacter.Text = Val(word2)
                    ElseIf word = "WEATHER" Then
                        cmbWeather.ListIndex = Val(word2)
                    ElseIf word = "SKYNUM" Then
                        txtSkynum.Text = Val(word2)
                    ElseIf word = "SCRIPTNAME" Then
                        txtScriptname.Text = word2
                    ElseIf word = "SCRIPTISLUMP" Then
                        chkScriptislump.Value = Val(word2)
                    ElseIf word = "PRECUTSCENENUM" Then
                        txtPrecutscenenum.Text = Val(word2)
                    ElseIf word = "CUTSCENENUM" Then
                        txtCutscenenum.Text = Val(word2)
                    ElseIf word = "COUNTDOWN" Then
                        txtCountdown.Text = Val(word2)
                    ElseIf word = "HIDDEN" Then
                        chkHidden.Value = Val(word2)
                    ElseIf word = "NOSSMUSIC" Then
                        chkNossmusic.Value = Val(word2)
                    ElseIf word = "NORELOAD" Then
                        chkNoReload.Value = Val(word2)
                    ElseIf word = "TIMEATTACK" Then
                        chkTimeAttack.Value = Val(word2)
                    ElseIf word = "LEVELSELECT" Then
                        chkLevelSelect.Value = Val(word2)
                    ElseIf word = "RUNSOC" Then
                        txtRunSOC.Text = word2
                   ElseIf Len(line) > 0 And Left(line, 1) <> "#" Then
                        MsgBox "Error in SOC!" & vbCrLf & "Unknown line: " & line
                    End If
                Loop
                Exit Do
            End If
        End If
    Loop
    
    ts.Close
    Set myFSO = Nothing
End Sub

Private Sub ProcessMapFlags(flags As Long)
    Dim j As Integer
    
    For j = 0 To 11
        If flags And chkTypeoflevel(j).Tag Then
            chkTypeoflevel(j).Value = 1
        Else
            chkTypeoflevel(j).Value = 0
        End If
    Next j
End Sub

Private Sub WriteLevel(Remove As Boolean, Mapnum As Integer)
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
        
        i = InStr(lstMaps.List(lstMaps.ListIndex), " ") + 1
    
        i = Mid(lstMaps.List(lstMaps.ListIndex), i, Len(lstMaps.List(lstMaps.ListIndex)) - i + 1)
        i = Val(i)
        'If the current level exists in the SOC, delete it.
        If word = "LEVEL" And Val(word2) = i Then
            Do While Len(TrimComplete(tsSource.ReadLine)) > 0 And Not (tsSource.AtEndOfStream)
            Loop
        Else
            tsTarget.WriteLine line
        End If
    Loop
    
    tsSource.Close
    Set myFSOSource = Nothing
    
    If Remove = False Then
        If line <> "" Then tsTarget.WriteLine ""
    
        tsTarget.WriteLine UCase(lstMaps.List(lstMaps.ListIndex))
        txtLevelName.Text = TrimComplete(txtLevelName.Text)
        txtInterscreen.Text = TrimComplete(txtInterscreen.Text)
        txtAct.Text = TrimComplete(txtAct.Text)
        txtNextlevel.Text = TrimComplete(txtNextlevel.Text)
        cmbMusicslot.Text = TrimComplete(cmbMusicslot.Text)
        txtForcecharacter.Text = TrimComplete(txtForcecharacter.Text)
        cmbWeather.Text = TrimComplete(cmbWeather.Text)
        txtSkynum.Text = TrimComplete(txtSkynum.Text)
        txtScriptname.Text = TrimComplete(txtScriptname.Text)
        txtPrecutscenenum.Text = TrimComplete(txtPrecutscenenum.Text)
        txtCutscenenum.Text = TrimComplete(txtCutscenenum.Text)
        txtCountdown.Text = TrimComplete(txtCountdown.Text)
        txtRunSOC.Text = TrimComplete(txtRunSOC.Text)
        
        If txtLevelName.Text <> "" Then tsTarget.WriteLine "LEVELNAME = " & txtLevelName.Text
        If txtInterscreen.Text <> "" Then tsTarget.WriteLine "INTERSCREEN = " & txtInterscreen.Text
        If txtAct.Text <> "" Then tsTarget.WriteLine "ACT = " & Val(txtAct.Text)
        If txtNextlevel.Text <> "" Then tsTarget.WriteLine "NEXTLEVEL = " & Val(txtNextlevel.Text)
        If cmbMusicslot.Text <> "" Then tsTarget.WriteLine "MUSICSLOT = " & cmbMusicslot.ListIndex
        If txtForcecharacter.Text <> "" Then tsTarget.WriteLine "FORCECHARACTER = " & Val(txtForcecharacter.Text)
        If cmbWeather.Text <> "" Then tsTarget.WriteLine "WEATHER = " & cmbWeather.ListIndex
        If txtSkynum.Text <> "" Then tsTarget.WriteLine "SKYNUM = " & Val(txtSkynum.Text)
        If txtScriptname.Text <> "" Then tsTarget.WriteLine "SCRIPTNAME = " & txtScriptname.Text
        If txtPrecutscenenum.Text <> "" Then tsTarget.WriteLine "PRECUTSCENENUM = " & Val(txtPrecutscenenum.Text)
        If txtCutscenenum.Text <> "" Then tsTarget.WriteLine "CUTSCENENUM = " & Val(txtCutscenenum.Text)
        If txtCountdown.Text <> "" Then tsTarget.WriteLine "COUNTDOWN = " & Val(txtCountdown.Text)
        If chkScriptislump.Value = 1 Then tsTarget.WriteLine "SCRIPTISLUMP = 1"
        If chkNozone.Value = 1 Then tsTarget.WriteLine "NOZONE = 1"
        If chkHidden.Value = 1 Then tsTarget.WriteLine "HIDDEN = 1"
        If chkNossmusic.Value = 1 Then tsTarget.WriteLine "NOSSMUSIC = 1"
        If chkNoReload.Value = 1 Then tsTarget.WriteLine "NORELOAD = 1"
        If chkTimeAttack.Value = 1 Then tsTarget.WriteLine "TIMEATTACK = 1"
        If chkLevelSelect.Value = 1 Then tsTarget.WriteLine "LEVELSELECT = 1"
        If txtRunSOC.Text <> "" Then tsTarget.WriteLine "RUNSOC = " & txtRunSOC.Text
    
        flags = 0
        For i = 0 To 11
            If chkTypeoflevel(i).Value = 1 Then
                flags = flags + Val(chkTypeoflevel(i).Tag)
            End If
        Next
        
        If flags > 0 Then tsTarget.WriteLine "TYPEOFLEVEL = " & flags
    End If
    
    tsTarget.Close
    Set myFSOTarget = Nothing
    
    FileCopy SOCTemp, SOCFile
    
    Kill SOCTemp
    
    If Remove = True Then
        MsgBox "Level Deleted."
    Else
        MsgBox "Level Saved."
    End If
End Sub

