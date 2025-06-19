[BotW_UKL_TimeMgrInterceptor_V208]
moduleMatches = 0x6267BFD0

.origin = codecave

UKL_TimeMgrInterceptor_ModuleName:
.string "ukl_timemgrinterceptor" ; Unfortunately it doesn't seem like this can be capitalized

UKL_TimeMgrInterceptor_ModuleHandle:
.int 0

UKL_TimeMgrInterceptor_OnInit_Name:
.string "OnInit"

UKL_TimeMgrInterceptor_OnInit_Loc:
.int 0


UKL_TimeMgrInterceptor_OnInit:
; Backup LR
addi r1, r1, -8
stw r3, 4(r1)
mflr r3
stw r3, 0(r1)
lwz r3, 4(r1)
; Backup the rest of the registers
bl UKL_Utils_CreateRegStore
addi r1, r1, -4
stw r3, 0(r1) ; RegStore* will be in stack

lis r3, UKL_TimeMgrInterceptor_OnInit_Name@ha
addi r3, r3, UKL_TimeMgrInterceptor_OnInit_Name@l
lis r4, UKL_TimeMgrInterceptor_ModuleName@ha
addi r4, r4, UKL_TimeMgrInterceptor_ModuleName@l
lis r5, UKL_TimeMgrInterceptor_OnInit_Loc@ha
addi r5, r5, UKL_TimeMgrInterceptor_OnInit_Loc@l
lis r6, UKL_TimeMgrInterceptor_ModuleHandle@ha
addi r6, r6, UKL_TimeMgrInterceptor_ModuleHandle@l
lwz r7, 0(r1)
bl UKL_Utils_DynamicBranch

UKL_NPCWaitActionInterceptor_OnLoadParams_Exit:
; Restore the rest of the registers
lwz r3, 0(r1)
addi r1, r1, 4
bl UKL_Utils_LoadRegStore
bl UKL_Utils_DisposeRegStore
; Restore LR
stw r3, 4(r1)
lwz r3, 0(r1)
mtlr r3
lwz r3, 4(r1)
addi r1, r1, 8

; Execute original instruction
lis r12, 0x1030

; Return to execution
b 0x0365ec88

; After static params are loaded in the loadParams_ function
0x0365ec84 = b UKL_TimeMgrInterceptor_OnInit
