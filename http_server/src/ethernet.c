#include "ethernet.h"
#include "stm32h753xx.h"

#define NUM_TX_DESCRIPTORS 4
#define NUM_RX_DESCRIPTORS 4

typedef struct {
    volatile uint32_t tdes0; // Own bit, control, status
    volatile uint32_t tdes1; // Buffer sizes, control
    volatile uint32_t tdes2; // Buffer 1 address
    volatile uint32_t tdes3; // Buffer 2 address or next descriptor address
} eth_tx_desc_t;

typedef struct {
    volatile uint32_t rdes0; // Own bit, control, status
    volatile uint32_t rdes1; // Buffer sizes, control
    volatile uint32_t rdes2; // Buffer 1 address
    volatile uint32_t rdes3; // Buffer 2 address or next descriptor address
} eth_rx_desc_t;

eth_tx_desc_t tx_descriptors[4] __attribute__((aligned(4)));
eth_rx_desc_t rx_descriptors[4] __attribute__((aligned(4)));

uint8_t tx_buffers[NUM_TX_DESCRIPTORS][1536] __attribute__((aligned(4)));
uint8_t rx_buffers[NUM_RX_DESCRIPTORS][1536] __attribute__((aligned(4)));

// Track current descriptor positions
static uint32_t current_tx_desc = 0;
static uint32_t current_rx_desc = 0;

void ethernet_init(void) {
    RCC->AHB1ENR |= (1 << 17); // Ethernet RX enable
    RCC->AHB1ENR |= (1 << 16); // Ethernet TX enable
    RCC->AHB1ENR |= (1 << 15); // Ethernet enable

    RCC->AHB4ENR |= (1 << 0); // GPIOA enable
    RCC->AHB4ENR |= (1 << 2); // GPIOC enable
    RCC->AHB4ENR |= (1 << 6); // GPIOG enable

    // Configure GPIO pins for RMII interface (Alternate Function 11)
    // PA1 - ETH_REF_CLK (50MHz reference clock input from PHY)
    GPIOA->MODER &= ~(3 << (1 * 2));     // Clear PA1 mode bits (bits 3:2)
    GPIOA->MODER |= (2 << (1 * 2));      // Set PA1 to alternate function mode (10b)
    GPIOA->AFR[0] &= ~(0xF << (1 * 4));  // Clear PA1 alternate function bits (7:4)
    GPIOA->AFR[0] |= (11 << (1 * 4));    // Set PA1 to AF11 (Ethernet)
    GPIOA->OSPEEDR |= (3 << (1 * 2));    // Set PA1 to very high speed (11b)
    
    // PA2 - ETH_MDIO (Management Data I/O - bidirectional PHY control)
    GPIOA->MODER &= ~(3 << (2 * 2));     // Clear PA2 mode bits (bits 5:4)
    GPIOA->MODER |= (2 << (2 * 2));      // Set PA2 to alternate function mode
    GPIOA->AFR[0] &= ~(0xF << (2 * 4));  // Clear PA2 alternate function bits (11:8)
    GPIOA->AFR[0] |= (11 << (2 * 4));    // Set PA2 to AF11 (Ethernet)
    GPIOA->OSPEEDR |= (3 << (2 * 2));    // Set PA2 to very high speed
    
    // PA7 - ETH_CRS_DV (Carrier Sense/Data Valid - indicates valid data)
    GPIOA->MODER &= ~(3 << (7 * 2));     // Clear PA7 mode bits (bits 15:14)
    GPIOA->MODER |= (2 << (7 * 2));      // Set PA7 to alternate function mode
    GPIOA->AFR[0] &= ~(0xF << (7 * 4));  // Clear PA7 alternate function bits (31:28)
    GPIOA->AFR[0] |= (11 << (7 * 4));    // Set PA7 to AF11 (Ethernet)
    GPIOA->OSPEEDR |= (3 << (7 * 2));    // Set PA7 to very high speed
    
    // PC1 - ETH_MDC (Management Data Clock - output clock for PHY control)
    GPIOC->MODER &= ~(3 << (1 * 2));     // Clear PC1 mode bits (bits 3:2)
    GPIOC->MODER |= (2 << (1 * 2));      // Set PC1 to alternate function mode
    GPIOC->AFR[0] &= ~(0xF << (1 * 4));  // Clear PC1 alternate function bits (7:4)
    GPIOC->AFR[0] |= (11 << (1 * 4));    // Set PC1 to AF11 (Ethernet)
    GPIOC->OSPEEDR |= (3 << (1 * 2));    // Set PC1 to very high speed
    
    // PC4 - ETH_RXD0 (Receive Data bit 0 - input from PHY)
    GPIOC->MODER &= ~(3 << (4 * 2));     // Clear PC4 mode bits (bits 9:8)
    GPIOC->MODER |= (2 << (4 * 2));      // Set PC4 to alternate function mode
    GPIOC->AFR[0] &= ~(0xF << (4 * 4));  // Clear PC4 alternate function bits (19:16)
    GPIOC->AFR[0] |= (11 << (4 * 4));    // Set PC4 to AF11 (Ethernet)
    GPIOC->OSPEEDR |= (3 << (4 * 2));    // Set PC4 to very high speed
    
    // PC5 - ETH_RXD1 (Receive Data bit 1 - input from PHY)
    GPIOC->MODER &= ~(3 << (5 * 2));     // Clear PC5 mode bits (bits 11:10)
    GPIOC->MODER |= (2 << (5 * 2));      // Set PC5 to alternate function mode
    GPIOC->AFR[0] &= ~(0xF << (5 * 4));  // Clear PC5 alternate function bits (23:20)
    GPIOC->AFR[0] |= (11 << (5 * 4));    // Set PC5 to AF11 (Ethernet)
    GPIOC->OSPEEDR |= (3 << (5 * 2));    // Set PC5 to very high speed
    
    // PG11 - ETH_TX_EN (Transmit Enable - output to PHY to enable transmission)
    GPIOG->MODER &= ~(3 << (11 * 2));    // Clear PG11 mode bits (bits 23:22)
    GPIOG->MODER |= (2 << (11 * 2));     // Set PG11 to alternate function mode
    GPIOG->AFR[1] &= ~(0xF << ((11 - 8) * 4)); // Clear PG11 AF bits in AFRH (15:12)
    GPIOG->AFR[1] |= (11 << ((11 - 8) * 4));   // Set PG11 to AF11 (Ethernet)
    GPIOG->OSPEEDR |= (3 << (11 * 2));   // Set PG11 to very high speed
    
    // PG13 - ETH_TXD0 (Transmit Data bit 0 - output to PHY)
    GPIOG->MODER &= ~(3 << (13 * 2));    // Clear PG13 mode bits (bits 27:26)
    GPIOG->MODER |= (2 << (13 * 2));     // Set PG13 to alternate function mode
    GPIOG->AFR[1] &= ~(0xF << ((13 - 8) * 4)); // Clear PG13 AF bits in AFRH (23:20)
    GPIOG->AFR[1] |= (11 << ((13 - 8) * 4));   // Set PG13 to AF11 (Ethernet)
    GPIOG->OSPEEDR |= (3 << (13 * 2));   // Set PG13 to very high speed
    
    // PG14 - ETH_TXD1 (Transmit Data bit 1 - output to PHY)
    GPIOG->MODER &= ~(3 << (14 * 2));    // Clear PG14 mode bits (bits 29:28)
    GPIOG->MODER |= (2 << (14 * 2));     // Set PG14 to alternate function mode
    GPIOG->AFR[1] &= ~(0xF << ((14 - 8) * 4)); // Clear PG14 AF bits in AFRH (27:24)
    GPIOG->AFR[1] |= (11 << ((14 - 8) * 4));   // Set PG14 to AF11 (Ethernet)
    GPIOG->OSPEEDR |= (3 << (14 * 2));   // Set PG14 to very high speed

    ETH->DMAMR |= (1 << 0); // Software reset
    while (ETH->DMAMR & (1 << 0)); // Wait for reset to clear

    ETH->DMAMR &= ~(7 << 12); // 1:1 priority ratio
    ETH->DMAMR &= ~(3 << 16); // Normal interrupt mode
    ETH->DMAMR &= ~(1 << 11); // No transmit priority
    ETH->DMAMR &= ~(1 << 1); // Weighted round robin
    ETH->DMAMR &= ~(1 << 0); // Clear software reset

    ETH->DMACTCR |= (1 << 20); // 16 bytes burst length
    ETH->DMACTCR |= (1 << 0); // Start DMA transmission

    ETH->DMACRCR |= (1 << 20); // 16 bytes burst length
    ETH->DMACRCR |= (1536 << 1); // Buffer size 1536 bytes 
    ETH->DMACRCR |= (1 << 0); // Start DMA reception 

    ETH->DMACTDLAR = (uint32_t)(tx_descriptors); // Transmit descriptor list address
    ETH->DMACRDLAR = (uint32_t)(rx_descriptors); // Receive descriptor list address
    ETH->DMACTDTPR = (uint32_t)(&tx_descriptors[NUM_TX_DESCRIPTORS - 1]); // Transmit descriptor tail pointer
    ETH->DMACRDTPR = (uint32_t)(&rx_descriptors[NUM_RX_DESCRIPTORS - 1]); // Receive descriptor tail pointer
    ETH->DMACTDRLR = NUM_TX_DESCRIPTORS; // Transmit descriptor ring length
    ETH->DMACRDRLR = NUM_RX_DESCRIPTORS; // Receive descriptor ring length

    ETH->DMACIER |= (1 << 6); // Receive interrupt enable 
    ETH->DMACIER |= (1 << 0); // Transmit interrupt enable

    ETH->MTLOMR |= (1 << 9); // Reset counters
    
    ETH->MTLTQOMR |= (7 << 16); // 2048 bytes threshold
    ETH->MTLTQOMR |= (2 << 2); // Transmit queue enabled
    ETH->MTLTQOMR |= (1 << 0); // Transmit store and forward

    ETH->MTLRQOMR |= (7 << 16); // 2048 bytes threshold
    ETH->MTLRQOMR |= (1 << 5); // Recieve queue store and forward

    // Initialize TX descriptors
    for (int i = 0; i < NUM_TX_DESCRIPTORS; i++) {
        tx_descriptors[i].tdes0 = 0; // Clear status/control
        tx_descriptors[i].tdes1 = 0; // Clear buffer sizes
        tx_descriptors[i].tdes2 = (uint32_t)&tx_buffers[i][0]; // Buffer 1 address
        tx_descriptors[i].tdes3 = (uint32_t)&tx_descriptors[(i + 1) % NUM_TX_DESCRIPTORS]; // Next descriptor
    }
    
    // Initialize RX descriptors
    for (int i = 0; i < NUM_RX_DESCRIPTORS; i++) {
        rx_descriptors[i].rdes0 = (1 << 31); // Set OWN bit (DMA owns descriptor)
        rx_descriptors[i].rdes1 = 0; // Clear control
        rx_descriptors[i].rdes2 = (uint32_t)&rx_buffers[i][0]; // Buffer 1 address
        rx_descriptors[i].rdes3 = (uint32_t)&rx_descriptors[(i + 1) % NUM_RX_DESCRIPTORS]; // Next descriptor
    }

    // Set MAC address (00:80:E1:00:00:00 - default test address)
    ETH->MACA0HR = 0x000080E1; // High part: bytes 4-5
    ETH->MACA0LR = 0x00000000; // Low part: bytes 0-3

    // Configure packet filtering (promiscuous mode for now)
    ETH->MACPFR = (1 << 0); // Promiscuous mode - accept all packets

    ETH->MACCR |= (1 << 27); // Checksum offload enable
    ETH->MACCR |= (1 << 20); // Autopadding/CRC stripping enable
    ETH->MACCR |= (1 << 14); // Speed 100Mbps
    ETH->MACCR |= (1 << 13); // Full duplex mode
    ETH->MACCR |= (1 << 1); // Transmitter enable
    ETH->MACCR |= (1 << 0); // Receiver enable
}

void ethernet_transfer(const uint8_t *tx_buf, uint8_t *rx_buf, uint32_t len) {
    // 1. Send packet using current TX descriptor
    // Copy data to TX buffer
    for (uint32_t i = 0; i < len && i < 1536; i++) {
        tx_buffers[current_tx_desc][i] = tx_buf[i];
    }
    
    // Set up TX descriptor
    tx_descriptors[current_tx_desc].tdes0 = (1 << 31) | (1 << 30); // OWN + First descriptor
    tx_descriptors[current_tx_desc].tdes1 = len; // Buffer length
    tx_descriptors[current_tx_desc].tdes2 = (uint32_t)&tx_buffers[current_tx_desc][0];
    
    // Advance TX tail pointer to trigger transmission
    ETH->DMACTDTPR = (uint32_t)&tx_descriptors[current_tx_desc];
    current_tx_desc = (current_tx_desc + 1) % NUM_TX_DESCRIPTORS;
    
    // 2. Wait for response (simple polling - no timeout for now)
    while (1) {
        // Check if current RX descriptor has packet ready (OWN bit clear)
        if ((rx_descriptors[current_rx_desc].rdes0 & (1 << 31)) == 0) {
            // Get packet length from status
            uint32_t packet_len = (rx_descriptors[current_rx_desc].rdes0 >> 16) & 0x3FFF;
            
            // Copy received packet to output buffer
            for (uint32_t i = 0; i < packet_len && i < 1536; i++) {
                rx_buf[i] = rx_buffers[current_rx_desc][i];
            }
            
            // Give descriptor back to DMA
            rx_descriptors[current_rx_desc].rdes0 = (1 << 31); // Set OWN bit
            ETH->DMACRDTPR = (uint32_t)&rx_descriptors[current_rx_desc];
            current_rx_desc = (current_rx_desc + 1) % NUM_RX_DESCRIPTORS;
            
            return; // Got a packet, we're done
        }
    }
}

void ethernet_rx_handler(void) {
    // Check if packet is ready
    if ((rx_descriptors[current_rx_desc].rdes0 & (1 << 31)) == 0) {
        // Packet received - you can process it here
        // For now, just give descriptor back to DMA
        rx_descriptors[current_rx_desc].rdes0 = (1 << 31); // Set OWN bit
        ETH->DMACRDTPR = (uint32_t)&rx_descriptors[current_rx_desc];
        current_rx_desc = (current_rx_desc + 1) % NUM_RX_DESCRIPTORS;
    }
}
