#include "controller_bridge.h"

/* Platform-specific serial implementations are in separate files:
 * - windows_serial.c for Windows
 * - posix_serial.c for Linux/macOS
 * 
 * This file exists to satisfy the build system and provide
 * a common compilation unit for serial port functionality.
 */

/* The actual implementations are in the platform-specific files */
