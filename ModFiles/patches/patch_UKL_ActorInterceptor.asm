[BotW_UKL_ActorInterceptor_V208]
moduleMatches = 0x6267BFD0

.origin = codecave

UKL_ActorInterceptor_ModuleName:
.string "ukl_actorinterceptor" ; Unfortunately it doesn't seem like this can be capitalized

UKL_ActorInterceptor_ModuleHandle:
.int 0

UKL_ActorInterceptor_OnActorCreate_Name:
.string "OnActorCreate"

UKL_ActorInterceptor_OnActorCreate_Loc:
.int 0

UKL_ActorInterceptor_OnActorDeleteLater_Name:
.string "OnActorDeleteLater"

UKL_ActorInterceptor_OnActorDeleteLater_Loc:
.int 0

UKL_ActorInterceptor_OnActorErase_Name:
.string "OnActorErase"

UKL_ActorInterceptor_OnActorErase_Loc:
.int 0



ActorCreateHook:
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

lis r3, UKL_ActorInterceptor_OnActorCreate_Name@ha
addi r3, r3, UKL_ActorInterceptor_OnActorCreate_Name@l
lis r4, UKL_ActorInterceptor_ModuleName@ha
addi r4, r4, UKL_ActorInterceptor_ModuleName@l
lis r5, UKL_ActorInterceptor_OnActorCreate_Loc@ha
addi r5, r5, UKL_ActorInterceptor_OnActorCreate_Loc@l
lis r6, UKL_ActorInterceptor_ModuleHandle@ha
addi r6, r6, UKL_ActorInterceptor_ModuleHandle@l
lwz r7, 0(r1)
bl UKL_Utils_DynamicBranch

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
blr

; Return to execution
; (Not needed because original instruction is a branch)

; ksys::act::ActorCreator::createActor_(?) (The thing that createActor_ calls)
;0x037b5bdc = ba ActorCreateHook

; ksys::act::ActorCreator::doCreateProc (Where actors are actually generated after request)
0x037b72d0 = ba ActorCreateHook
0x037b7654 = ba ActorCreateHook
0x037b76cc = ba ActorCreateHook

ActorEraseHook:
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

lis r3, UKL_ActorInterceptor_OnActorErase_Name@ha
addi r3, r3, UKL_ActorInterceptor_OnActorErase_Name@l
lis r4, UKL_ActorInterceptor_ModuleName@ha
addi r4, r4, UKL_ActorInterceptor_ModuleName@l
lis r5, UKL_ActorInterceptor_OnActorErase_Loc@ha
addi r5, r5, UKL_ActorInterceptor_OnActorErase_Loc@l
lis r6, UKL_ActorInterceptor_ModuleHandle@ha
addi r6, r6, UKL_ActorInterceptor_ModuleHandle@l
lwz r7, 0(r1)
bl UKL_Utils_DynamicBranch

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
mflr r0

; Return to execution
b 0x037b6140

0x037b613c = ba ActorEraseHook


ActorDeleteLaterHook:
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

lis r3, UKL_ActorInterceptor_OnActorDeleteLater_Name@ha
addi r3, r3, UKL_ActorInterceptor_OnActorDeleteLater_Name@l
lis r4, UKL_ActorInterceptor_ModuleName@ha
addi r4, r4, UKL_ActorInterceptor_ModuleName@l
lis r5, UKL_ActorInterceptor_OnActorDeleteLater_Loc@ha
addi r5, r5, UKL_ActorInterceptor_OnActorDeleteLater_Loc@l
lis r6, UKL_ActorInterceptor_ModuleHandle@ha
addi r6, r6, UKL_ActorInterceptor_ModuleHandle@l
lwz r7, 0(r1)
bl UKL_Utils_DynamicBranch

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
mflr r0

; Return to execution
b 0x0378a378

0x0378a374 = ba ActorDeleteLaterHook
