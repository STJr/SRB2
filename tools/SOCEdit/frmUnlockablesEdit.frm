VERSION 5.00
Begin VB.Form frmUnlockablesEdit 
   Caption         =   "Unlockables Edit"
   ClientHeight    =   3675
   ClientLeft      =   60
   ClientTop       =   345
   ClientWidth     =   8130
   Icon            =   "frmUnlockablesEdit.frx":0000
   LinkTopic       =   "Form1"
   MaxButton       =   0   'False
   ScaleHeight     =   3675
   ScaleWidth      =   8130
   StartUpPosition =   3  'Windows Default
   Begin VB.CheckBox chkGrade 
      Caption         =   "Must have beaten Ultimate"
      Height          =   255
      Index           =   4
      Left            =   5400
      TabIndex        =   20
      Tag             =   "1024"
      Top             =   2160
      Width           =   2655
   End
   Begin VB.CheckBox chkGrade 
      Caption         =   "Must have beaten Very Hard"
      Height          =   255
      Index           =   3
      Left            =   5400
      TabIndex        =   19
      Tag             =   "128"
      Top             =   1800
      Width           =   2655
   End
   Begin VB.CheckBox chkGrade 
      Caption         =   "Must have all emblems"
      Height          =   255
      Index           =   2
      Left            =   5400
      TabIndex        =   18
      Tag             =   "16"
      Top             =   1440
      Width           =   2055
   End
   Begin VB.CheckBox chkGrade 
      Caption         =   "Must have gotten all 7 emeralds"
      Height          =   255
      Index           =   1
      Left            =   5400
      TabIndex        =   17
      Tag             =   "8"
      Top             =   1080
      Width           =   2655
   End
   Begin VB.CheckBox chkGrade 
      Caption         =   "Game must be completed"
      Height          =   255
      Index           =   0
      Left            =   5400
      TabIndex        =   16
      Tag             =   "1"
      Top             =   720
      Width           =   2175
   End
   Begin VB.TextBox txtVar 
      Height          =   285
      Left            =   4320
      TabIndex        =   14
      Top             =   2640
      Width           =   615
   End
   Begin VB.ComboBox cmbType 
      Height          =   315
      ItemData        =   "frmUnlockablesEdit.frx":0442
      Left            =   3360
      List            =   "frmUnlockablesEdit.frx":044C
      TabIndex        =   12
      Top             =   2160
      Width           =   1575
   End
   Begin VB.TextBox txtNeededTime 
      Height          =   285
      Left            =   4080
      TabIndex        =   10
      Top             =   1680
      Width           =   855
   End
   Begin VB.TextBox txtNeededEmblems 
      Height          =   285
      Left            =   4440
      TabIndex        =   9
      Top             =   1200
      Width           =   495
   End
   Begin VB.TextBox txtObjective 
      Height          =   285
      Left            =   3240
      TabIndex        =   7
      Top             =   720
      Width           =   1695
   End
   Begin VB.TextBox txtName 
      Height          =   285
      Left            =   3240
      TabIndex        =   5
      Top             =   240
      Width           =   1695
   End
   Begin VB.CommandButton cmdDelete 
      Caption         =   "&Delete from SOC"
      Height          =   375
      Left            =   3480
      TabIndex        =   3
      Top             =   3120
      Width           =   1575
   End
   Begin VB.CommandButton cmdSave 
      Caption         =   "&Save Changes"
      Height          =   375
      Left            =   1800
      TabIndex        =   2
      Top             =   3120
      Width           =   1575
   End
   Begin VB.ListBox lstUnlockables 
      Height          =   2985
      ItemData        =   "frmUnlockablesEdit.frx":046A
      Left            =   120
      List            =   "frmUnlockablesEdit.frx":049B
      TabIndex        =   0
      Top             =   480
      Width           =   1215
   End
   Begin VB.Label lblNote 
      Caption         =   "Note: All requirements are combinable."
      Height          =   495
      Left            =   6000
      TabIndex        =   22
      Top             =   2760
      Width           =   1695
   End
   Begin VB.Label lblOtherReqs 
      Caption         =   "Other Requirements:"
      Height          =   255
      Left            =   5400
      TabIndex        =   21
      Top             =   360
      Width           =   1935
   End
   Begin VB.Label lblVar 
      Alignment       =   1  'Right Justify
      Caption         =   "Map # to warp to:"
      Height          =   255
      Left            =   2880
      TabIndex        =   15
      Top             =   2640
      Width           =   1335
   End
   Begin VB.Label lblType 
      Alignment       =   1  'Right Justify
      Caption         =   "Type of Unlockable:"
      Height          =   255
      Left            =   1800
      TabIndex        =   13
      Top             =   2160
      Width           =   1455
   End
   Begin VB.Label lblNeededTime 
      Alignment       =   1  'Right Justify
      Caption         =   "Needed time on Time Attack rank (in seconds):"
      Height          =   375
      Left            =   1440
      TabIndex        =   11
      Top             =   1560
      Width           =   2535
   End
   Begin VB.Label lblNeededEmblems 
      Alignment       =   1  'Right Justify
      Caption         =   "# of Emblems Needed:"
      Height          =   255
      Left            =   2640
      TabIndex        =   8
      Top             =   1200
      Width           =   1695
   End
   Begin VB.Label lblObjective 
      Alignment       =   1  'Right Justify
      Caption         =   "Objective:"
      Height          =   255
      Left            =   2400
      TabIndex        =   6
      Top             =   720
      Width           =   735
   End
   Begin VB.Label lblName 
      Alignment       =   1  'Right Justify
      Caption         =   "Name:"
      Height          =   255
      Left            =   2640
      TabIndex        =   4
      Top             =   240
      Width           =   495
   End
   Begin VB.Label lblHUDItems 
      Caption         =   "Unlockables:"
      Height          =   255
      Left            =   120
      TabIndex        =   1
      Top             =   240
      Width           =   975
   End
End
Attribute VB_Name = "frmUnlockablesEdit"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Option Explicit

Private Sub cmdDelete_Click()
    Call WriteUnlockableItem(True)
End Sub

Private Sub cmdSave_Click()
    Call WriteUnlockableItem(False)
End Sub

Private Sub Form_Load()
    Call Reload
End Sub

Private Sub Reload()
    txtName.Text = ""
    txtObjective.Text = ""
    txtNeededEmblems.Text = ""
    txtNeededTime.Text = ""
    cmbType.Text = ""
    txtVar.Text = ""
    
    Dim i As Integer
    
    For i = 0 To chkGrade.Count - 1
        chkGrade(i).Value = 0
    Next i
    
    lstUnlockables.ListIndex = 0
End Sub

Private Sub lstUnlockables_Click()
    Call ReadSOC(lstUnlockables.ListIndex + 1)
End Sub

Private Sub ReadSOC(UnlockableNum As Integer)
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
        
        If UCase(word) = "UNLOCKABLE" And Val(word2) = UnlockableNum Then
            Do While Len(line) > 0 And Not ts.AtEndOfStream
                line = ts.ReadLine
                word = UCase(FirstToken(line))
                word2 = UCase(SecondTokenEqual(line))
                    
                If word = "NAME" Then
                    txtName.Text = word2
                ElseIf word = "OBJECTIVE" Then
                    txtObjective.Text = word2
                ElseIf word = "NEEDEDEMBLEMS" Then
                    txtNeededEmblems.Text = Val(word2)
                ElseIf word = "NEEDEDTIME" Then
                    txtNeededTime.Text = Val(word2)
                ElseIf word = "TYPE" Then
                    cmbType.ListIndex = Val(word2)
                ElseIf word = "VAR" Then
                    txtVar.Text = Val(word2)
                ElseIf word = "NEEDEDGRADE" Then
                    Dim i As Integer
                    
                    For i = 0 To chkGrade.Count - 1
                        If Val(word2) And chkGrade(i).Tag Then
                            chkGrade(i).Tag = True
                        End If
                    Next i
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

Private Sub WriteUnlockableItem(Remove As Boolean)
    Dim myFSOSource As New Scripting.FileSystemObject
    Dim tsSource As TextStream
    Dim myFSOTarget As New Scripting.FileSystemObject
    Dim tsTarget As TextStream
    Dim line As String
    Dim word As String
    Dim word2 As String
    Dim unlockableremoved As Boolean
    
    unlockableremoved = False
    
    Set tsSource = myFSOSource.OpenTextFile(SOCFile, ForReading, False)
    Set tsTarget = myFSOTarget.OpenTextFile(SOCTemp, ForWriting, True)
    
    Do While Not tsSource.AtEndOfStream
        line = tsSource.ReadLine
        word = UCase(FirstToken(line))
        word2 = UCase(SecondToken(line))
        
        'If the current item exists in the SOC, delete it.
        If word = "UNLOCKABLE" And Val(word2) = lstUnlockables.ListIndex + 1 Then
            unlockableremoved = True
            Do While Len(TrimComplete(tsSource.ReadLine)) > 0 And Not tsSource.AtEndOfStream
            Loop
        Else
            tsTarget.WriteLine line
        End If
    Loop
    
    tsSource.Close
    Set myFSOSource = Nothing
    
    If Remove = False Then
        If line <> "" Then tsTarget.WriteLine ""
    
        tsTarget.WriteLine "UNLOCKABLE " & lstUnlockables.ListIndex + 1
        txtName.Text = TrimComplete(txtName.Text)
        txtObjective.Text = TrimComplete(txtObjective.Text)
        txtNeededEmblems.Text = TrimComplete(txtNeededEmblems.Text)
        txtNeededTime.Text = TrimComplete(txtNeededTime.Text)
        txtVar.Text = TrimComplete(txtVar.Text)
        
        If txtName.Text <> "" Then tsTarget.WriteLine "NAME = " & txtName.Text
        If txtObjective.Text <> "" Then tsTarget.WriteLine "OBJECTIVE = " & txtObjective.Text
        If txtNeededEmblems.Text <> "" Then tsTarget.WriteLine "NEEDEDEMBLEMS = " & txtNeededEmblems.Text
        If txtNeededTime.Text <> "" Then tsTarget.WriteLine "NEEDEDTIME = " & txtNeededTime.Text
        If cmbType.ListIndex <> -1 Then tsTarget.WriteLine "TYPE = " & cmbType.ListIndex
        If txtVar.Text <> "" Then tsTarget.WriteLine "VAR = " & txtVar.Text
        
        Dim writegrade As Long
        Dim i As Integer
        writegrade = 0
        
        For i = 0 To chkGrade.Count - 1
            If chkGrade(i).Value = 1 Then
                writegrade = writegrade + chkGrade(i).Tag
            End If
        Next i
        
        If writegrade > 0 Then tsTarget.WriteLine "NEEDEDGRADE = " & writegrade
    End If
    
    tsTarget.Close
    Set myFSOTarget = Nothing
    
    FileCopy SOCTemp, SOCFile
    
    Kill SOCTemp
    
    If Remove = True Then
        If unlockableremoved = True Then
            MsgBox "Unlockable deleted from SOC."
        Else
            MsgBox "Couldn't find Unlockable in SOC."
        End If
    Else
        MsgBox "Unlockable Saved."
    End If
End Sub
