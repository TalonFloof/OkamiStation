.origin 0x8000_0000
_start:
    ; Setup some basic CPU stuff
    0x100 -> %xreg.ind ; Enable all Supervisor Features
    0xF80 -> %xreg.val

