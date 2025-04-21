[BotW_UKL_Utils_RegStore_V208]
moduleMatches = 0x6267BFD0

.origin = codecave

; We could make various store and load functions specific to what registers to include, suffixed with some sort of mask like _0010101000.
; But to save codecave we instead just have functions to write to stored registers, since we have limited codecave space.

UKL_Utils_RegStore_Size:
.int 128

; Due to the nature of this function params cannot be passed in.
; Registers are stored directly to the stack.
; Return = RegStore* Registers
UKL_Utils_CreateRegStore:
addi r1, r1, -128
stw r0, 0(r1)
stw r1, 4(r1)
stw r2, 8(r1)
stw r3, 12(r1)
stw r4, 16(r1)
stw r5, 20(r1)
stw r6, 24(r1)
stw r7, 28(r1)
stw r8, 32(r1)
stw r9, 36(r1)
stw r10, 40(r1)
stw r11, 44(r1)
stw r12, 48(r1)
stw r13, 52(r1)
stw r14, 56(r1)
stw r15, 60(r1)
stw r16, 64(r1)
stw r17, 68(r1)
stw r18, 72(r1)
stw r19, 76(r1)
stw r20, 80(r1)
stw r21, 84(r1)
stw r22, 88(r1)
stw r23, 92(r1)
stw r24, 96(r1)
stw r25, 100(r1)
stw r26, 104(r1)
stw r27, 108(r1)
stw r28, 112(r1)
stw r29, 116(r1)
stw r30, 120(r1)
stw r31, 124(r1)

mr r3, r1
blr

; r3 = RegStore* Registers
UKL_Utils_LoadRegStore:
lwz r0, 0(r3)
;lwz r1, 4(r3) // Don't load r1
lwz r2, 8(r3)
lwz r4, 16(r3)
lwz r5, 20(r3)
lwz r6, 24(r3)
lwz r7, 28(r3)
lwz r8, 32(r3)
lwz r9, 36(r3)
lwz r10, 40(r3)
lwz r11, 44(r3)
lwz r12, 48(r3)
lwz r13, 52(r3)
lwz r14, 56(r3)
lwz r15, 60(r3)
lwz r16, 64(r3)
lwz r17, 68(r3)
lwz r18, 72(r3)
lwz r19, 76(r3)
lwz r20, 80(r3)
lwz r21, 84(r3)
lwz r22, 88(r3)
lwz r23, 92(r3)
lwz r24, 96(r3)
lwz r25, 100(r3)
lwz r26, 104(r3)
lwz r27, 108(r3)
lwz r28, 112(r3)
lwz r29, 116(r3)
lwz r30, 120(r3)
lwz r31, 124(r3)

; Load me last!
lwz r3, 12(r3)
blr

; Should be called when r1 is aligned to a RegStore
UKL_Utils_DisposeRegStore:
addi r1, r1, 128
blr

; r3 = value to write
UKL_Utils_WriteR0:
stw r3, 0(r1)
blr

; r3 = value to write
UKL_Utils_WriteR1:
stw r3, 4(r1)
blr

; r3 = value to write
UKL_Utils_WriteR2:
stw r3, 8(r1)
blr

; r3 = value to write
UKL_Utils_WriteR3:
stw r3, 12(r1)
blr

; r3 = value to write
UKL_Utils_WriteR4:
stw r3, 16(r1)
blr

; r3 = value to write
UKL_Utils_WriteR5:
stw r3, 20(r1)
blr

; r3 = value to write
UKL_Utils_WriteR6:
stw r3, 24(r1)
blr

; r3 = value to write
UKL_Utils_WriteR7:
stw r3, 28(r1)
blr

; r3 = value to write
UKL_Utils_WriteR8:
stw r3, 32(r1)
blr

; r3 = value to write
UKL_Utils_WriteR9:
stw r3, 36(r1)
blr

; r3 = value to write
UKL_Utils_WriteR10:
stw r3, 40(r1)
blr

; r3 = value to write
UKL_Utils_WriteR11:
stw r3, 44(r1)
blr

; r3 = value to write
UKL_Utils_WriteR12:
stw r3, 48(r1)
blr

; r3 = value to write
UKL_Utils_WriteR13:
stw r3, 52(r1)
blr

; r3 = value to write
UKL_Utils_WriteR14:
stw r3, 56(r1)
blr

; r3 = value to write
UKL_Utils_WriteR15:
stw r3, 60(r1)
blr

; r3 = value to write
UKL_Utils_WriteR16:
stw r3, 64(r1)
blr

; r3 = value to write
UKL_Utils_WriteR17:
stw r3, 68(r1)
blr

; r3 = value to write
UKL_Utils_WriteR18:
stw r3, 72(r1)
blr

; r3 = value to write
UKL_Utils_WriteR19:
stw r3, 76(r1)
blr

; r3 = value to write
UKL_Utils_WriteR20:
stw r3, 80(r1)
blr

; r3 = value to write
UKL_Utils_WriteR21:
stw r3, 84(r1)
blr

; r3 = value to write
UKL_Utils_WriteR22:
stw r3, 88(r1)
blr

; r3 = value to write
UKL_Utils_WriteR23:
stw r3, 92(r1)
blr

; r3 = value to write
UKL_Utils_WriteR24:
stw r3, 96(r1)
blr

; r3 = value to write
UKL_Utils_WriteR25:
stw r3, 100(r1)
blr

; r3 = value to write
UKL_Utils_WriteR26:
stw r3, 104(r1)
blr

; r3 = value to write
UKL_Utils_WriteR27:
stw r3, 108(r1)
blr

; r3 = value to write
UKL_Utils_WriteR28:
stw r3, 112(r1)
blr

; r3 = value to write
UKL_Utils_WriteR29:
stw r3, 116(r1)
blr

; r3 = value to write
UKL_Utils_WriteR30:
stw r3, 120(r1)
blr

; r3 = value to write
UKL_Utils_WriteR31:
stw r3, 124(r1)
blr
