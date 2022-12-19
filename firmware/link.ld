ENTRY(_start)

SECTIONS
{
	. = 0xf0000000;

	.text : ALIGN(16) {
		__TEXT_BEGIN__ = .;
		*(.header);
        *(.text .text.*);
		__TEXT_END__ = .;
	}

	.rodata : ALIGN(16) {
		__RODATA_BEGIN__ = .;
		*(.rodata .rodata.* .rdata .rdata.*)
		__RODATA_END__ = .;
	}

	. = 0x00000000;

	.data ALIGN(16) : AT(__RODATA_END__) {
		__DATA_BEGIN__ = ABSOLUTE(.);
		*(.data)
		*(.data.*)
		*(.sdata)
		__DATA_END__ = ABSOLUTE(.);
	}

	.bss ALIGN(16) (NOLOAD) : AT(__DATA_END__) {
		__BSS_BEGIN__ = ABSOLUTE(.);
		*(.bss)
		*(.bss.*)
		*(.sbss)
		__BSS_END__ = ABSOLUTE(.);
	}

	PROVIDE( TIMERL = 0x1100bff8 );
	PROVIDE( TIMERH = 0x1100bffc );
}