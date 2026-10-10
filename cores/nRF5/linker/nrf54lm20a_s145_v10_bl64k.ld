/* nRF54LM20A application with the s145 v10.0.1 SoftDevice, behind a 64 KB
 * bootloader (USB DFU, as on the nRF54LM20-DK).
 *
 * Same as nrf54lm20a_s145_v10.ld except for where the application starts:
 * the USB stack does not fit in a 32 KB bootloader. Keep the rest of the two
 * in step.
 *
 * The layout is a contract with linker/nrf54lm20a_bl64k.ld in nRF54_Bootloader
 * (CODE_REGION_1_START=0x10000 in the board's board.cmake) and has to change
 * together with it. s145 has no MBR, so the bootloader owns RRAM 0x0 and the
 * reset vector:
 *
 *   0x000000 - 0x010000  bootloader (63 KB code + 1 KB config)
 *   0x010000 - 0x1C9000  application: this image, own vector table
 *   0x1C9000 - 0x1D1000  InternalFS / LittleFS (32 KB, __flash_arduino_size)
 *   0x1D1000 - 0x1D2000  bootloader settings page
 *   0x1D2000 - 0x1DA800  unused
 *   0x1DA800 - 0x1FCC00  s145 v10.0.1; RRAM is 0x1FD000 (2036 KB)
 */
SEARCH_DIR(.)
GROUP(-lgcc -lc -lnosys)

MEMORY
{
  FLASH (rx) : ORIGIN = 0x00010000, LENGTH = 0x1B9000

  /* 0x20000000 - 0x20004800 belongs to the SoftDevice (s145 v10.0.1 needs
   * 0x1480 minimum, the rest covers sd_ble_enable()), the application runs
   * from 0x20004800 to 0x2003FF80, and the last 128 bytes of the first RAM
   * bank are the bootloader handover area below.
   *
   * The LM20A has a second 256 KB bank at 0x20040000, which this script does
   * not use. The bootloader only uses the first bank and keeps its handover
   * area at the top of it, so application RAM spanning both would load and
   * zero straight over that area. The second bank's top is also not usable:
   * measured on a XIAO nRF54LM20A over SWD, 0x2007FEE0 and above bus-faults
   * for both the debugger and the CPU, and a __StackTop at a nominal
   * 0x20080000 locks the core up on the first push in SystemInit. */
  RAM (rwx) :  ORIGIN = 0x20004800, LENGTH = 0x3B780

  /* Bootloader handover area, which the application must never load or zero:
   * 60 bytes of DFU peer data at 0x2003FF80 followed by its 16-bit CRC
   * (BLEDfu.cpp writes them, dfu_ble_svc.c in the bootloader reads them), and
   * directly above, at 0x2003FFF8, the bootloader's double-reset word. */
  NOINIT (rwx) :  ORIGIN = 0x2003FF80, LENGTH = 0x78
}

/* InternalFS size; see __flash_arduino_size in nrf54_common.ld. */
__flash_arduino_size = 0x8000;

/* Base of the SoftDevice image; see __softdevice_start__ in nrf54_common.ld. */
__softdevice_start__ = 0x001DA800;

INCLUDE "nrf54_common.ld"
