/**
 * @file XMC1200x0200.ld
 * @date 2015-07-07
 *
 * @cond
 *********************************************************************************************************************
 * Linker file for the GNU C Compiler v1.7
 * Supported devices: XMC1200-T038F0200
 *                    XMC1201-T038F0200
 *                    XMC1201-Q040F0200
 *
 * Copyright (c) 2015, Infineon Technologies AG
 * All rights reserved.                        
 *                                             
 * Redistribution and use in source and binary forms, with or without modification,are permitted provided that the 
 * following conditions are met:   
 *                                                                              
 * Redistributions of source code must retain the above copyright notice, this list of conditions and the following 
 * disclaimer.                        
 * 
 * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following 
 * disclaimer in the documentation and/or other materials provided with the distribution.                       
 * 
 * Neither the name of the copyright holders nor the names of its contributors may be used to endorse or promote 
 * products derived from this software without specific prior written permission.                                           
 *                                                                              
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, 
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE  
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE  FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR  
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
 * WHETHER IN CONTRACT, STRICT LIABILITY,OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.                                                  
 *                                                                              
 * To improve the quality of the software, users are encouraged to share modifications, enhancements or bug fixes with 
 * Infineon Technologies AG dave@infineon.com).                                                          
 *********************************************************************************************************************
 *
 * Change History
 * --------------
 *
 * 2015-07-07:
 *     - Product splitting
 *     - Copyright notice update
 *      
 * @endcond 
 *
 */

OUTPUT_FORMAT("elf32-littlearm")
OUTPUT_ARCH(arm)
ENTRY(Reset_Handler)

MEMORY
{
	FLASH(RX) : ORIGIN = 0x10001000, LENGTH = 0x31a00
	SRAM(!RX) : ORIGIN = 0x20000000, LENGTH = 0x4000
}

stack_size = 1536;

SECTIONS
{
    /* TEXT section */

    .text : 
    {
        sText = .;
        KEEP(*(.reset));
        *(.text .text.* .gnu.linkonce.t.*);

        /* ARM <->THUMB interworking */
        *(.glue*)
        *(.v4*)
        *(.vfp11_veneer)

        /* C++ Support */
        KEEP(*(.init))
        __preinit_array_start = .;
        KEEP (*(.preinit_array))
        __preinit_array_end = .;
        __init_array_start = .;
        KEEP (*(SORT(.init_array.*)))
        KEEP (*(.init_array))
        __init_array_end = .;
        KEEP (*crtbegin.o(.ctors))
        KEEP (*(EXCLUDE_FILE (*crtend.o) .ctors))
        KEEP (*(SORT(.ctors.*)))
        KEEP (*crtend.o(.ctors))
        KEEP(*(.fini))
        __fini_array_start = .;
        KEEP (*(.fini_array))
        KEEP (*(SORT(.fini_array.*)))
        __fini_array_end = .;

        KEEP (*crtbegin.o(.dtors))
        KEEP (*(EXCLUDE_FILE (*crtend.o) .dtors))
        KEEP (*(SORT(.dtors.*)))
        KEEP (*crtend.o(.dtors))

        /* Exception handling support */
        __extab_start = .;
        *(.ARM.extab* .gnu.linkonce.armextab.*)
        . = ALIGN(4);
        __extab_end = ABSOLUTE(.);
    } > FLASH

    /* Exception handling, exidx needs a dedicated section */
    .ARM.exidx : 
    {
        __exidx_start = .;
        *(.ARM.exidx* .gnu.linkonce.armexidx.*)
        . = ALIGN(4);
        __exidx_end = ABSOLUTE(.);
    } > FLASH

    /* CONST data section */
    .rodata : 
    {
        *(.rodata .rodata.*)
        *(.gnu.linkonce.r*)
    } > FLASH

	. = ALIGN(16);
		
    /* End of RO-DATA and start of LOAD region for the veneers */
    eROData = . ;

    /* DSRAM layout (Lowest to highest)*/
    /* Veneer <-> Stack <-> DATA <-> BSS <-> HEAP */

    .VENEER_Code ABSOLUTE(0x2000000C): AT(eROData)
    {
        VeneerStart = .;
        KEEP(*(.XmcVeneerCode));
        . = ALIGN(4);
        VeneerEnd = .;
    } > SRAM

    VeneerSize = ABSOLUTE(VeneerEnd) - ABSOLUTE(VeneerStart);

    /* Dummy section for stack */
    Stack (NOLOAD) : AT(0)
    {
        . = ALIGN(8);
        . = . + stack_size;
        __initial_sp = .;
    } > SRAM

     /* Standard DATA and user defined DATA/BSS/CONST sections */
    DataLoadAddr = eROData + VeneerSize;
    .data : AT(DataLoadAddr)
    {
        __data_start = .;
        * (.data);
        * (.data*);
        *(*.data);
        *(.gnu.linkonce.d*)
        . = ALIGN(4);
        __data_end = .;
    } > SRAM
	__data_size = __data_end - __data_start;

    .ram_code : AT(DataLoadAddr + __data_size)
    {
        __ram_code_start = .;
        /* functions with __attribute__ ((section (".ram_code")))*/
        *(.ram_code)   
        . = ALIGN(4);        
        __ram_code_end = .;
    } > SRAM
    __ram_code_load = LOADADDR (.ram_code);
    __ram_code_size = __ram_code_end - __ram_code_start;
    
    /* BSS section */
    .bss (NOLOAD) :
    {
        __bss_start = .;
        * (.bss);
        * (.bss*);
        * (COMMON);
        *(.gnu.linkonce.b*)
        . = ALIGN(4);
        __bss_end = .;
        . = ALIGN(8);
        Heap_Bank1_Start = .;
    } > SRAM
    __bss_size = __bss_end - __bss_start;
    
    /* .no_init section */
    .no_init 0x20003FFC (NOLOAD) : 
    {
        Heap_Bank1_End = .;
        * (.no_init);
    } > SRAM
    
    /* Heap - Bank1*/
    Heap_Bank1_Size  = Heap_Bank1_End - Heap_Bank1_Start;

    /DISCARD/ :
    {
        *(.comment)
    }

    .stab              0 (NOLOAD) : { *(.stab) }
    .stabstr           0 (NOLOAD) : { *(.stabstr) }

    /* DWARF 1 */
    .debug             0 : { *(.debug) }
    .line              0 : { *(.line) }

    /* GNU DWARF 1 extensions */
    .debug_srcinfo     0 : { *(.debug_srcinfo) }
    .debug_sfnames     0 : { *(.debug_sfnames) }

    /* DWARF 1.1 and DWARF 2 */
    .debug_aranges     0 : { *(.debug_aranges) }
    .debug_pubnames    0 : { *(.debug_pubnames) }
    .debug_pubtypes    0 : { *(.debug_pubtypes) }

    /* DWARF 2 */
    .debug_info        0 : { *(.debug_info .gnu.linkonce.wi.*) }
    .debug_abbrev      0 : { *(.debug_abbrev) }
    .debug_line        0 : { *(.debug_line) }
    .debug_frame       0 : { *(.debug_frame) }
    .debug_str         0 : { *(.debug_str) }
    .debug_loc         0 : { *(.debug_loc) }
    .debug_macinfo     0 : { *(.debug_macinfo) }

    /* DWARF 2.1 */
    .debug_ranges      0 : { *(.debug_ranges) }

    /* SGI/MIPS DWARF 2 extensions */
    .debug_weaknames   0 : { *(.debug_weaknames) }
    .debug_funcnames   0 : { *(.debug_funcnames) }
    .debug_typenames   0 : { *(.debug_typenames) }
    .debug_varnames    0 : { *(.debug_varnames) }

    /* Build attributes */
    .build_attributes  0 : { *(.ARM.attributes) }
}
