Attribute VB_Name = "Module1"
Option Explicit
Public SOCFile As String
Public SOCTemp As String
Public SourcePath As String

Public Function FirstToken(ByVal line As String)
    Dim index As Integer

    index = InStr(line, " ") - 1
    
    If index < 1 Then
        index = Len(line)
    End If
    
    FirstToken = TrimComplete(Left(line, index))
End Function

Public Function SecondToken(ByVal line As String)
    Dim startclip As Integer
    Dim endclip As Integer
    
    startclip = InStr(line, " ")
    
    startclip = startclip + 1
    
    SecondToken = TrimComplete(Mid(line, startclip, Len(line)))
End Function

Public Function SecondTokenEqual(ByVal line As String)
    Dim startclip As Integer
    Dim endclip As Integer
    
    startclip = InStr(line, "=")
    
    startclip = startclip + 2
    
    line = Mid(line, startclip, Len(line))
    
    SecondTokenEqual = TrimComplete(line)
End Function

Public Function TrimComplete(ByVal sValue As String) As String
    Dim sAns As String
    Dim sWkg As String
    Dim sChar As String
    Dim lLen As Long
    Dim lCtr As Long
    
    sAns = sValue
    lLen = Len(sValue)
    
    If lLen > 0 Then
        'Ltrim
        For lCtr = 1 To lLen
            sChar = Mid(sAns, lCtr, 1)
            If Asc(sChar) > 32 Then Exit For
        Next
        
        sAns = Mid(sAns, lCtr)
        lLen = Len(sAns)
        
        'Rtrim
        If lLen > 0 Then
            For lCtr = lLen To 1 Step -1
                sChar = Mid(sAns, lCtr, 1)
                If Asc(sChar) > 32 Then Exit For
            Next
        End If
        sAns = Left$(sAns, lCtr)
    End If
        
    TrimComplete = sAns
End Function

Public Function RTrimComplete(ByVal sValue As String) As String
    Dim sAns As String
    Dim sWkg As String
    Dim sChar As String
    Dim lLen As Long
    Dim lCtr As Long
    
    sAns = sValue
    lLen = Len(sValue)
    
    'Rtrim
    If lLen > 0 Then
        For lCtr = lLen To 1 Step -1
            sChar = Mid(sAns, lCtr, 1)
            If Asc(sChar) > 32 Then Exit For
        Next
    End If
    sAns = Left$(sAns, lCtr)
    
    RTrimComplete = sAns
End Function
