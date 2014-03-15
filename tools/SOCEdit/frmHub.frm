VERSION 5.00
Begin VB.Form frmHub 
   Caption         =   "SOC Editor"
   ClientHeight    =   6960
   ClientLeft      =   60
   ClientTop       =   345
   ClientWidth     =   4920
   Icon            =   "frmHub.frx":0000
   LinkTopic       =   "Form1"
   MaxButton       =   0   'False
   ScaleHeight     =   6960
   ScaleWidth      =   4920
   StartUpPosition =   3  'Windows Default
   Begin VB.CommandButton cmdCreateBlank 
      Caption         =   "Make a &Blank SOC"
      Height          =   255
      Left            =   240
      TabIndex        =   22
      Top             =   2520
      Width           =   2055
   End
   Begin VB.CommandButton cmdUnlockables 
      Caption         =   "Edit &Unlockables"
      Enabled         =   0   'False
      Height          =   495
      Left            =   2760
      Style           =   1  'Graphical
      TabIndex        =   21
      Top             =   6360
      Width           =   1095
   End
   Begin VB.CommandButton cmdAuthor 
      Caption         =   "Enter &Author Info"
      Enabled         =   0   'False
      Height          =   495
      Left            =   120
      Style           =   1  'Graphical
      TabIndex        =   19
      Top             =   3960
      Width           =   1215
   End
   Begin VB.CommandButton cmdHelp 
      Caption         =   "Getting Starte&d / READ ME FIRST!"
      Height          =   495
      Left            =   480
      TabIndex        =   18
      Top             =   2880
      Width           =   1575
   End
   Begin VB.CommandButton cmdEditCutscenes 
      Caption         =   "Edit C&utscenes"
      Enabled         =   0   'False
      Height          =   495
      Left            =   120
      TabIndex        =   17
      Top             =   6360
      Width           =   1215
   End
   Begin VB.CommandButton cmdCharacterEdit 
      Caption         =   "Edit &Character Select Screen"
      Enabled         =   0   'False
      Height          =   495
      Left            =   120
      Style           =   1  'Graphical
      TabIndex        =   16
      Top             =   5760
      Width           =   1215
   End
   Begin VB.PictureBox Picture1 
      Height          =   1965
      Left            =   2760
      Picture         =   "frmHub.frx":0442
      ScaleHeight     =   1905
      ScaleWidth      =   1905
      TabIndex        =   15
      Top             =   3960
      Width           =   1965
   End
   Begin VB.CommandButton cmdSoundEdit 
      Caption         =   "Edit &Sounds"
      Enabled         =   0   'False
      Height          =   495
      Left            =   120
      TabIndex        =   14
      Top             =   5160
      Width           =   1215
   End
   Begin VB.CommandButton cmdEmblemEdit 
      Caption         =   "Edit &Emblem Locations"
      Enabled         =   0   'False
      Height          =   495
      Left            =   120
      Style           =   1  'Graphical
      TabIndex        =   13
      Top             =   4560
      Width           =   1215
   End
   Begin VB.CommandButton cmdHUDEdit 
      Caption         =   "Edit &HUD Coordinates"
      Enabled         =   0   'False
      Height          =   495
      Left            =   1440
      Style           =   1  'Graphical
      TabIndex        =   12
      Top             =   3960
      Width           =   1215
   End
   Begin VB.CommandButton cmdMaincfg 
      Caption         =   "Edit &Global Game Settings"
      Enabled         =   0   'False
      Height          =   495
      Left            =   1440
      Style           =   1  'Graphical
      TabIndex        =   11
      Top             =   4560
      Width           =   1215
   End
   Begin VB.DriveListBox Drive2 
      Height          =   315
      Left            =   2640
      TabIndex        =   9
      Top             =   360
      Width           =   2175
   End
   Begin VB.DirListBox Dir2 
      Height          =   1665
      Left            =   2520
      TabIndex        =   8
      Top             =   720
      Width           =   2295
   End
   Begin VB.FileListBox File1 
      Height          =   1455
      Left            =   2520
      Pattern         =   "*.soc"
      TabIndex        =   7
      Top             =   2400
      Width           =   2295
   End
   Begin VB.DriveListBox Drive1 
      Height          =   315
      Left            =   120
      TabIndex        =   6
      Top             =   360
      Width           =   2295
   End
   Begin VB.DirListBox Dir1 
      Height          =   1665
      Left            =   120
      TabIndex        =   4
      Top             =   720
      Width           =   2295
   End
   Begin VB.CommandButton cmdAbout 
      Caption         =   "&About"
      Height          =   375
      Left            =   3960
      TabIndex        =   3
      Top             =   6000
      Width           =   735
   End
   Begin VB.CommandButton cmdStateEdit 
      Caption         =   "Edit St&ates"
      Enabled         =   0   'False
      Height          =   495
      Left            =   1440
      TabIndex        =   2
      Top             =   6360
      Width           =   1215
   End
   Begin VB.CommandButton cmdLevelHeader 
      Caption         =   "Edit &Level Headers"
      Enabled         =   0   'False
      Height          =   495
      Left            =   1440
      Style           =   1  'Graphical
      TabIndex        =   1
      Top             =   5160
      Width           =   1215
   End
   Begin VB.CommandButton cmdThingEdit 
      Caption         =   "Edit &Things"
      Enabled         =   0   'False
      Height          =   495
      Left            =   1440
      TabIndex        =   0
      Top             =   5760
      Width           =   1215
   End
   Begin VB.Label lblAuthor 
      Caption         =   "Modification By:"
      Height          =   495
      Left            =   120
      TabIndex        =   20
      Top             =   3480
      Width           =   2295
   End
   Begin VB.Label lblSOCFile 
      Caption         =   "SOC File to use (double click):"
      Height          =   255
      Left            =   2640
      TabIndex        =   10
      Top             =   120
      Width           =   2175
   End
   Begin VB.Label lblSourcePath 
      Caption         =   "Path to SRB2 Source Code:"
      Height          =   255
      Left            =   120
      TabIndex        =   5
      Top             =   120
      Width           =   2175
   End
End
Attribute VB_Name = "frmHub"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Option Explicit

Private Sub cmdAbout_Click()
    MsgBox App.Title & " v" & App.Major & "." & App.Minor & "." & App.Revision & vbCrLf & "By " & App.CompanyName & vbCrLf & "(SSNTails)" & vbCrLf & App.Comments & vbCrLf & App.FileDescription
End Sub

Private Sub cmdAuthor_Click()
    Dim Response As String
    
    Response$ = InputBox("Enter name to appear on credits (type in NOBODY to delete):", "Modification By", GetAuthor)
    
    If Response = "" Then Exit Sub
    
    Response = TrimComplete(Response)
    
    If UCase(Response) = "NOBODY" Then
        Call WriteAuthor(True, Response)
        lblAuthor.Caption = "Modification By: "
    Else
        Call WriteAuthor(False, Response)
        lblAuthor.Caption = "Modification By: " & Response
    End If
End Sub

Private Sub cmdCharacterEdit_Click()
    frmCharacterEdit.Show vbModal, Me
End Sub

Private Sub cmdCreateBlank_Click()
    Dim socname As String
    
    socname = InputBox("This file will be created in the directory you have selected on the main window." & vbCrLf & vbCrLf & "Enter the filename you want (do not include .SOC at the end):", "Make A Blank SOC")
    Trim (socname)
    
    If InStr(LCase(socname), ".soc") > 0 Then
        MsgBox "The thing says not to include the .SOC at the end, stupid.", vbOKOnly, "You goofed!"
        Exit Sub
    End If
    
    If Len(socname) > 0 Then
        socname = socname & ".soc"
        
        Dim myFSOSOC As New Scripting.FileSystemObject
        Dim tsSOC As TextStream
    
        Set tsSOC = myFSOSOC.OpenTextFile(File1.Path & "\" & socname, ForWriting, True)
        tsSOC.Close
        Set myFSOSOC = Nothing
        
        MsgBox "Blank SOC named " & socname & " created in " & File1.Path, vbOKOnly, "Success!"
    End If
End Sub

Private Sub cmdEditCutscenes_Click()
    frmCutsceneEdit.Show vbModal, Me
End Sub

Private Sub cmdEmblemEdit_Click()
    frmEmblemEdit.Show vbModal, Me
End Sub

Private Sub cmdHelp_Click()
    frmHelp.Show vbModal, Me
End Sub

Private Sub cmdHUDEdit_Click()
    frmHUDEdit.Show vbModal, Me
End Sub

Private Sub cmdLevelHeader_Click()
    frmLevelHeader.Show vbModal, Me
End Sub

Private Sub cmdMaincfg_Click()
    frmMaincfg.Show vbModal, Me
End Sub

Private Sub cmdSoundEdit_Click()
    frmSoundEdit.Show vbModal, Me
End Sub

Private Sub cmdStateEdit_Click()
    frmStateEdit.Show vbModal, Me
End Sub

Private Sub cmdThingEdit_Click()
    frmThingEdit.Show vbModal, Me
End Sub

Private Sub cmdUnlockables_Click()
    frmUnlockablesEdit.Show vbModal, Me
End Sub

Private Sub Dir1_Change()
    SourcePath = Dir1.Path
End Sub

Private Sub Dir2_Change()
    File1.Path = Dir2.Path
End Sub

Private Sub Drive1_Change()
    Dir1.Path = Drive1.Drive
End Sub

Private Sub Drive2_Change()
    Dir2.Path = Drive2.Drive
End Sub

Private Sub File1_DblClick()
    SOCTemp = File1.Path & "\" & "socedit.tmp"
    SOCFile = File1.Path & "\" & File1.List(File1.ListIndex)
    MsgBox "You are now using the file: " & vbCrLf & SOCFile
    cmdLevelHeader.Enabled = True
    cmdThingEdit.Enabled = True
    cmdStateEdit.Enabled = True
    cmdHUDEdit.Enabled = True
    cmdMaincfg.Enabled = True
    cmdEmblemEdit.Enabled = True
    cmdSoundEdit.Enabled = True
    cmdCharacterEdit.Enabled = True
    cmdEditCutscenes.Enabled = True
    cmdAuthor.Enabled = True
    cmdUnlockables.Enabled = True
    lblAuthor.Caption = "Modification By: " & GetAuthor
End Sub

Private Sub Form_Load()
    SourcePath = App.Path
    Dir1.Path = SourcePath
End Sub

Private Function GetAuthor() As String
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
        
        If UCase(word) = "MODBY" Then
            GetAuthor = word2
            Exit Do
        End If
    Loop
    
    ts.Close
    Set myFSO = Nothing
End Function

Private Sub WriteAuthor(Remove As Boolean, ModderName As String)
    Dim myFSOSource As New Scripting.FileSystemObject
    Dim tsSource As TextStream
    Dim myFSOTarget As New Scripting.FileSystemObject
    Dim tsTarget As TextStream
    Dim line As String
    Dim word As String
    Dim word2 As String
   
    Set tsSource = myFSOSource.OpenTextFile(SOCFile, ForReading, False)
    Set tsTarget = myFSOTarget.OpenTextFile(SOCTemp, ForWriting, True)
    
    Do While Not tsSource.AtEndOfStream
        line = tsSource.ReadLine
        word = UCase(FirstToken(line))
        word2 = UCase(SecondToken(line))
        
        'If the entry exists in the SOC, delete it.
        If word <> "MODBY" Then
            tsTarget.WriteLine line
        End If
    Loop
    
    tsSource.Close
    Set myFSOSource = Nothing
    
    If Remove = False Then
        If line <> "" Then tsTarget.WriteLine ""
    
        tsTarget.WriteLine "ModBy " & ModderName
    End If
    
    tsTarget.Close
    Set myFSOTarget = Nothing
    
    FileCopy SOCTemp, SOCFile
    
    Kill SOCTemp
    
    If Remove = True Then
        MsgBox "Name removed."
    Else
        MsgBox "Name Saved."
    End If
End Sub

