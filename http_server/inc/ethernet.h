// Public Ethernet interface (MCU-agnostic)
#ifndef ETHERNET_H
#define ETHERNET_H

#include <stdint.h>

// Initialize the PHY clock, pins, etc.
void ethernet_init(void);

// Transfer data over PHY interface; full-duplex requires tx and rx buffers
void ethernet_transfer(const uint8_t *tx_buf, uint8_t *rx_buf, uint32_t len);

// Receive handler flag
void ethernet_rx_handler(void);

#endif // ETHERNET_H
