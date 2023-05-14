OKB_SCSISelect:
    mcall 0x200
    br ra

OKB_SCSIRead:
    mcall 0x201
    br ra

OKB_SCSIWrite:
    mcall 0x202
    br ra

OKB_SCSIGetCapacity:
    mcall 0x203
    br ra

OKB_SCSIStartStop:
    mcall 0x204
    br ra

.global HALRAMSize:
    mcall 0x300
    mv a1, a0
    br ra
