VERSION 5.00
Begin VB.Form frmStateEdit 
   Caption         =   "State Edit"
   ClientHeight    =   6750
   ClientLeft      =   60
   ClientTop       =   345
   ClientWidth     =   8970
   Icon            =   "frmStateEdit.frx":0000
   LinkTopic       =   "Form1"
   MaxButton       =   0   'False
   ScaleHeight     =   6750
   ScaleWidth      =   8970
   StartUpPosition =   3  'Windows Default
   Begin VB.TextBox lblVar2Desc 
      Height          =   495
      Left            =   4440
      Locked          =   -1  'True
      MultiLine       =   -1  'True
      ScrollBars      =   2  'Vertical
      TabIndex        =   25
      Top             =   3000
      Width           =   4455
   End
   Begin VB.TextBox lblVar1Desc 
      Height          =   495
      Left            =   4440
      Locked          =   -1  'True
      MultiLine       =   -1  'True
      ScrollBars      =   2  'Vertical
      TabIndex        =   24
      Top             =   2400
      Width           =   4455
   End
   Begin VB.TextBox lblActionDesc 
      Height          =   735
      Left            =   4440
      Locked          =   -1  'True
      MultiLine       =   -1  'True
      ScrollBars      =   2  'Vertical
      TabIndex        =   23
      Top             =   1560
      Width           =   4455
   End
   Begin VB.ListBox lstThings 
      Height          =   450
      ItemData        =   "frmStateEdit.frx":0442
      Left            =   7440
      List            =   "frmStateEdit.frx":0444
      TabIndex        =   22
      Top             =   6120
      Visible         =   0   'False
      Width           =   975
   End
   Begin VB.TextBox txtFuncVar2 
      Height          =   285
      Left            =   7200
      TabIndex        =   19
      Top             =   3600
      Width           =   1215
   End
   Begin VB.TextBox txtFuncVar1 
      Height          =   285
      Left            =   5520
      TabIndex        =   18
      Top             =   3600
      Width           =   1095
   End
   Begin VB.CommandButton cmdCopy 
      Caption         =   "&Copy state to..."
      Height          =   495
      Left            =   6120
      Style           =   1  'Graphical
      TabIndex        =   17
      Top             =   6120
      Width           =   1095
   End
   Begin VB.CommandButton cmdDelete 
      Caption         =   "&Delete State from SOC"
      Height          =   495
      Left            =   6120
      Style           =   1  'Graphical
      TabIndex        =   16
      Top             =   5520
      Width           =   1095
   End
   Begin VB.CommandButton cmdReload 
      Caption         =   "&Load Code Default"
      Height          =   495
      Left            =   4920
      Style           =   1  'Graphical
      TabIndex        =   15
      Top             =   5520
      Width           =   1095
   End
   Begin VB.CommandButton cmdSave 
      Caption         =   "&Save"
      Height          =   495
      Left            =   7320
      TabIndex        =   14
      Top             =   5520
      Width           =   1095
   End
   Begin VB.ComboBox cmbTranslucency 
      Height          =   315
      ItemData        =   "frmStateEdit.frx":0446
      Left            =   6720
      List            =   "frmStateEdit.frx":045C
      TabIndex        =   12
      Top             =   5040
      Width           =   1695
   End
   Begin VB.CheckBox chkFullbright 
      Caption         =   "Make sprite full-brightness (unaffected by lighting)"
      Height          =   495
      Left            =   6240
      TabIndex        =   11
      Top             =   4440
      Width           =   2175
   End
   Begin VB.ComboBox cmbNextstate 
      Height          =   315
      Left            =   6120
      TabIndex        =   9
      Top             =   3960
      Width           =   2295
   End
   Begin VB.ComboBox cmbAction 
      Height          =   315
      Left            =   6000
      TabIndex        =   7
      Top             =   1200
      Width           =   2295
   End
   Begin VB.TextBox txtTics 
      Height          =   285
      Left            =   7800
      TabIndex        =   5
      Top             =   720
      Width           =   495
   End
   Begin VB.TextBox txtFrame 
      Height          =   285
      Left            =   5880
      MaxLength       =   2
      TabIndex        =   3
      Top             =   720
      Width           =   495
   End
   Begin VB.ComboBox cmbSprite 
      Height          =   315
      Left            =   5880
      TabIndex        =   1
      Top             =   120
      Width           =   2415
   End
   Begin VB.ListBox lstStates 
      Height          =   6495
      Left            =   120
      TabIndex        =   0
      Top             =   120
      Width           =   4215
   End
   Begin VB.Label lblFuncVar2 
      Alignment       =   1  'Right Justify
      Caption         =   "Var2:"
      Height          =   255
      Left            =   6600
      TabIndex        =   21
      Top             =   3600
      Width           =   495
   End
   Begin VB.Label lblFuncVar1 
      Alignment       =   1  'Right Justify
      Caption         =   "Var1:"
      Height          =   255
      Left            =   4920
      TabIndex        =   20
      Top             =   3600
      Width           =   495
   End
   Begin VB.Label lblTranslucency 
      Alignment       =   1  'Right Justify
      Caption         =   "Translucency:"
      Height          =   255
      Left            =   5520
      TabIndex        =   13
      Top             =   5040
      Width           =   1095
   End
   Begin VB.Label lblNextstate 
      Alignment       =   1  'Right Justify
      Caption         =   "Next State:"
      Height          =   255
      Left            =   5160
      TabIndex        =   10
      Top             =   3960
      Width           =   855
   End
   Begin VB.Label lblAction 
      Alignment       =   1  'Right Justify
      Caption         =   "Function to Call:"
      Height          =   375
      Left            =   5040
      TabIndex        =   8
      Top             =   1080
      Width           =   855
   End
   Begin VB.Label lblTics 
      Alignment       =   1  'Right Justify
      Caption         =   "Tics (-1 for infinite duration):"
      Height          =   495
      Left            =   6480
      TabIndex        =   6
      Top             =   600
      Width           =   1215
   End
   Begin VB.Label lblFrame 
      Alignment       =   1  'Right Justify
      Caption         =   "Frame:"
      Height          =   255
      Left            =   5160
      TabIndex        =   4
      Top             =   720
      Width           =   615
   End
   Begin VB.Label lblSprite 
      Alignment       =   1  'Right Justify
      Caption         =   "Sprite:"
      Height          =   255
      Left            =   5160
      TabIndex        =   2
      Top             =   120
      Width           =   615
   End
End
Attribute VB_Name = "frmStateEdit"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Option Explicit

Private Sub cmbAction_Click()
    Dim myFSO As New Scripting.FileSystemObject
    Dim ts As TextStream
    Dim line As String
    Dim index As Integer
    Dim ActionName As String
    
    ActionName = cmbAction.List(cmbAction.ListIndex)
    
    If cmbAction.ListIndex = 0 Then
        lblActionDesc.Text = ""
        lblVar1Desc.Text = ""
        lblVar2Desc.Text = ""
        Exit Sub
    End If
    
    ChDir SourcePath
    Set ts = myFSO.OpenTextFile("p_enemy.c", ForReading, False)
    
    Do While Not ts.AtEndOfStream
        line = ts.ReadLine
        
        If Mid(line, 4, 9) = "Function:" And InStr(line, ActionName) > 0 Then
            ts.ReadLine ' //
            line = ts.ReadLine ' // Description:
            index = InStr(line, ":")
            lblActionDesc.Text = Mid(line, index + 2, Len(line) - (index + 1))
            
            ts.ReadLine ' //
            line = ts.ReadLine ' // var1 =
            If InStr(line, "var1:") Then
                lblVar1Desc.Text = Mid(line, 4, Len(line) - 3)
                line = ts.ReadLine
                Do While Left(line, 7) <> "// var2"
                    lblVar1Desc.Text = lblVar1Desc.Text & vbCrLf & TrimComplete(Mid(line, 4, Len(line) - 3))
                    line = ts.ReadLine
                Loop
            Else
                lblVar1Desc.Text = Mid(line, 4, Len(line) - 3)
            End If
            
            If Left(line, 7) <> "// var2" Then
                line = ts.ReadLine ' // var2 =
            End If
            
            If InStr(line, "var2:") Then
                lblVar2Desc.Text = Mid(line, 4, Len(line) - 3)
                line = ts.ReadLine
                Do While Len(line) > 4
                    lblVar2Desc.Text = lblVar2Desc.Text & vbCrLf & TrimComplete(Mid(line, 4, Len(line) - 3))
                    line = ts.ReadLine
                Loop
            Else
                lblVar2Desc.Text = Mid(line, 4, Len(line) - 3)
            End If
        End If
    Loop
    
    ts.Close
    Set myFSO = Nothing
End Sub

Private Sub cmdCopy_Click()
    Dim Response As String
    
    Response$ = InputBox("Copy state to #:", "Copy State")
    
    If Response = "" Then Exit Sub
    
    Response = TrimComplete(Response)
    
    Call WriteState(False, Val(Response))
    
    MsgBox "State copied to #" & Val(Response)
End Sub

Private Sub cmdDelete_Click()
    Call WriteState(True, lstStates.ListIndex)
End Sub

Private Sub cmdReload_Click()
    Call ClearForm
    
    If InStr(lstStates.List(lstStates.ListIndex), "S_FREESLOT") = 0 Then
        LoadStateInfo (lstStates.ListIndex)
    Else
        MsgBox "Free slots do not have a code default."
    End If
End Sub

Private Sub cmdSave_Click()
    If TrimComplete(txtFrame.Text) = "" And (chkFullbright.Value = 1 Or cmbTranslucency.ListIndex > 0) Then
        MsgBox "ERROR: Frame field required for fullbright/translucency."
        Exit Sub
    End If
    Call WriteState(False, lstStates.ListIndex)
End Sub

Private Sub Form_Load()
    Call Reload
    lstStates.ListIndex = 0
End Sub

Private Sub ClearForm()
    cmbNextstate.Text = ""
    cmbSprite.Text = ""
    txtFrame.Text = ""
    cmbAction.Text = ""
    txtFuncVar1.Text = ""
    txtFuncVar2.Text = ""
    lblActionDesc.Text = ""
    lblVar1Desc.Text = ""
    lblVar2Desc.Text = ""
    chkFullbright.Value = False
    cmbTranslucency.ListIndex = 0
End Sub

Private Sub Reload()
    LoadStates
    LoadSprites
    LoadActions
End Sub

Private Sub LoadStates()
    Dim myFSO As New Scripting.FileSystemObject
    Dim ts As TextStream
    Dim line As String
    Dim number As Integer
    Dim startclip As Integer, endclip As Integer
    Dim addstring As String
    Dim numfreeslots As Integer, i As Integer
    
    ChDir SourcePath
    Set ts = myFSO.OpenTextFile("info.h", ForReading, False)
    
    Do While InStr(ts.ReadLine, "Object states (don't modify this comment!)") = 0
    Loop
    
    ts.SkipLine ' typedef enum
    ts.SkipLine ' {
    
    line = ts.ReadLine
    number = 0
    
    lstStates.Clear
    
    Do While InStr(line, "S_FIRSTFREESLOT") = 0
        startclip = InStr(line, "S_")
        If InStr(line, "S_") <> 0 Then
            endclip = InStr(line, ",")
            line = Mid(line, startclip, endclip - startclip)
            addstring = number & " - " & line
            lstStates.AddItem addstring
            cmbNextstate.AddItem addstring
            number = number + 1
        End If
        line = ts.ReadLine
    Loop
    
    ts.Close
    
    'Populate the free slots!
    Set ts = myFSO.OpenTextFile("info.h", ForReading, False)

    line = ts.ReadLine
    Do While InStr(line, "#define NUMMOBJFREESLOTS") = 0
        line = ts.ReadLine
    Loop
    
    startclip = InStr(line, "SLOTS ") + 6
    numfreeslots = Val(Mid(line, startclip, Len(line) - startclip + 1)) * 6

    For i = 1 To numfreeslots
        addstring = number & " - " & "S_FREESLOT" & i
        lstStates.AddItem addstring
        cmbNextstate.AddItem addstring
        number = number + 1
    Next

    ts.Close
    Set myFSO = Nothing
End Sub

Private Sub LoadSprites()
    Dim myFSO As New Scripting.FileSystemObject
    Dim ts As TextStream
    Dim line As String
    Dim number As Integer
    Dim startclip As Integer, endclip As Integer
    Dim addstring As String
    Dim numfreeslots As Integer, i As Integer
    
    ChDir SourcePath
    Set ts = myFSO.OpenTextFile("info.h", ForReading, False)
    
    Do While InStr(ts.ReadLine, "Hey, moron! If you change this table, don't forget about") = 0
    Loop
    
    ts.SkipLine ' typedef enum
    ts.SkipLine ' {
    
    line = ts.ReadLine
    number = 0
    
    cmbSprite.Clear
    
    Do While InStr(line, "SPR_FIRSTFREESLOT") = 0
        startclip = InStr(line, "SPR_")
        If InStr(line, "SPR_") <> 0 Then
            endclip = InStr(line, ",")
            line = Mid(line, startclip, endclip - startclip)
            addstring = number & " - " & line
            cmbSprite.AddItem addstring
            number = number + 1
        End If
        line = ts.ReadLine
    Loop
    
    ts.Close
    
    'Populate the free slots!
    Set ts = myFSO.OpenTextFile("info.h", ForReading, False)

    line = ts.ReadLine
    Do While InStr(line, "#define NUMMOBJFREESLOTS") = 0
        line = ts.ReadLine
    Loop
    
    startclip = InStr(line, "SLOTS ") + 6
    numfreeslots = Val(Mid(line, startclip, Len(line) - startclip + 1))

    For i = 1 To numfreeslots
        If i < 10 Then
            addstring = number & " - " & "SPR_F00" & i & " (Free slot)"
        ElseIf i < 100 Then
            addstring = number & " - " & "SPR_F0" & i & " (Free slot)"
        Else
            addstring = number & " - " & "SPR_F" & i & " (Free slot)"
        End If
        
        cmbSprite.AddItem addstring
        number = number + 1
    Next

    ts.Close

    Set myFSO = Nothing
End Sub

Private Sub LoadActions()
    Dim myFSO As New Scripting.FileSystemObject
    Dim ts As TextStream
    Dim line As String
    Dim number As Integer
    Dim startclip As Integer, endclip As Integer
    Dim addstring As String
    
    ChDir SourcePath
    Set ts = myFSO.OpenTextFile("dehacked.c", ForReading, False)
    
    Do While InStr(ts.ReadLine, "actionpointer_t actionpointers[]") = 0
    Loop
    
    ts.SkipLine ' {
    
    line = ts.ReadLine
    number = 0
    
    cmbAction.Clear
    
    cmbAction.AddItem "None"
    
    Do While InStr(line, "NULL") = 0
        startclip = InStr(line, "A_")
        If InStr(line, "A_") <> 0 Then
            endclip = InStr(line, "}")
            line = Mid(line, startclip, endclip - startclip)
            cmbAction.AddItem line
            number = number + 1
        End If
        line = ts.ReadLine
    Loop
    
    ts.Close
    Set myFSO = Nothing
End Sub

Private Sub lstStates_Click()
    Call ClearForm

    If InStr(lstStates.List(lstStates.ListIndex), "S_FREESLOT") = 0 Then
        LoadStateInfo (lstStates.ListIndex)
    End If
    
    LoadSOCStateInfo (lstStates.ListIndex)
End Sub

Private Sub LoadSOCStateInfo(StateNum As Integer)
    Dim myFSO As New Scripting.FileSystemObject
    Dim ts As TextStream
    Dim line As String
    Dim word As String
    Dim word2 As String
    Dim frameNum As Long
    
    Set ts = myFSO.OpenTextFile(SOCFile, ForReading, False)
    
SOCLoad:
    Do While Not ts.AtEndOfStream
        line = ts.ReadLine
        
        If Left(line, 1) = "#" Then GoTo SOCLoad
        
        If Left(line, 1) = vbCrLf Then GoTo SOCLoad
        
        If Len(line) < 1 Then GoTo SOCLoad
        
        word = FirstToken(line)
        word2 = SecondToken(line)
        
        If UCase(word) = "FRAME" And Val(word2) = StateNum Then
            Do While Len(line) > 0 And Not ts.AtEndOfStream
                line = ts.ReadLine
                word = UCase(FirstToken(line))
                word2 = UCase(SecondTokenEqual(line))
                    
                If word = "SPRITENUMBER" Then
                    cmbSprite.ListIndex = Val(word2)
                ElseIf word = "SPRITESUBNUMBER" Then
                    frameNum = Val(word2)
                    
                    If frameNum >= 327680 Then ' 5 << 16
                        cmbTranslucency.ListIndex = 5
                        frameNum = frameNum And Not 327680
                    ElseIf frameNum >= 262144 Then ' 4 << 16
                        cmbTranslucency.ListIndex = 4
                        frameNum = frameNum And Not 262144
                    ElseIf frameNum >= 196608 Then ' 3 << 16
                        cmbTranslucency.ListIndex = 3
                        frameNum = frameNum And Not 196608
                    ElseIf frameNum >= 131072 Then ' 2 << 16
                        cmbTranslucency.ListIndex = 2
                        frameNum = frameNum And Not 131072
                    ElseIf frameNum >= 65536 Then ' 1 << 16
                        cmbTranslucency.ListIndex = 1
                        frameNum = frameNum And Not 65536
                    End If
                    
                    If frameNum >= 32768 Then
                        chkFullbright.Value = 1
                        frameNum = frameNum And Not 32768
                    Else
                        chkFullbright.Value = 0
                    End If
                    
                    txtFrame.Text = frameNum
                ElseIf word = "DURATION" Then
                    txtTics.Text = Val(word2)
                ElseIf word = "NEXT" Then
                    cmbNextstate.ListIndex = Val(word2)
                ElseIf word = "ACTION" Then
                    Call FindComboIndex(cmbAction, UCase(SecondToken(line)))
                ElseIf word = "VAR1" Then
                    txtFuncVar1.Text = Val(word2)
                ElseIf word = "VAR2" Then
                    txtFuncVar2.Text = Val(word2)
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

Private Sub LoadStateInfo(StateNum As Integer)
    Dim myFSO As New Scripting.FileSystemObject
    Dim ts As TextStream
    Dim line As String
    Dim number As Integer
    Dim startclip As Integer, endclip As Integer
    Dim token As String
    Dim frame As Long
    Dim templine As String
    
    ChDir SourcePath
    Set ts = myFSO.OpenTextFile("info.c", ForReading, False)
    
    Do While InStr(ts.ReadLine, "Keep this comment directly above S_NULL") = 0
    Loop
    
    number = 0
    
    Do While number <> StateNum
        Do While InStr(ts.ReadLine, "SPR_") = 0
        Loop
        number = number + 1
    Loop
    
    Do While InStr(line, "SPR_") = 0
        line = ts.ReadLine
    Loop
    
    startclip = InStr(line, "SPR_")
    line = Mid(line, startclip, Len(line) - startclip)
    endclip = InStr(line, ",") - 1
    token = Left(line, endclip)
    Call FindComboIndex(cmbSprite, token)
    
    startclip = InStr(line, ",") + 1
    line = TrimComplete(Mid(line, startclip, Len(line) - startclip))
    endclip = InStr(line, ",") - 1
    frame = Val(Left(line, endclip))
    
    If frame >= 32768 Then
        chkFullbright.Value = 1
        frame = frame - 32768
    Else
        chkFullbright.Value = 0
    End If
        
    txtFrame.Text = frame
    
    cmbTranslucency.ListIndex = 0
    
    startclip = InStr(line, ",") + 1
    line = TrimComplete(Mid(line, startclip, Len(line) - startclip))
    endclip = InStr(line, ",") - 1
    txtTics.Text = Val(Left(line, endclip))
    
    startclip = InStr(line, "{") + 1
    line = TrimComplete(Mid(line, startclip, Len(line) - startclip))
    endclip = InStr(line, "}") - 1
    cmbAction.Text = TrimComplete(Left(line, endclip))
    If cmbAction.Text = "NULL" Then cmbAction.Text = "None"

    startclip = InStr(line, ",") + 1
    line = TrimComplete(Mid(line, startclip, Len(line) - startclip))
    endclip = InStr(line, ",") - 1
    templine = Left(line, endclip)
    
    templine = TrimComplete(templine)
    'Check for *FRACUNIT values
    endclip = InStr(templine, "*FRACUNIT")
    If endclip <> 0 Then
        templine = Left(templine, endclip - 1)
        templine = Val(templine) * 65536
    End If
    'Check for crazy-odd MT_ usage
    endclip = InStr(templine, "MT_")
    If endclip <> 0 Then
        templine = FindThingNum(templine) & " - " & templine
    End If
    'Check for crazy-odd pw_ usage
    endclip = InStr(templine, "pw_")
    If endclip <> 0 Then
        templine = FindPowerNum(templine) & " - " & templine
    End If
    txtFuncVar1.Text = templine
    
    startclip = InStr(line, ",") + 1
    line = TrimComplete(Mid(line, startclip, Len(line) - startclip))
    endclip = InStr(line, ",") - 1
    templine = Left(line, endclip)
    
    templine = TrimComplete(templine)
    'Check for *FRACUNIT values
    endclip = InStr(templine, "*FRACUNIT")
    If endclip <> 0 Then
        templine = Left(templine, endclip - 1)
        templine = Val(templine) * 65536
    End If
    'Check for crazy-odd MT_ usage
    endclip = InStr(templine, "MT_")
    If endclip <> 0 Then
        templine = FindThingNum(templine) & " - " & templine
    End If
    'Check for crazy-odd pw_ usage
    endclip = InStr(templine, "pw_")
    If endclip <> 0 Then
        templine = FindPowerNum(templine) & " - " & templine
    End If
    txtFuncVar2.Text = templine
    
    startclip = InStr(line, ",") + 1
    line = TrimComplete(Mid(line, startclip, Len(line) - startclip))
    endclip = InStr(line, "}") - 1
    Call FindComboIndex(cmbNextstate, TrimComplete(Left(line, endclip)))
    
    ts.Close
    Set myFSO = Nothing
End Sub

Private Sub FindComboIndex(ByRef Box As ComboBox, line As String)
    Dim i As Integer
    
    For i = 0 To Box.ListCount
        If InStr(UCase(Box.List(i)), UCase(line)) Then
            Box.ListIndex = i
            Exit For
        End If
    Next
End Sub

Private Sub WriteState(Remove As Boolean, num As Integer)
    Dim myFSOSource As New Scripting.FileSystemObject
    Dim tsSource As TextStream
    Dim myFSOTarget As New Scripting.FileSystemObject
    Dim tsTarget As TextStream
    Dim line As String
    Dim word As String
    Dim word2 As String
    Dim flags As Long
    Dim statefound As Boolean
    
    statefound = False
    
    Set tsSource = myFSOSource.OpenTextFile(SOCFile, ForReading, False)
    Set tsTarget = myFSOTarget.OpenTextFile(SOCTemp, ForWriting, True)
    
    Do While Not tsSource.AtEndOfStream
        line = tsSource.ReadLine
        word = UCase(FirstToken(line))
        word2 = UCase(SecondToken(line))

        'If the current sound exists in the SOC, delete it.
        If word = "FRAME" And Val(word2) = num Then
            statefound = True
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
    
        tsTarget.WriteLine "FRAME " & num
        cmbSprite.Text = TrimComplete(cmbSprite.Text)
        txtFrame.Text = TrimComplete(txtFrame.Text)
        txtTics.Text = TrimComplete(txtTics.Text)
        cmbAction.Text = TrimComplete(cmbAction.Text)
        txtFuncVar1.Text = TrimComplete(txtFuncVar1.Text)
        txtFuncVar2.Text = TrimComplete(txtFuncVar2.Text)
        cmbNextstate.Text = TrimComplete(cmbNextstate.Text)
        cmbTranslucency.Text = TrimComplete(cmbTranslucency.Text)
        
        If cmbSprite.Text <> "" Then tsTarget.WriteLine "SPRITENUMBER = " & cmbSprite.ListIndex
        
        flags = Val(txtFrame.Text)
        If chkFullbright.Value = 1 Then flags = flags + 32768
        
        ' Grrr VB doesn't have bitshifts!!
        If cmbTranslucency.Text <> "" Then
            flags = flags + cmbTranslucency.ListIndex * 65536
        End If
        
        If txtFrame.Text <> "" Then tsTarget.WriteLine "SPRITESUBNUMBER = " & flags
        If txtTics.Text <> "" Then tsTarget.WriteLine "DURATION = " & Val(txtTics.Text)
        If cmbNextstate.Text <> "" Then tsTarget.WriteLine "NEXT = " & cmbNextstate.ListIndex
        If cmbAction.Text <> "" Then tsTarget.WriteLine "ACTION " & cmbAction.Text
        If txtFuncVar1.Text <> "" Then tsTarget.WriteLine "VAR1 = " & Val(txtFuncVar1.Text)
        If txtFuncVar2.Text <> "" Then tsTarget.WriteLine "VAR2 = " & Val(txtFuncVar2.Text)
    End If
    
    tsTarget.Close
    Set myFSOTarget = Nothing
    
    FileCopy SOCTemp, SOCFile
    
    Kill SOCTemp
    
    If Remove = True Then
        If statefound = True Then
            MsgBox "State removed from SOC."
        Else
            MsgBox "State not found in SOC."
        End If
    Else
        MsgBox "State Saved."
    End If
End Sub

Private Sub LoadThings()
    Dim myFSO As New Scripting.FileSystemObject
    Dim ts As TextStream
    Dim line As String
    Dim number As Integer
    Dim startclip As Integer, endclip As Integer
    Dim numfreeslots As Integer, i As Integer
    
    ChDir SourcePath
    Set ts = myFSO.OpenTextFile("info.h", ForReading, False)
    
    Do While InStr(ts.ReadLine, "Little flag for SOC editor (don't change this comment!)") = 0
    Loop
    
    ts.SkipLine ' typedef enum
    ts.SkipLine ' {
    
    line = ts.ReadLine
    number = 0
    
    lstThings.Clear
    
    Do While InStr(line, "MT_FIRSTFREESLOT") = 0
        startclip = InStr(line, "MT_")
        If InStr(line, "MT_") <> 0 Then
            endclip = InStr(line, ",")
            line = Mid(line, startclip, endclip - startclip)
            lstThings.AddItem number & " - " & line
            number = number + 1
        End If
        line = ts.ReadLine
    Loop
    
    ts.Close
    
    'Populate the free slots!
    Set ts = myFSO.OpenTextFile("info.h", ForReading, False)

    line = ts.ReadLine
    Do While InStr(line, "#define NUMMOBJFREESLOTS") = 0
        line = ts.ReadLine
    Loop
    
    startclip = InStr(line, "SLOTS ") + 6
    numfreeslots = Val(Mid(line, startclip, Len(line) - startclip + 1))

    For i = 1 To numfreeslots
        lstThings.AddItem number & " - " & "MT_FREESLOT" & i
        number = number + 1
    Next

    ts.Close
    Set myFSO = Nothing
End Sub

Private Function FindThingNum(ThingName As String) As Integer
    Dim i As Integer
    Dim temp As String
    Dim startpoint As Integer
    Dim endpoint As Integer
    
    lstThings.Clear
    LoadThings
    
    For i = 0 To lstThings.ListCount - 1
        temp = lstThings.List(i)
        startpoint = InStr(temp, "-") + 2
        endpoint = Len(temp) - startpoint + 1
        temp = Mid(temp, startpoint, endpoint)
        If temp = ThingName Then
            FindThingNum = Val(lstThings.List(i))
            Exit For
        End If
    Next
End Function

Private Function FindPowerNum(PowerName As String) As Integer
    Dim myFSO As New Scripting.FileSystemObject
    Dim ts As TextStream
    Dim line As String
    Dim number As Integer
    Dim startclip As Integer
    
    ChDir SourcePath
    Set ts = myFSO.OpenTextFile("d_player.h", ForReading, False)
    
    Do While InStr(ts.ReadLine, "Player powers. (don't edit this comment)") = 0
    Loop
    
    ts.SkipLine ' typedef enum
    ts.SkipLine ' {
    
    line = ts.ReadLine
    number = 0
    
    Do While InStr(line, "NUMPOWERS") = 0
        startclip = InStr(line, PowerName)
        If startclip <> 0 Then
            FindPowerNum = number
            Exit Do
        End If
        number = number + 1
        line = ts.ReadLine
    Loop
    
    ts.Close
    Set myFSO = Nothing
End Function
