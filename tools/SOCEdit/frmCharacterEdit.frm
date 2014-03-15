VERSION 5.00
Begin VB.Form frmCharacterEdit 
   Caption         =   "Character Edit"
   ClientHeight    =   3345
   ClientLeft      =   60
   ClientTop       =   345
   ClientWidth     =   4680
   Icon            =   "frmCharacterEdit.frx":0000
   LinkTopic       =   "Form1"
   MaxButton       =   0   'False
   ScaleHeight     =   3345
   ScaleWidth      =   4680
   Begin VB.CommandButton cmdExample 
      Caption         =   "Show Me An &Example"
      Height          =   495
      Left            =   1320
      Style           =   1  'Graphical
      TabIndex        =   14
      Top             =   2400
      Width           =   975
   End
   Begin VB.CheckBox chkEnabled 
      Caption         =   "Enable this player selection."
      Height          =   495
      Left            =   1080
      TabIndex        =   13
      Top             =   1560
      Width           =   1455
   End
   Begin VB.CommandButton cmdDelete 
      Caption         =   "&Delete from SOC"
      Height          =   495
      Left            =   120
      Style           =   1  'Graphical
      TabIndex        =   12
      Top             =   2760
      Width           =   855
   End
   Begin VB.CommandButton cmdSave 
      Caption         =   "&Save"
      Height          =   495
      Left            =   120
      TabIndex        =   11
      Top             =   2160
      Width           =   855
   End
   Begin VB.TextBox txtSkinname 
      Height          =   285
      Left            =   3240
      TabIndex        =   9
      Top             =   1200
      Width           =   1335
   End
   Begin VB.TextBox txtPicname 
      Height          =   285
      Left            =   3240
      MaxLength       =   8
      TabIndex        =   7
      Top             =   840
      Width           =   1095
   End
   Begin VB.TextBox txtMenuposition 
      Height          =   285
      Left            =   3240
      MaxLength       =   3
      TabIndex        =   5
      Top             =   480
      Width           =   495
   End
   Begin VB.TextBox txtPlayername 
      Height          =   285
      Left            =   3240
      MaxLength       =   64
      TabIndex        =   3
      Top             =   120
      Width           =   1335
   End
   Begin VB.TextBox txtPlayertext 
      Height          =   1455
      Left            =   2640
      MultiLine       =   -1  'True
      TabIndex        =   1
      Top             =   1800
      Width           =   1935
   End
   Begin VB.ListBox lstPlayers 
      Height          =   1815
      ItemData        =   "frmCharacterEdit.frx":0442
      Left            =   120
      List            =   "frmCharacterEdit.frx":0461
      TabIndex        =   0
      Top             =   240
      Width           =   855
   End
   Begin VB.Label lblSkinname 
      Caption         =   "Name of player (skin) to use:"
      Height          =   255
      Left            =   1080
      TabIndex        =   10
      Top             =   1200
      Width           =   2055
   End
   Begin VB.Label lblPicname 
      Alignment       =   1  'Right Justify
      Caption         =   "Picture to display:"
      Height          =   255
      Left            =   1560
      TabIndex        =   8
      Top             =   840
      Width           =   1575
   End
   Begin VB.Label lblMenuposition 
      Alignment       =   1  'Right Justify
      Caption         =   "Vertical menu position:"
      Height          =   255
      Left            =   1320
      TabIndex        =   6
      Top             =   480
      Width           =   1815
   End
   Begin VB.Label lblPlayername 
      Alignment       =   1  'Right Justify
      Caption         =   "Displayed name of player:"
      Height          =   255
      Left            =   1320
      TabIndex        =   4
      Top             =   120
      Width           =   1815
   End
   Begin VB.Label lblPlayertext 
      Caption         =   "Short Description:"
      Height          =   255
      Left            =   2640
      TabIndex        =   2
      Top             =   1560
      Width           =   1455
   End
End
Attribute VB_Name = "frmCharacterEdit"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Option Explicit

Private Sub cmdDelete_Click()
    Call WriteCharacter(True)
End Sub

Private Sub cmdExample_Click()
    txtPlayername.Text = "SONIC"
    txtMenuposition.Text = "20"
    txtPicname.Text = "SONCCHAR"
    txtSkinname.Text = "SONIC"
    chkEnabled.Value = 1
    txtPlayertext.Text = "             Fastest" & vbCrLf & "                 Speed Thok" & vbCrLf & "             Not a good pick" & vbCrLf & "for starters, but when" & vbCrLf & "controlled properly," & vbCrLf & "Sonic is the most" & vbCrLf & "powerful of the three."
End Sub

Private Sub cmdSave_Click()
    Call WriteCharacter(False)
End Sub

Private Sub ClearForm()
    txtPlayername.Text = ""
    txtMenuposition.Text = ""
    txtPicname.Text = ""
    txtSkinname.Text = ""
    chkEnabled.Value = 0
    txtPlayertext.Text = ""
End Sub

Private Sub ReadSOCPlayer(num As Integer)
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
        
        If UCase(word) = "CHARACTER" And Val(word2) = num Then
            Do While Len(line) > 0 And Not ts.AtEndOfStream
                line = ts.ReadLine
                word = UCase(FirstToken(line))
                word2 = UCase(SecondTokenEqual(line))
                    
                If word = "PLAYERTEXT" Then
                    Dim startclip As Integer, endclip As Integer
                    startclip = InStr(line, "=")
    
                    startclip = startclip + 2
    
                    line = Mid(line, startclip, Len(line))
                    
                    txtPlayertext.Text = line & vbCrLf
                    
                    Do While InStr(line, "#") = 0 And Not ts.AtEndOfStream
                        line = ts.ReadLine & vbCrLf
                        txtPlayertext.Text = txtPlayertext.Text & line
                    Loop
                    
                    txtPlayertext.Text = RTrimComplete(txtPlayertext.Text)
                    If Right(txtPlayertext.Text, 1) = "#" Then
                        txtPlayertext.Text = Left(txtPlayertext.Text, Len(txtPlayertext.Text) - 1)
                    End If
                ElseIf word = "PLAYERNAME" Then
                    txtPlayername.Text = word2
                ElseIf word = "MENUPOSITION" Then
                    txtMenuposition.Text = Val(word2)
                ElseIf word = "PICNAME" Then
                    txtPicname.Text = word2
                ElseIf word = "STATUS" Then
                    If Val(word2) = 32 Then
                        chkEnabled.Value = 1
                    Else
                        chkEnabled.Value = 0
                    End If
                ElseIf word = "SKINNAME" Then
                    txtSkinname.Text = word2
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

Private Sub lstPlayers_Click()
    Call ClearForm
    Call ReadSOCPlayer(lstPlayers.ListIndex)
End Sub

Private Sub WriteCharacter(Remove As Boolean)
    Dim myFSOSource As New Scripting.FileSystemObject
    Dim tsSource As TextStream
    Dim myFSOTarget As New Scripting.FileSystemObject
    Dim tsTarget As TextStream
    Dim line As String
    Dim word As String
    Dim word2 As String
    Dim charfound As Boolean
    
    charfound = False
    
    Set tsSource = myFSOSource.OpenTextFile(SOCFile, ForReading, False)
    Set tsTarget = myFSOTarget.OpenTextFile(SOCTemp, ForWriting, True)
    
    Do While Not tsSource.AtEndOfStream
        line = tsSource.ReadLine
        word = UCase(FirstToken(line))
        word2 = UCase(SecondToken(line))
        
        'If the current character exists in the SOC, delete it.
        If word = "CHARACTER" And Val(word2) = lstPlayers.ListIndex Then
            charfound = True
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
    
        tsTarget.WriteLine "CHARACTER " & lstPlayers.ListIndex
        txtPlayername.Text = TrimComplete(txtPlayername.Text)
        txtMenuposition.Text = TrimComplete(txtMenuposition.Text)
        txtPicname.Text = TrimComplete(txtPicname.Text)
        txtSkinname.Text = TrimComplete(txtSkinname.Text)
        If txtPlayername.Text <> "" Then tsTarget.WriteLine "PLAYERNAME = " & txtPlayername.Text
        If txtMenuposition.Text <> "" Then tsTarget.WriteLine "MENUPOSITION = " & Val(txtMenuposition.Text)
        If txtPicname.Text <> "" Then tsTarget.WriteLine "PICNAME = " & txtPicname.Text
        If txtSkinname.Text <> "" Then tsTarget.WriteLine "SKINNAME = " & txtSkinname.Text
        If chkEnabled.Value = 1 Then
            tsTarget.WriteLine "STATUS = 32"
        Else
            tsTarget.WriteLine "STATUS = 0"
        End If
        If txtPlayertext.Text <> "" Then tsTarget.WriteLine "PLAYERTEXT = " & txtPlayertext.Text & "#"
    End If
    
    tsTarget.Close
    Set myFSOTarget = Nothing
    
    FileCopy SOCTemp, SOCFile
    
    Kill SOCTemp
    
    If Remove = True Then
        If charfound = True Then
            MsgBox "Player choice removed from SOC."
        Else
            MsgBox "Player choice not found in SOC."
        End If
    Else
        MsgBox "Character Saved."
    End If
End Sub

