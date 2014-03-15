VERSION 5.00
Begin VB.Form frmHelp 
   BorderStyle     =   3  'Fixed Dialog
   Caption         =   "Getting Started"
   ClientHeight    =   7395
   ClientLeft      =   45
   ClientTop       =   330
   ClientWidth     =   6360
   Icon            =   "frmHelp.frx":0000
   LinkTopic       =   "Form1"
   MaxButton       =   0   'False
   MinButton       =   0   'False
   ScaleHeight     =   7395
   ScaleWidth      =   6360
   ShowInTaskbar   =   0   'False
   StartUpPosition =   1  'CenterOwner
   Begin VB.CommandButton cmdOK 
      Caption         =   "&OK, I know what I'm doing now."
      Height          =   495
      Left            =   1920
      Style           =   1  'Graphical
      TabIndex        =   8
      Top             =   6840
      Width           =   2535
   End
   Begin VB.Line Line9 
      X1              =   120
      X2              =   6120
      Y1              =   4800
      Y2              =   4800
   End
   Begin VB.Line Line7 
      X1              =   120
      X2              =   6120
      Y1              =   5400
      Y2              =   5400
   End
   Begin VB.Label Label12 
      Caption         =   $"frmHelp.frx":0442
      Height          =   615
      Left            =   120
      TabIndex        =   12
      Top             =   4800
      Width           =   6015
   End
   Begin VB.Line Line6 
      X1              =   120
      X2              =   6120
      Y1              =   4200
      Y2              =   4200
   End
   Begin VB.Label Label11 
      Caption         =   $"frmHelp.frx":04F8
      Height          =   615
      Left            =   120
      TabIndex        =   11
      Top             =   4200
      Width           =   6015
   End
   Begin VB.Line Line8 
      X1              =   120
      X2              =   6120
      Y1              =   6360
      Y2              =   6360
   End
   Begin VB.Line Line5 
      X1              =   120
      X2              =   6120
      Y1              =   3720
      Y2              =   3720
   End
   Begin VB.Line Line4 
      X1              =   120
      X2              =   6120
      Y1              =   2880
      Y2              =   2880
   End
   Begin VB.Line Line3 
      X1              =   120
      X2              =   6120
      Y1              =   2400
      Y2              =   2400
   End
   Begin VB.Line Line2 
      X1              =   120
      X2              =   6120
      Y1              =   1800
      Y2              =   1800
   End
   Begin VB.Line Line1 
      X1              =   120
      X2              =   6120
      Y1              =   1200
      Y2              =   1200
   End
   Begin VB.Label Label10 
      Caption         =   $"frmHelp.frx":05EC
      Height          =   495
      Left            =   120
      TabIndex        =   10
      Top             =   3720
      Width           =   6135
   End
   Begin VB.Label Label9 
      Caption         =   $"frmHelp.frx":068F
      Height          =   615
      Left            =   120
      TabIndex        =   9
      Top             =   1200
      Width           =   6135
   End
   Begin VB.Label Label8 
      Caption         =   $"frmHelp.frx":0772
      Height          =   495
      Left            =   120
      TabIndex        =   7
      Top             =   6360
      Width           =   6135
   End
   Begin VB.Label Label7 
      Caption         =   "However, if you have these settings in the SOC you are using, don't worry - the editor will not erase them from your file."
      Height          =   495
      Left            =   120
      TabIndex        =   6
      Top             =   5880
      Width           =   6135
   End
   Begin VB.Label Label6 
      Caption         =   $"frmHelp.frx":0816
      Height          =   495
      Left            =   120
      TabIndex        =   5
      Top             =   5400
      Width           =   6135
   End
   Begin VB.Label Label5 
      Caption         =   $"frmHelp.frx":08A9
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
      Left            =   120
      TabIndex        =   4
      Top             =   2880
      Width           =   6135
   End
   Begin VB.Label Label4 
      Caption         =   $"frmHelp.frx":09B1
      Height          =   495
      Left            =   120
      TabIndex        =   3
      Top             =   2400
      Width           =   6135
   End
   Begin VB.Label Label3 
      Caption         =   $"frmHelp.frx":0A5A
      Height          =   495
      Left            =   120
      TabIndex        =   2
      Top             =   1920
      Width           =   6135
   End
   Begin VB.Label Label2 
      Caption         =   "Finally! A way to easily edit SOC files! I know you're anxious to get started, but here are some things you should know first:"
      BeginProperty Font 
         Name            =   "MS Sans Serif"
         Size            =   9.75
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   495
      Left            =   120
      TabIndex        =   1
      Top             =   600
      Width           =   6135
   End
   Begin VB.Label Label1 
      Caption         =   "How To Use This Program"
      BeginProperty Font 
         Name            =   "MS Sans Serif"
         Size            =   13.5
         Charset         =   0
         Weight          =   700
         Underline       =   -1  'True
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   495
      Left            =   120
      TabIndex        =   0
      Top             =   120
      Width           =   3855
   End
End
Attribute VB_Name = "frmHelp"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Option Explicit

Private Sub cmdOK_Click()
    frmHelp.Hide
End Sub
