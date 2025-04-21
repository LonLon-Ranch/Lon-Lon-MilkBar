[BotW_UKL_Utils_DynamicBranch_V208]
moduleMatches = 0x6267BFD0

.origin = codecave

; r3 = str* HLEName
; r4 = str* ModuleName
; r5 = void** HLELoc
; r6 = int* ModuleHandle
; r7 = RegStore* RegisterStore
; Return = bool success
; Extra registers used: r11
UKL_Utils_DynamicBranch:
; Back up LR
addi r1, r1, -8
stw r3, 4(r1)
mflr r3
stw r3, 0(r1)
lwz r3, 4(r1)

; Check if we already have our dll func addr
lwz r11, 0x0(r5) ; (HLELoc)
cmpwi r11, 0x0
bne UKL_Utils_DynamicBranch_CallDllFunc

; We're gonna need to do some register juggling
addi r1, r1, -16
stw r3, 0(r1) ; (HLEName)
stw r4, 4(r1) ; (ModuleName)
stw r5, 8(r1) ; (HLELoc)
stw r6, 12(r1) ; (ModuleHandle)

lwz r3, 4(r1) ; (ModuleName)
lwz r4, 12(r1) ; (ModuleHandle)
bla import.coreinit.OSDynLoad_Acquire

cmpwi r3, 0x0 ; Anything other than 0x0 is an error code.. 
bne UKL_Utils_DynamicBranch_AcquireFailed ; In this case the module isn't registered (yet).

lwz r3, 12(r1) ; (ModuleHandle)
lwz r3, 0(r3)
li r4, 0x0
lwz r5, 0(r1) ; (HLEName)
lwz r6, 8(r1) ; (HLELoc)
bla import.coreinit.OSDynLoad_FindExport

lwz r11, 8(r1) ; (HLELoc)
lwz r11, 0(r11)

addi r1, r1, 16

; Expects &HLELoc to be in r11
UKL_Utils_DynamicBranch_CallDllFunc:
addi r1, r1, -8
stw r11, 0x0(r1)
mr r3, r7
bl UKL_Utils_LoadRegStore
stw r11, 0x4(r1)
lwz r11, 0x0(r1)
mtctr r11
lwz r11, 0x4(r1)
addi r1, r1, 8
bctrl

UKL_Utils_DynamicBranch_Return_True:
; Restore LR
stw r3, 4(r1)
lwz r3, 0(r1)
mtlr r3
lwz r3, 4(r1)
addi r1, r1, 8

li r3, 0x1
blr

UKL_Utils_DynamicBranch_AcquireFailed:
lwz r3, 12(r1) ; (ModuleHandle)
lwz r3, 0(r3)
bla import.coreinit.OSDynLoad_Release
addi r1, r1, 16

UKL_Utils_DynamicBranch_Return_False:
; Restore LR
stw r3, 4(r1)
lwz r3, 0(r1)
mtlr r3
lwz r3, 4(r1)
addi r1, r1, 8

li r3, 0x0
blr