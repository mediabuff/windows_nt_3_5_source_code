/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    cirrus.h

Abstract:

    This module contains the definitions for the code that implements the
    Cirrus Logic VGA 6410/6420/542x device driver.

Environment:

    Kernel mode

Revision History:


--*/


#define INT10_MODE_SET

//---------------------------------------------------------------------------

#ifndef INT10_MODE_SET
#error !!ERROR: int10_mode_set not defined!
#endif


//
// Do full save and restore.
//

#define EXTENDED_REGISTER_SAVE_RESTORE 1

//
// Banking ifdefs to enable banking
// the banking type MUST match the type in clhard.asm
//

#define ONE_64K_BANK             0
#define TWO_32K_BANKS            1
#define MULTIPLE_REFRESH_TABLES  0

//---------------------------------------------------------------------------
//
// only one banking variable must be defined
//
#if TWO_32K_BANKS
#if ONE_64K_BANK
#error !!ERROR: two types of banking defined!
#endif
#elif ONE_64K_BANK
#else
#error !!ERROR: banking type must be defined!
#endif

//
// Base address of VGA memory range.  Also used as base address of VGA
// memory when loading a font, which is done with the VGA mapped at A0000.
//

#ifdef _X86_

#define MEM_VGA      0xA0000
#define MEM_VGA_SIZE 0x20000

#else

//
// For MIPS NEC machine only
//

#define LA_MASK      0x1
#define MEM_VGA      (LA_MASK << 20)
#define MEM_VGA_SIZE 0x100000

#endif
//
// Port definitions for filling the ACCSES_RANGES structure in the miniport
// information, defines the range of I/O ports the VGA spans.
// There is a break in the IO ports - a few ports are used for the parallel
// port. Those cannot be defined in the ACCESS_RANGE, but are still mapped
// so all VGA ports are in one address range.
//

#define VGA_BASE_IO_PORT      0x000003B0
#define VGA_START_BREAK_PORT  0x000003BB
#define VGA_END_BREAK_PORT    0x000003C0
#define VGA_MAX_IO_PORT       0x000003DF

//
// VGA port-related definitions.
//

//
// VGA register definitions
//
                                            // ports in monochrome mode
#define CRTC_ADDRESS_PORT_MONO      0x0004  // CRT Controller Address and
#define CRTC_DATA_PORT_MONO         0x0005  // Data registers in mono mode
#define FEAT_CTRL_WRITE_PORT_MONO   0x000A  // Feature Control write port
                                            // in mono mode
#define INPUT_STATUS_1_MONO         0x000A  // Input Status 1 register read
                                            // port in mono mode
#define ATT_INITIALIZE_PORT_MONO    INPUT_STATUS_1_MONO
                                            // Register to read to reset
                                            // Attribute Controller index/data
#define ATT_ADDRESS_PORT            0x0010  // Attribute Controller Address and
#define ATT_DATA_WRITE_PORT         0x0010  // Data registers share one port
                                            // for writes, but only Address is
                                            // readable at 0x010
#define ATT_DATA_READ_PORT          0x0011  // Attribute Controller Data reg is
                                            // readable here
#define MISC_OUTPUT_REG_WRITE_PORT  0x0012  // Miscellaneous Output reg write
                                            // port
#define INPUT_STATUS_0_PORT         0x0012  // Input Status 0 register read
                                            // port
#define VIDEO_SUBSYSTEM_ENABLE_PORT 0x0013  // Bit 0 enables/disables the
                                            // entire VGA subsystem
#define SEQ_ADDRESS_PORT            0x0014  // Sequence Controller Address and
#define SEQ_DATA_PORT               0x0015  // Data registers
#define DAC_PIXEL_MASK_PORT         0x0016  // DAC pixel mask reg
#define DAC_ADDRESS_READ_PORT       0x0017  // DAC register read index reg,
                                            // write-only
#define DAC_STATE_PORT              0x0017  // DAC state (read/write),
                                            // read-only
#define DAC_ADDRESS_WRITE_PORT      0x0018  // DAC register write index reg
#define DAC_DATA_REG_PORT           0x0019  // DAC data transfer reg
#define FEAT_CTRL_READ_PORT         0x001A  // Feature Control read port
#define MISC_OUTPUT_REG_READ_PORT   0x001C  // Miscellaneous Output reg read
                                            // port
#define GRAPH_ADDRESS_PORT          0x001E  // Graphics Controller Address
#define GRAPH_DATA_PORT             0x001F  // and Data registers

                                            // ports in color mode
#define CRTC_ADDRESS_PORT_COLOR     0x0024  // CRT Controller Address and
#define CRTC_DATA_PORT_COLOR        0x0025  // Data registers in color mode
#define FEAT_CTRL_WRITE_PORT_COLOR  0x002A  // Feature Control write port
#define INPUT_STATUS_1_COLOR        0x002A  // Input Status 1 register read
                                            // port in color mode
#define ATT_INITIALIZE_PORT_COLOR   INPUT_STATUS_1_COLOR
                                            // Register to read to reset
                                            // Attribute Controller index/data
                                            // toggle in color mode
//
// Offsets in HardwareStateHeader->PortValue[] of save areas for non-indexed
// VGA registers.
//

#define CRTC_ADDRESS_MONO_OFFSET      0x04
#define FEAT_CTRL_WRITE_MONO_OFFSET   0x0A
#define ATT_ADDRESS_OFFSET            0x10
#define MISC_OUTPUT_REG_WRITE_OFFSET  0x12
#define VIDEO_SUBSYSTEM_ENABLE_OFFSET 0x13
#define SEQ_ADDRESS_OFFSET            0x14
#define DAC_PIXEL_MASK_OFFSET         0x16
#define DAC_STATE_OFFSET              0x17
#define DAC_ADDRESS_WRITE_OFFSET      0x18
#define GRAPH_ADDRESS_OFFSET          0x1E
#define CRTC_ADDRESS_COLOR_OFFSET     0x24
#define FEAT_CTRL_WRITE_COLOR_OFFSET  0x2A
                                                                     
                                            // toggle in color mode
//
// VGA indexed register indexes.
//

// CL-GD542x specific registers:
//
#define IND_CL_EXTS_ENB         0x06    // index in Sequencer to enable exts
#define IND_CL_SCRATCH_PAD      0x0A    // index in Seq of 542x scratch pad
#define IND_ALP_SCRATCH_PAD     0x15    // index in Seq of Alpine scratch pad
#define IND_CL_ID_REG           0x27    // index in CRTC of ID Register
//
#define IND_CURSOR_START        0x0A    // index in CRTC of the Cursor Start
#define IND_CURSOR_END          0x0B    //  and End registers
#define IND_CURSOR_HIGH_LOC     0x0E    // index in CRTC of the Cursor Location
#define IND_CURSOR_LOW_LOC      0x0F    //  High and Low Registers
#define IND_VSYNC_END           0x11    // index in CRTC of the Vertical Sync
                                        //  End register, which has the bit
                                        //  that protects/unprotects CRTC
                                        //  index registers 0-7
#define IND_SET_RESET_ENABLE    0x01    // index of Set/Reset Enable reg in GC
#define IND_DATA_ROTATE         0x03    // index of Data Rotate reg in GC
#define IND_READ_MAP            0x04    // index of Read Map reg in Graph Ctlr
#define IND_GRAPH_MODE          0x05    // index of Mode reg in Graph Ctlr
#define IND_GRAPH_MISC          0x06    // index of Misc reg in Graph Ctlr
#define IND_BIT_MASK            0x08    // index of Bit Mask reg in Graph Ctlr
#define IND_SYNC_RESET          0x00    // index of Sync Reset reg in Seq
#define IND_MAP_MASK            0x02    // index of Map Mask in Sequencer
#define IND_MEMORY_MODE         0x04    // index of Memory Mode reg in Seq
#define IND_CRTC_PROTECT        0x11    // index of reg containing regs 0-7 in
                                        //  CRTC
#define IND_CRTC_COMPAT         0x34    // index of CRTC Compatibility reg
                                        //  in CRTC
#define START_SYNC_RESET_VALUE  0x01    // value for Sync Reset reg to start
                                        //  synchronous reset
#define END_SYNC_RESET_VALUE    0x03    // value for Sync Reset reg to end
                                        //  synchronous reset
 
//
// Value to write to Extensions Control register values extensions.
//

#define CL64xx_EXTENSION_ENABLE_INDEX     0x0A     // GR0A to be exact!
#define CL64xx_EXTENSION_ENABLE_VALUE     0xEC
#define CL64xx_EXTENSION_DISABLE_VALUE    0xCE
#define CL64xx_TRISTATE_CONTROL_REG       0xA1

#define CL6340_ENABLE_READBACK_REGISTER   0xE0
#define CL6340_ENABLE_READBACK_ALLSEL_VALUE 0xF0
#define CL6340_ENABLE_READBACK_OFF_VALUE  0x00
#define CL6340_IDENTIFICATION_REGISTER    0xE9     
//
// Values for Attribute Controller Index register to turn video off
// and on, by setting bit 5 to 0 (off) or 1 (on).
//

#define VIDEO_DISABLE 0
#define VIDEO_ENABLE  0x20

// Masks to keep only the significant bits of the Graphics Controller and
// Sequencer Address registers. Masking is necessary because some VGAs, such
// as S3-based ones, don't return unused bits set to 0, and some SVGAs use
// these bits if extensions are enabled.
//

#define GRAPH_ADDR_MASK 0x0F
#define SEQ_ADDR_MASK   0x07

//
// Mask used to toggle Chain4 bit in the Sequencer's Memory Mode register.
//

#define CHAIN4_MASK 0x08

//
// Value written to the Read Map register when identifying the existence of
// a VGA in VgaInitialize. This value must be different from the final test
// value written to the Bit Mask in that routine.
//

#define READ_MAP_TEST_SETTING 0x03

//
// Default text mode setting for various registers, used to restore their
// states if VGA detection fails after they've been modified.
//

#define MEMORY_MODE_TEXT_DEFAULT 0x02
#define BIT_MASK_DEFAULT 0xFF
#define READ_MAP_DEFAULT 0x00


//
// Palette-related info.
//

//
// Highest valid DAC color register index.
//

#define VIDEO_MAX_COLOR_REGISTER  0xFF

//
// Highest valid palette register index
//

#define VIDEO_MAX_PALETTE_REGISTER 0x0F


//
// Indices for type of memory mapping; used in ModesVGA[], must match
// MemoryMap[].
//

typedef enum _VIDEO_MEMORY_MAP {
    MemMap_Mono,
    MemMap_CGA,
    MemMap_VGA,
    MemMap_LINEAR
} VIDEO_MEMORY_MAP, *PVIDEO_MEMORY_MAP;

//
// Memory map table definition
//

typedef struct {
    ULONG   MaxSize;        // Maximum addressable size of memory
    ULONG   Offset;          // Start address of display memory
} MEMORYMAPS;

//
// For a mode, the type of banking supported. Controls the information
// returned in VIDEO_BANK_SELECT. PlanarHCBanking includes NormalBanking.
//

typedef enum _BANK_TYPE {
    NoBanking = 0,
    NormalBanking,
    PlanarHCBanking
} BANK_TYPE, *PBANK_TYPE;


//
// Define type of cirrus boards
//

typedef enum _BOARD_TYPE {
    SPEEDSTARPRO = 1,
    OTHER
} BOARD_TYPE;


#define  CL6410   0x0001
#define  CL6420   0x0002
#define  CL542x   0x0004
#define  CL543x   0x0008

// bitfields for the DisplayType 
#define  crt      0x0001
#define  panel    0x0002
#define  simulscan 0x0004        // this means both, but is unused for now.

typedef struct {
    PUSHORT pCL6410_crt;
    PUSHORT pCL6410_panel;
    PUSHORT pCL6420_crt;
    PUSHORT pCL6420_panel;
    PUSHORT pCL542x;
    PUSHORT pCL543x;
    PUSHORT pStretchScan;
} CLCMD, *PCLCMD;

typedef struct {
    USHORT BiosModeCL6410;       // bios modes are different across the 
    USHORT BiosModeCL6420;       // products. that's why we need multiple
    USHORT BiosModeCL542x;       // values.
} CLMODE, *PCLMODE;


//
// Structure used to describe each video mode in ModesVGA[].
//

typedef struct {
    USHORT  fbType; // color or monochrome, text or graphics, via
                    //  VIDEO_MODE_COLOR and VIDEO_MODE_GRAPHICS
    USHORT  numPlanes;    // # of video memory planes
    USHORT  bitsPerPlane; // # of bits of color in each plane
    SHORT   col;    // # of text columns across screen with default font
    SHORT   row;    // # of text rows down screen with default font
    USHORT  hres;   // # of pixels across screen
    USHORT  vres;   // # of scan lines down screen
    USHORT  wbytes; // # of bytes from start of one scan line to start of next
    ULONG   sbytes; // total size of addressable display memory in bytes
    ULONG   Frequency;         // Vertical Frequency
    ULONG   Interlaced;        // Determines if the mode is interlaced or not
    ULONG   MonitorType;       // Sets the desired vertical freq in an int10
    ULONG   MonTypeAX;         // Sets the desired horizontal freq in an int10
    ULONG   MonTypeBX;
    ULONG   MonTypeCX;
    BANK_TYPE banktype;        // NoBanking, NormalBanking, PlanarHCBanking
    VIDEO_MEMORY_MAP   MemMap; // index from VIDEO_MEMORY_MAP of memory
                               //  mapping used by this mode
    USHORT     ChipType;       // flags that say which chipset runs this mode
    USHORT     DisplayType;    // display type this mode runs on(crt or panel)
    BOOLEAN    ValidMode;      // TRUE if mode valid, FALSE if not

    CLMODE     BiosModes;
                              
//
// the mode will be TRUE if there is enough video memory to support the 
// mode, and the display type(it could be a panel), will support the mode.
// PANELS only support 640x480 for now. 
//
    CLCMD CmdStrings;   // pointer to array of register-setting commands to 
                        //  set up mode
} VIDEOMODE, *PVIDEOMODE;

//
// Mode into which to put the VGA before starting a VDM, so it's a plain
// vanilla VGA.  (This is the mode's index in ModesVGA[], currently standard
// 80x25 text mode.)
//

#define DEFAULT_MODE 0


//
// Info used by the Validator functions and save/restore code.
// Structure used to trap register accesses that must be done atomically.
//

#define VGA_MAX_VALIDATOR_DATA             100

#define VGA_VALIDATOR_UCHAR_ACCESS   1
#define VGA_VALIDATOR_USHORT_ACCESS  2
#define VGA_VALIDATOR_ULONG_ACCESS   3

typedef struct _VGA_VALIDATOR_DATA {
   ULONG Port;
   UCHAR AccessType;
   ULONG Data;
} VGA_VALIDATOR_DATA, *PVGA_VALIDATOR_DATA;

//
// Number of bytes to save in each plane.
//

#define VGA_PLANE_SIZE 0x10000
 
//
// Number of each type of indexed register in a standard VGA, used by
// validator and state save/restore functions.
//
// Note: VDMs currently only support basic VGAs only.
//

#define VGA_NUM_SEQUENCER_PORTS     5
#define VGA_NUM_CRTC_PORTS         25
#define VGA_NUM_GRAPH_CONT_PORTS    9
#define VGA_NUM_ATTRIB_CONT_PORTS  21
#define VGA_NUM_DAC_ENTRIES       256

#ifdef EXTENDED_REGISTER_SAVE_RESTORE

//
// Indices to start save/restore in extension registers:
// For both chip types

#define CL64xx_GRAPH_EXT_START          0x0b  // does not include ext. enable
#define CL64xx_GRAPH_EXT_END            0xFF

#define CL542x_GRAPH_EXT_START          0x09
#define CL542x_GRAPH_EXT_END            0x39
#define CL542x_SEQUENCER_EXT_START      0x07  // does not include ext. enable
#define CL542x_SEQUENCER_EXT_END        0x1F
#define CL542x_CRTC_EXT_START           0x19
#define CL542x_CRTC_EXT_END             0x1B

//
// Number of extended regs for both chip types
//

#define CL64xx_NUM_GRAPH_EXT_PORTS     (CL64xx_GRAPH_EXT_END - CL64xx_GRAPH_EXT_START + 1)

#define CL542x_NUM_GRAPH_EXT_PORTS     (CL542x_GRAPH_EXT_END - CL542x_GRAPH_EXT_START + 1)
#define CL542x_NUM_SEQUENCER_EXT_PORTS (CL542x_SEQUENCER_EXT_END - CL542x_SEQUENCER_EXT_START + 1)
#define CL542x_NUM_CRTC_EXT_PORTS      (CL542x_CRTC_EXT_END - CL542x_CRTC_EXT_START + 1)

//
// set values for save/restore area based on largest value for a chipset.
//

#define EXT_NUM_GRAPH_CONT_PORTS    ((CL64xx_NUM_GRAPH_EXT_PORTS >   \
                                     CL542x_NUM_GRAPH_EXT_PORTS) ?   \
                                     CL64xx_NUM_GRAPH_EXT_PORTS :    \
                                     CL542x_NUM_GRAPH_EXT_PORTS)
#define EXT_NUM_SEQUENCER_PORTS     CL542x_NUM_SEQUENCER_EXT_PORTS
#define EXT_NUM_CRTC_PORTS          CL542x_NUM_CRTC_EXT_PORTS
#define EXT_NUM_ATTRIB_CONT_PORTS   0
#define EXT_NUM_DAC_ENTRIES         0

#else 

#define EXT_NUM_GRAPH_CONT_PORTS    0
#define EXT_NUM_SEQUENCER_PORTS     0
#define EXT_NUM_CRTC_PORTS          0
#define EXT_NUM_ATTRIB_CONT_PORTS   0
#define EXT_NUM_DAC_ENTRIES         0

#endif        

//
// These constants determine the offsets within the
// VIDEO_HARDWARE_STATE_HEADER structure that are used to save and
// restore the VGA's state.
//

#define VGA_HARDWARE_STATE_SIZE sizeof(VIDEO_HARDWARE_STATE_HEADER)

#define VGA_BASIC_SEQUENCER_OFFSET (VGA_HARDWARE_STATE_SIZE + 0)
#define VGA_BASIC_CRTC_OFFSET (VGA_BASIC_SEQUENCER_OFFSET + \
         VGA_NUM_SEQUENCER_PORTS)
#define VGA_BASIC_GRAPH_CONT_OFFSET (VGA_BASIC_CRTC_OFFSET + \
         VGA_NUM_CRTC_PORTS)
#define VGA_BASIC_ATTRIB_CONT_OFFSET (VGA_BASIC_GRAPH_CONT_OFFSET + \
         VGA_NUM_GRAPH_CONT_PORTS)
#define VGA_BASIC_DAC_OFFSET (VGA_BASIC_ATTRIB_CONT_OFFSET + \
         VGA_NUM_ATTRIB_CONT_PORTS)
#define VGA_BASIC_LATCHES_OFFSET (VGA_BASIC_DAC_OFFSET + \
         (3 * VGA_NUM_DAC_ENTRIES))

#define VGA_EXT_SEQUENCER_OFFSET (VGA_BASIC_LATCHES_OFFSET + 4)
#define VGA_EXT_CRTC_OFFSET (VGA_EXT_SEQUENCER_OFFSET + \
         EXT_NUM_SEQUENCER_PORTS)
#define VGA_EXT_GRAPH_CONT_OFFSET (VGA_EXT_CRTC_OFFSET + \
         EXT_NUM_CRTC_PORTS)
#define VGA_EXT_ATTRIB_CONT_OFFSET (VGA_EXT_GRAPH_CONT_OFFSET +\
         EXT_NUM_GRAPH_CONT_PORTS)
#define VGA_EXT_DAC_OFFSET (VGA_EXT_ATTRIB_CONT_OFFSET + \
         EXT_NUM_ATTRIB_CONT_PORTS)

#define VGA_VALIDATOR_OFFSET (VGA_EXT_DAC_OFFSET + 4 * EXT_NUM_DAC_ENTRIES)

#define VGA_VALIDATOR_AREA_SIZE  sizeof (ULONG) + (VGA_MAX_VALIDATOR_DATA * \
                                 sizeof (VGA_VALIDATOR_DATA)) +             \
                                 sizeof (ULONG) +                           \
                                 sizeof (ULONG) +                           \
                                 sizeof (PVIDEO_ACCESS_RANGE)

#define VGA_MISC_DATA_AREA_OFFSET VGA_VALIDATOR_OFFSET + VGA_VALIDATOR_AREA_SIZE

#define VGA_MISC_DATA_AREA_SIZE  0

#define VGA_PLANE_0_OFFSET VGA_MISC_DATA_AREA_OFFSET + VGA_MISC_DATA_AREA_SIZE

#define VGA_PLANE_1_OFFSET VGA_PLANE_0_OFFSET + VGA_PLANE_SIZE
#define VGA_PLANE_2_OFFSET VGA_PLANE_1_OFFSET + VGA_PLANE_SIZE
#define VGA_PLANE_3_OFFSET VGA_PLANE_2_OFFSET + VGA_PLANE_SIZE

//
// Space needed to store all state data.
//

#define VGA_TOTAL_STATE_SIZE VGA_PLANE_3_OFFSET + VGA_PLANE_SIZE


//
// Device extension for the driver object.  This data is only used
// locally, so this structure can be added to as needed.
//

typedef struct _HW_DEVICE_EXTENSION {

    PUCHAR  IOAddress;            // base I/O address of VGA ports
    PUCHAR  VideoMemoryAddress;   // base virtual memory address of VGA memory
    ULONG   NumAvailableModes;    // number of available modes this session
    ULONG   ModeIndex;            // index of current mode in ModesVGA[]
    PVIDEOMODE  CurrentMode;      // pointer to VIDEOMODE structure for
                                  // current mode

    USHORT  FontPelColumns;          // Width of the font in pels
    USHORT  FontPelRows;          // height of the font in pels

    VIDEO_CURSOR_POSITION CursorPosition;  // current cursor position

                                 
    UCHAR CursorEnable;           // whether cursor is enabled or not
    UCHAR CursorTopScanLine;      // Cursor Start register setting (top scan)
    UCHAR CursorBottomScanLine;   // Cursor End register setting (bottom scan)
// add HW cursor data here
    PHYSICAL_ADDRESS PhysicalVideoMemoryBase; // physical memory address and
    ULONG PhysicalVideoMemoryLength;          // length of display memory
    PHYSICAL_ADDRESS PhysicalFrameOffset;   // physical memory address and
    ULONG PhysicalFrameLength;            // length of display memory for the
                                          // current mode.

    USHORT ChipType;                      // CL6410, CL6420, CL542x, or CL543x
    USHORT ChipRevision;                  // chip revision value
    USHORT DisplayType;                   // crt, panel or simulscan
    USHORT BoardType;                     // Diamond, etc ...
    ULONG AdapterMemorySize;              // amount of installed video ram

    //
    // These 4 fields must be at the end of the device extension and must be
    // kept in this order since this data will be copied to and from the save
    // state buffer that is passed to and from the VDM.
    //

    ULONG TrappedValidatorCount;   // number of entries in the Trapped
                                   // validator data Array.
    VGA_VALIDATOR_DATA TrappedValidatorData[VGA_MAX_VALIDATOR_DATA];
                                   // Data trapped by the validator routines
                                   // but not yet played back into the VGA
                                   // register.

    ULONG SequencerAddressValue;   // Determines if the Sequencer Address Port
                                   // is currently selecting the SyncReset data
                                   // register.

    ULONG CurrentNumVdmAccessRanges;           // Number of access ranges in
                                               // the access range array pointed
                                               // to by the next field
    PVIDEO_ACCESS_RANGE CurrentVdmAccessRange; // Access range currently
                                               // associated to the VDM

} HW_DEVICE_EXTENSION, *PHW_DEVICE_EXTENSION;


//
// Function prototypes.
//

//
// Entry points for the VGA validator. Used in VgaEmulatorAccessEntries[].
//

VP_STATUS
VgaValidatorUcharEntry (
    ULONG Context,
    ULONG Port,
    UCHAR AccessMode,
    PUCHAR Data
    );

VP_STATUS
VgaValidatorUshortEntry (
    ULONG Context,
    ULONG Port,
    UCHAR AccessMode,
    PUSHORT Data
    );

VP_STATUS
VgaValidatorUlongEntry (
    ULONG Context,
    ULONG Port,
    UCHAR AccessMode,
    PULONG Data
    );

BOOLEAN
VgaPlaybackValidatorData (
    PVOID Context
    );

#ifdef _X86_

//
// Bank switch code start and end labels, defined in CLHARD.ASM
//
// three versions for Cirrus Logic products
//

extern UCHAR CL64xxBankSwitchStart;
extern UCHAR CL64xxBankSwitchEnd;
extern UCHAR CL64xxPlanarHCBankSwitchStart;
extern UCHAR CL64xxPlanarHCBankSwitchEnd;
extern UCHAR CL64xxEnablePlanarHCStart;
extern UCHAR CL64xxEnablePlanarHCEnd;
extern UCHAR CL64xxDisablePlanarHCStart;
extern UCHAR CL64xxDisablePlanarHCEnd;

extern UCHAR CL542xBankSwitchStart;
extern UCHAR CL542xBankSwitchEnd;
extern UCHAR CL542xPlanarHCBankSwitchStart;
extern UCHAR CL542xPlanarHCBankSwitchEnd;
extern UCHAR CL542xEnablePlanarHCStart;
extern UCHAR CL542xEnablePlanarHCEnd;
extern UCHAR CL542xDisablePlanarHCStart;
extern UCHAR CL542xDisablePlanarHCEnd;

extern UCHAR CL543xBankSwitchStart;
extern UCHAR CL543xBankSwitchEnd;
extern UCHAR CL543xPlanarHCBankSwitchStart;
extern UCHAR CL543xPlanarHCBankSwitchEnd;

#endif

//
// Vga init scripts for font loading
//

extern USHORT EnableA000Data[];
extern USHORT DisableA000Color[];

//
// Mode Information
//

extern MEMORYMAPS MemoryMaps[];
extern ULONG NumVideoModes;
extern VIDEOMODE ModesVGA[];

#define NUM_VGA_ACCESS_RANGES  3
extern VIDEO_ACCESS_RANGE VgaAccessRange[];

#define VGA_NUM_EMULATOR_ACCESS_ENTRIES     6
extern EMULATOR_ACCESS_ENTRY VgaEmulatorAccessEntries[];

#define NUM_MINIMAL_VGA_VALIDATOR_ACCESS_RANGE 4
extern VIDEO_ACCESS_RANGE MinimalVgaValidatorAccessRange[];

#define NUM_FULL_VGA_VALIDATOR_ACCESS_RANGE 2
extern VIDEO_ACCESS_RANGE FullVgaValidatorAccessRange[];
