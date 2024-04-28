//-----------------------------------------------------------------------------
//
// Copyright (C) 2015 David Hill
//
// See COPYING for license information.
//
//-----------------------------------------------------------------------------
//
// List of all codes.
//
//-----------------------------------------------------------------------------


#ifdef ACSVM_CodeList

ACSVM_CodeList(Nop,          0)
ACSVM_CodeList(Kill,         2)

// Binary operator codes.
#define ACSVM_CodeList_BinaryOpSet(name) \
   ACSVM_CodeList(name,           0) \
   ACSVM_CodeList(name##_GblArr,  1) \
   ACSVM_CodeList(name##_GblReg,  1) \
   ACSVM_CodeList(name##_HubArr,  1) \
   ACSVM_CodeList(name##_HubReg,  1) \
   ACSVM_CodeList(name##_LocArr,  1) \
   ACSVM_CodeList(name##_LocReg,  1) \
   ACSVM_CodeList(name##_ModArr,  1) \
   ACSVM_CodeList(name##_ModReg,  1)
ACSVM_CodeList_BinaryOpSet(AddU)
ACSVM_CodeList_BinaryOpSet(AndU)
ACSVM_CodeList_BinaryOpSet(DivI)
ACSVM_CodeList_BinaryOpSet(ModI)
ACSVM_CodeList_BinaryOpSet(MulU)
ACSVM_CodeList_BinaryOpSet(OrIU)
ACSVM_CodeList_BinaryOpSet(OrXU)
ACSVM_CodeList_BinaryOpSet(ShLU)
ACSVM_CodeList_BinaryOpSet(ShRI)
ACSVM_CodeList_BinaryOpSet(SubU)
#undef ACSVM_CodeList_BinaryOpSet
ACSVM_CodeList(CmpI_GE,      0)
ACSVM_CodeList(CmpI_GT,      0)
ACSVM_CodeList(CmpI_LE,      0)
ACSVM_CodeList(CmpI_LT,      0)
ACSVM_CodeList(CmpU_EQ,      0)
ACSVM_CodeList(CmpU_NE,      0)
ACSVM_CodeList(DivX,         0)
ACSVM_CodeList(LAnd,         0)
ACSVM_CodeList(LOrI,         0)
ACSVM_CodeList(MulX,         0)

// Call codes.
ACSVM_CodeList(Call_Lit,     1)
ACSVM_CodeList(Call_Stk,     0)
ACSVM_CodeList(CallFunc,     2)
ACSVM_CodeList(CallFunc_Lit, 0)
ACSVM_CodeList(CallSpec,     2)
ACSVM_CodeList(CallSpec_Lit, 0)
ACSVM_CodeList(CallSpec_R1,  2)
ACSVM_CodeList(Retn,         0)

// Drop codes.
ACSVM_CodeList(Drop_GblArr,  1)
ACSVM_CodeList(Drop_GblReg,  1)
ACSVM_CodeList(Drop_HubArr,  1)
ACSVM_CodeList(Drop_HubReg,  1)
ACSVM_CodeList(Drop_LocArr,  1)
ACSVM_CodeList(Drop_LocReg,  1)
ACSVM_CodeList(Drop_ModArr,  1)
ACSVM_CodeList(Drop_ModReg,  1)
ACSVM_CodeList(Drop_Nul,     0)
ACSVM_CodeList(Drop_ScrRet,  0)

// Jump codes.
ACSVM_CodeList(Jcnd_Lit,     2)
ACSVM_CodeList(Jcnd_Nil,     1)
ACSVM_CodeList(Jcnd_Tab,     1)
ACSVM_CodeList(Jcnd_Tru,     1)
ACSVM_CodeList(Jump_Lit,     1)
ACSVM_CodeList(Jump_Stk,     0)

// Push codes.
ACSVM_CodeList(Pfun_Lit,     1)
ACSVM_CodeList(Pstr_Stk,     0)
ACSVM_CodeList(Push_GblArr,  1)
ACSVM_CodeList(Push_GblReg,  1)
ACSVM_CodeList(Push_HubArr,  1)
ACSVM_CodeList(Push_HubReg,  1)
ACSVM_CodeList(Push_Lit,     1)
ACSVM_CodeList(Push_LitArr,  0)
ACSVM_CodeList(Push_LocArr,  1)
ACSVM_CodeList(Push_LocReg,  1)
ACSVM_CodeList(Push_ModArr,  1)
ACSVM_CodeList(Push_ModReg,  1)
ACSVM_CodeList(Push_StrArs,  0)

// Script control codes.
ACSVM_CodeList(ScrDelay,     0)
ACSVM_CodeList(ScrDelay_Lit, 1)
ACSVM_CodeList(ScrHalt,      0)
ACSVM_CodeList(ScrRestart,   0)
ACSVM_CodeList(ScrTerm,      0)
ACSVM_CodeList(ScrWaitI,     0)
ACSVM_CodeList(ScrWaitI_Lit, 1)
ACSVM_CodeList(ScrWaitS,     0)
ACSVM_CodeList(ScrWaitS_Lit, 1)

// Stack control codes.
ACSVM_CodeList(Copy,         0)
ACSVM_CodeList(Swap,         0)

// Unary operator codes.
#define ACSVM_CodeList_UnaryOpSet(name) \
   ACSVM_CodeList(name##_GblArr,  1) \
   ACSVM_CodeList(name##_GblReg,  1) \
   ACSVM_CodeList(name##_HubArr,  1) \
   ACSVM_CodeList(name##_HubReg,  1) \
   ACSVM_CodeList(name##_LocArr,  1) \
   ACSVM_CodeList(name##_LocReg,  1) \
   ACSVM_CodeList(name##_ModArr,  1) \
   ACSVM_CodeList(name##_ModReg,  1)
ACSVM_CodeList_UnaryOpSet(DecU)
ACSVM_CodeList_UnaryOpSet(IncU)
#undef ACSVM_CodeList_UnaryOpSet
ACSVM_CodeList(InvU,         0)
ACSVM_CodeList(NegI,         0)
ACSVM_CodeList(NotU,         0)

#undef ACSVM_CodeList
#endif


#ifdef ACSVM_CodeListACS0

ACSVM_CodeListACS0(Nop,            0, "",       Nop,          0, None)
ACSVM_CodeListACS0(ScrTerm,        1, "",       ScrTerm,      0, None)
ACSVM_CodeListACS0(ScrHalt,        2, "",       ScrHalt,      0, None)
ACSVM_CodeListACS0(Push_Lit,       3, "W",      Push_Lit,     0, None)
ACSVM_CodeListACS0(CallSpec_1,     4, "b",      CallSpec,     1, None)
ACSVM_CodeListACS0(CallSpec_2,     5, "b",      CallSpec,     2, None)
ACSVM_CodeListACS0(CallSpec_3,     6, "b",      CallSpec,     3, None)
ACSVM_CodeListACS0(CallSpec_4,     7, "b",      CallSpec,     4, None)
ACSVM_CodeListACS0(CallSpec_5,     8, "b",      CallSpec,     5, None)
ACSVM_CodeListACS0(CallSpec_1L,    9, "bW",     CallSpec_Lit, 0, None)
ACSVM_CodeListACS0(CallSpec_2L,   10, "bWW",    CallSpec_Lit, 0, None)
ACSVM_CodeListACS0(CallSpec_3L,   11, "bWWW",   CallSpec_Lit, 0, None)
ACSVM_CodeListACS0(CallSpec_4L,   12, "bWWWW",  CallSpec_Lit, 0, None)
ACSVM_CodeListACS0(CallSpec_5L,   13, "bWWWWW", CallSpec_Lit, 0, None)
ACSVM_CodeListACS0(AddU,          14, "",       AddU,         2, None)
ACSVM_CodeListACS0(SubU,          15, "",       SubU,         2, None)
ACSVM_CodeListACS0(MulU,          16, "",       MulU,         2, None)
ACSVM_CodeListACS0(DivI,          17, "",       DivI,         2, None)
ACSVM_CodeListACS0(ModI,          18, "",       ModI,         2, None)
ACSVM_CodeListACS0(CmpU_EQ,       19, "",       CmpU_EQ,      2, None)
ACSVM_CodeListACS0(CmpU_NE,       20, "",       CmpU_NE,      2, None)
ACSVM_CodeListACS0(CmpI_LT,       21, "",       CmpI_LT,      2, None)
ACSVM_CodeListACS0(CmpI_GT,       22, "",       CmpI_GT,      2, None)
ACSVM_CodeListACS0(CmpI_LE,       23, "",       CmpI_LE,      2, None)
ACSVM_CodeListACS0(CmpI_GE,       24, "",       CmpI_GE,      2, None)
ACSVM_CodeListACS0(Drop_LocReg,   25, "bL",     Drop_LocReg,  1, None)
ACSVM_CodeListACS0(Drop_ModReg,   26, "bO",     Drop_ModReg,  1, None)
ACSVM_CodeListACS0(Drop_HubReg,   27, "bU",     Drop_HubReg,  1, None)
ACSVM_CodeListACS0(Push_LocReg,   28, "bL",     Push_LocReg,  1, None)
ACSVM_CodeListACS0(Push_ModReg,   29, "bO",     Push_ModReg,  1, None)
ACSVM_CodeListACS0(Push_HubReg,   30, "bU",     Push_HubReg,  1, None)
ACSVM_CodeListACS0(AddU_LocReg,   31, "bL",     AddU_LocReg,  1, None)
ACSVM_CodeListACS0(AddU_ModReg,   32, "bO",     AddU_ModReg,  1, None)
ACSVM_CodeListACS0(AddU_HubReg,   33, "bU",     AddU_HubReg,  1, None)
ACSVM_CodeListACS0(SubU_LocReg,   34, "bL",     SubU_LocReg,  1, None)
ACSVM_CodeListACS0(SubU_ModReg,   35, "bO",     SubU_ModReg,  1, None)
ACSVM_CodeListACS0(SubU_HubReg,   36, "bU",     SubU_HubReg,  1, None)
ACSVM_CodeListACS0(MulU_LocReg,   37, "bL",     MulU_LocReg,  1, None)
ACSVM_CodeListACS0(MulU_ModReg,   38, "bO",     MulU_ModReg,  1, None)
ACSVM_CodeListACS0(MulU_HubReg,   39, "bU",     MulU_HubReg,  1, None)
ACSVM_CodeListACS0(DivI_LocReg,   40, "bL",     DivI_LocReg,  1, None)
ACSVM_CodeListACS0(DivI_ModReg,   41, "bO",     DivI_ModReg,  1, None)
ACSVM_CodeListACS0(DivI_HubReg,   42, "bU",     DivI_HubReg,  1, None)
ACSVM_CodeListACS0(ModI_LocReg,   43, "bL",     ModI_LocReg,  1, None)
ACSVM_CodeListACS0(ModI_ModReg,   44, "bO",     ModI_ModReg,  1, None)
ACSVM_CodeListACS0(ModI_HubReg,   45, "bU",     ModI_HubReg,  1, None)
ACSVM_CodeListACS0(IncU_LocReg,   46, "bL",     IncU_LocReg,  1, None)
ACSVM_CodeListACS0(IncU_ModReg,   47, "bO",     IncU_ModReg,  1, None)
ACSVM_CodeListACS0(IncU_HubReg,   48, "bU",     IncU_HubReg,  1, None)
ACSVM_CodeListACS0(DecU_LocReg,   49, "bL",     DecU_LocReg,  1, None)
ACSVM_CodeListACS0(DecU_ModReg,   50, "bO",     DecU_ModReg,  1, None)
ACSVM_CodeListACS0(DecU_HubReg,   51, "bU",     DecU_HubReg,  1, None)
ACSVM_CodeListACS0(Jump_Lit,      52, "WJ",     Jump_Lit,     0, None)
ACSVM_CodeListACS0(Jcnd_Tru,      53, "WJ",     Jcnd_Tru,     1, None)
ACSVM_CodeListACS0(Drop_Nul,      54, "",       Drop_Nul,     1, None)
ACSVM_CodeListACS0(ScrDelay,      55, "",       ScrDelay,     1, None)
ACSVM_CodeListACS0(ScrDelay_Lit,  56, "W",      ScrDelay_Lit, 0, None)

ACSVM_CodeListACS0(ScrRestart,    69, "",       ScrRestart,   0, None)
ACSVM_CodeListACS0(LAnd,          70, "",       LAnd,         2, None)
ACSVM_CodeListACS0(LOrI,          71, "",       LOrI,         2, None)
ACSVM_CodeListACS0(AndU,          72, "",       AndU,         2, None)
ACSVM_CodeListACS0(OrIU,          73, "",       OrIU,         2, None)
ACSVM_CodeListACS0(OrXU,          74, "",       OrXU,         2, None)
ACSVM_CodeListACS0(NotU,          75, "",       NotU,         1, None)
ACSVM_CodeListACS0(ShLU,          76, "",       ShLU,         2, None)
ACSVM_CodeListACS0(ShRI,          77, "",       ShRI,         2, None)
ACSVM_CodeListACS0(NegI,          78, "",       NegI,         1, None)
ACSVM_CodeListACS0(Jcnd_Nil,      79, "WJ",     Jcnd_Nil,     1, None)

ACSVM_CodeListACS0(ScrWaitI,      81, "",       ScrWaitI,     1, None)
ACSVM_CodeListACS0(ScrWaitI_Lit,  82, "W",      ScrWaitI_Lit, 0, None)

ACSVM_CodeListACS0(Jcnd_Lit,      84, "WWJ",    Jcnd_Lit,     1, None)
ACSVM_CodeListACS0(PrintPush,     85, "",       CallFunc,     0, PrintPush)

ACSVM_CodeListACS0(PrintString,   87, "",       CallFunc,     1, PrintString)
ACSVM_CodeListACS0(PrintIntD,     88, "",       CallFunc,     1, PrintIntD)
ACSVM_CodeListACS0(PrintChar,     89, "",       CallFunc,     1, PrintChar)

ACSVM_CodeListACS0(MulX,         136, "",       MulX,         2, None)
ACSVM_CodeListACS0(DivX,         137, "",       DivX,         2, None)

ACSVM_CodeListACS0(PrintFixD,    157, "",       CallFunc,     1, PrintFixD)

ACSVM_CodeListACS0(Push_LitB,    167, "B",      Push_Lit,     0, None)
ACSVM_CodeListACS0(CallSpec_1LB, 168, "BB",     CallSpec_Lit, 0, None)
ACSVM_CodeListACS0(CallSpec_2LB, 169, "BBB",    CallSpec_Lit, 0, None)
ACSVM_CodeListACS0(CallSpec_3LB, 170, "BBBB",   CallSpec_Lit, 0, None)
ACSVM_CodeListACS0(CallSpec_4LB, 171, "BBBBB",  CallSpec_Lit, 0, None)
ACSVM_CodeListACS0(CallSpec_5LB, 172, "BBBBBB", CallSpec_Lit, 0, None)
ACSVM_CodeListACS0(ScrDelay_LB,  173, "B",      ScrDelay_Lit, 0, None)
ACSVM_CodeListACS0(Push_LitArrB, 175, "",       Push_LitArr,  0, None)
ACSVM_CodeListACS0(Push_Lit2B,   176, "BB",     Push_LitArr,  0, None)
ACSVM_CodeListACS0(Push_Lit3B,   177, "BBB",    Push_LitArr,  0, None)
ACSVM_CodeListACS0(Push_Lit4B,   178, "BBBB",   Push_LitArr,  0, None)
ACSVM_CodeListACS0(Push_Lit5B,   179, "BBBBB",  Push_LitArr,  0, None)

ACSVM_CodeListACS0(Drop_GblReg,  181, "bG",     Drop_GblReg,  1, None)
ACSVM_CodeListACS0(Push_GblReg,  182, "bG",     Push_GblReg,  0, None)
ACSVM_CodeListACS0(AddU_GblReg,  183, "bG",     AddU_GblReg,  1, None)
ACSVM_CodeListACS0(SubU_GblReg,  184, "bG",     SubU_GblReg,  1, None)
ACSVM_CodeListACS0(MulU_GblReg,  185, "bG",     MulU_GblReg,  1, None)
ACSVM_CodeListACS0(DivI_GblReg,  186, "bG",     DivI_GblReg,  1, None)
ACSVM_CodeListACS0(ModI_GblReg,  187, "bG",     ModI_GblReg,  1, None)
ACSVM_CodeListACS0(IncU_GblReg,  188, "bG",     IncU_GblReg,  1, None)
ACSVM_CodeListACS0(DecU_GblReg,  189, "bG",     DecU_GblReg,  1, None)

ACSVM_CodeListACS0(Call_Lit,     203, "b",      Call_Lit,     0, None)
ACSVM_CodeListACS0(Call_Nul,     204, "b",      Call_Lit,     0, None)
ACSVM_CodeListACS0(Retn_Nul,     205, "",       Retn,         0, None)
ACSVM_CodeListACS0(Retn_Stk,     206, "",       Retn,         0, None)
ACSVM_CodeListACS0(Push_ModArr,  207, "bo",     Push_ModArr,  1, None)
ACSVM_CodeListACS0(Drop_ModArr,  208, "bo",     Drop_ModArr,  2, None)
ACSVM_CodeListACS0(AddU_ModArr,  209, "bo",     AddU_ModArr,  2, None)
ACSVM_CodeListACS0(SubU_ModArr,  210, "bo",     SubU_ModArr,  2, None)
ACSVM_CodeListACS0(MulU_ModArr,  211, "bo",     MulU_ModArr,  2, None)
ACSVM_CodeListACS0(DivI_ModArr,  212, "bo",     DivI_ModArr,  2, None)
ACSVM_CodeListACS0(ModI_ModArr,  213, "bo",     ModI_ModArr,  2, None)
ACSVM_CodeListACS0(IncU_ModArr,  214, "bo",     IncU_ModArr,  2, None)
ACSVM_CodeListACS0(DecU_ModArr,  215, "bo",     DecU_ModArr,  2, None)
ACSVM_CodeListACS0(Copy,         216, "",       Copy,         1, None)
ACSVM_CodeListACS0(Swap,         217, "",       Swap,         2, None)

ACSVM_CodeListACS0(Pstr_Stk,     225, "",       Pstr_Stk,     1, None)
ACSVM_CodeListACS0(Push_HubArr,  226, "bu",     Push_HubArr,  1, None)
ACSVM_CodeListACS0(Drop_HubArr,  227, "bu",     Drop_HubArr,  2, None)
ACSVM_CodeListACS0(AddU_HubArr,  228, "bu",     AddU_HubArr,  2, None)
ACSVM_CodeListACS0(SubU_HubArr,  229, "bu",     SubU_HubArr,  2, None)
ACSVM_CodeListACS0(MulU_HubArr,  230, "bu",     MulU_HubArr,  2, None)
ACSVM_CodeListACS0(DivI_HubArr,  231, "bu",     DivI_HubArr,  2, None)
ACSVM_CodeListACS0(ModI_HubArr,  232, "bu",     ModI_HubArr,  2, None)
ACSVM_CodeListACS0(IncU_HubArr,  233, "bu",     IncU_HubArr,  2, None)
ACSVM_CodeListACS0(DecU_HubArr,  234, "bu",     DecU_HubArr,  2, None)
ACSVM_CodeListACS0(Push_GblArr,  235, "bg",     Push_GblArr,  1, None)
ACSVM_CodeListACS0(Drop_GblArr,  236, "bg",     Drop_GblArr,  2, None)
ACSVM_CodeListACS0(AddU_GblArr,  237, "bg",     AddU_GblArr,  2, None)
ACSVM_CodeListACS0(SubU_GblArr,  238, "bg",     SubU_GblArr,  2, None)
ACSVM_CodeListACS0(MulU_GblArr,  239, "bg",     MulU_GblArr,  2, None)
ACSVM_CodeListACS0(DivI_GblArr,  240, "bg",     DivI_GblArr,  2, None)
ACSVM_CodeListACS0(ModI_GblArr,  241, "bg",     ModI_GblArr,  2, None)
ACSVM_CodeListACS0(IncU_GblArr,  242, "bg",     IncU_GblArr,  2, None)
ACSVM_CodeListACS0(DecU_GblArr,  243, "bg",     DecU_GblArr,  2, None)

ACSVM_CodeListACS0(StrLen,       253, "",       CallFunc,     1, StrLen)

ACSVM_CodeListACS0(Jcnd_Tab,     256, "",       Jcnd_Tab,     1, None)
ACSVM_CodeListACS0(Drop_ScrRet,  257, "",       Drop_ScrRet,  1, None)

ACSVM_CodeListACS0(CallSpec_5R1, 263, "b",      CallSpec_R1,  5, None)

ACSVM_CodeListACS0(PrintModArr,  273, "",       CallFunc,     2, PrintModArr)
ACSVM_CodeListACS0(PrintHubArr,  274, "",       CallFunc,     2, PrintHubArr)
ACSVM_CodeListACS0(PrintGblArr,  275, "",       CallFunc,     2, PrintGblArr)

ACSVM_CodeListACS0(AndU_LocReg,  291, "bL",     AndU_LocReg,  1, None)
ACSVM_CodeListACS0(AndU_ModReg,  292, "bO",     AndU_ModReg,  1, None)
ACSVM_CodeListACS0(AndU_HubReg,  293, "bU",     AndU_HubReg,  1, None)
ACSVM_CodeListACS0(AndU_GblReg,  294, "bG",     AndU_GblReg,  1, None)
ACSVM_CodeListACS0(AndU_ModArr,  295, "bo",     AndU_ModArr,  2, None)
ACSVM_CodeListACS0(AndU_HubArr,  296, "bu",     AndU_HubArr,  2, None)
ACSVM_CodeListACS0(AndU_GblArr,  297, "bg",     AndU_GblArr,  2, None)
ACSVM_CodeListACS0(OrXU_LocReg,  298, "bL",     OrXU_LocReg,  1, None)
ACSVM_CodeListACS0(OrXU_ModReg,  299, "bO",     OrXU_ModReg,  1, None)
ACSVM_CodeListACS0(OrXU_HubReg,  300, "bU",     OrXU_HubReg,  1, None)
ACSVM_CodeListACS0(OrXU_GblReg,  301, "bG",     OrXU_GblReg,  1, None)
ACSVM_CodeListACS0(OrXU_ModArr,  302, "bo",     OrXU_ModArr,  2, None)
ACSVM_CodeListACS0(OrXU_HubArr,  303, "bu",     OrXU_HubArr,  2, None)
ACSVM_CodeListACS0(OrXU_GblArr,  304, "bg",     OrXU_GblArr,  2, None)
ACSVM_CodeListACS0(OrIU_LocReg,  305, "bL",     OrIU_LocReg,  1, None)
ACSVM_CodeListACS0(OrIU_ModReg,  306, "bO",     OrIU_ModReg,  1, None)
ACSVM_CodeListACS0(OrIU_HubReg,  307, "bU",     OrIU_HubReg,  1, None)
ACSVM_CodeListACS0(OrIU_GblReg,  308, "bG",     OrIU_GblReg,  1, None)
ACSVM_CodeListACS0(OrIU_ModArr,  309, "bo",     OrIU_ModArr,  2, None)
ACSVM_CodeListACS0(OrIU_HubArr,  310, "bu",     OrIU_HubArr,  2, None)
ACSVM_CodeListACS0(OrIU_GblArr,  311, "bg",     OrIU_GblArr,  2, None)
ACSVM_CodeListACS0(ShLU_LocReg,  312, "bL",     ShLU_LocReg,  1, None)
ACSVM_CodeListACS0(ShLU_ModReg,  313, "bO",     ShLU_ModReg,  1, None)
ACSVM_CodeListACS0(ShLU_HubReg,  314, "bU",     ShLU_HubReg,  1, None)
ACSVM_CodeListACS0(ShLU_GblReg,  315, "bG",     ShLU_GblReg,  1, None)
ACSVM_CodeListACS0(ShLU_ModArr,  316, "bo",     ShLU_ModArr,  2, None)
ACSVM_CodeListACS0(ShLU_HubArr,  317, "bu",     ShLU_HubArr,  2, None)
ACSVM_CodeListACS0(ShLU_GblArr,  318, "bg",     ShLU_GblArr,  2, None)
ACSVM_CodeListACS0(ShRI_LocReg,  319, "bL",     ShRI_LocReg,  1, None)
ACSVM_CodeListACS0(ShRI_ModReg,  320, "bO",     ShRI_ModReg,  1, None)
ACSVM_CodeListACS0(ShRI_HubReg,  321, "bU",     ShRI_HubReg,  1, None)
ACSVM_CodeListACS0(ShRI_GblReg,  322, "bG",     ShRI_GblReg,  1, None)
ACSVM_CodeListACS0(ShRI_ModArr,  323, "bo",     ShRI_ModArr,  2, None)
ACSVM_CodeListACS0(ShRI_HubArr,  324, "bu",     ShRI_HubArr,  2, None)
ACSVM_CodeListACS0(ShRI_GblArr,  325, "bg",     ShRI_GblArr,  2, None)

ACSVM_CodeListACS0(InvU,         330, "",       InvU,         1, None)

ACSVM_CodeListACS0(PrintIntB,    349, "",       CallFunc,     1, PrintIntB)
ACSVM_CodeListACS0(PrintIntX,    350, "",       CallFunc,     1, PrintIntX)
ACSVM_CodeListACS0(CallFunc,     351, "bh",     CallFunc,     0, None)
ACSVM_CodeListACS0(PrintEndStr,  352, "",       CallFunc,     0, PrintEndStr)
ACSVM_CodeListACS0(PrintModArrR, 353, "",       CallFunc,     4, PrintModArr)
ACSVM_CodeListACS0(PrintHubArrR, 354, "",       CallFunc,     4, PrintHubArr)
ACSVM_CodeListACS0(PrintGblArrR, 355, "",       CallFunc,     4, PrintGblArr)
ACSVM_CodeListACS0(StrCpyModArr, 356, "",       CallFunc,     6, StrCpyModArr)
ACSVM_CodeListACS0(StrCpyHubArr, 357, "",       CallFunc,     6, StrCpyHubArr)
ACSVM_CodeListACS0(StrCpyGblArr, 358, "",       CallFunc,     6, StrCpyGblArr)
ACSVM_CodeListACS0(Pfun_Lit,     359, "b",      Pfun_Lit,     0, None)
ACSVM_CodeListACS0(Call_Stk,     360, "",       Call_Stk,     1, None)
ACSVM_CodeListACS0(ScrWaitS,     361, "",       ScrWaitS,     1, None)

ACSVM_CodeListACS0(Jump_Stk,     363, "",       Jump_Stk,     1, None)
ACSVM_CodeListACS0(Drop_LocArr,  364, "bl",     Drop_LocArr,  2, None)
ACSVM_CodeListACS0(Push_LocArr,  365, "bl",     Push_LocArr,  1, None)
ACSVM_CodeListACS0(AddU_LocArr,  366, "bl",     AddU_LocArr,  2, None)
ACSVM_CodeListACS0(SubU_LocArr,  367, "bl",     SubU_LocArr,  2, None)
ACSVM_CodeListACS0(MulU_LocArr,  368, "bl",     MulU_LocArr,  2, None)
ACSVM_CodeListACS0(DivI_LocArr,  369, "bl",     DivI_LocArr,  2, None)
ACSVM_CodeListACS0(ModI_LocArr,  370, "bl",     ModI_LocArr,  2, None)
ACSVM_CodeListACS0(IncU_LocArr,  371, "bl",     IncU_LocArr,  2, None)
ACSVM_CodeListACS0(DecU_LocArr,  372, "bl",     DecU_LocArr,  2, None)
ACSVM_CodeListACS0(AndU_LocArr,  373, "bl",     AndU_LocArr,  2, None)
ACSVM_CodeListACS0(OrXU_LocArr,  374, "bl",     OrXU_LocArr,  2, None)
ACSVM_CodeListACS0(OrIU_LocArr,  375, "bl",     OrIU_LocArr,  2, None)
ACSVM_CodeListACS0(ShLU_LocArr,  376, "bl",     ShLU_LocArr,  2, None)
ACSVM_CodeListACS0(ShRI_LocArr,  377, "bl",     ShRI_LocArr,  2, None)
ACSVM_CodeListACS0(PrintLocArr,  378, "",       CallFunc,     2, PrintLocArr)
ACSVM_CodeListACS0(PrintLocArrR, 379, "",       CallFunc,     4, PrintLocArr)
ACSVM_CodeListACS0(StrCpyLocArr, 380, "",       CallFunc,     6, StrCpyLocArr)

ACSVM_CodeListACS0(CallSpec_5Ex, 381, "W",      CallSpec,     5, None)
ACSVM_CodeListACS0(CallSpec_6,   500, "b",      CallSpec,     6, None)
ACSVM_CodeListACS0(CallSpec_7,   501, "b",      CallSpec,     7, None)
ACSVM_CodeListACS0(CallSpec_8,   502, "b",      CallSpec,     8, None)
ACSVM_CodeListACS0(CallSpec_9,   503, "b",      CallSpec,     9, None)
ACSVM_CodeListACS0(CallSpec_10,  504, "b",      CallSpec,    10, None)
ACSVM_CodeListACS0(CallSpec_6L,  505, "bWWWWWW",      CallSpec_Lit, 0, None)
ACSVM_CodeListACS0(CallSpec_7L,  506, "bWWWWWWWW",    CallSpec_Lit, 0, None)
ACSVM_CodeListACS0(CallSpec_8L,  507, "bWWWWWWWWW",   CallSpec_Lit, 0, None)
ACSVM_CodeListACS0(CallSpec_9L,  508, "bWWWWWWWWWW",  CallSpec_Lit, 0, None)
ACSVM_CodeListACS0(CallSpec_10L, 509, "bWWWWWWWWWWW", CallSpec_Lit, 0, None)
ACSVM_CodeListACS0(CallSpec_6LB, 510, "BBBBBBB",     CallSpec_Lit, 0, None)
ACSVM_CodeListACS0(CallSpec_7LB, 511, "BBBBBBBB",    CallSpec_Lit, 0, None)
ACSVM_CodeListACS0(CallSpec_8LB, 512, "BBBBBBBBB",   CallSpec_Lit, 0, None)
ACSVM_CodeListACS0(CallSpec_9LB, 513, "BBBBBBBBBB",  CallSpec_Lit, 0, None)
ACSVM_CodeListACS0(CallSpec_10LB,514, "BBBBBBBBBBB", CallSpec_Lit, 0, None)
ACSVM_CodeListACS0(Push_Lit6B,   515, "BBBBBB",     Push_LitArr, 0, None)
ACSVM_CodeListACS0(Push_Lit7B,   516, "BBBBBBB",    Push_LitArr, 0, None)
ACSVM_CodeListACS0(Push_Lit8B,   517, "BBBBBBBB",   Push_LitArr, 0, None)
ACSVM_CodeListACS0(Push_Lit9B,   518, "BBBBBBBBB",  Push_LitArr, 0, None)
ACSVM_CodeListACS0(Push_Lit10B,  519, "BBBBBBBBBB", Push_LitArr, 0, None)
ACSVM_CodeListACS0(CallSpec_10R1,520, "b",      CallSpec_R1, 10, None)

#undef ACSVM_CodeListACS0
#endif


#ifdef ACSVM_FuncList

ACSVM_FuncList(Nop)
ACSVM_FuncList(Kill)

// Printing functions.
ACSVM_FuncList(PrintChar)
ACSVM_FuncList(PrintEndStr)
ACSVM_FuncList(PrintFixD)
ACSVM_FuncList(PrintGblArr)
ACSVM_FuncList(PrintHubArr)
ACSVM_FuncList(PrintIntB)
ACSVM_FuncList(PrintIntD)
ACSVM_FuncList(PrintIntX)
ACSVM_FuncList(PrintLocArr)
ACSVM_FuncList(PrintModArr)
ACSVM_FuncList(PrintPush)
ACSVM_FuncList(PrintString)

// Script functions.
ACSVM_FuncList(ScrPauseS)
ACSVM_FuncList(ScrStartS)
ACSVM_FuncList(ScrStartSD) // Locked Door
ACSVM_FuncList(ScrStartSF) // Forced
ACSVM_FuncList(ScrStartSL) // Locked
ACSVM_FuncList(ScrStartSR) // Result
ACSVM_FuncList(ScrStopS)

// String functions.
ACSVM_FuncList(GetChar)
ACSVM_FuncList(StrCaseCmp)
ACSVM_FuncList(StrCmp)
ACSVM_FuncList(StrCpyGblArr)
ACSVM_FuncList(StrCpyHubArr)
ACSVM_FuncList(StrCpyLocArr)
ACSVM_FuncList(StrCpyModArr)
ACSVM_FuncList(StrLeft)
ACSVM_FuncList(StrLen)
ACSVM_FuncList(StrMid)
ACSVM_FuncList(StrRight)

#undef ACSVM_FuncList
#endif


#ifdef ACSVM_FuncListACS0

ACSVM_FuncListACS0(GetChar, 15, GetChar, {{2, Code::Push_StrArs}})

ACSVM_FuncListACS0(ScrStartS,  39, ScrStartS,  {})
ACSVM_FuncListACS0(ScrPauseS,  40, ScrPauseS,  {})
ACSVM_FuncListACS0(ScrStopS,   41, ScrStopS,   {})
ACSVM_FuncListACS0(ScrStartSL, 42, ScrStartSL, {})
ACSVM_FuncListACS0(ScrStartSD, 43, ScrStartSD, {})
ACSVM_FuncListACS0(ScrStartSR, 44, ScrStartSR, {})
ACSVM_FuncListACS0(ScrStartSF, 45, ScrStartSF, {})

ACSVM_FuncListACS0(StrCmp,     63, StrCmp,     {})
ACSVM_FuncListACS0(StrCaseCmp, 64, StrCaseCmp, {})
ACSVM_FuncListACS0(StrLeft,    65, StrLeft,    {})
ACSVM_FuncListACS0(StrRight,   66, StrRight,   {})
ACSVM_FuncListACS0(StrMid,     67, StrMid,     {})

#undef ACSVM_FuncListACS0
#endif

// EOF

