VERSION 5.00
Begin VB.Form frmCutsceneEdit 
   Caption         =   "Cutscene Edit"
   ClientHeight    =   6495
   ClientLeft      =   60
   ClientTop       =   345
   ClientWidth     =   10410
   Icon            =   "frmCutsceneEdit.frx":0000
   LinkTopic       =   "Form1"
   MaxButton       =   0   'False
   ScaleHeight     =   6495
   ScaleWidth      =   10410
   StartUpPosition =   3  'Windows Default
   Begin VB.TextBox txtNumScenes 
      Height          =   285
      Left            =   3480
      MaxLength       =   3
      TabIndex        =   97
      Top             =   240
      Width           =   615
   End
   Begin VB.ListBox lstScene 
      Height          =   450
      ItemData        =   "frmCutsceneEdit.frx":0442
      Left            =   3360
      List            =   "frmCutsceneEdit.frx":0444
      TabIndex        =   94
      Top             =   600
      Width           =   735
   End
   Begin VB.ComboBox cmbMusicslot 
      Height          =   315
      Left            =   8760
      TabIndex        =   93
      Top             =   1680
      Width           =   1575
   End
   Begin VB.Frame frmPic1 
      Caption         =   "Picture 8:"
      Height          =   1335
      Index           =   7
      Left            =   7440
      TabIndex        =   80
      Top             =   5040
      Width           =   2895
      Begin VB.TextBox txtPicycoord 
         Height          =   285
         Index           =   7
         Left            =   600
         MaxLength       =   3
         TabIndex        =   85
         Top             =   960
         Width           =   495
      End
      Begin VB.TextBox txtPicXcoord 
         Height          =   285
         Index           =   7
         Left            =   600
         MaxLength       =   3
         TabIndex        =   84
         Top             =   600
         Width           =   495
      End
      Begin VB.TextBox txtPicduration 
         Height          =   285
         Index           =   7
         Left            =   2040
         MaxLength       =   5
         TabIndex        =   83
         Top             =   600
         Width           =   735
      End
      Begin VB.TextBox txtPicname 
         Height          =   285
         Index           =   7
         Left            =   1320
         TabIndex        =   82
         Top             =   240
         Width           =   1455
      End
      Begin VB.CheckBox chkPichires 
         Caption         =   "High-Resolution"
         Height          =   255
         Index           =   7
         Left            =   1320
         TabIndex        =   81
         Top             =   960
         Width           =   1455
      End
      Begin VB.Label lblPicycoord 
         Alignment       =   1  'Right Justify
         Caption         =   "Y:"
         Height          =   255
         Index           =   7
         Left            =   240
         TabIndex        =   89
         Top             =   960
         Width           =   255
      End
      Begin VB.Label lblPicxcoord 
         Alignment       =   1  'Right Justify
         Caption         =   "X:"
         Height          =   255
         Index           =   7
         Left            =   240
         TabIndex        =   88
         Top             =   600
         Width           =   255
      End
      Begin VB.Label lblPicduration 
         Alignment       =   1  'Right Justify
         Caption         =   "Duration:"
         Height          =   255
         Index           =   7
         Left            =   1200
         TabIndex        =   87
         Top             =   600
         Width           =   735
      End
      Begin VB.Label lblPicname 
         Alignment       =   1  'Right Justify
         Caption         =   "Picture Name:"
         Height          =   255
         Index           =   7
         Left            =   120
         TabIndex        =   86
         Top             =   240
         Width           =   1095
      End
   End
   Begin VB.Frame frmPic1 
      Caption         =   "Picture 7:"
      Height          =   1335
      Index           =   6
      Left            =   4440
      TabIndex        =   70
      Top             =   5040
      Width           =   2895
      Begin VB.TextBox txtPicycoord 
         Height          =   285
         Index           =   6
         Left            =   600
         MaxLength       =   3
         TabIndex        =   75
         Top             =   960
         Width           =   495
      End
      Begin VB.TextBox txtPicXcoord 
         Height          =   285
         Index           =   6
         Left            =   600
         MaxLength       =   3
         TabIndex        =   74
         Top             =   600
         Width           =   495
      End
      Begin VB.TextBox txtPicduration 
         Height          =   285
         Index           =   6
         Left            =   2040
         MaxLength       =   5
         TabIndex        =   73
         Top             =   600
         Width           =   735
      End
      Begin VB.TextBox txtPicname 
         Height          =   285
         Index           =   6
         Left            =   1320
         TabIndex        =   72
         Top             =   240
         Width           =   1455
      End
      Begin VB.CheckBox chkPichires 
         Caption         =   "High-Resolution"
         Height          =   255
         Index           =   6
         Left            =   1320
         TabIndex        =   71
         Top             =   960
         Width           =   1455
      End
      Begin VB.Label lblPicycoord 
         Alignment       =   1  'Right Justify
         Caption         =   "Y:"
         Height          =   255
         Index           =   6
         Left            =   240
         TabIndex        =   79
         Top             =   960
         Width           =   255
      End
      Begin VB.Label lblPicxcoord 
         Alignment       =   1  'Right Justify
         Caption         =   "X:"
         Height          =   255
         Index           =   6
         Left            =   240
         TabIndex        =   78
         Top             =   600
         Width           =   255
      End
      Begin VB.Label lblPicduration 
         Alignment       =   1  'Right Justify
         Caption         =   "Duration:"
         Height          =   255
         Index           =   6
         Left            =   1200
         TabIndex        =   77
         Top             =   600
         Width           =   735
      End
      Begin VB.Label lblPicname 
         Alignment       =   1  'Right Justify
         Caption         =   "Picture Name:"
         Height          =   255
         Index           =   6
         Left            =   120
         TabIndex        =   76
         Top             =   240
         Width           =   1095
      End
   End
   Begin VB.Frame frmPic1 
      Caption         =   "Picture 6:"
      Height          =   1335
      Index           =   5
      Left            =   7440
      TabIndex        =   60
      Top             =   3600
      Width           =   2895
      Begin VB.TextBox txtPicycoord 
         Height          =   285
         Index           =   5
         Left            =   600
         MaxLength       =   3
         TabIndex        =   65
         Top             =   960
         Width           =   495
      End
      Begin VB.TextBox txtPicXcoord 
         Height          =   285
         Index           =   5
         Left            =   600
         MaxLength       =   3
         TabIndex        =   64
         Top             =   600
         Width           =   495
      End
      Begin VB.TextBox txtPicduration 
         Height          =   285
         Index           =   5
         Left            =   2040
         MaxLength       =   5
         TabIndex        =   63
         Top             =   600
         Width           =   735
      End
      Begin VB.TextBox txtPicname 
         Height          =   285
         Index           =   5
         Left            =   1320
         TabIndex        =   62
         Top             =   240
         Width           =   1455
      End
      Begin VB.CheckBox chkPichires 
         Caption         =   "High-Resolution"
         Height          =   255
         Index           =   5
         Left            =   1320
         TabIndex        =   61
         Top             =   960
         Width           =   1455
      End
      Begin VB.Label lblPicycoord 
         Alignment       =   1  'Right Justify
         Caption         =   "Y:"
         Height          =   255
         Index           =   5
         Left            =   240
         TabIndex        =   69
         Top             =   960
         Width           =   255
      End
      Begin VB.Label lblPicxcoord 
         Alignment       =   1  'Right Justify
         Caption         =   "X:"
         Height          =   255
         Index           =   5
         Left            =   240
         TabIndex        =   68
         Top             =   600
         Width           =   255
      End
      Begin VB.Label lblPicduration 
         Alignment       =   1  'Right Justify
         Caption         =   "Duration:"
         Height          =   255
         Index           =   5
         Left            =   1200
         TabIndex        =   67
         Top             =   600
         Width           =   735
      End
      Begin VB.Label lblPicname 
         Alignment       =   1  'Right Justify
         Caption         =   "Picture Name:"
         Height          =   255
         Index           =   5
         Left            =   120
         TabIndex        =   66
         Top             =   240
         Width           =   1095
      End
   End
   Begin VB.Frame frmPic1 
      Caption         =   "Picture 5:"
      Height          =   1335
      Index           =   4
      Left            =   4440
      TabIndex        =   50
      Top             =   3600
      Width           =   2895
      Begin VB.TextBox txtPicycoord 
         Height          =   285
         Index           =   4
         Left            =   600
         MaxLength       =   3
         TabIndex        =   55
         Top             =   960
         Width           =   495
      End
      Begin VB.TextBox txtPicXcoord 
         Height          =   285
         Index           =   4
         Left            =   600
         MaxLength       =   3
         TabIndex        =   54
         Top             =   600
         Width           =   495
      End
      Begin VB.TextBox txtPicduration 
         Height          =   285
         Index           =   4
         Left            =   2040
         MaxLength       =   5
         TabIndex        =   53
         Top             =   600
         Width           =   735
      End
      Begin VB.TextBox txtPicname 
         Height          =   285
         Index           =   4
         Left            =   1320
         TabIndex        =   52
         Top             =   240
         Width           =   1455
      End
      Begin VB.CheckBox chkPichires 
         Caption         =   "High-Resolution"
         Height          =   255
         Index           =   4
         Left            =   1320
         TabIndex        =   51
         Top             =   960
         Width           =   1455
      End
      Begin VB.Label lblPicycoord 
         Alignment       =   1  'Right Justify
         Caption         =   "Y:"
         Height          =   255
         Index           =   4
         Left            =   240
         TabIndex        =   59
         Top             =   960
         Width           =   255
      End
      Begin VB.Label lblPicxcoord 
         Alignment       =   1  'Right Justify
         Caption         =   "X:"
         Height          =   255
         Index           =   4
         Left            =   240
         TabIndex        =   58
         Top             =   600
         Width           =   255
      End
      Begin VB.Label lblPicduration 
         Alignment       =   1  'Right Justify
         Caption         =   "Duration:"
         Height          =   255
         Index           =   4
         Left            =   1200
         TabIndex        =   57
         Top             =   600
         Width           =   735
      End
      Begin VB.Label lblPicname 
         Alignment       =   1  'Right Justify
         Caption         =   "Picture Name:"
         Height          =   255
         Index           =   4
         Left            =   120
         TabIndex        =   56
         Top             =   240
         Width           =   1095
      End
   End
   Begin VB.Frame frmPic1 
      Caption         =   "Picture 4:"
      Height          =   1335
      Index           =   3
      Left            =   1440
      TabIndex        =   40
      Top             =   3600
      Width           =   2895
      Begin VB.CheckBox chkPichires 
         Caption         =   "High-Resolution"
         Height          =   255
         Index           =   3
         Left            =   1320
         TabIndex        =   45
         Top             =   960
         Width           =   1455
      End
      Begin VB.TextBox txtPicname 
         Height          =   285
         Index           =   3
         Left            =   1320
         TabIndex        =   44
         Top             =   240
         Width           =   1455
      End
      Begin VB.TextBox txtPicduration 
         Height          =   285
         Index           =   3
         Left            =   2040
         MaxLength       =   5
         TabIndex        =   43
         Top             =   600
         Width           =   735
      End
      Begin VB.TextBox txtPicXcoord 
         Height          =   285
         Index           =   3
         Left            =   600
         MaxLength       =   3
         TabIndex        =   42
         Top             =   600
         Width           =   495
      End
      Begin VB.TextBox txtPicycoord 
         Height          =   285
         Index           =   3
         Left            =   600
         MaxLength       =   3
         TabIndex        =   41
         Top             =   960
         Width           =   495
      End
      Begin VB.Label lblPicname 
         Alignment       =   1  'Right Justify
         Caption         =   "Picture Name:"
         Height          =   255
         Index           =   3
         Left            =   120
         TabIndex        =   49
         Top             =   240
         Width           =   1095
      End
      Begin VB.Label lblPicduration 
         Alignment       =   1  'Right Justify
         Caption         =   "Duration:"
         Height          =   255
         Index           =   3
         Left            =   1200
         TabIndex        =   48
         Top             =   600
         Width           =   735
      End
      Begin VB.Label lblPicxcoord 
         Alignment       =   1  'Right Justify
         Caption         =   "X:"
         Height          =   255
         Index           =   3
         Left            =   240
         TabIndex        =   47
         Top             =   600
         Width           =   255
      End
      Begin VB.Label lblPicycoord 
         Alignment       =   1  'Right Justify
         Caption         =   "Y:"
         Height          =   255
         Index           =   3
         Left            =   240
         TabIndex        =   46
         Top             =   960
         Width           =   255
      End
   End
   Begin VB.Frame frmPic1 
      Caption         =   "Picture 3:"
      Height          =   1335
      Index           =   2
      Left            =   7440
      TabIndex        =   30
      Top             =   2160
      Width           =   2895
      Begin VB.CheckBox chkPichires 
         Caption         =   "High-Resolution"
         Height          =   255
         Index           =   2
         Left            =   1320
         TabIndex        =   35
         Top             =   960
         Width           =   1455
      End
      Begin VB.TextBox txtPicname 
         Height          =   285
         Index           =   2
         Left            =   1320
         TabIndex        =   34
         Top             =   240
         Width           =   1455
      End
      Begin VB.TextBox txtPicduration 
         Height          =   285
         Index           =   2
         Left            =   2040
         MaxLength       =   5
         TabIndex        =   33
         Top             =   600
         Width           =   735
      End
      Begin VB.TextBox txtPicXcoord 
         Height          =   285
         Index           =   2
         Left            =   600
         MaxLength       =   3
         TabIndex        =   32
         Top             =   600
         Width           =   495
      End
      Begin VB.TextBox txtPicycoord 
         Height          =   285
         Index           =   2
         Left            =   600
         MaxLength       =   3
         TabIndex        =   31
         Top             =   960
         Width           =   495
      End
      Begin VB.Label lblPicname 
         Alignment       =   1  'Right Justify
         Caption         =   "Picture Name:"
         Height          =   255
         Index           =   2
         Left            =   120
         TabIndex        =   39
         Top             =   240
         Width           =   1095
      End
      Begin VB.Label lblPicduration 
         Alignment       =   1  'Right Justify
         Caption         =   "Duration:"
         Height          =   255
         Index           =   2
         Left            =   1200
         TabIndex        =   38
         Top             =   600
         Width           =   735
      End
      Begin VB.Label lblPicxcoord 
         Alignment       =   1  'Right Justify
         Caption         =   "X:"
         Height          =   255
         Index           =   2
         Left            =   240
         TabIndex        =   37
         Top             =   600
         Width           =   255
      End
      Begin VB.Label lblPicycoord 
         Alignment       =   1  'Right Justify
         Caption         =   "Y:"
         Height          =   255
         Index           =   2
         Left            =   240
         TabIndex        =   36
         Top             =   960
         Width           =   255
      End
   End
   Begin VB.Frame frmPic1 
      Caption         =   "Picture 2:"
      Height          =   1335
      Index           =   1
      Left            =   4440
      TabIndex        =   20
      Top             =   2160
      Width           =   2895
      Begin VB.CheckBox chkPichires 
         Caption         =   "High-Resolution"
         Height          =   255
         Index           =   1
         Left            =   1320
         TabIndex        =   25
         Top             =   960
         Width           =   1455
      End
      Begin VB.TextBox txtPicname 
         Height          =   285
         Index           =   1
         Left            =   1320
         TabIndex        =   24
         Top             =   240
         Width           =   1455
      End
      Begin VB.TextBox txtPicduration 
         Height          =   285
         Index           =   1
         Left            =   2040
         MaxLength       =   5
         TabIndex        =   23
         Top             =   600
         Width           =   735
      End
      Begin VB.TextBox txtPicXcoord 
         Height          =   285
         Index           =   1
         Left            =   600
         MaxLength       =   3
         TabIndex        =   22
         Top             =   600
         Width           =   495
      End
      Begin VB.TextBox txtPicycoord 
         Height          =   285
         Index           =   1
         Left            =   600
         MaxLength       =   3
         TabIndex        =   21
         Top             =   960
         Width           =   495
      End
      Begin VB.Label lblPicname 
         Alignment       =   1  'Right Justify
         Caption         =   "Picture Name:"
         Height          =   255
         Index           =   1
         Left            =   120
         TabIndex        =   29
         Top             =   240
         Width           =   1095
      End
      Begin VB.Label lblPicduration 
         Alignment       =   1  'Right Justify
         Caption         =   "Duration:"
         Height          =   255
         Index           =   1
         Left            =   1200
         TabIndex        =   28
         Top             =   600
         Width           =   735
      End
      Begin VB.Label lblPicxcoord 
         Alignment       =   1  'Right Justify
         Caption         =   "X:"
         Height          =   255
         Index           =   1
         Left            =   240
         TabIndex        =   27
         Top             =   600
         Width           =   255
      End
      Begin VB.Label lblPicycoord 
         Alignment       =   1  'Right Justify
         Caption         =   "Y:"
         Height          =   255
         Index           =   1
         Left            =   240
         TabIndex        =   26
         Top             =   960
         Width           =   255
      End
   End
   Begin VB.TextBox txtTextypos 
      Height          =   285
      Left            =   3120
      MaxLength       =   3
      TabIndex        =   19
      Top             =   5880
      Width           =   615
   End
   Begin VB.TextBox txtTextxpos 
      Height          =   285
      Left            =   3120
      MaxLength       =   3
      TabIndex        =   18
      Top             =   5520
      Width           =   615
   End
   Begin VB.Frame frmPic1 
      Caption         =   "Picture 1:"
      Height          =   1335
      Index           =   0
      Left            =   1440
      TabIndex        =   6
      Top             =   2160
      Width           =   2895
      Begin VB.TextBox txtPicycoord 
         Height          =   285
         Index           =   0
         Left            =   600
         MaxLength       =   3
         TabIndex        =   14
         Top             =   960
         Width           =   495
      End
      Begin VB.TextBox txtPicXcoord 
         Height          =   285
         Index           =   0
         Left            =   600
         MaxLength       =   3
         TabIndex        =   12
         Top             =   600
         Width           =   495
      End
      Begin VB.TextBox txtPicduration 
         Height          =   285
         Index           =   0
         Left            =   2040
         MaxLength       =   5
         TabIndex        =   11
         Top             =   600
         Width           =   735
      End
      Begin VB.TextBox txtPicname 
         Height          =   285
         Index           =   0
         Left            =   1320
         TabIndex        =   8
         Top             =   240
         Width           =   1455
      End
      Begin VB.CheckBox chkPichires 
         Caption         =   "High-Resolution"
         Height          =   255
         Index           =   0
         Left            =   1320
         TabIndex        =   7
         Top             =   960
         Width           =   1455
      End
      Begin VB.Label lblPicycoord 
         Alignment       =   1  'Right Justify
         Caption         =   "Y:"
         Height          =   255
         Index           =   0
         Left            =   240
         TabIndex        =   15
         Top             =   960
         Width           =   255
      End
      Begin VB.Label lblPicxcoord 
         Alignment       =   1  'Right Justify
         Caption         =   "X:"
         Height          =   255
         Index           =   0
         Left            =   240
         TabIndex        =   13
         Top             =   600
         Width           =   255
      End
      Begin VB.Label lblPicduration 
         Alignment       =   1  'Right Justify
         Caption         =   "Duration:"
         Height          =   255
         Index           =   0
         Left            =   1200
         TabIndex        =   10
         Top             =   600
         Width           =   735
      End
      Begin VB.Label lblPicname 
         Alignment       =   1  'Right Justify
         Caption         =   "Picture Name:"
         Height          =   255
         Index           =   0
         Left            =   120
         TabIndex        =   9
         Top             =   240
         Width           =   1095
      End
   End
   Begin VB.TextBox txtNumberofpics 
      Height          =   285
      Left            =   3840
      MaxLength       =   1
      TabIndex        =   5
      Top             =   1800
      Width           =   375
   End
   Begin VB.TextBox txtScenetext 
      Height          =   1815
      Left            =   4440
      MultiLine       =   -1  'True
      TabIndex        =   3
      Top             =   240
      Width           =   3135
   End
   Begin VB.CommandButton cmdSave 
      Caption         =   "&Save Scene"
      Height          =   495
      Left            =   3360
      Style           =   1  'Graphical
      TabIndex        =   1
      Top             =   1200
      Width           =   855
   End
   Begin VB.ListBox lstCutscenes 
      Height          =   6300
      ItemData        =   "frmCutsceneEdit.frx":0446
      Left            =   120
      List            =   "frmCutsceneEdit.frx":0448
      TabIndex        =   0
      Top             =   120
      Width           =   1215
   End
   Begin VB.Label Label3 
      Caption         =   "Note: The cutscene editor is not fully functional. Only use it to get an idea of the proper syntax to use."
      BeginProperty Font 
         Name            =   "MS Sans Serif"
         Size            =   9.75
         Charset         =   0
         Weight          =   700
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   1335
      Left            =   7680
      TabIndex        =   98
      Top             =   240
      Width           =   2655
   End
   Begin VB.Label Label2 
      Caption         =   "For Scene Text:"
      BeginProperty Font 
         Name            =   "MS Sans Serif"
         Size            =   8.25
         Charset         =   0
         Weight          =   400
         Underline       =   -1  'True
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   255
      Left            =   1560
      TabIndex        =   96
      Top             =   5160
      Width           =   1695
   End
   Begin VB.Label Label1 
      Caption         =   "Enter all time durations in game tics (35 = 1 second)"
      BeginProperty Font 
         Name            =   "MS Sans Serif"
         Size            =   8.25
         Charset         =   0
         Weight          =   700
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   855
      Left            =   1560
      TabIndex        =   95
      Top             =   960
      Width           =   1335
   End
   Begin VB.Label lblMusicslot 
      Alignment       =   1  'Right Justify
      Caption         =   "Music to play:"
      Height          =   255
      Left            =   7680
      TabIndex        =   92
      Top             =   1680
      Width           =   975
   End
   Begin VB.Label lblCurrentScene 
      Alignment       =   1  'Right Justify
      Caption         =   "Current Scene:"
      Height          =   255
      Left            =   1920
      TabIndex        =   91
      Top             =   720
      Width           =   1215
   End
   Begin VB.Label lblNumScenes 
      Alignment       =   1  'Right Justify
      Caption         =   "Number of Scenes in this Cutscene:"
      Height          =   375
      Left            =   1440
      TabIndex        =   90
      Top             =   120
      Width           =   1935
   End
   Begin VB.Label lblTextypos 
      Alignment       =   1  'Right Justify
      Caption         =   "Text Y Position:"
      Height          =   255
      Left            =   1800
      TabIndex        =   17
      Top             =   5880
      Width           =   1215
   End
   Begin VB.Label lblTextxpos 
      Alignment       =   1  'Right Justify
      Caption         =   "Text X Position:"
      Height          =   255
      Left            =   1800
      TabIndex        =   16
      Top             =   5520
      Width           =   1215
   End
   Begin VB.Label lblNumberofpics 
      Alignment       =   1  'Right Justify
      Caption         =   "Number of Pictures (max 8):"
      Height          =   255
      Left            =   1560
      TabIndex        =   4
      Top             =   1800
      Width           =   2175
   End
   Begin VB.Label lblScenetext 
      Caption         =   "Scene Text:"
      Height          =   255
      Left            =   4440
      TabIndex        =   2
      Top             =   0
      Width           =   1215
   End
End
Attribute VB_Name = "frmCutsceneEdit"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Option Explicit

Private Sub cmdSave_Click()
    Call WriteScene(False)
End Sub

Private Sub lstScene_Click()
    Call ClearForm
    Call LoadSOCCutscene(Val(lstCutscenes.List(lstCutscenes.ListIndex)), Val(lstScene.List(lstScene.ListIndex)))
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

Private Sub cmdReload_Click()
    Call Reload
End Sub

Private Sub Form_Load()
    Call Reload
End Sub

Private Sub Reload()
    ClearForm
    Call ReadCutsceneSOCNumbers
    Call LoadMusic
    If lstCutscenes.ListCount > 0 Then
        lstCutscenes.ListIndex = 0
    End If
End Sub

Private Sub LoadSOCCutscene(CutNum As Integer, SceneNum As Integer)
    Dim myFSO As New Scripting.FileSystemObject
    Dim ts As TextStream
    Dim line As String
    Dim word As String
    Dim word2 As String
    Dim ind As Integer
    
    Set ts = myFSO.OpenTextFile(SOCFile, ForReading, False)

SOCLoad:
    Do While Not ts.AtEndOfStream
        line = ts.ReadLine
        
        If Left(line, 1) = "#" Then GoTo SOCLoad
        
        If Left(line, 1) = vbCrLf Then GoTo SOCLoad
        
        If Len(line) < 1 Then GoTo SOCLoad
        
        word = FirstToken(line)
        word2 = SecondToken(line)
        
        ' WOW! This looks fun, don't it?!
        If UCase(word) = "CUTSCENE" And Val(word2) = CutNum Then
            Do While Len(line) > 0 And Not ts.AtEndOfStream
                line = ts.ReadLine
                word = UCase(FirstToken(line))
                word2 = UCase(SecondToken(line))

                If word = "NUMSCENES" Then
                    txtNumScenes.Text = Val(word2)
                ElseIf UCase(word) = "SCENE" And Val(word2) = SceneNum Then
                    Do While Len(line) > 0 And Not ts.AtEndOfStream
                        line = ts.ReadLine
                        word = UCase(FirstToken(line))
                        word2 = UCase(SecondTokenEqual(line))
                    
                        If word = "SCENETEXT" Then
                            Dim startclip As Integer, endclip As Integer
                            startclip = InStr(line, "=")
    
                            startclip = startclip + 2
    
                            line = Mid(line, startclip, Len(line))
                    
                            txtScenetext.Text = line & vbCrLf
                    
                            Do While InStr(line, "#") = 0 And Not ts.AtEndOfStream
                                line = ts.ReadLine & vbCrLf
                                txtScenetext.Text = txtScenetext.Text & line
                            Loop
                    
                            txtScenetext.Text = RTrimComplete(txtScenetext.Text)
                            If Right(txtScenetext.Text, 1) = "#" Then
                                txtScenetext.Text = Left(txtScenetext.Text, Len(txtScenetext.Text) - 1)
                            End If
                        ElseIf word = "PIC1NAME" Or word = "PIC2NAME" Or word = "PIC3NAME" Or word = "PIC4NAME" Or word = "PIC5NAME" Or word = "PIC6NAME" Or word = "PIC7NAME" Or word = "PIC8NAME" Then
                            ind = Val(Mid(word, 4, 1)) - 1
                            txtPicname(ind).Text = word2
                        ElseIf word = "PIC1HIRES" Or word = "PIC2HIRES" Or word = "PIC3HIRES" Or word = "PIC4HIRES" Or word = "PIC5HIRES" Or word = "PIC6HIRES" Or word = "PIC7HIRES" Or word = "PIC8HIRES" Then
                            ind = Val(Mid(word, 4, 1)) - 1
                            chkPichires(ind).Value = Val(word2)
                        ElseIf word = "PIC1DURATION" Or word = "PIC2DURATION" Or word = "PIC3DURATION" Or word = "PIC4DURATION" Or word = "PIC5DURATION" Or word = "PIC6DURATION" Or word = "PIC7DURATION" Or word = "PIC8DURATION" Then
                            ind = Val(Mid(word, 4, 1)) - 1
                            txtPicduration(ind).Text = Val(word2)
                        ElseIf word = "PIC1XCOORD" Or word = "PIC2XCOORD" Or word = "PIC3XCOORD" Or word = "PIC4XCOORD" Or word = "PIC5XCOORD" Or word = "PIC6XCOORD" Or word = "PIC7XCOORD" Or word = "PIC8XCOORD" Then
                            ind = Val(Mid(word, 4, 1)) - 1
                            txtPicXcoord(ind).Text = Val(word2)
                        ElseIf word = "PIC1YCOORD" Or word = "PIC2YCOORD" Or word = "PIC3YCOORD" Or word = "PIC4YCOORD" Or word = "PIC5YCOORD" Or word = "PIC6YCOORD" Or word = "PIC7YCOORD" Or word = "PIC8YCOORD" Then
                            ind = Val(Mid(word, 4, 1)) - 1
                            txtPicycoord(ind).Text = Val(word2)
                        ElseIf word = "TEXTXPOS" Then
                            txtTextxpos.Text = Val(word2)
                        ElseIf word = "TEXTYPOS" Then
                            txtTextypos.Text = Val(word2)
                        ElseIf word = "MUSICSLOT" Then
                            cmbMusicslot.ListIndex = Val(word2)
                        ElseIf word = "NUMBEROFPICS" Then
                            txtNumberofpics.Text = Val(word2)
                        ElseIf word = "SCENE" Then 'End of scene data
                            line = ""
                        ElseIf Len(line) > 0 And Left(line, 1) <> "#" Then
                            MsgBox "Error in SOC!" & vbCrLf & "Unknown line: " & line
                        End If
                    Loop
                End If
            Loop
            Exit Do
        End If
    Loop
    
    ts.Close
    Set myFSO = Nothing
End Sub

Private Sub ReadCutsceneSOCNumbers()
    Dim myFSO As New Scripting.FileSystemObject
    Dim ts As TextStream
    Dim line As String
    Dim word As String
    Dim word2 As String
    
    Set ts = myFSO.OpenTextFile(SOCFile, ForReading, False)
    
    lstCutscenes.Clear
    
CutsceneLoad:
    Do While Not ts.AtEndOfStream
        line = ts.ReadLine
        
        If Left(line, 1) = "#" Then GoTo CutsceneLoad
        
        If Left(line, 1) = vbCrLf Then GoTo CutsceneLoad
        
        If Len(line) < 1 Then GoTo CutsceneLoad
        
        word = FirstToken(line)
        word2 = SecondToken(line)
        
        If UCase(word) = "CUTSCENE" Then
            lstCutscenes.AddItem (Val(word2))
        End If
    Loop
    
    ts.Close
    Set myFSO = Nothing
End Sub

Private Sub ClearForm()
    Dim i As Integer
    
    For i = 0 To 7
        chkPichires(i).Value = 0
        txtPicXcoord(i).Text = ""
        txtPicycoord(i).Text = ""
        txtPicname(i).Text = ""
        txtPicduration(i).Text = ""
    Next
    
    txtScenetext.Text = ""
    txtNumberofpics.Text = ""
    txtTextxpos.Text = ""
    txtTextypos.Text = ""
    cmbMusicslot.Text = ""
End Sub

Private Sub lstCutscenes_Click()
    Dim i As Integer
    
    LoadNumScenes (Val(lstCutscenes.List(lstCutscenes.ListIndex)))
    
    lstScene.Clear
    For i = 1 To Val(txtNumScenes.Text)
        lstScene.AddItem i
    Next
    
    If Val(txtNumScenes) > 0 Then
        lstScene.ListIndex = lstScene.ListCount - 1
        lstScene.ListIndex = 0
    End If
End Sub

Private Sub LoadNumScenes(CutNum As Integer)
    Dim myFSO As New Scripting.FileSystemObject
    Dim ts As TextStream
    Dim line As String
    Dim word As String
    Dim word2 As String
    Dim ind As Integer
    
    Set ts = myFSO.OpenTextFile(SOCFile, ForReading, False)

SOCLoad:
    Do While Not ts.AtEndOfStream
        line = ts.ReadLine
        
        If Left(line, 1) = "#" Then GoTo SOCLoad
        
        If Left(line, 1) = vbCrLf Then GoTo SOCLoad
        
        If Len(line) < 1 Then GoTo SOCLoad
        
        word = FirstToken(line)
        word2 = SecondToken(line)
        
        ' WOW! This looks fun, don't it?!
        If UCase(word) = "CUTSCENE" And Val(word2) = CutNum Then
            Do While Len(line) > 0 And Not ts.AtEndOfStream
                line = ts.ReadLine
                word = UCase(FirstToken(line))
                word2 = UCase(SecondToken(line))

                If word = "NUMSCENES" Then
                    txtNumScenes.Text = Val(word2)
                End If
            Loop
            Exit Do
        End If
    Loop
    
    ts.Close
    Set myFSO = Nothing
End Sub

Private Sub WriteScene(Remove As Boolean)
    Dim myFSOSource As New Scripting.FileSystemObject
    Dim tsSource As TextStream
    Dim myFSOTarget As New Scripting.FileSystemObject
    Dim tsTarget As TextStream
    Dim line As String
    Dim word As String
    Dim word2 As String
    Dim flags As Long
    Dim i As Integer
    Dim CutsceneNum As Integer
    Dim InCutScene As Boolean
    Dim scenefound As Boolean
    Dim nevercheckagain As Boolean
    
    ' This whole sub is a mess, but it works,
    ' so I'd better not touch it...
    scenefound = False
    nevercheckagain = False
    
    InCutScene = False
    CutsceneNum = Val(lstCutscenes.List(lstCutscenes.ListIndex))
    
    Set tsSource = myFSOSource.OpenTextFile(SOCFile, ForReading, False)
    Set tsTarget = myFSOTarget.OpenTextFile(SOCTemp, ForWriting, True)
    
    Do While Not tsSource.AtEndOfStream
        line = tsSource.ReadLine
        word = UCase(FirstToken(line))
        word2 = UCase(SecondToken(line))

        If nevercheckagain = False And word = "CUTSCENE" And Val(word2) = CutsceneNum Then
            InCutScene = True
            tsTarget.WriteLine "CUTSCENE " & Val(lstCutscenes.List(lstCutscenes.ListIndex))
            tsTarget.WriteLine "NUMSCENES " & Val(txtNumScenes.Text)
            line = tsSource.ReadLine
            word = UCase(FirstToken(line))
            word2 = UCase(SecondToken(line))
            If word = "NUMSCENES" Then
                line = tsSource.ReadLine
                word = UCase(FirstToken(line))
                word2 = UCase(SecondToken(line))
            End If
        End If
        
        'If the current scene exists in the SOC, delete it.
        If nevercheckagain = False And InCutScene = True And word = "SCENE" And Val(word2) = Val(lstScene.List(lstScene.ListIndex)) Then
            scenefound = True
            line = tsSource.ReadLine
            Do While (Left(UCase(line), 6) <> "SCENE " And Len(TrimComplete(line)) > 0) And Not (tsSource.AtEndOfStream)
                line = tsSource.ReadLine
            Loop
            If Remove = False Then
                tsTarget.WriteLine "SCENE " & Val(lstScene.List(lstScene.ListIndex))
                txtNumberofpics.Text = TrimComplete(txtNumberofpics.Text)
                txtTextxpos.Text = TrimComplete(txtTextxpos.Text)
                txtTextypos.Text = TrimComplete(txtTextypos.Text)
                cmbMusicslot.Text = TrimComplete(cmbMusicslot.Text)
        
                For i = 0 To 7
                    txtPicname(i).Text = TrimComplete(txtPicname(i).Text)
                    txtPicXcoord(i).Text = TrimComplete(txtPicXcoord(i).Text)
                    txtPicycoord(i).Text = TrimComplete(txtPicycoord(i).Text)
                    txtPicduration(i).Text = TrimComplete(txtPicduration(i).Text)
                Next
        
                If txtNumberofpics.Text <> "" Then tsTarget.WriteLine "NUMBEROFPICS = " & Val(txtNumberofpics.Text)
                If txtTextxpos.Text <> "" Then tsTarget.WriteLine "TEXTXPOS = " & Val(txtTextxpos.Text)
                If txtTextypos.Text <> "" Then tsTarget.WriteLine "TEXTYPOS = " & Val(txtTextypos.Text)
                If cmbMusicslot.Text <> "" Then tsTarget.WriteLine "MUSICSLOT = " & Val(cmbMusicslot.Text)
        
                For i = 0 To 7
                    If txtPicname(i).Text <> "" Then tsTarget.WriteLine "PIC" & (i + 1) & "NAME = " & txtPicname(i).Text
            
                    If chkPichires(i).Value = 1 Then tsTarget.WriteLine "PIC" & (i + 1) & "HIRES = 1"
            
                    If txtPicXcoord(i).Text <> "" Then tsTarget.WriteLine "PIC" & (i + 1) & "XCOORD = " & Val(txtPicXcoord(i).Text)
                    If txtPicycoord(i).Text <> "" Then tsTarget.WriteLine "PIC" & (i + 1) & "YCOORD = " & Val(txtPicycoord(i).Text)
                    If txtPicduration(i).Text <> "" Then tsTarget.WriteLine "PIC" & (i + 1) & "DURATION = " & Val(txtPicduration(i).Text)
                Next
                If txtScenetext.Text <> "" Then tsTarget.WriteLine "SCENETEXT = " & txtScenetext.Text & "#"
            End If
            InCutScene = False
            nevercheckagain = True
        End If
            tsTarget.WriteLine line
    Loop
    
    tsSource.Close
    Set myFSOSource = Nothing
    
    If Remove = False And scenefound = False Then
        If line <> "" Then tsTarget.WriteLine ""
    
        tsTarget.WriteLine "CUTSCENE " & Val(lstCutscenes.List(lstCutscenes.ListIndex))
        tsTarget.WriteLine "NUMSCENES " & Val(txtNumScenes.Text)
        tsTarget.WriteLine "SCENE " & Val(lstScene.List(lstScene.ListIndex))
        txtNumberofpics.Text = TrimComplete(txtNumberofpics.Text)
        txtScenetext.Text = TrimComplete(txtScenetext.Text)
        txtTextxpos.Text = TrimComplete(txtTextxpos.Text)
        txtTextypos.Text = TrimComplete(txtTextypos.Text)
        cmbMusicslot.Text = TrimComplete(cmbMusicslot.Text)
        
        For i = 0 To 7
            txtPicname(i).Text = TrimComplete(txtPicname(i).Text)
            txtPicXcoord(i).Text = TrimComplete(txtPicXcoord(i).Text)
            txtPicycoord(i).Text = TrimComplete(txtPicycoord(i).Text)
            txtPicduration(i).Text = TrimComplete(txtPicduration(i).Text)
        Next
        
        If txtNumberofpics.Text <> "" Then tsTarget.WriteLine "NUMBEROFPICS = " & Val(txtNumberofpics.Text)
        If txtScenetext.Text <> "" Then tsTarget.WriteLine "SCENETEXT = " & txtScenetext.Text & "#"
        If txtTextxpos.Text <> "" Then tsTarget.WriteLine "TEXTXPOS = " & Val(txtTextxpos.Text)
        If txtTextypos.Text <> "" Then tsTarget.WriteLine "TEXTYPOS = " & Val(txtTextypos.Text)
        If cmbMusicslot.Text <> "" Then tsTarget.WriteLine "MUSICSLOT = " & Val(cmbMusicslot.Text)
        
        For i = 0 To 7
            If txtPicname(i).Text <> "" Then tsTarget.WriteLine "PIC" & (i + 1) & "NAME = " & txtPicname(i).Text
            
            If chkPichires(i).Value = 1 Then tsTarget.WriteLine "PIC" & (i + 1) & "HIRES = 1"
            
            If txtPicXcoord(i).Text <> "" Then tsTarget.WriteLine "PIC" & (i + 1) & "XCOORD = " & Val(txtPicXcoord(i).Text)
            If txtPicycoord(i).Text <> "" Then tsTarget.WriteLine "PIC" & (i + 1) & "YCOORD = " & Val(txtPicycoord(i).Text)
            If txtPicduration(i).Text <> "" Then tsTarget.WriteLine "PIC" & (i + 1) & "DURATION = " & Val(txtPicduration(i).Text)
        Next
    End If
            
    tsTarget.Close
    Set myFSOTarget = Nothing
    
    FileCopy SOCTemp, SOCFile
    
    Kill SOCTemp
    
    If Remove = True Then
        If scenefound = True Then
            MsgBox "Scene removed from SOC."
        Else
            MsgBox "Scene not found in SOC."
        End If
    Else
        MsgBox "Scene Saved."
    End If
End Sub
