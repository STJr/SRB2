VERSION 5.00
Begin VB.Form frmHUDEdit 
   Caption         =   "HUD Edit"
   ClientHeight    =   2505
   ClientLeft      =   60
   ClientTop       =   345
   ClientWidth     =   5160
   Icon            =   "frmHUDEdit.frx":0000
   LinkTopic       =   "Form1"
   MaxButton       =   0   'False
   ScaleHeight     =   2505
   ScaleWidth      =   5160
   StartUpPosition =   3  'Windows Default
   Begin VB.CommandButton cmdCodeDefault 
      Caption         =   "&Load Code Default"
      Height          =   375
      Left            =   3480
      TabIndex        =   9
      Top             =   1080
      Width           =   1575
   End
   Begin VB.CommandButton cmdDelete 
      Caption         =   "&Delete from SOC"
      Height          =   375
      Left            =   3480
      TabIndex        =   7
      Top             =   2040
      Width           =   1575
   End
   Begin VB.CommandButton cmdSave 
      Caption         =   "&Save Changes"
      Height          =   375
      Left            =   3480
      TabIndex        =   6
      Top             =   1560
      Width           =   1575
   End
   Begin VB.TextBox txtY 
      Height          =   285
      Left            =   4080
      MaxLength       =   3
      TabIndex        =   3
      Top             =   720
      Width           =   615
   End
   Begin VB.TextBox txtX 
      Height          =   285
      Left            =   4080
      MaxLength       =   3
      TabIndex        =   2
      Top             =   360
      Width           =   615
   End
   Begin VB.ListBox lstHUD 
      Height          =   2010
      Left            =   120
      TabIndex        =   0
      Top             =   360
      Width           =   3255
   End
   Begin VB.Label lblNote 
      Caption         =   "HUD items are placed on a 320x200 grid."
      Height          =   255
      Left            =   1680
      TabIndex        =   8
      Top             =   120
      Width           =   3015
   End
   Begin VB.Label lblY 
      Alignment       =   1  'Right Justify
      Caption         =   "Y:"
      Height          =   255
      Left            =   3600
      TabIndex        =   5
      Top             =   720
      Width           =   375
   End
   Begin VB.Label lblX 
      Alignment       =   1  'Right Justify
      Caption         =   "X:"
      Height          =   255
      Left            =   3720
      TabIndex        =   4
      Top             =   360
      Width           =   255
   End
   Begin VB.Label lblHUDItems 
      Caption         =   "HUD Items:"
      Height          =   255
      Left            =   120
      TabIndex        =   1
      Top             =   120
      Width           =   975
   End
End
Attribute VB_Name = "frmHUDEdit"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Option Explicit

Private Sub cmdCodeDefault_Click()
    LoadHUDInfo (lstHUD.ListIndex)
End Sub

Private Sub cmdDelete_Click()
    Call WriteHUDItem(True)
End Sub

Private Sub cmdSave_Click()
    Call WriteHUDItem(False)
End Sub

Private Sub Form_Load()
    Call Reload
End Sub

Private Sub Reload()
    txtX.Text = ""
    txtY.Text = ""
    Call LoadCode
    lstHUD.ListIndex = 0
End Sub

Private Sub LoadCode()
    Dim myFSO As New Scripting.FileSystemObject
    Dim ts As TextStream
    Dim line As String
    Dim number As Integer
    Dim startclip As Integer, endclip As Integer
    Dim addstring As String
    
    ChDir SourcePath
    Set ts = myFSO.OpenTextFile("st_stuff.h", ForReading, False)
    
    Do While ts.ReadLine <> "/** HUD location information (don't move this comment)"
    Loop
    
    ts.ReadLine ' */
    ts.ReadLine ' typedef struct
    ts.ReadLine ' {
    ts.ReadLine ' int x, y;
    ts.ReadLine ' } hudinfo_t;
    ts.ReadLine '
    ts.ReadLine ' typedef enum
    ts.ReadLine ' {
    
    line = ts.ReadLine
    number = 0
    
    lstHUD.Clear
    
    Do While InStr(line, "NUMHUDITEMS") = 0
        startclip = InStr(line, "HUD_")
        If InStr(line, "HUD_") <> 0 Then
            endclip = InStr(line, ",")
            line = Mid(line, startclip, endclip - startclip)
            addstring = number & " - " & line
            lstHUD.AddItem addstring
            number = number + 1
        End If
        line = ts.ReadLine
    Loop
    
    ts.Close
    Set myFSO = Nothing
End Sub

Private Sub lstHUD_Click()
    LoadHUDInfo (lstHUD.ListIndex)
    Call ReadSOC(lstHUD.ListIndex)
End Sub

Private Sub LoadHUDInfo(HUDNum As Integer)
    Dim myFSO As New Scripting.FileSystemObject
    Dim ts As TextStream
    Dim line As String
    Dim number As Integer
    Dim startclip As Integer, endclip As Integer
    
    ChDir SourcePath
    Set ts = myFSO.OpenTextFile("st_stuff.c", ForReading, False)
    
    Do While InStr(ts.ReadLine, "hudinfo[NUMHUDITEMS] =") = 0
    Loop
    
    ts.SkipLine ' {
    line = ts.ReadLine ' First HUD item
    
    number = 0
    
    Do While number <> HUDNum
        line = ts.ReadLine
        number = number + 1
    Loop
    
    startclip = InStr(line, "{") + 1
    endclip = InStr(line, ",")
    
    txtX.Text = TrimComplete(Mid(line, startclip, endclip - startclip))
    
    startclip = endclip + 2
    endclip = InStr(startclip, line, "}") - 1
    
    txtY.Text = TrimComplete(Mid(line, startclip, endclip - startclip))
        
    ts.Close
    Set myFSO = Nothing
End Sub

Private Sub ReadSOC(HUDNum As Integer)
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
        
        If UCase(word) = "HUDITEM" And Val(word2) = HUDNum Then
            Do While Len(line) > 0 And Not ts.AtEndOfStream
                line = ts.ReadLine
                word = UCase(FirstToken(line))
                word2 = UCase(SecondTokenEqual(line))
                    
                If word = "X" Then
                    txtX.Text = Val(word2)
                ElseIf word = "Y" Then
                    txtY.Text = Val(word2)
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

Private Sub WriteHUDItem(Remove As Boolean)
    Dim myFSOSource As New Scripting.FileSystemObject
    Dim tsSource As TextStream
    Dim myFSOTarget As New Scripting.FileSystemObject
    Dim tsTarget As TextStream
    Dim line As String
    Dim word As String
    Dim word2 As String
    Dim hudremoved As Boolean
    
    hudremoved = False
    
    Set tsSource = myFSOSource.OpenTextFile(SOCFile, ForReading, False)
    Set tsTarget = myFSOTarget.OpenTextFile(SOCTemp, ForWriting, True)
    
    Do While Not tsSource.AtEndOfStream
        line = tsSource.ReadLine
        word = UCase(FirstToken(line))
        word2 = UCase(SecondToken(line))
        
        'If the current item exists in the SOC, delete it.
        If word = "HUDITEM" And Val(word2) = lstHUD.ListIndex Then
            hudremoved = True
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
    
        tsTarget.WriteLine "HUDITEM " & lstHUD.ListIndex
        txtX.Text = TrimComplete(txtX.Text)
        txtY.Text = TrimComplete(txtY.Text)
        If txtX.Text <> "" Then tsTarget.WriteLine "X = " & Val(txtX.Text)
        If txtY.Text <> "" Then tsTarget.WriteLine "Y = " & Val(txtY.Text)
    End If
    
    tsTarget.Close
    Set myFSOTarget = Nothing
    
    FileCopy SOCTemp, SOCFile
    
    Kill SOCTemp
    
    If Remove = True Then
        If hudremoved = True Then
            MsgBox "HUD Item deleted from SOC."
        Else
            MsgBox "Couldn't find HUD Item in SOC."
        End If
    Else
        MsgBox "HUD Item Saved."
    End If
End Sub
