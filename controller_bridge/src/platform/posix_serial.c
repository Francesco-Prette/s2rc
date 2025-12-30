#if defined(__unix__) || defined(__APPLE__)

#include "controller_bridge.h"
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    int fd;
    bool is_open;
} posix_serial_t;

serial_port_t serial_open(const char *port_name, int baud_rate) {
    posix_serial_t *port = malloc(sizeof(posix_serial_t));
    if (!port) {
        return NULL;
    }
    
    port->is_open = false;
    
    /* Open serial port */
    port->fd = open(port_name, O_RDWR | O_NOCTTY | O_NDELAY);
    if (port->fd == -1) {
        fprintf(stderr, "Error: Could not open serial port %s\n", port_name);
        perror("open");
        free(port);
        return NULL;
    }
    
    /* Configure port */
    struct termios options;
    if (tcgetattr(port->fd, &options) != 0) {
        fprintf(stderr, "Error: Could not get terminal attributes\n");
        close(port->fd);
        free(port);
        return NULL;
    }
    
    /* Set baud rate */
    speed_t speed;
    switch (baud_rate) {
        case 9600:   speed = B9600;   break;
        case 19200:  speed = B19200;  break;
        case 38400:  speed = B38400;  break;
        case 57600:  speed = B57600;  break;
        case 115200: speed = B115200; break;
        case 230400: speed = B230400; break;
        default:     speed = B115200; break;
    }
    
    cfsetispeed(&options, speed);
    cfsetospeed(&options, speed);
    
    /* 8N1 mode */
    options.c_cflag &= ~PARENB;  /* No parity */
    options.c_cflag &= ~CSTOPB;  /* 1 stop bit */
    options.c_cflag &= ~CSIZE;
    options.c_cflag |= CS8;      /* 8 data bits */
    
    /* No flow control */
    options.c_cflag &= ~CRTSCTS;
    
    /* Enable receiver, ignore modem control lines */
    options.c_cflag |= (CLOCAL | CREAD);
    
    /* Raw input */
    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    
    /* Raw output */
    options.c_oflag &= ~OPOST;
    
    /* No input processing */
    options.c_iflag &= ~(IXON | IXOFF | IXANY);
    options.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL);
    
    /* Set timeout */
    options.c_cc[VMIN] = 0;
    options.c_cc[VTIME] = 1;  /* 0.1 seconds */
    
    /* Apply settings */
    if (tcsetattr(port->fd, TCSANOW, &options) != 0) {
        fprintf(stderr, "Error: Could not set terminal attributes\n");
        close(port->fd);
        free(port);
        return NULL;
    }
    
    /* Flush any pending data */
    tcflush(port->fd, TCIOFLUSH);
    
    /* Allow Pico to reset and stabilize */
    sleep(2);  /* 2 seconds, matching Python's time.sleep(2) */
    
    port->is_open = true;
    return (serial_port_t)port;
}

void serial_close(serial_port_t port) {
    if (!port) return;
    
    posix_serial_t *posix_port = (posix_serial_t *)port;
    
    if (posix_port->is_open && posix_port->fd >= 0) {
        close(posix_port->fd);
    }
    
    free(posix_port);
}

bool serial_write(serial_port_t port, const uint8_t *data, size_t len) {
    if (!port) return false;
    
    posix_serial_t *posix_port = (posix_serial_t *)port;
    
    if (!posix_port->is_open || posix_port->fd < 0) return false;
    
    ssize_t bytes_written = write(posix_port->fd, data, len);
    
    if (bytes_written < 0) {
        return false;
    }
    
    /* Flush to ensure immediate transmission */
    tcdrain(posix_port->fd);
    
    return (size_t)bytes_written == len;
}

bool serial_is_open(serial_port_t port) {
    if (!port) return false;
    posix_serial_t *posix_port = (posix_serial_t *)port;
    return posix_port->is_open;
}

#endif /* __unix__ || __APPLE__ */
