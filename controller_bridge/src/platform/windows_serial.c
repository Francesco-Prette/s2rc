#ifdef _WIN32

#include "controller_bridge.h"
#include <windows.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    HANDLE handle;
    bool is_open;
} windows_serial_t;

serial_port_t serial_open(const char *port_name, int baud_rate) {
    windows_serial_t *port = malloc(sizeof(windows_serial_t));
    if (!port) {
        return NULL;
    }
    
    port->is_open = false;
    
    /* Format port name for Windows (add \\.\  prefix if not present) */
    char formatted_port[256];
    if (strncmp(port_name, "\\\\.\\", 4) != 0) {
        snprintf(formatted_port, sizeof(formatted_port), "\\\\.\\%s", port_name);
    } else {
        strncpy(formatted_port, port_name, sizeof(formatted_port) - 1);
        formatted_port[sizeof(formatted_port) - 1] = '\0';
    }
    
    /* Open COM port */
    port->handle = CreateFileA(
        formatted_port,
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        0,
        NULL
    );
    
    if (port->handle == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "Error: Could not open serial port %s\n", port_name);
        fprintf(stderr, "Error code: %lu\n", GetLastError());
        free(port);
        return NULL;
    }
    
    /* Configure port */
    DCB dcb = { 0 };
    dcb.DCBlength = sizeof(DCB);
    
    if (!GetCommState(port->handle, &dcb)) {
        fprintf(stderr, "Error: Could not get COM state\n");
        CloseHandle(port->handle);
        free(port);
        return NULL;
    }
    
    dcb.BaudRate = baud_rate;
    dcb.ByteSize = 8;
    dcb.StopBits = ONESTOPBIT;
    dcb.Parity = NOPARITY;
    dcb.fBinary = TRUE;
    dcb.fParity = FALSE;
    dcb.fOutxCtsFlow = FALSE;
    dcb.fOutxDsrFlow = FALSE;
    dcb.fDtrControl = DTR_CONTROL_ENABLE;  /* Enable DTR to properly reset Pico */
    dcb.fDsrSensitivity = FALSE;
    dcb.fTXContinueOnXoff = FALSE;
    dcb.fOutX = FALSE;
    dcb.fInX = FALSE;
    dcb.fErrorChar = FALSE;
    dcb.fNull = FALSE;
    dcb.fRtsControl = RTS_CONTROL_ENABLE;  /* Enable RTS to match Python's serial.Serial() behavior */
    dcb.fAbortOnError = FALSE;
    
    if (!SetCommState(port->handle, &dcb)) {
        fprintf(stderr, "Error: Could not set COM state\n");
        CloseHandle(port->handle);
        free(port);
        return NULL;
    }
    
    /* Set timeouts - make writes non-blocking for better performance */
    COMMTIMEOUTS timeouts = { 0 };
    timeouts.ReadIntervalTimeout = 50;
    timeouts.ReadTotalTimeoutConstant = 50;
    timeouts.ReadTotalTimeoutMultiplier = 10;
    timeouts.WriteTotalTimeoutConstant = 0;  /* Non-blocking writes */
    timeouts.WriteTotalTimeoutMultiplier = 0;
    
    if (!SetCommTimeouts(port->handle, &timeouts)) {
        fprintf(stderr, "Error: Could not set COM timeouts\n");
        CloseHandle(port->handle);
        free(port);
        return NULL;
    }
    
    /* Allow Pico to reset and stabilize (DTR/RTS signals may trigger reset) */
    Sleep(2000);  /* 2 seconds, matching Python's time.sleep(2) */
    
    port->is_open = true;
    return (serial_port_t)port;
}

void serial_close(serial_port_t port) {
    if (!port) return;
    
    windows_serial_t *win_port = (windows_serial_t *)port;
    
    if (win_port->is_open && win_port->handle != INVALID_HANDLE_VALUE) {
        CloseHandle(win_port->handle);
    }
    
    free(win_port);
}

bool serial_write(serial_port_t port, const uint8_t *data, size_t len) {
    if (!port) return false;
    
    windows_serial_t *win_port = (windows_serial_t *)port;
    
    if (!win_port->is_open) return false;
    
    DWORD bytes_written;
    if (!WriteFile(win_port->handle, data, (DWORD)len, &bytes_written, NULL)) {
        DWORD error = GetLastError();
        fprintf(stderr, "WriteFile failed with error code: %lu\n", error);
        return false;
    }
    
    if (bytes_written != len) {
        fprintf(stderr, "WriteFile partial write: %lu of %zu bytes\n", bytes_written, len);
        return false;
    }
    
    
    return true;
}

bool serial_is_open(serial_port_t port) {
    if (!port) return false;
    windows_serial_t *win_port = (windows_serial_t *)port;
    return win_port->is_open;
}

#endif /* _WIN32 */
