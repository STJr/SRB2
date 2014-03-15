VERSION 5.00
Begin VB.Form frmThingEdit 
   Caption         =   "Thing Edit"
   ClientHeight    =   5745
   ClientLeft      =   60
   ClientTop       =   345
   ClientWidth     =   11880
   Icon            =   "Things.frx":0000
   LinkTopic       =   "Form1"
   MaxButton       =   0   'False
   ScaleHeight     =   5745
   ScaleWidth      =   11880
   StartUpPosition =   3  'Windows Default
   Begin VB.CommandButton cmdCopy 
      Caption         =   "&Copy Thing"
      Height          =   615
      Left            =   6600
      TabIndex        =   77
      Top             =   4920
      Width           =   975
   End
   Begin VB.CommandButton cmdLoadDefault 
      Caption         =   "&Load Code Default"
      Height          =   615
      Left            =   4440
      Style           =   1  'Graphical
      TabIndex        =   76
      Top             =   4920
      Width           =   975
   End
   Begin VB.CommandButton cmdDelete 
      Caption         =   "&Delete Thing from SOC"
      Height          =   615
      Left            =   3240
      Style           =   1  'Graphical
      TabIndex        =   74
      Top             =   4920
      Width           =   1095
   End
   Begin VB.CommandButton cmdSave 
      Caption         =   "&Save"
      Height          =   615
      Left            =   5520
      TabIndex        =   73
      Top             =   4920
      Width           =   975
   End
   Begin VB.Frame frmFlags 
      Caption         =   "Flags"
      Height          =   3735
      Left            =   7680
      TabIndex        =   45
      Top             =   1920
      Width           =   4095
      Begin VB.CheckBox chkFlags 
         Caption         =   "MF_FIRE"
         Height          =   255
         Index           =   26
         Left            =   2040
         TabIndex        =   72
         Tag             =   "4194304"
         ToolTipText     =   "Fire object. Doesn't harm if you have red shield."
         Top             =   2160
         Width           =   1695
      End
      Begin VB.CheckBox chkFlags 
         Caption         =   "MF_NOCLIPTHING"
         Height          =   255
         Index           =   25
         Left            =   2040
         TabIndex        =   71
         Tag             =   "1073741824"
         ToolTipText     =   "Don't be blocked by things (partial clipping)"
         Top             =   3120
         Width           =   1815
      End
      Begin VB.CheckBox chkFlags 
         Caption         =   "MF_SCENERY"
         Height          =   255
         Index           =   24
         Left            =   2040
         TabIndex        =   70
         Tag             =   "33554432"
         ToolTipText     =   "Scenery (uses scenery thinker). Uses less CPU than a standard object, but generally can't move, etc."
         Top             =   2880
         Width           =   1455
      End
      Begin VB.CheckBox chkFlags 
         Caption         =   "MF_ENEMY"
         Height          =   255
         Index           =   23
         Left            =   2040
         TabIndex        =   69
         Tag             =   "16777216"
         ToolTipText     =   "This mobj is an enemy!"
         Top             =   2640
         Width           =   1335
      End
      Begin VB.CheckBox chkFlags 
         Caption         =   "MF_COUNTITEM"
         Height          =   255
         Index           =   22
         Left            =   2040
         TabIndex        =   68
         Tag             =   "8388608"
         ToolTipText     =   "On picking up, count this item object towards intermission item total."
         Top             =   2400
         Width           =   1695
      End
      Begin VB.CheckBox chkFlags 
         Caption         =   "MF_NOTHINK"
         Height          =   255
         Index           =   21
         Left            =   2040
         TabIndex        =   67
         Tag             =   "2097152"
         ToolTipText     =   "Don't run this thing's thinker."
         Top             =   1920
         Width           =   1695
      End
      Begin VB.CheckBox chkFlags 
         Caption         =   "MF_MONITOR"
         Height          =   255
         Index           =   20
         Left            =   2040
         TabIndex        =   66
         Tag             =   "1048576"
         ToolTipText     =   "Item box"
         Top             =   1680
         Width           =   1575
      End
      Begin VB.CheckBox chkFlags 
         Caption         =   "MF_HIRES"
         Height          =   255
         Index           =   19
         Left            =   2040
         TabIndex        =   65
         Tag             =   "524288"
         ToolTipText     =   "Object uses a high-resolution sprite"
         Top             =   1440
         Width           =   1215
      End
      Begin VB.CheckBox chkFlags 
         Caption         =   "MF_BOUNCE"
         Height          =   255
         Index           =   18
         Left            =   2040
         TabIndex        =   64
         Tag             =   "262144"
         ToolTipText     =   "Bounce off walls and things."
         Top             =   1200
         Width           =   1335
      End
      Begin VB.CheckBox chkFlags 
         Caption         =   "MF_SPRING"
         Height          =   255
         Index           =   17
         Left            =   2040
         TabIndex        =   63
         Tag             =   "131072"
         ToolTipText     =   "Item is a spring."
         Top             =   960
         Width           =   1575
      End
      Begin VB.CheckBox chkFlags 
         Caption         =   "MF_MISSILE"
         Height          =   255
         Index           =   16
         Left            =   2040
         TabIndex        =   62
         Tag             =   "65536"
         ToolTipText     =   "Any kind of projectile currently flying through the air, waiting to hit something"
         Top             =   720
         Width           =   1335
      End
      Begin VB.CheckBox chkFlags 
         Caption         =   "MF_BOXICON"
         Height          =   255
         Index           =   15
         Left            =   2040
         TabIndex        =   61
         Tag             =   "32768"
         ToolTipText     =   "Monitor powerup icon. These rise a bit."
         Top             =   480
         Width           =   1815
      End
      Begin VB.CheckBox chkFlags 
         Caption         =   "MF_FLOAT"
         Height          =   255
         Index           =   14
         Left            =   2040
         TabIndex        =   60
         Tag             =   "16384"
         ToolTipText     =   "Allow moves to any height, no gravity. For active floaters."
         Top             =   240
         Width           =   1215
      End
      Begin VB.CheckBox chkFlags 
         Caption         =   "MF_SPECIALFLAGS"
         Height          =   255
         Index           =   13
         Left            =   120
         TabIndex        =   59
         Tag             =   "8192"
         ToolTipText     =   "This object does not adhere to regular flag/z properties for object placing."
         Top             =   3360
         Width           =   1815
      End
      Begin VB.CheckBox chkFlags 
         Caption         =   "MF_NOCLIP"
         Height          =   255
         Index           =   12
         Left            =   120
         TabIndex        =   58
         Tag             =   "4096"
         ToolTipText     =   "Don't clip against objects, walls, etc."
         Top             =   3120
         Width           =   1335
      End
      Begin VB.CheckBox chkFlags 
         Caption         =   "MF_SLIDEME"
         Height          =   255
         Index           =   11
         Left            =   120
         TabIndex        =   57
         Tag             =   "2048"
         ToolTipText     =   "Slide this object when it hits a wall."
         Top             =   2880
         Width           =   1335
      End
      Begin VB.CheckBox chkFlags 
         Caption         =   "MF_AMBIENT"
         Height          =   255
         Index           =   10
         Left            =   120
         TabIndex        =   56
         Tag             =   "1024"
         ToolTipText     =   "This object is an ambient sound."
         Top             =   2640
         Width           =   1695
      End
      Begin VB.CheckBox chkFlags 
         Caption         =   "MF_NOGRAVITY"
         Height          =   255
         Index           =   9
         Left            =   120
         TabIndex        =   55
         Tag             =   "512"
         ToolTipText     =   "Don't apply gravity"
         Top             =   2400
         Width           =   1695
      End
      Begin VB.CheckBox chkFlags 
         Caption         =   "MF_SPAWNCEILING"
         Height          =   255
         Index           =   8
         Left            =   120
         TabIndex        =   54
         Tag             =   "256"
         ToolTipText     =   "On level spawning (initial position), hang from ceiling instead of stand on floor."
         Top             =   2160
         Width           =   1935
      End
      Begin VB.CheckBox chkFlags 
         Caption         =   "MF_BOSS"
         Height          =   255
         Index           =   7
         Left            =   120
         TabIndex        =   53
         Tag             =   "128"
         ToolTipText     =   "Object is a boss."
         Top             =   1920
         Width           =   1575
      End
      Begin VB.CheckBox chkFlags 
         Caption         =   "MF_PUSHABLE"
         Height          =   255
         Index           =   6
         Left            =   120
         TabIndex        =   52
         Tag             =   "64"
         ToolTipText     =   "You can push this object. It can activate switches and things by pushing it on top."
         Top             =   1680
         Width           =   1575
      End
      Begin VB.CheckBox chkFlags 
         Caption         =   "MF_AMBUSH"
         Height          =   255
         Index           =   5
         Left            =   120
         TabIndex        =   51
         Tag             =   "32"
         ToolTipText     =   "Special attributes"
         Top             =   1440
         Width           =   1455
      End
      Begin VB.CheckBox chkFlags 
         Caption         =   "MF_NOBLOCKMAP"
         Height          =   255
         Index           =   4
         Left            =   120
         TabIndex        =   50
         Tag             =   "16"
         ToolTipText     =   "Don't use the blocklinks (inert but displayable)"
         Top             =   1200
         Width           =   1815
      End
      Begin VB.CheckBox chkFlags 
         Caption         =   "MF_NOSECTOR"
         Height          =   255
         Index           =   3
         Left            =   120
         TabIndex        =   49
         Tag             =   "8"
         ToolTipText     =   "Don't use the sector links (invisible but touchable)."
         Top             =   960
         Width           =   1575
      End
      Begin VB.CheckBox chkFlags 
         Caption         =   "MF_SHOOTABLE"
         Height          =   255
         Index           =   2
         Left            =   120
         TabIndex        =   48
         Tag             =   "4"
         ToolTipText     =   "Can be hit."
         Top             =   720
         Width           =   1695
      End
      Begin VB.CheckBox chkFlags 
         Caption         =   "MF_SOLID"
         Height          =   255
         Index           =   1
         Left            =   120
         TabIndex        =   47
         Tag             =   "2"
         ToolTipText     =   "Blocks."
         Top             =   480
         Width           =   1335
      End
      Begin VB.CheckBox chkFlags 
         Caption         =   "MF_SPECIAL"
         Height          =   255
         Index           =   0
         Left            =   120
         TabIndex        =   46
         Tag             =   "1"
         ToolTipText     =   "Call P_TouchSpecialThing when touched."
         Top             =   240
         Width           =   1455
      End
   End
   Begin VB.ComboBox cmbRaisestate 
      Height          =   315
      Left            =   4320
      TabIndex        =   43
      Text            =   "cmbRaisestate"
      Top             =   4440
      Width           =   3300
   End
   Begin VB.ComboBox cmbActivesound 
      Height          =   315
      Left            =   4320
      TabIndex        =   41
      Text            =   "cmbActivesound"
      Top             =   4080
      Width           =   3300
   End
   Begin VB.TextBox txtDamage 
      Height          =   285
      Left            =   10680
      TabIndex        =   39
      Text            =   "0"
      Top             =   1200
      Width           =   1095
   End
   Begin VB.TextBox txtMass 
      Height          =   285
      Left            =   10680
      TabIndex        =   37
      Text            =   "0"
      Top             =   840
      Width           =   1095
   End
   Begin VB.TextBox txtHeight 
      Height          =   285
      Left            =   10680
      TabIndex        =   35
      Text            =   "0"
      Top             =   480
      Width           =   1095
   End
   Begin VB.TextBox txtRadius 
      Height          =   285
      Left            =   10680
      TabIndex        =   33
      Text            =   "0"
      Top             =   120
      Width           =   1095
   End
   Begin VB.TextBox txtSpeed 
      Height          =   285
      Left            =   8760
      TabIndex        =   31
      Text            =   "0"
      Top             =   1560
      Width           =   1095
   End
   Begin VB.ComboBox cmbDeathsound 
      Height          =   315
      Left            =   4320
      TabIndex        =   29
      Text            =   "cmbDeathsound"
      Top             =   3720
      Width           =   3300
   End
   Begin VB.ComboBox cmbXdeathstate 
      Height          =   315
      Left            =   4320
      TabIndex        =   27
      Text            =   "cmbXdeathstate"
      Top             =   3360
      Width           =   3300
   End
   Begin VB.ComboBox cmbDeathstate 
      Height          =   315
      Left            =   4320
      TabIndex        =   25
      Text            =   "cmbDeathstate"
      Top             =   3000
      Width           =   3300
   End
   Begin VB.ComboBox cmbMissilestate 
      Height          =   315
      Left            =   4320
      TabIndex        =   23
      Text            =   "cmbMissilestate"
      Top             =   2640
      Width           =   3300
   End
   Begin VB.ComboBox cmbMeleestate 
      Height          =   315
      Left            =   4320
      TabIndex        =   21
      Text            =   "cmbMeleestate"
      Top             =   2280
      Width           =   3300
   End
   Begin VB.ComboBox cmbPainsound 
      Height          =   315
      Left            =   4320
      TabIndex        =   19
      Text            =   "cmbPainsound"
      Top             =   1920
      Width           =   3300
   End
   Begin VB.TextBox txtPainchance 
      Height          =   285
      Left            =   8760
      TabIndex        =   17
      Text            =   "0"
      Top             =   1200
      Width           =   1095
   End
   Begin VB.ComboBox cmbPainstate 
      Height          =   315
      Left            =   4320
      TabIndex        =   15
      Text            =   "cmbPainstate"
      Top             =   1560
      Width           =   3300
   End
   Begin VB.ComboBox cmbAttacksound 
      Height          =   315
      Left            =   4320
      TabIndex        =   13
      Text            =   "cmbAttacksound"
      Top             =   1200
      Width           =   3300
   End
   Begin VB.TextBox txtReactiontime 
      Height          =   285
      Left            =   8760
      TabIndex        =   11
      Text            =   "0"
      Top             =   840
      Width           =   1095
   End
   Begin VB.ComboBox cmbSeesound 
      Height          =   315
      Left            =   4320
      TabIndex        =   9
      Text            =   "cmbSeesound"
      Top             =   840
      Width           =   3300
   End
   Begin VB.ComboBox cmbSeestate 
      Height          =   315
      Left            =   4320
      TabIndex        =   7
      Text            =   "cmbSeestate"
      Top             =   480
      Width           =   3300
   End
   Begin VB.TextBox txtSpawnhealth 
      Height          =   285
      Left            =   8760
      TabIndex        =   6
      Text            =   "0"
      Top             =   480
      Width           =   1095
   End
   Begin VB.ComboBox cmbSpawnstate 
      Height          =   315
      Left            =   4320
      TabIndex        =   3
      Text            =   "cmbSpawnstate"
      Top             =   120
      Width           =   3300
   End
   Begin VB.TextBox txtDoomednum 
      Height          =   285
      Left            =   8760
      TabIndex        =   1
      Text            =   "0"
      Top             =   120
      Width           =   1095
   End
   Begin VB.ListBox lstThings 
      Height          =   5520
      ItemData        =   "Things.frx":0442
      Left            =   120
      List            =   "Things.frx":0444
      TabIndex        =   0
      Top             =   120
      Width           =   3015
   End
   Begin VB.Label lblStatusInfo 
      Alignment       =   2  'Center
      Caption         =   "Idle"
      BeginProperty Font 
         Name            =   "MS Sans Serif"
         Size            =   8.25
         Charset         =   0
         Weight          =   700
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   375
      Left            =   9960
      TabIndex        =   75
      Top             =   1560
      Width           =   1815
   End
   Begin VB.Label lblRaisestate 
      Alignment       =   1  'Right Justify
      Caption         =   "Raisestate:"
      Height          =   255
      Left            =   3360
      TabIndex        =   44
      Top             =   4440
      Width           =   855
   End
   Begin VB.Label lblActivesound 
      Alignment       =   1  'Right Justify
      Caption         =   "Activesound:"
      Height          =   255
      Left            =   3240
      TabIndex        =   42
      Top             =   4080
      Width           =   975
   End
   Begin VB.Label lblDamage 
      Alignment       =   1  'Right Justify
      Caption         =   "Damage:"
      Height          =   255
      Left            =   9840
      TabIndex        =   40
      Top             =   1200
      Width           =   735
   End
   Begin VB.Label lblMass 
      Alignment       =   1  'Right Justify
      Caption         =   "Mass:"
      Height          =   255
      Left            =   9960
      TabIndex        =   38
      Top             =   840
      Width           =   615
   End
   Begin VB.Label lblHeight 
      Alignment       =   1  'Right Justify
      Caption         =   "Height:"
      Height          =   255
      Left            =   9960
      TabIndex        =   36
      Top             =   480
      Width           =   615
   End
   Begin VB.Label lblRadius 
      Alignment       =   1  'Right Justify
      Caption         =   "Radius:"
      Height          =   255
      Left            =   9960
      TabIndex        =   34
      Top             =   120
      Width           =   615
   End
   Begin VB.Label lblSpeed 
      Alignment       =   1  'Right Justify
      Caption         =   "Speed:"
      Height          =   255
      Left            =   7680
      TabIndex        =   32
      Top             =   1560
      Width           =   975
   End
   Begin VB.Label lblDeathsound 
      Alignment       =   1  'Right Justify
      Caption         =   "Deathsound:"
      Height          =   255
      Left            =   3240
      TabIndex        =   30
      Top             =   3720
      Width           =   975
   End
   Begin VB.Label lblXdeathstate 
      Alignment       =   1  'Right Justify
      Caption         =   "Xdeathstate:"
      Height          =   255
      Left            =   3240
      TabIndex        =   28
      Top             =   3360
      Width           =   975
   End
   Begin VB.Label lblDeathstate 
      Alignment       =   1  'Right Justify
      Caption         =   "Deathstate:"
      Height          =   255
      Left            =   3240
      TabIndex        =   26
      Top             =   3000
      Width           =   975
   End
   Begin VB.Label lblMissilestate 
      Alignment       =   1  'Right Justify
      Caption         =   "Missilestate:"
      Height          =   255
      Left            =   3240
      TabIndex        =   24
      Top             =   2640
      Width           =   975
   End
   Begin VB.Label lblMeleestate 
      Alignment       =   1  'Right Justify
      Caption         =   "Meleestate:"
      Height          =   255
      Left            =   3240
      TabIndex        =   22
      Top             =   2280
      Width           =   975
   End
   Begin VB.Label lblPainsound 
      Alignment       =   1  'Right Justify
      Caption         =   "Painsound:"
      Height          =   255
      Left            =   3240
      TabIndex        =   20
      Top             =   1920
      Width           =   975
   End
   Begin VB.Label lblPainchance 
      Alignment       =   1  'Right Justify
      Caption         =   "Painchance:"
      Height          =   255
      Left            =   7680
      TabIndex        =   18
      Top             =   1200
      Width           =   975
   End
   Begin VB.Label lblPainstate 
      Alignment       =   1  'Right Justify
      Caption         =   "Painstate:"
      Height          =   255
      Left            =   3240
      TabIndex        =   16
      Top             =   1560
      Width           =   975
   End
   Begin VB.Label lblAttacksound 
      Alignment       =   1  'Right Justify
      Caption         =   "Attacksound:"
      Height          =   255
      Left            =   3240
      TabIndex        =   14
      Top             =   1200
      Width           =   975
   End
   Begin VB.Label lblReactiontime 
      Alignment       =   1  'Right Justify
      Caption         =   "Reactiontime:"
      Height          =   255
      Left            =   7680
      TabIndex        =   12
      Top             =   840
      Width           =   975
   End
   Begin VB.Label lblSeesound 
      Alignment       =   1  'Right Justify
      Caption         =   "Seesound:"
      Height          =   255
      Left            =   3240
      TabIndex        =   10
      Top             =   840
      Width           =   975
   End
   Begin VB.Label lblSeestate 
      Alignment       =   1  'Right Justify
      Caption         =   "Seestate:"
      Height          =   255
      Left            =   3240
      TabIndex        =   8
      Top             =   480
      Width           =   975
   End
   Begin VB.Label lblSpawnhealth 
      Alignment       =   1  'Right Justify
      Caption         =   "Spawnhealth:"
      Height          =   255
      Left            =   7680
      TabIndex        =   5
      Top             =   480
      Width           =   975
   End
   Begin VB.Label lblSpawnstate 
      Alignment       =   1  'Right Justify
      Caption         =   "Spawnstate:"
      Height          =   255
      Left            =   3240
      TabIndex        =   4
      Top             =   120
      Width           =   975
   End
   Begin VB.Label lblDoomednum 
      Alignment       =   1  'Right Justify
      Caption         =   "Thing Map #:"
      Height          =   255
      Left            =   7680
      TabIndex        =   2
      Top             =   120
      Width           =   975
   End
End
Attribute VB_Name = "frmThingEdit"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Option Explicit

Private Sub cmdCopy_Click()
    Dim Response As String
    
    Response$ = InputBox("Copy state to #:", "Copy State")
    
    If Response = "" Then Exit Sub
    
    Response = TrimComplete(Response)
    
    Call WriteThing(False, Val(Response))
    
    MsgBox "Thing copied to #" & Val(Response)
End Sub

Private Sub cmdDelete_Click()
    Call WriteThing(True, lstThings.ListIndex)
End Sub

Private Sub cmdLoadDefault_Click()
    Call ClearForm
    If InStr(lstThings.List(lstThings.ListIndex), "MT_FREESLOT") = 0 Then
        LoadObjectInfo (lstThings.ListIndex)
    Else
        MsgBox "Free slots do not have a code default."
    End If
End Sub

Private Sub cmdSave_Click()
    Call WriteThing(False, lstThings.ListIndex)
End Sub

Private Sub Form_Load()
    Call Reload
End Sub

Private Sub ClearForm()
    Dim i As Integer
    cmbSpawnstate.Text = ""
    cmbSeestate.Text = ""
    cmbSeesound.Text = ""
    cmbAttacksound.Text = ""
    cmbPainstate.Text = ""
    cmbPainsound.Text = ""
    cmbMeleestate.Text = ""
    cmbMissilestate.Text = ""
    cmbDeathstate.Text = ""
    cmbXdeathstate.Text = ""
    cmbDeathsound.Text = ""
    cmbActivesound.Text = ""
    cmbRaisestate.Text = ""
    txtDoomednum.Text = ""
    txtSpawnhealth.Text = ""
    txtReactiontime.Text = ""
    txtPainchance.Text = ""
    txtSpeed.Text = ""
    txtRadius.Text = ""
    txtHeight.Text = ""
    txtMass.Text = ""
    txtDamage.Text = ""
    
    For i = 0 To 26
        chkFlags(i).Value = 0
    Next
End Sub

Private Sub Reload()
    lblStatusInfo.Caption = "Loading Sounds Info..."
    DoEvents
    LoadSounds
    lblStatusInfo.Caption = "Loading Things Info..."
    DoEvents
    LoadThings
    lblStatusInfo.Caption = "Loading States Info..."
    DoEvents
    LoadStates
    lblStatusInfo.Caption = "Idle"
    lstThings.ListIndex = 0
End Sub

Private Sub LoadSounds()
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
    
    cmbSeesound.Clear
    cmbAttacksound.Clear
    cmbPainsound.Clear
    cmbDeathsound.Clear
    cmbActivesound.Clear
    
    Do While InStr(line, "sfx_freeslot0") = 0
        startclip = InStr(line, "sfx_")
        If InStr(line, "sfx_") <> 0 Then
            endclip = InStr(line, ",")
            line = Mid(line, startclip, endclip - startclip)
            addstring = number & " - " & line
            cmbSeesound.AddItem addstring
            cmbAttacksound.AddItem addstring
            cmbPainsound.AddItem addstring
            cmbDeathsound.AddItem addstring
            cmbActivesound.AddItem addstring
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
        cmbSeesound.AddItem addstring
        cmbAttacksound.AddItem addstring
        cmbPainsound.AddItem addstring
        cmbDeathsound.AddItem addstring
        cmbActivesound.AddItem addstring
        number = number + 1
    Next
End Sub
Private Sub LoadStates()
    Dim myFSO As New Scripting.FileSystemObject
    Dim ts As TextStream
    Dim line As String
    Dim number As Integer
    Dim startclip As Integer, endclip As Integer
    Dim addstring As String
    Dim i As Integer
    Dim numfreeslots As Integer
    
    ChDir SourcePath
    Set ts = myFSO.OpenTextFile("info.h", ForReading, False)
    
    Do While InStr(ts.ReadLine, "Object states (don't modify this comment!)") = 0
    Loop
    
    ts.SkipLine ' typedef enum
    ts.SkipLine ' {
    
    line = ts.ReadLine
    number = 0
    
    cmbSpawnstate.Clear
    cmbSeestate.Clear
    cmbPainstate.Clear
    cmbMeleestate.Clear
    cmbMissilestate.Clear
    cmbDeathstate.Clear
    cmbXdeathstate.Clear
    cmbRaisestate.Clear
    
    Do While InStr(line, "S_FIRSTFREESLOT") = 0
        startclip = InStr(line, "S_")
        If InStr(line, "S_") <> 0 Then
            endclip = InStr(line, ",")
            line = Mid(line, startclip, endclip - startclip)
            addstring = number & " - " & line
            cmbSpawnstate.AddItem addstring
            cmbSeestate.AddItem addstring
            cmbPainstate.AddItem addstring
            cmbMeleestate.AddItem addstring
            cmbMissilestate.AddItem addstring
            cmbDeathstate.AddItem addstring
            cmbXdeathstate.AddItem addstring
            cmbRaisestate.AddItem addstring
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
        cmbSpawnstate.AddItem addstring
        cmbSeestate.AddItem addstring
        cmbPainstate.AddItem addstring
        cmbMeleestate.AddItem addstring
        cmbMissilestate.AddItem addstring
        cmbDeathstate.AddItem addstring
        cmbXdeathstate.AddItem addstring
        cmbRaisestate.AddItem addstring
        number = number + 1
    Next

    ts.Close
    Set myFSO = Nothing
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

Private Sub LoadObjectInfo(ThingNum As Integer)
    Dim myFSO As New Scripting.FileSystemObject
    Dim ts As TextStream
    Dim line As String
    Dim number As Integer
    Dim startclip As Integer, endclip As Integer
    
    ChDir SourcePath
    Set ts = myFSO.OpenTextFile("info.c", ForReading, False)
    
    Do While InStr(ts.ReadLine, "mobjinfo[NUMMOBJTYPES] =") = 0
    Loop
    
    number = 0
    
    Do While number <> ThingNum
        Do While InStr(ts.ReadLine, "}") = 0
        Loop
        number = number + 1
    Loop
    
    Do While InStr(line, "doomednum") = 0
        line = ts.ReadLine
    Loop
    
    endclip = InStr(line, ",")
    line = Left(line, endclip - 1)
    line = TrimComplete(line)
    'Check for *FRACUNIT values
    endclip = InStr(line, "*FRACUNIT")
    If endclip <> 0 Then
        line = Left(line, endclip - 1)
        line = Val(line) * 65536
    End If
    'Check for crazy-odd MT_ usage
    endclip = InStr(line, "MT_")
    If endclip <> 0 Then
        line = FindThingNum(line) & " - " & line
    End If
    'Check for crazy-odd pw_ usage
    endclip = InStr(line, "pw_")
    If endclip <> 0 Then
        line = FindPowerNum(line) & " - " & line
    End If
    txtDoomednum.Text = line
    
    line = ts.ReadLine
    Do While InStr(line, "spawnstate") = 0
    Loop
    endclip = InStr(line, ",")
    line = Left(line, endclip - 1)
    line = TrimComplete(line)
    Call FindComboIndex(cmbSpawnstate, line)
    'Check for crazy-odd MT_ usage
    endclip = InStr(line, "MT_")
    If endclip <> 0 Then
        number = FindThingNum(line)
        cmbSpawnstate.Text = number & " - " & line
    End If
    'Check for crazy-odd pw_ usage
    endclip = InStr(line, "pw_")
    If endclip <> 0 Then
        number = FindPowerNum(line)
        cmbSpawnstate.Text = number & " - " & line
    End If
    
    line = ts.ReadLine
    Do While InStr(line, "spawnhealth") = 0
    Loop
    endclip = InStr(line, ",")
    line = Left(line, endclip - 1)
    line = TrimComplete(line)
    'Check for *FRACUNIT values
    endclip = InStr(line, "*FRACUNIT")
    If endclip <> 0 Then
        line = Left(line, endclip - 1)
        line = Val(line) * 65536
    End If
    'Check for crazy-odd MT_ usage
    endclip = InStr(line, "MT_")
    If endclip <> 0 Then
        line = FindThingNum(line) & " - " & line
    End If
    'Check for crazy-odd pw_ usage
    endclip = InStr(line, "pw_")
    If endclip <> 0 Then
        line = FindPowerNum(line) & " - " & line
    End If
    txtSpawnhealth.Text = line
    
    line = ts.ReadLine
    Do While InStr(line, "seestate") = 0
    Loop
    endclip = InStr(line, ",")
    line = Left(line, endclip - 1)
    line = TrimComplete(line)
    Call FindComboIndex(cmbSeestate, line)
    'Check for crazy-odd MT_ usage
    endclip = InStr(line, "MT_")
    If endclip <> 0 Then
        number = FindThingNum(line)
        cmbSeestate.Text = number & " - " & line
    End If
    'Check for crazy-odd pw_ usage
    endclip = InStr(line, "pw_")
    If endclip <> 0 Then
        number = FindPowerNum(line)
        cmbSeestate.Text = number & " - " & line
    End If
    
    line = ts.ReadLine
    Do While InStr(line, "seesound") = 0
    Loop
    endclip = InStr(line, ",")
    line = Left(line, endclip - 1)
    line = TrimComplete(line)
    Call FindComboIndex(cmbSeesound, line)
    'Check for crazy-odd MT_ usage
    endclip = InStr(line, "MT_")
    If endclip <> 0 Then
        number = FindThingNum(line)
        cmbSeesound.Text = number & " - " & line
    End If
    'Check for crazy-odd pw_ usage
    endclip = InStr(line, "pw_")
    If endclip <> 0 Then
        number = FindPowerNum(line)
        cmbSeesound.Text = number & " - " & line
    End If
    
    line = ts.ReadLine
    Do While InStr(line, "reactiontime") = 0
    Loop
    endclip = InStr(line, ",")
    line = Left(line, endclip - 1)
    line = TrimComplete(line)
    'Check for *FRACUNIT values
    endclip = InStr(line, "*FRACUNIT")
    If endclip <> 0 Then
        line = Left(line, endclip - 1)
        line = Val(line) * 65536
    End If
    'Check for crazy-odd MT_ usage
    endclip = InStr(line, "MT_")
    If endclip <> 0 Then
        line = FindThingNum(line) & " - " & line
    End If
    'Check for crazy-odd pw_ usage
    endclip = InStr(line, "pw_")
    If endclip <> 0 Then
        line = FindPowerNum(line) & " - " & line
    End If
    txtReactiontime.Text = line
    
    line = ts.ReadLine
    Do While InStr(line, "attacksound") = 0
    Loop
    endclip = InStr(line, ",")
    line = Left(line, endclip - 1)
    line = TrimComplete(line)
    Call FindComboIndex(cmbAttacksound, line)
    'Check for crazy-odd MT_ usage
    endclip = InStr(line, "MT_")
    If endclip <> 0 Then
        number = FindThingNum(line)
        cmbAttacksound.Text = number & " - " & line
    End If
    'Check for crazy-odd pw_ usage
    endclip = InStr(line, "pw_")
    If endclip <> 0 Then
        number = FindPowerNum(line)
        cmbAttacksound.Text = number & " - " & line
    End If
    
    line = ts.ReadLine
    Do While InStr(line, "painstate") = 0
    Loop
    endclip = InStr(line, ",")
    line = Left(line, endclip - 1)
    line = TrimComplete(line)
    Call FindComboIndex(cmbPainstate, line)
    'Check for crazy-odd MT_ usage
    endclip = InStr(line, "MT_")
    If endclip <> 0 Then
        number = FindThingNum(line)
        cmbPainstate.Text = number & " - " & line
    End If
    'Check for crazy-odd pw_ usage
    endclip = InStr(line, "pw_")
    If endclip <> 0 Then
        number = FindPowerNum(line)
        cmbPainstate.Text = number & " - " & line
    End If
    
    line = ts.ReadLine
    Do While InStr(line, "painchance") = 0
    Loop
    endclip = InStr(line, ",")
    line = Left(line, endclip - 1)
    line = TrimComplete(line)
    'Check for *FRACUNIT values
    endclip = InStr(line, "*FRACUNIT")
    If endclip <> 0 Then
        line = Left(line, endclip - 1)
        line = Val(line) * 65536
    End If
    'Check for crazy-odd MT_ usage
    endclip = InStr(line, "MT_")
    If endclip <> 0 Then
        line = FindThingNum(line) & " - " & line
    End If
    'Check for crazy-odd pw_ usage
    endclip = InStr(line, "pw_")
    If endclip <> 0 Then
        line = FindPowerNum(line) & " - " & line
    End If
    txtPainchance.Text = line
    
    line = ts.ReadLine
    Do While InStr(line, "painsound") = 0
    Loop
    endclip = InStr(line, ",")
    line = Left(line, endclip - 1)
    line = TrimComplete(line)
    Call FindComboIndex(cmbPainsound, line)
    'Check for crazy-odd MT_ usage
    endclip = InStr(line, "MT_")
    If endclip <> 0 Then
        number = FindThingNum(line)
        cmbPainsound.Text = number & " - " & line
    End If
    'Check for crazy-odd pw_ usage
    endclip = InStr(line, "pw_")
    If endclip <> 0 Then
        number = FindPowerNum(line)
        cmbPainsound.Text = number & " - " & line
    End If
    
    line = ts.ReadLine
    Do While InStr(line, "meleestate") = 0
    Loop
    endclip = InStr(line, ",")
    line = Left(line, endclip - 1)
    line = TrimComplete(line)
    Call FindComboIndex(cmbMeleestate, line)
    'Check for crazy-odd MT_ usage
    endclip = InStr(line, "MT_")
    If endclip <> 0 Then
        number = FindThingNum(line)
        cmbMeleestate.Text = number & " - " & line
    End If
    'Check for crazy-odd pw_ usage
    endclip = InStr(line, "pw_")
    If endclip <> 0 Then
        number = FindPowerNum(line)
        cmbMeleestate.Text = number & " - " & line
    End If
    
    line = ts.ReadLine
    Do While InStr(line, "missilestate") = 0
    Loop
    endclip = InStr(line, ",")
    line = Left(line, endclip - 1)
    line = TrimComplete(line)
    Call FindComboIndex(cmbMissilestate, line)
    'Check for crazy-odd MT_ usage
    endclip = InStr(line, "MT_")
    If endclip <> 0 Then
        number = FindThingNum(line)
        cmbMissilestate.Text = number & " - " & line
    End If
    'Check for crazy-odd pw_ usage
    endclip = InStr(line, "pw_")
    If endclip <> 0 Then
        number = FindPowerNum(line)
        cmbMissilestate.Text = number & " - " & line
    End If
    
    line = ts.ReadLine
    Do While InStr(line, "deathstate") = 0
    Loop
    endclip = InStr(line, ",")
    line = Left(line, endclip - 1)
    line = TrimComplete(line)
    Call FindComboIndex(cmbDeathstate, line)
    'Check for crazy-odd MT_ usage
    endclip = InStr(line, "MT_")
    If endclip <> 0 Then
        number = FindThingNum(line)
        cmbDeathstate.Text = number & " - " & line
    End If
    'Check for crazy-odd pw_ usage
    endclip = InStr(line, "pw_")
    If endclip <> 0 Then
        number = FindPowerNum(line)
        cmbDeathstate.Text = number & " - " & line
    End If
    
    line = ts.ReadLine
    Do While InStr(line, "xdeathstate") = 0
    Loop
    endclip = InStr(line, ",")
    line = Left(line, endclip - 1)
    line = TrimComplete(line)
    Call FindComboIndex(cmbXdeathstate, line)
    'Check for crazy-odd MT_ usage
    endclip = InStr(line, "MT_")
    If endclip <> 0 Then
        number = FindThingNum(line)
        cmbXdeathstate.Text = number & " - " & line
    End If
    'Check for crazy-odd pw_ usage
    endclip = InStr(line, "pw_")
    If endclip <> 0 Then
        number = FindPowerNum(line)
        cmbXdeathstate.Text = number & " - " & line
    End If
    
    line = ts.ReadLine
    Do While InStr(line, "deathsound") = 0
    Loop
    endclip = InStr(line, ",")
    line = Left(line, endclip - 1)
    line = TrimComplete(line)
    Call FindComboIndex(cmbDeathsound, line)
    'Check for crazy-odd MT_ usage
    endclip = InStr(line, "MT_")
    If endclip <> 0 Then
        number = FindThingNum(line)
        cmbDeathsound.Text = number & " - " & line
    End If
    'Check for crazy-odd pw_ usage
    endclip = InStr(line, "pw_")
    If endclip <> 0 Then
        number = FindPowerNum(line)
        cmbDeathsound.Text = number & " - " & line
    End If
    
    
    line = ts.ReadLine
    Do While InStr(line, "speed") = 0
    Loop
    endclip = InStr(line, ",")
    line = Left(line, endclip - 1)
    line = TrimComplete(line)
    'Check for *FRACUNIT values
    endclip = InStr(line, "*FRACUNIT")
    If endclip <> 0 Then
        line = Left(line, endclip - 1)
        line = Val(line) * 65536
    End If
    'Check for crazy-odd MT_ usage
    endclip = InStr(line, "MT_")
    If endclip <> 0 Then
        line = FindThingNum(line) & " - " & line
    End If
    'Check for crazy-odd pw_ usage
    endclip = InStr(line, "pw_")
    If endclip <> 0 Then
        line = FindPowerNum(line) & " - " & line
    End If
    txtSpeed.Text = line
    
    line = ts.ReadLine
    Do While InStr(line, "radius") = 0
    Loop
    endclip = InStr(line, ",")
    line = Left(line, endclip - 1)
    line = TrimComplete(line)
    'Check for *FRACUNIT values
    endclip = InStr(line, "*FRACUNIT")
    If endclip <> 0 Then
        line = Left(line, endclip - 1)
        line = Val(line) * 65536
    End If
    'Check for crazy-odd MT_ usage
    endclip = InStr(line, "MT_")
    If endclip <> 0 Then
        line = FindThingNum(line) & " - " & line
    End If
    'Check for crazy-odd pw_ usage
    endclip = InStr(line, "pw_")
    If endclip <> 0 Then
        line = FindPowerNum(line) & " - " & line
    End If
    txtRadius.Text = line
    
    line = ts.ReadLine
    Do While InStr(line, "height") = 0
    Loop
    endclip = InStr(line, ",")
    line = Left(line, endclip - 1)
    line = TrimComplete(line)
    'Check for *FRACUNIT values
    endclip = InStr(line, "*FRACUNIT")
    If endclip <> 0 Then
        line = Left(line, endclip - 1)
        line = Val(line) * 65536
    End If
    'Check for crazy-odd MT_ usage
    endclip = InStr(line, "MT_")
    If endclip <> 0 Then
        line = FindThingNum(line) & " - " & line
    End If
    'Check for crazy-odd pw_ usage
    endclip = InStr(line, "pw_")
    If endclip <> 0 Then
        line = FindPowerNum(line) & " - " & line
    End If
    txtHeight.Text = line

    line = ts.ReadLine 'Display order offset (add support, please!)

    line = ts.ReadLine
    Do While InStr(line, "mass") = 0
    Loop
    endclip = InStr(line, ",")
    line = Left(line, endclip - 1)
    line = TrimComplete(line)
    'Check for *FRACUNIT values
    endclip = InStr(line, "*FRACUNIT")
    If endclip <> 0 Then
        line = Left(line, endclip - 1)
        line = Val(line) * 65536
    End If
    'Check for crazy-odd MT_ usage
    endclip = InStr(line, "MT_")
    If endclip <> 0 Then
        line = FindThingNum(line) & " - " & line
    End If
    'Check for crazy-odd pw_ usage
    endclip = InStr(line, "pw_")
    If endclip <> 0 Then
        line = FindPowerNum(line) & " - " & line
    End If
    txtMass.Text = line
    
    line = ts.ReadLine
    Do While InStr(line, "damage") = 0
    Loop
    endclip = InStr(line, ",")
    line = Left(line, endclip - 1)
    line = TrimComplete(line)
    'Check for *FRACUNIT values
    endclip = InStr(line, "*FRACUNIT")
    If endclip <> 0 Then
        line = Left(line, endclip - 1)
        line = Val(line) * 65536
    End If
    'Check for crazy-odd MT_ usage
    endclip = InStr(line, "MT_")
    If endclip <> 0 Then
        line = FindThingNum(line) & " - " & line
    End If
    'Check for crazy-odd pw_ usage
    endclip = InStr(line, "pw_")
    If endclip <> 0 Then
        line = FindPowerNum(line) & " - " & line
    End If
    txtDamage.Text = line
    
    line = ts.ReadLine
    Do While InStr(line, "activesound") = 0
    Loop
    endclip = InStr(line, ",")
    line = Left(line, endclip - 1)
    line = TrimComplete(line)
    Call FindComboIndex(cmbActivesound, line)
    'Check for crazy-odd MT_ usage
    endclip = InStr(line, "MT_")
    If endclip <> 0 Then
        number = FindThingNum(line)
        cmbActivesound.Text = number & " - " & line
    End If
    'Check for crazy-odd pw_ usage
    endclip = InStr(line, "pw_")
    If endclip <> 0 Then
        number = FindPowerNum(line)
        cmbActivesound.Text = number & " - " & line
    End If
    
    line = ts.ReadLine
    Do While InStr(line, "flags") = 0
    Loop
    endclip = InStr(line, ",")
    line = Left(line, endclip - 1)
    line = TrimComplete(line)
    ProcessFlags (line)
    
    line = ts.ReadLine
    Do While InStr(line, "raisestate") = 0
    Loop
    endclip = InStr(line, "//")
    line = Left(line, endclip - 1)
    line = TrimComplete(line)
    Call FindComboIndex(cmbRaisestate, line)
    'Check for crazy-odd MT_ usage
    endclip = InStr(line, "MT_")
    If endclip <> 0 Then
        number = FindThingNum(line)
        cmbRaisestate.Text = number & " - " & line
    End If
    'Check for crazy-odd pw_ usage
    endclip = InStr(line, "pw_")
    If endclip <> 0 Then
        number = FindPowerNum(line)
        cmbRaisestate.Text = number & " - " & line
    End If
        
    ts.Close
    Set myFSO = Nothing
End Sub

Private Sub ProcessFlags(flags As String)
    Dim FlagList(32) As String
    Dim endpoint As Integer
    Dim ListCount As Integer
    Dim FlagString As String
    Dim myFSO As New Scripting.FileSystemObject
    Dim ts As TextStream
    Dim line As String
    Dim j As Integer, i As Integer
    Dim number As Long
    Dim startclip As Integer, endclip As Integer
    
    For j = 0 To 26
        chkFlags(j).Value = 0
    Next j
    
    FlagString = flags
    
    flags = flags & "||"
    
    ListCount = 0
    
    Do While Len(flags) > 3
        endpoint = InStr(flags, "|")
        FlagString = Left(flags, endpoint - 1)
        flags = Right(flags, Len(flags) - endpoint)
        FlagList(ListCount) = FlagString
        ListCount = ListCount + 1
    Loop
    
    ChDir SourcePath
    
    For i = 0 To ListCount - 1
        Set ts = myFSO.OpenTextFile("p_mobj.h", ForReading, False)
        
        line = ts.ReadLine
        
        Do While Not ts.AtEndOfStream
            line = ts.ReadLine
            If InStr(line, FlagList(i)) Then
                If InStr(line, "//") = 0 Or (InStr(line, "//") > InStr(line, FlagList(i))) Then
                    Exit Do
                End If
            End If
        Loop
        
        If InStr(line, FlagList(i)) Then
            startclip = InStr(line, "0x")
            endclip = InStr(line, ",")
            line = Mid(line, startclip + 2, endclip - 1)
            line = "&H" & line
            TrimComplete (line)
            line = Left(line, Len(line) - 1)
            number = CLng(line)

            For j = 0 To 26
                If chkFlags(j).Tag = number Then
                    chkFlags(j).Value = 1
                End If
            Next j
        End If
        ts.Close
    Next i
    
    Set myFSO = Nothing
End Sub

Private Sub FindComboIndex(ByRef Box As ComboBox, line As String)
    Dim i As Integer
    
    For i = 0 To Box.ListCount
        If InStr(Box.List(i), line) Then
            Box.ListIndex = i
            Exit For
        End If
    Next
End Sub

Private Sub lstThings_Click()
    lblStatusInfo.Caption = "Loading thing info..."
    DoEvents
    Call ClearForm
    If InStr(lstThings.List(lstThings.ListIndex), "MT_FREESLOT") = 0 Then
        LoadObjectInfo (lstThings.ListIndex)
    End If
    LoadSOCObjectInfo (lstThings.ListIndex)
    lblStatusInfo.Caption = "Idle"
End Sub

Private Sub LoadSOCObjectInfo(ThingNum As Integer)
    Dim myFSO As New Scripting.FileSystemObject
    Dim ts As TextStream
    Dim line As String
    Dim word As String
    Dim word2 As String
    Dim j As Integer
    Dim temp As Long
    
    Set ts = myFSO.OpenTextFile(SOCFile, ForReading, False)
    
SOCLoad:
    Do While Not ts.AtEndOfStream
        line = ts.ReadLine
        
        If Left(line, 1) = "#" Then GoTo SOCLoad
        
        If Left(line, 1) = vbCrLf Then GoTo SOCLoad
        
        If Len(line) < 1 Then GoTo SOCLoad
        
        word = FirstToken(line)
        word2 = SecondToken(line)
        
        If UCase(word) = "THING" And Val(word2) = ThingNum Then
            Do While Len(line) > 0 And Not ts.AtEndOfStream
                line = ts.ReadLine
                word = UCase(FirstToken(line))
                word2 = UCase(SecondTokenEqual(line))
                    
                If word = "MAPTHINGNUM" Then
                    txtDoomednum.Text = Val(word2)
                ElseIf word = "SPAWNSTATE" Then
                    cmbSpawnstate.ListIndex = Val(word2)
                ElseIf word = "SPAWNHEALTH" Then
                    txtSpawnhealth.Text = Val(word2)
                ElseIf word = "SEESTATE" Then
                    cmbSeestate.ListIndex = Val(word2)
                ElseIf word = "SEESOUND" Then
                    cmbSeesound.ListIndex = Val(word2)
                ElseIf word = "REACTIONTIME" Then
                    txtReactiontime.Text = Val(word2)
                ElseIf word = "ATTACKSOUND" Then
                    cmbAttacksound.ListIndex = Val(word2)
                ElseIf word = "PAINSTATE" Then
                    cmbPainstate.ListIndex = Val(word2)
                ElseIf word = "PAINCHANCE" Then
                    txtPainchance.Text = Val(word2)
                ElseIf word = "PAINSOUND" Then
                    cmbPainsound.ListIndex = Val(word2)
                ElseIf word = "MELEESTATE" Then
                    cmbMeleestate.ListIndex = Val(word2)
                ElseIf word = "MISSILESTATE" Then
                    cmbMissilestate.ListIndex = Val(word2)
                ElseIf word = "DEATHSTATE" Then
                    cmbDeathstate.ListIndex = Val(word2)
                ElseIf word = "DEATHSOUND" Then
                    cmbDeathsound.ListIndex = Val(word2)
                ElseIf word = "XDEATHSTATE" Then
                    cmbXdeathstate.ListIndex = Val(word2)
                ElseIf word = "SPEED" Then
                    txtSpeed.Text = Val(word2)
                ElseIf word = "RADIUS" Then
                    txtRadius.Text = Val(word2)
                ElseIf word = "HEIGHT" Then
                    txtHeight.Text = Val(word2)
                ElseIf word = "MASS" Then
                    txtMass.Text = Val(word2)
                ElseIf word = "DAMAGE" Then
                    txtDamage.Text = Val(word2)
                ElseIf word = "ACTIVESOUND" Then
                    cmbActivesound.ListIndex = Val(word2)
                ElseIf word = "FLAGS" Then
                    For j = 0 To 26
                        temp = Val(word2)
                        If temp And chkFlags(j).Tag Then
                            chkFlags(j).Value = 1
                        Else
                            chkFlags(j).Value = 0
                        End If
                    Next j
                ElseIf word = "RAISESTATE" Then
                    cmbRaisestate.ListIndex = Val(word2)
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

Private Function FindThingNum(ThingName As String) As Integer
    Dim i As Integer
    Dim temp As String
    Dim startpoint As Integer
    Dim endpoint As Integer
    
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

Private Sub WriteThing(Remove As Boolean, num As Integer)
    Dim myFSOSource As New Scripting.FileSystemObject
    Dim tsSource As TextStream
    Dim myFSOTarget As New Scripting.FileSystemObject
    Dim tsTarget As TextStream
    Dim line As String
    Dim word As String
    Dim word2 As String
    Dim flags As Long
    Dim thingfound As Boolean
    Dim i As Integer
    
    thingfound = False
    
    Set tsSource = myFSOSource.OpenTextFile(SOCFile, ForReading, False)
    Set tsTarget = myFSOTarget.OpenTextFile(SOCTemp, ForWriting, True)
    
    Do While Not tsSource.AtEndOfStream
        line = tsSource.ReadLine
        word = UCase(FirstToken(line))
        word2 = UCase(SecondToken(line))

        'If the current thing exists in the SOC, delete it.
        If word = "THING" And Val(word2) = num Then
            thingfound = True
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
        tsTarget.WriteLine "Thing " & num
        txtDoomednum.Text = TrimComplete(txtDoomednum.Text)
        cmbSpawnstate.Text = TrimComplete(cmbSpawnstate.Text)
        txtSpawnhealth.Text = TrimComplete(txtSpawnhealth.Text)
        cmbSeestate.Text = TrimComplete(cmbSeestate.Text)
        cmbSeesound.Text = TrimComplete(cmbSeesound.Text)
        txtReactiontime.Text = TrimComplete(txtReactiontime.Text)
        cmbAttacksound.Text = TrimComplete(cmbAttacksound.Text)
        cmbPainstate.Text = TrimComplete(cmbPainstate.Text)
        txtPainchance.Text = TrimComplete(txtPainchance.Text)
        cmbPainsound.Text = TrimComplete(cmbPainsound.Text)
        cmbMeleestate.Text = TrimComplete(cmbMeleestate.Text)
        cmbMissilestate.Text = TrimComplete(cmbMissilestate.Text)
        cmbDeathstate.Text = TrimComplete(cmbDeathstate.Text)
        cmbDeathsound.Text = TrimComplete(cmbDeathsound.Text)
        cmbXdeathstate.Text = TrimComplete(cmbXdeathstate.Text)
        txtSpeed.Text = TrimComplete(txtSpeed.Text)
        txtRadius.Text = TrimComplete(txtRadius.Text)
        txtHeight.Text = TrimComplete(txtHeight.Text)
        txtMass.Text = TrimComplete(txtMass.Text)
        txtDamage.Text = TrimComplete(txtDamage.Text)
        cmbActivesound.Text = TrimComplete(cmbActivesound.Text)
        cmbRaisestate.Text = TrimComplete(cmbRaisestate.Text)
        flags = 0
        ' Only 31 bits can be used, because VB is stupid.
        For i = 0 To 26
            If chkFlags(i).Value = 1 Then flags = flags + Val(chkFlags(i).Tag)
        Next
        If txtDoomednum.Text <> "" Then tsTarget.WriteLine "MAPTHINGNUM = " & Val(txtDoomednum.Text)
        If cmbSpawnstate.Text <> "" Then tsTarget.WriteLine "SPAWNSTATE = " & Val(cmbSpawnstate.Text)
        If txtSpawnhealth.Text <> "" Then tsTarget.WriteLine "SPAWNHEALTH = " & Val(txtSpawnhealth.Text)
        If cmbSeestate.Text <> "" Then tsTarget.WriteLine "SEESTATE = " & Val(cmbSeestate.Text)
        If cmbSeesound.Text <> "" Then tsTarget.WriteLine "SEESOUND = " & Val(cmbSeesound.Text)
        If txtReactiontime.Text <> "" Then tsTarget.WriteLine "REACTIONTIME = " & Val(txtReactiontime.Text)
        If cmbAttacksound.Text <> "" Then tsTarget.WriteLine "ATTACKSOUND = " & Val(cmbAttacksound.Text)
        If cmbPainstate.Text <> "" Then tsTarget.WriteLine "PAINSTATE = " & Val(cmbPainstate.Text)
        If txtPainchance.Text <> "" Then tsTarget.WriteLine "PAINCHANCE = " & Val(txtPainchance.Text)
        If cmbPainsound.Text <> "" Then tsTarget.WriteLine "PAINSOUND = " & Val(cmbPainsound.Text)
        If cmbMeleestate.Text <> "" Then tsTarget.WriteLine "MELEESTATE = " & Val(cmbMeleestate.Text)
        If cmbMissilestate.Text <> "" Then tsTarget.WriteLine "MISSILESTATE = " & Val(cmbMissilestate.Text)
        If cmbDeathstate.Text <> "" Then tsTarget.WriteLine "DEATHSTATE = " & Val(cmbDeathstate.Text)
        If cmbDeathsound.Text <> "" Then tsTarget.WriteLine "DEATHSOUND = " & Val(cmbDeathsound.Text)
        If cmbXdeathstate.Text <> "" Then tsTarget.WriteLine "XDEATHSTATE = " & Val(cmbXdeathstate.Text)
        If txtSpeed.Text <> "" Then tsTarget.WriteLine "SPEED = " & Val(txtSpeed.Text)
        If txtRadius.Text <> "" Then tsTarget.WriteLine "RADIUS = " & Val(txtRadius.Text)
        If txtHeight.Text <> "" Then tsTarget.WriteLine "HEIGHT = " & Val(txtHeight.Text)
        If txtMass.Text <> "" Then tsTarget.WriteLine "MASS = " & Val(txtMass.Text)
        If txtDamage.Text <> "" Then tsTarget.WriteLine "DAMAGE = " & Val(txtDamage.Text)
        If cmbActivesound.Text <> "" Then tsTarget.WriteLine "ACTIVESOUND = " & Val(cmbActivesound.Text)
        If cmbRaisestate.Text <> "" Then tsTarget.WriteLine "RAISESTATE = " & Val(cmbRaisestate.Text)
        tsTarget.WriteLine "FLAGS = " & flags
    End If
    
    tsTarget.Close
    Set myFSOTarget = Nothing
    
    FileCopy SOCTemp, SOCFile
    
    Kill SOCTemp
    
    If Remove = True Then
        If thingfound = True Then
            MsgBox "Thing removed from SOC."
        Else
            MsgBox "Thing not found in SOC."
        End If
    Else
        MsgBox "Thing Saved."
    End If
End Sub
