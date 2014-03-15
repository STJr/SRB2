VERSION 5.00
Begin VB.Form frmSoundEdit 
   Caption         =   "Sound Edit"
   ClientHeight    =   4995
   ClientLeft      =   60
   ClientTop       =   345
   ClientWidth     =   6180
   Icon            =   "frmSoundEdit.frx":0000
   LinkTopic       =   "Form1"
   MaxButton       =   0   'False
   ScaleHeight     =   4995
   ScaleWidth      =   6180
   StartUpPosition =   3  'Windows Default
   Begin VB.CommandButton cmdDelete 
      Caption         =   "&Delete sound from SOC"
      Height          =   495
      Left            =   5040
      Style           =   1  'Graphical
      TabIndex        =   13
      Top             =   4320
      Width           =   1095
   End
   Begin VB.CommandButton cmdSave 
      Caption         =   "&Save"
      Height          =   495
      Left            =   3840
      TabIndex        =   6
      Top             =   4320
      Width           =   1095
   End
   Begin VB.CommandButton cmdReload 
      Caption         =   "&Load Code Default"
      Height          =   495
      Left            =   2640
      Style           =   1  'Graphical
      TabIndex        =   5
      Top             =   4320
      Width           =   1095
   End
   Begin VB.Frame frmSpecial 
      Caption         =   "Special Properties"
      Height          =   3375
      Left            =   2640
      TabIndex        =   4
      Top             =   840
      Width           =   3495
      Begin VB.CheckBox chkTotallySingle 
         Caption         =   "Make sure only one sound of this is playing at a time on any sound channel."
         Height          =   615
         Left            =   120
         TabIndex        =   12
         Tag             =   "1"
         Top             =   2640
         Width           =   3255
      End
      Begin VB.CheckBox chkEightEx 
         Caption         =   "Sound can be heard across 8x the distance"
         Height          =   375
         Left            =   120
         TabIndex        =   10
         Tag             =   "16"
         Top             =   2160
         Width           =   2295
      End
      Begin VB.CheckBox chkOutside 
         Caption         =   "Volume dependent on how close you are to outside"
         Height          =   375
         Left            =   120
         TabIndex        =   9
         Tag             =   "4"
         Top             =   360
         Width           =   2295
      End
      Begin VB.CheckBox chkFourEx 
         Caption         =   "Sound can be heard across 4x the distance"
         Height          =   375
         Left            =   120
         TabIndex        =   8
         Tag             =   "8"
         Top             =   1560
         Width           =   2055
      End
      Begin VB.CheckBox chkMultiple 
         Caption         =   "More than one of this sound can be played per object at a time (i.e., thunder)"
         Height          =   615
         Left            =   120
         TabIndex        =   7
         Tag             =   "2"
         Top             =   840
         Width           =   2535
      End
      Begin VB.Label Label1 
         Caption         =   "Combine for 32x"
         Height          =   495
         Left            =   2760
         TabIndex        =   11
         Top             =   1800
         Width           =   615
      End
      Begin VB.Line Line4 
         X1              =   2400
         X2              =   2640
         Y1              =   2400
         Y2              =   2400
      End
      Begin VB.Line Line2 
         X1              =   2400
         X2              =   2640
         Y1              =   1800
         Y2              =   1800
      End
      Begin VB.Line Line1 
         X1              =   2640
         X2              =   2640
         Y1              =   2400
         Y2              =   1800
      End
   End
   Begin VB.ComboBox cmbPriority 
      Height          =   315
      ItemData        =   "frmSoundEdit.frx":0442
      Left            =   3360
      List            =   "frmSoundEdit.frx":0444
      TabIndex        =   2
      Top             =   120
      Width           =   855
   End
   Begin VB.CheckBox chkSingularity 
      Caption         =   "Only one can be played at a time per object."
      Height          =   255
      Left            =   2640
      TabIndex        =   1
      Top             =   480
      Width           =   3495
   End
   Begin VB.ListBox lstSounds 
      Height          =   4740
      Left            =   120
      TabIndex        =   0
      Top             =   120
      Width           =   2415
   End
   Begin VB.Line Line3 
      X1              =   0
      X2              =   720
      Y1              =   0
      Y2              =   0
   End
   Begin VB.Label lblPriority 
      Alignment       =   1  'Right Justify
      Caption         =   "Priority:"
      Height          =   255
      Left            =   2640
      TabIndex        =   3
      Top             =   120
      Width           =   615
   End
End
Attribute VB_Name = "frmSoundEdit"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Option Explicit

Private Sub cmdDelete_Click()
    Call WriteSound(True)
End Sub

Private Sub cmdReload_Click()
    Call ClearForm
    If InStr(lstSounds.List(lstSounds.ListIndex), "(free slot)") = 0 Then
        Call LoadSoundInfo(lstSounds.ListIndex)
    Else
        MsgBox "Free slots do not have a code default."
    End If
End Sub

Private Sub cmdSave_Click()
    Call WriteSound(False)
End Sub

Private Sub Form_Load()
    Call Reload
End Sub

Private Sub ClearForm()
    cmbPriority.Text = ""
    chkSingularity.Value = 0
    chkOutside.Value = 0
    chkMultiple.Value = 0
    chkFourEx.Value = 0
    chkEightEx.Value = 0
    chkTotallySingle.Value = 0
End Sub

Private Sub Reload()
    Call ClearForm
    Call LoadCode
    lstSounds.ListIndex = 0
End Sub

Private Sub LoadCode()
    Dim myFSO As New Scripting.FileSystemObject
    Dim ts As TextStream
    Dim line As String
    Dim number As Integer
    Dim startclip As Integer, endclip As Integer
    Dim addstring As String
    Dim i As Integer, numfreeslots As Integer
    
    ChDir SourcePath
    Set ts = myFSO.OpenTextFile("sounds.h", ForReading, False)
    
    Do While InStr(ts.ReadLine, "List of sounds (don't modify this comment!)") = 0
    Loop
    
    ts.SkipLine ' typedef enum
    ts.SkipLine ' {
    
    line = ts.ReadLine
    number = 0
    
    lstSounds.Clear
    
    Do While InStr(line, "sfx_freeslot0") = 0
        startclip = InStr(line, "sfx_")
        If InStr(line, "sfx_") <> 0 Then
            endclip = InStr(line, ",")
            line = Mid(line, startclip, endclip - startclip)
            addstring = number & " - " & line
            lstSounds.AddItem addstring
            number = number + 1
        End If
        line = ts.ReadLine
    Loop
    
    ts.Close
    Set myFSO = Nothing
    
'Populate the free slots!
    numfreeslots = 800

    For i = 1 To numfreeslots
        If i < 10 Then
            addstring = number & " - " & "sfx_fre00" & i & " (free slot)"
        ElseIf i < 100 Then
            addstring = number & " - " & "sfx_fre0" & i & " (free slot)"
        Else
            addstring = number & " - " & "sfx_fre" & i & " (free slot)"
        End If
        lstSounds.AddItem addstring
        number = number + 1
    Next
    
    For i = 0 To 127
        cmbPriority.AddItem i
    Next
End Sub

Private Sub lstSounds_Click()
    Call ClearForm
    If InStr(lstSounds.List(lstSounds.ListIndex), "(free slot)") = 0 Then
        Call LoadSoundInfo(lstSounds.ListIndex)
    End If
    Call LoadSOCSoundInfo(lstSounds.ListIndex)
End Sub

Private Sub LoadSOCSoundInfo(SoundNum As Integer)
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
        
        If UCase(word) = "SOUND" And Val(word2) = SoundNum Then
            Do While Len(line) > 0 And Not ts.AtEndOfStream
                line = ts.ReadLine
                word = UCase(FirstToken(line))
                word2 = UCase(SecondTokenEqual(line))
                    
                If word = "SINGULAR" Then
                    If Val(word2) = 1 Then
                        chkSingularity.Value = 1
                    Else
                        chkSingularity.Value = 0
                    End If
                ElseIf word = "PRIORITY" Then
                    cmbPriority.Text = Val(word2)
                ElseIf word = "FLAGS" Then
                    ProcessSoundFlags (Val(word2))
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

Private Sub LoadSoundInfo(StateNum As Integer)
    Dim myFSO As New Scripting.FileSystemObject
    Dim ts As TextStream
    Dim line As String
    Dim number As Integer
    Dim startclip As Integer, endclip As Integer
    Dim token As String
    Dim frame As Long
    
    ChDir SourcePath
    Set ts = myFSO.OpenTextFile("sounds.c", ForReading, False)
    
    Do While InStr(ts.ReadLine, "S_sfx[0] needs to be a dummy for odd reasons.") = 0
    Loop
    
    number = 0
    
    Do While number <> StateNum
        Do While InStr(ts.ReadLine, """") = 0
        Loop
        number = number + 1
    Loop
    
    Do While InStr(line, """") = 0
        line = ts.ReadLine
    Loop
    
    startclip = InStr(line, """") + 1
    line = Mid(line, startclip, Len(line) - startclip)
    endclip = InStr(line, """") - 1
    token = TrimComplete(Left(line, endclip))
    
    'txtName.Text = line
    
    startclip = InStr(line, ",") + 1
    line = Mid(line, startclip, Len(line) - startclip)
    endclip = InStr(line, ",") - 1
    token = TrimComplete(Left(line, endclip))
    
    If token = "true" Then
        chkSingularity.Value = 1
    Else
        chkSingularity.Value = 0
    End If
    
    startclip = InStr(line, ",") + 1
    line = Mid(line, startclip, Len(line) - startclip)
    endclip = InStr(line, ",") - 1
    token = TrimComplete(Left(line, endclip))
    cmbPriority.Text = token
    
    startclip = InStr(line, ",") + 1
    line = Mid(line, startclip, Len(line) - startclip)
    endclip = InStr(line, ",") - 1
    token = TrimComplete(Left(line, endclip))
    
    ProcessSoundFlags (Val(token))
    
    ts.Close
    Set myFSO = Nothing
End Sub

Private Sub ProcessSoundFlags(flags As Long)

    chkTotallySingle.Value = 0
    chkMultiple.Value = 0
    chkOutside.Value = 0
    chkFourEx.Value = 0
    chkEightEx.Value = 0
    
    If flags = -1 Then
        Exit Sub
    End If
    
    If flags And 1 Then
        chkTotallySingle.Value = 1
    End If
    
    If flags And 2 Then
        chkMultiple.Value = 1
    End If
    
    If flags And 4 Then
        chkOutside.Value = 1
    End If
    
    If flags And 8 Then
        chkFourEx.Value = 1
    End If
    
    If flags And 16 Then
        chkEightEx.Value = 1
    End If
    
End Sub

Private Sub WriteSound(Remove As Boolean)
    Dim myFSOSource As New Scripting.FileSystemObject
    Dim tsSource As TextStream
    Dim myFSOTarget As New Scripting.FileSystemObject
    Dim tsTarget As TextStream
    Dim line As String
    Dim word As String
    Dim word2 As String
    Dim flags As Long
    Dim soundfound As Boolean
    
    soundfound = False
    
    Set tsSource = myFSOSource.OpenTextFile(SOCFile, ForReading, False)
    Set tsTarget = myFSOTarget.OpenTextFile(SOCTemp, ForWriting, True)
    
    Do While Not tsSource.AtEndOfStream
        line = tsSource.ReadLine
        word = UCase(FirstToken(line))
        word2 = UCase(SecondToken(line))

        'If the current sound exists in the SOC, delete it.
        If word = "SOUND" And Val(word2) = lstSounds.ListIndex Then
            soundfound = True
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
    
        tsTarget.WriteLine "SOUND " & lstSounds.ListIndex
        cmbPriority.Text = TrimComplete(cmbPriority.Text)
        If cmbPriority.Text <> "" Then tsTarget.WriteLine "PRIORITY = " & Val(cmbPriority.Text)
        If chkSingularity.Value = 1 Then tsTarget.WriteLine "SINGULAR = 1"
        
        flags = 0
        
        If chkOutside.Value = 1 Then flags = flags + Val(chkOutside.Tag)
        If chkMultiple.Value = 1 Then flags = flags + Val(chkMultiple.Tag)
        If chkFourEx.Value = 1 Then flags = flags + Val(chkFourEx.Tag)
        If chkEightEx.Value = 1 Then flags = flags + Val(chkEightEx.Tag)
        If chkTotallySingle.Value = 1 Then flags = flags + Val(chkTotallySingle.Tag)
        
        If flags > 0 Then tsTarget.WriteLine "FLAGS = " & flags
    End If
    
    tsTarget.Close
    Set myFSOTarget = Nothing
    
    FileCopy SOCTemp, SOCFile
    
    Kill SOCTemp
    
    If Remove = True Then
        If soundfound = True Then
            MsgBox "Sound removed from SOC."
        Else
            MsgBox "Sound not found in SOC."
        End If
    Else
        MsgBox "Sound Saved."
    End If
End Sub

