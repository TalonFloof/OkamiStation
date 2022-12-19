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

	. = 0x00000010;

	.data ALIGN(16) : AT(__RODATA_END__) {
		*(.data)
		*(.data.*)
		*(.sdata)
	}
	__DATA_BEGIN__ = __RODATA_END__;
	__DATA_END__ = __RODATA_END__ + SIZEOF(.data);

	.bss ALIGN(16) (NOLOAD) : AT(ADDR(.data) + SIZEOF(.data)) {
		__BSS_BEGIN__ = .;
		*(.bss)
		*(.bss.*)
		*(.sbss)
		__BSS_END__ = .;
	}

	PROVIDE( TIMERL = 0x1100bff8 );
	PROVIDE( TIMERH = 0x1100bffc );
}