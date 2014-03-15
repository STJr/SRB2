VERSION 5.00
Begin VB.Form frmEmblemEdit 
   Caption         =   "Emblem Edit"
   ClientHeight    =   2865
   ClientLeft      =   60
   ClientTop       =   345
   ClientWidth     =   5160
   Icon            =   "frmEmblemEdit.frx":0000
   LinkTopic       =   "Form1"
   MaxButton       =   0   'False
   ScaleHeight     =   2865
   ScaleWidth      =   5160
   StartUpPosition =   3  'Windows Default
   Begin VB.CommandButton cmdDelete 
      Caption         =   "&Delete Last Emblem"
      Height          =   735
      Left            =   1560
      Style           =   1  'Graphical
      TabIndex        =   17
      Top             =   600
      Width           =   855
   End
   Begin VB.CommandButton cmdAdd 
      Caption         =   "&Add"
      Height          =   375
      Left            =   1560
      TabIndex        =   16
      Top             =   120
      Width           =   855
   End
   Begin VB.CommandButton cmdSave 
      Caption         =   "&Save Emblem"
      Height          =   495
      Left            =   4200
      Style           =   1  'Graphical
      TabIndex        =   13
      Top             =   2280
      Width           =   855
   End
   Begin VB.CommandButton cmdReload 
      Caption         =   "&Reload"
      Height          =   495
      Left            =   3120
      TabIndex        =   12
      Top             =   2280
      Width           =   975
   End
   Begin VB.TextBox txtPlayernum 
      Height          =   285
      Left            =   4320
      MaxLength       =   3
      TabIndex        =   9
      Top             =   1800
      Width           =   735
   End
   Begin VB.TextBox txtMapnum 
      Height          =   285
      Left            =   4320
      MaxLength       =   4
      TabIndex        =   7
      Top             =   1320
      Width           =   735
   End
   Begin VB.TextBox txtZ 
      Height          =   285
      Left            =   4320
      MaxLength       =   5
      TabIndex        =   3
      Top             =   960
      Width           =   735
   End
   Begin VB.TextBox txtY 
      Height          =   285
      Left            =   4320
      MaxLength       =   5
      TabIndex        =   2
      Top             =   600
      Width           =   735
   End
   Begin VB.TextBox txtX 
      Height          =   285
      Left            =   4320
      MaxLength       =   5
      TabIndex        =   1
      Top             =   240
      Width           =   735
   End
   Begin VB.ListBox lstEmblems 
      Height          =   2400
      Left            =   120
      TabIndex        =   0
      Top             =   120
      Width           =   1335
   End
   Begin VB.Label Label1 
      Caption         =   "Emblem #s must be linear, sorry!"
      Height          =   495
      Left            =   1560
      TabIndex        =   18
      Top             =   2400
      Width           =   1455
   End
   Begin VB.Label lblNumEmblems 
      Caption         =   "# of Emblems:"
      Height          =   255
      Left            =   120
      TabIndex        =   15
      Top             =   2520
      Width           =   1335
   End
   Begin VB.Label lblNote2 
      Caption         =   "Don't forget to set Game Data file and # of Emblems in Global Game Settings!"
      Height          =   855
      Left            =   1560
      TabIndex        =   14
      Top             =   1440
      Width           =   1575
   End
   Begin VB.Label lblNote 
      Appearance      =   0  'Flat
      BorderStyle     =   1  'Fixed Single
      Caption         =   "Note: Enter map coordinates, not game coordinates. (I.e., 128, not 8388608)"
      ForeColor       =   &H80000008&
      Height          =   1095
      Left            =   2640
      TabIndex        =   11
      Top             =   120
      Width           =   1335
   End
   Begin VB.Label lblPlayernum 
      Caption         =   "Player # (255 for all players):"
      Height          =   495
      Left            =   3240
      TabIndex        =   10
      Top             =   1680
      Width           =   1095
   End
   Begin VB.Label lblMapnum 
      Alignment       =   1  'Right Justify
      Caption         =   "Map #:"
      Height          =   255
      Left            =   3600
      TabIndex        =   8
      Top             =   1320
      Width           =   615
   End
   Begin VB.Label lblZ 
      Alignment       =   1  'Right Justify
      Caption         =   "Z:"
      Height          =   255
      Left            =   3960
      TabIndex        =   6
      Top             =   960
      Width           =   255
   End
   Begin VB.Label lblY 
      Alignment       =   1  'Right Justify
      Caption         =   "Y:"
      Height          =   255
      Left            =   3960
      TabIndex        =   5
      Top             =   600
      Width           =   255
   End
   Begin VB.Label lblX 
      Alignment       =   1  'Right Justify
      Caption         =   "X:"
      Height          =   255
      Left            =   3960
      TabIndex        =   4
      Top             =   240
      Width           =   255
   End
End
Attribute VB_Name = "frmEmblemEdit"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Option Explicit

Private Sub cmdAdd_Click()
    lstEmblems.AddItem "Emblem " & lstEmblems.ListCount + 1
    lstEmblems.ListIndex = lstEmblems.ListCount - 1
    lblNumEmblems.Caption = "# of Emblems: " & lstEmblems.ListCount
    txtX.Text = 0
    txtY.Text = 0
    txtZ.Text = 0
    txtPlayernum.Text = 255
    txtMapnum.Text = 1
End Sub

Private Sub cmdDelete_Click()
    Call WriteEmblem(True)
    lstEmblems.RemoveItem lstEmblems.ListCount - 1
    lstEmblems.ListIndex = lstEmblems.ListCount - 1
    lblNumEmblems.Caption = "# of Emblems: " & lstEmblems.ListCount
End Sub

Private Sub cmdReload_Click()
    Call Reload
End Sub

Private Sub Reload()
    lstEmblems.Clear
    txtX.Text = ""
    txtY.Text = ""
    txtZ.Text = ""
    txtMapnum.Text = ""
    txtPlayernum.Text = ""
    lblNumEmblems.Caption = "# of Emblems: " & lstEmblems.ListCount
    Call ReadSOCEmblems
End Sub

Private Sub cmdSave_Click()
    If lstEmblems.ListCount <= 0 Then
        MsgBox "You have no emblems to save!"
    Else
        Call WriteEmblem(False)
    End If
End Sub

Private Sub Form_Load()
    Call Reload
End Sub

Private Sub ReadSOCEmblems()
    Dim myFSO As New Scripting.FileSystemObject
    Dim ts As TextStream
    Dim line As String
    Dim word As String
    Dim word2 As String
    
    Set ts = myFSO.OpenTextFile(SOCFile, ForReading, False)
    
    lstEmblems.Clear
    
EmblemLoad:
    Do While Not ts.AtEndOfStream
        line = ts.ReadLine
        
        If Left(line, 1) = "#" Then GoTo EmblemLoad
        
        If Left(line, 1) = vbCrLf Then GoTo EmblemLoad
        
        If Len(line) < 1 Then GoTo EmblemLoad
        
        word = FirstToken(line)
        word2 = SecondToken(line)
        
        If UCase(word) = "EMBLEM" Then
            lstEmblems.AddItem ("Emblem " & Val(word2))
        End If
    Loop
    
    ts.Close
    Set myFSO = Nothing
End Sub

Private Sub ReadSOCEmblemNum(num As Integer)
    Dim myFSO As New Scripting.FileSystemObject
    Dim ts As TextStream
    Dim line As String
    Dim word As String
    Dim word2 As String
    
    Set ts = myFSO.OpenTextFile(SOCFile, ForReading, False)
    
EmblemLoad:
    Do While Not ts.AtEndOfStream
        line = ts.ReadLine
        
        If Left(line, 1) = "#" Then GoTo EmblemLoad
        
        If Left(line, 1) = vbCrLf Then GoTo EmblemLoad
        
        If Len(line) < 1 Then GoTo EmblemLoad
        
        word = UCase(FirstToken(line))
        word2 = UCase(SecondToken(line))
        
        If word = "EMBLEM" Then
            If Val(word2) = num Then
                Do While Len(line) > 0 And Not ts.AtEndOfStream
                    line = ts.ReadLine
                    word = UCase(FirstToken(line))
                    word2 = UCase(SecondTokenEqual(line))
                    
                    If word = "X" Then
                        txtX.Text = Val(word2)
                    ElseIf word = "Y" Then
                        txtY.Text = Val(word2)
                    ElseIf word = "Z" Then
                        txtZ.Text = Val(word2)
                    ElseIf word = "PLAYERNUM" Then
                        txtPlayernum.Text = Val(word2)
                    ElseIf word = "MAPNUM" Then
                        txtMapnum.Text = Val(word2)
                    ElseIf Len(line) > 0 And Left(line, 1) <> "#" Then
                        MsgBox "Error in SOC with Emblem " & num & vbCrLf & "Unknown line: " & line
                    End If
                Loop
                Exit Do
            End If
        End If
    Loop
    
    ts.Close
    Set myFSO = Nothing
End Sub

Private Sub lstEmblems_Click()
    Dim i As Integer
    
    i = InStr(lstEmblems.List(lstEmblems.ListIndex), " ") + 1
    
    i = Mid(lstEmblems.List(lstEmblems.ListIndex), i, Len(lstEmblems.List(lstEmblems.ListIndex)) - i + 1)
    i = Val(i)
    Call ReadSOCEmblemNum(i)
End Sub

Private Sub WriteEmblem(Remove As Boolean)
    Dim myFSOSource As New Scripting.FileSystemObject
    Dim tsSource As TextStream
    Dim myFSOTarget As New Scripting.FileSystemObject
    Dim tsTarget As TextStream
    Dim line As String
    Dim word As String
    Dim word2 As String
    Dim i As Integer
    
    Set tsSource = myFSOSource.OpenTextFile(SOCFile, ForReading, False)
    Set tsTarget = myFSOTarget.OpenTextFile(SOCTemp, ForWriting, True)
    
    Do While Not tsSource.AtEndOfStream
        line = tsSource.ReadLine
        word = UCase(FirstToken(line))
        word2 = UCase(SecondToken(line))
        i = InStr(lstEmblems.List(lstEmblems.ListIndex), " ") + 1
    
        i = Mid(lstEmblems.List(lstEmblems.ListIndex), i, Len(lstEmblems.List(lstEmblems.ListIndex)) - i + 1)
        i = Val(i)

        'If the current emblem exists in the SOC, delete it.
        If word = "EMBLEM" And Val(word2) = i Then
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
    
        tsTarget.WriteLine UCase(lstEmblems.List(lstEmblems.ListIndex))
        txtX.Text = TrimComplete(txtX.Text)
        txtY.Text = TrimComplete(txtY.Text)
        txtZ.Text = TrimComplete(txtZ.Text)
        txtMapnum.Text = TrimComplete(txtMapnum.Text)
        txtPlayernum.Text = TrimComplete(txtPlayernum.Text)
        If txtX.Text <> "" Then tsTarget.WriteLine "X = " & Val(txtX.Text)
        If txtY.Text <> "" Then tsTarget.WriteLine "Y = " & Val(txtY.Text)
        If txtZ.Text <> "" Then tsTarget.WriteLine "Z = " & Val(txtZ.Text)
        If txtMapnum.Text <> "" Then tsTarget.WriteLine "MAPNUM = " & Val(txtMapnum.Text)
        If txtPlayernum.Text <> "" Then tsTarget.WriteLine "PLAYERNUM = " & Val(txtPlayernum.Text)
    End If
    
    tsTarget.Close
    Set myFSOTarget = Nothing
    
    FileCopy SOCTemp, SOCFile
    
    Kill SOCTemp
    
    If Remove = True Then
        MsgBox "Emblem deleted."
    Else
        MsgBox "Emblem Saved."
    End If
End Sub
