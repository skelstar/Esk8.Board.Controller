#ifndef __RF24_H__
#define __RF24_H__

#include "RF24_config.h"

#if defined(RF24_LINUX) || defined(LITTLEWIRE)
#include "utility/includes.h"
#elif defined SOFTSPI
#include <DigitalIO.h>
#endif

typedef enum
{
    RF24_PA_MIN = 0,
    RF24_PA_LOW,
    RF24_PA_HIGH,
    RF24_PA_MAX,
    RF24_PA_ERROR
} rf24_pa_dbm_e;

typedef enum
{
    RF24_1MBPS = 0,
    RF24_2MBPS,
    RF24_250KBPS
} rf24_datarate_e;

typedef enum
{
    RF24_CRC_DISABLED = 0,
    RF24_CRC_8,
    RF24_CRC_16
} rf24_crclength_e;

/**
 * Driver for nRF24L01(+) 2.4GHz Wireless Transceiver
 */

class RF24
{
private:
#ifdef SOFTSPI
    SoftSPI<SOFT_SPI_MISO_PIN, SOFT_SPI_MOSI_PIN, SOFT_SPI_SCK_PIN, SPI_MODE> spi;
#elif defined(SPI_UART)
    SPIUARTClass uspi;
#endif

    uint16_t sclk_pin;
    uint16_t miso_pin;
    uint16_t mosi_pin;
    uint16_t ce_pin;    /**< "Chip Enable" pin, activates the RX or TX role */
    uint16_t csn_pin;   /**< SPI Chip select */
    uint16_t spi_speed; /**< SPI Bus Speed */
#if defined(RF24_LINUX) || defined(XMEGA_D3)
    uint8_t spi_rxbuff[32 + 1]; //SPI receive buffer (payload max 32 bytes)
    uint8_t spi_txbuff[32 + 1]; //SPI transmit buffer (payload max 32 bytes + 1 byte for the command)
#endif
    bool p_variant;                   /* False for RF24L01 and true for RF24L01P */
    uint8_t payload_size;             /**< Fixed size of payloads */
    bool dynamic_payloads_enabled;    /**< Whether dynamic payloads are enabled. */
    uint8_t pipe0_reading_address[5]; /**< Last address set on pipe 0 for reading. */
    uint8_t addr_width;               /**< The address width to use - 3,4 or 5 bytes. */

protected:
    inline void beginTransaction();

    inline void endTransaction();

public:
    RF24(uint16_t _cepin, uint16_t _cspin);
    RF24(uint16_t _sclkpin, uint16_t _misopin, uint16_t _mosipin, uint16_t _cepin, uint16_t _cspin);

    bool begin(void);

    bool isChipConnected();

    void startListening(void);

    void stopListening(void);

    bool available(void);

    void read(void *buf, uint8_t len);

    bool write(const void *buf, uint8_t len);

    void openWritingPipe(const uint8_t *address);

    void openReadingPipe(uint8_t number, const uint8_t *address);

    void printDetails(void);

    bool available(uint8_t *pipe_num);

    bool rxFifoFull();

    void powerDown(void);

    void powerUp(void);

    bool write(const void *buf, uint8_t len, const bool multicast);

    bool writeFast(const void *buf, uint8_t len);

    bool writeFast(const void *buf, uint8_t len, const bool multicast);

    bool writeBlocking(const void *buf, uint8_t len, uint32_t timeout);

    bool txStandBy();

    bool txStandBy(uint32_t timeout, bool startTx = 0);

    void writeAckPayload(uint8_t pipe, const void *buf, uint8_t len);

    bool isAckPayloadAvailable(void);

    void whatHappened(bool &tx_ok, bool &tx_fail, bool &rx_ready);

    void startFastWrite(const void *buf, uint8_t len, const bool multicast, bool startTx = 1);

    void startWrite(const void *buf, uint8_t len, const bool multicast);

    void reUseTX();

    uint8_t flush_tx(void);

    bool testCarrier(void);

    bool testRPD(void);

    bool isValid()
    {
        return ce_pin != 0xff && csn_pin != 0xff;
    }

    void closeReadingPipe(uint8_t pipe);

    bool failureDetected;

    void setAddressWidth(uint8_t a_width);

    void setRetries(uint8_t delay, uint8_t count);

    void setChannel(uint8_t channel);

    uint8_t getChannel(void);

    void setPayloadSize(uint8_t size);

    uint8_t getPayloadSize(void);
    uint8_t getDynamicPayloadSize(void);

    void enableAckPayload(void);

    void enableDynamicPayloads(void);

    void disableDynamicPayloads(void);

    void enableDynamicAck();

    bool isPVariant(void);

    void setAutoAck(bool enable);

    void setAutoAck(uint8_t pipe, bool enable);

    void setPALevel(uint8_t level);

    uint8_t getPALevel(void);

    /**
     * Returns automatic retransmission count (ARC_CNT)
     *
     * Value resets with each new transmission. Allows roughly estimating signal strength.
     *
     * @return Returns values from 0 to 15.
     */
    uint8_t getARC(void);

    /**
    * Set the transmission data rate
    *
    * @warning setting RF24_250KBPS will fail for non-plus units
    *
    * @param speed RF24_250KBPS for 250kbs, RF24_1MBPS for 1Mbps, or RF24_2MBPS for 2Mbps
    * @return true if the change was successful
    */
    bool setDataRate(rf24_datarate_e speed);

    /**
     * Fetches the transmission data rate
     *
     * @return Returns the hardware's currently configured datarate. The value
     * is one of 250kbs, RF24_1MBPS for 1Mbps, or RF24_2MBPS, as defined in the
     * rf24_datarate_e enum.
     */
    rf24_datarate_e getDataRate(void);

    /**
     * Set the CRC length
     * <br>CRC checking cannot be disabled if auto-ack is enabled
     * @param length RF24_CRC_8 for 8-bit or RF24_CRC_16 for 16-bit
     */
    void setCRCLength(rf24_crclength_e length);

    /**
     * Get the CRC length
     * <br>CRC checking cannot be disabled if auto-ack is enabled
     * @return RF24_CRC_DISABLED if disabled or RF24_CRC_8 for 8-bit or RF24_CRC_16 for 16-bit
     */
    rf24_crclength_e getCRCLength(void);

    /**
     * Disable CRC validation
     *
     * @warning CRC cannot be disabled if auto-ack/ESB is enabled.
     */
    void disableCRC(void);

    /**
    * The radio will generate interrupt signals when a transmission is complete,
    * a transmission fails, or a payload is received. This allows users to mask
    * those interrupts to prevent them from generating a signal on the interrupt
    * pin. Interrupts are enabled on the radio chip by default.
    *
    * @code
    * 	Mask all interrupts except the receive interrupt:
    *
    *		radio.maskIRQ(1,1,0);
    * @endcode
    *
    * @param tx_ok  Mask transmission complete interrupts
    * @param tx_fail  Mask transmit failure interrupts
    * @param rx_ready Mask payload received interrupts
    */
    void maskIRQ(bool tx_ok, bool tx_fail, bool rx_ready);

    /**
    *
    * The driver will delay for this duration when stopListening() is called
    *
    * When responding to payloads, faster devices like ARM(RPi) are much faster than Arduino:
    * 1. Arduino sends data to RPi, switches to RX mode
    * 2. The RPi receives the data, switches to TX mode and sends before the Arduino radio is in RX mode
    * 3. If AutoACK is disabled, this can be set as low as 0. If AA/ESB enabled, set to 100uS minimum on RPi
    *
    * @warning If set to 0, ensure 130uS delay after stopListening() and before any sends
    */

    uint32_t txDelay;

    /**
    *
    * On all devices but Linux and ATTiny, a small delay is added to the CSN toggling function
    *
    * This is intended to minimise the speed of SPI polling due to radio commands
    *
    * If using interrupts or timed requests, this can be set to 0 Default:5
    */

    uint32_t csDelay;

    /**@}*/
    /**
     * @name Deprecated
     *
     *  Methods provided for backwards compabibility.
     */
    /**@{*/

    /**
     * Open a pipe for reading
     * @note For compatibility with old code only, see new function
     *
     * @warning Pipes 1-5 should share the first 32 bits.
     * Only the least significant byte should be unique, e.g.
     * @code
     *   openReadingPipe(1,0xF0F0F0F0AA);
     *   openReadingPipe(2,0xF0F0F0F066);
     * @endcode
     *
     * @warning Pipe 0 is also used by the writing pipe.  So if you open
     * pipe 0 for reading, and then startListening(), it will overwrite the
     * writing pipe.  Ergo, do an openWritingPipe() again before write().
     *
     * @param number Which pipe# to open, 0-5.
     * @param address The 40-bit address of the pipe to open.
     */
    void openReadingPipe(uint8_t number, uint64_t address);

    /**
     * Open a pipe for writing
     * @note For compatibility with old code only, see new function
     *
     * Addresses are 40-bit hex values, e.g.:
     *
     * @code
     *   openWritingPipe(0xF0F0F0F0F0);
     * @endcode
     *
     * @param address The 40-bit address of the pipe to open.
     */
    void openWritingPipe(uint64_t address);

    /**
     * Empty the receive buffer
     *
     * @return Current value of status register
     */
    uint8_t flush_rx(void);

private:
    /**
     * @name Low-level internal interface.
     *
     *  Protected methods that address the chip directly.  Regular users cannot
     *  ever call these.  They are documented for completeness and for developers who
     *  may want to extend this class.
     */
    /**@{*/

    /**
     * Set chip select pin
     *
     * Running SPI bus at PI_CLOCK_DIV2 so we don't waste time transferring data
     * and best of all, we make use of the radio's FIFO buffers. A lower speed
     * means we're less likely to effectively leverage our FIFOs and pay a higher
     * AVR runtime cost as toll.
     *
     * @param mode HIGH to take this unit off the SPI bus, LOW to put it on
     */
    void csn(bool mode);

    /**
     * Set chip enable
     *
     * @param level HIGH to actively begin transmission or LOW to put in standby.  Please see data sheet
     * for a much more detailed description of this pin.
     */
    void ce(bool level);

    /**
     * Read a chunk of data in from a register
     *
     * @param reg Which register. Use constants from nRF24L01.h
     * @param buf Where to put the data
     * @param len How many bytes of data to transfer
     * @return Current value of status register
     */
    uint8_t read_register(uint8_t reg, uint8_t *buf, uint8_t len);

    /**
     * Read single byte from a register
     *
     * @param reg Which register. Use constants from nRF24L01.h
     * @return Current value of register @p reg
     */
    uint8_t read_register(uint8_t reg);

    /**
     * Write a chunk of data to a register
     *
     * @param reg Which register. Use constants from nRF24L01.h
     * @param buf Where to get the data
     * @param len How many bytes of data to transfer
     * @return Current value of status register
     */
    uint8_t write_register(uint8_t reg, const uint8_t *buf, uint8_t len);

    /**
     * Write a single byte to a register
     *
     * @param reg Which register. Use constants from nRF24L01.h
     * @param value The new value to write
     * @return Current value of status register
     */
    uint8_t write_register(uint8_t reg, uint8_t value);

    /**
     * Write the transmit payload
     *
     * The size of data written is the fixed payload size, see getPayloadSize()
     *
     * @param buf Where to get the data
     * @param len Number of bytes to be sent
     * @return Current value of status register
     */
    uint8_t write_payload(const void *buf, uint8_t len, const uint8_t writeType);

    /**
     * Read the receive payload
     *
     * The size of data read is the fixed payload size, see getPayloadSize()
     *
     * @param buf Where to put the data
     * @param len Maximum number of bytes to read
     * @return Current value of status register
     */
    uint8_t read_payload(void *buf, uint8_t len);

    /**
     * Retrieve the current status of the chip
     *
     * @return Current value of status register
     */
    uint8_t get_status(void);

#if !defined(MINIMAL)

    /**
     * Decode and print the given status to stdout
     *
     * @param status Status value to print
     *
     * @warning Does nothing if stdout is not defined.  See fdevopen in stdio.h
     */
    void print_status(uint8_t status);

    /**
     * Decode and print the given 'observe_tx' value to stdout
     *
     * @param value The observe_tx value to print
     *
     * @warning Does nothing if stdout is not defined.  See fdevopen in stdio.h
     */
    void print_observe_tx(uint8_t value);

    /**
     * Print the name and value of an 8-bit register to stdout
     *
     * Optionally it can print some quantity of successive
     * registers on the same line.  This is useful for printing a group
     * of related registers on one line.
     *
     * @param name Name of the register
     * @param reg Which register. Use constants from nRF24L01.h
     * @param qty How many successive registers to print
     */
    void print_byte_register(const char *name, uint8_t reg, uint8_t qty = 1);

    /**
     * Print the name and value of a 40-bit address register to stdout
     *
     * Optionally it can print some quantity of successive
     * registers on the same line.  This is useful for printing a group
     * of related registers on one line.
     *
     * @param name Name of the register
     * @param reg Which register. Use constants from nRF24L01.h
     * @param qty How many successive registers to print
     */
    void print_address_register(const char *name, uint8_t reg, uint8_t qty = 1);

#endif

    /**
     * Turn on or off the special features of the chip
     *
     * The chip has certain 'features' which are only available when the 'features'
     * are enabled.  See the datasheet for details.
     */
    void toggle_features(void);

    /**
     * Built in spi transfer function to simplify repeating code repeating code
     */

    uint8_t spiTrans(uint8_t cmd);

#if defined(FAILURE_HANDLING) || defined(RF24_LINUX)

    void errNotify(void);

#endif

    /**@}*/
};

/**
 * @example GettingStarted.ino
 * <b>For Arduino</b><br>
 * <b>Updated: TMRh20 2014 </b><br>
 *
 * This is an example of how to use the RF24 class to communicate on a basic level. Configure and write this sketch to two
 * different nodes. Put one of the nodes into 'transmit' mode by connecting with the serial monitor and <br>
 * sending a 'T'. The ping node sends the current time to the pong node, which responds by sending the value
 * back. The ping node can then see how long the whole cycle took. <br>
 * @note For a more efficient call-response scenario see the GettingStarted_CallResponse.ino example.
 * @note When switching between sketches, the radio may need to be powered down to clear settings that are not "un-set" otherwise
 */

/**
* @example gettingstarted.cpp
* <b>For Linux</b><br>
* <b>Updated: TMRh20 2014 </b><br>
*
* This is an example of how to use the RF24 class to communicate on a basic level. Configure and write this sketch to two
* different nodes. Put one of the nodes into 'transmit' mode by connecting with the serial monitor and <br>
* sending a 'T'. The ping node sends the current time to the pong node, which responds by sending the value
* back. The ping node can then see how long the whole cycle took. <br>
* @note For a more efficient call-response scenario see the GettingStarted_CallResponse.ino example.
*/

/**
 * @example GettingStarted_CallResponse.ino
 * <b>For Arduino</b><br>
 * <b>New: TMRh20 2014</b><br>
 *
 * This example continues to make use of all the normal functionality of the radios including
 * the auto-ack and auto-retry features, but allows ack-payloads to be written optionlly as well. <br>
 * This allows very fast call-response communication, with the responding radio never having to
 * switch out of Primary Receiver mode to send back a payload, but having the option to switch to <br>
 * primary transmitter if wanting to initiate communication instead of respond to a commmunication.
 */

/**
* @example gettingstarted_call_response.cpp
* <b>For Linux</b><br>
* <b>New: TMRh20 2014</b><br>
*
* This example continues to make use of all the normal functionality of the radios including
* the auto-ack and auto-retry features, but allows ack-payloads to be written optionlly as well. <br>
* This allows very fast call-response communication, with the responding radio never having to
* switch out of Primary Receiver mode to send back a payload, but having the option to switch to <br>
* primary transmitter if wanting to initiate communication instead of respond to a commmunication.
*/

/**
* @example GettingStarted_HandlingData.ino
* <b>Dec 2014 - TMRh20</b><br>
*
* This example demonstrates how to send multiple variables in a single payload and work with data. As usual, it is
* generally important to include an incrementing value like millis() in the payloads to prevent errors.
*/

/**
* @example GettingStarted_HandlingFailures.ino
*
* This example demonstrates the basic getting started functionality, but with failure handling for the radio chip.
* Addresses random radio failures etc, potentially due to loose wiring on breadboards etc.
*/

/**
 * @example Transfer.ino
 * <b>For Arduino</b><br>
 * This example demonstrates half-rate transfer using the FIFO buffers<br>
 *
 * It is an example of how to use the RF24 class.  Write this sketch to two
 * different nodes.  Put one of the nodes into 'transmit' mode by connecting <br>
 * with the serial monitor and sending a 'T'.  The data transfer will begin,
 * with the receiver displaying the payload count. (32Byte Payloads) <br>
 */

/**
* @example transfer.cpp
* <b>For Linux</b><br>
* This example demonstrates half-rate transfer using the FIFO buffers<br>
*
* It is an example of how to use the RF24 class.  Write this sketch to two
* different nodes.  Put one of the nodes into 'transmit' mode by connecting <br>
* with the serial monitor and sending a 'T'.  The data transfer will begin,
* with the receiver displaying the payload count. (32Byte Payloads) <br>
*/

/**
 * @example TransferTimeouts.ino
 * <b>New: TMRh20 </b><br>
 * This example demonstrates the use of and extended timeout period and
 * auto-retries/auto-reUse to increase reliability in noisy or low signal scenarios. <br>
 *
 * Write this sketch to two different nodes.  Put one of the nodes into 'transmit'
 * mode by connecting with the serial monitor and sending a 'T'.  The data <br>
 * transfer will begin, with the receiver displaying the payload count and the
 * data transfer rate.
 */

/**
 * @example starping.pde
 *
 * This sketch is a more complex example of using the RF24 library for Arduino.
 * Deploy this on up to six nodes.  Set one as the 'pong receiver' by tying the
 * role_pin low, and the others will be 'ping transmit' units.  The ping units
 * unit will send out the value of millis() once a second.  The pong unit will
 * respond back with a copy of the value.  Each ping unit can get that response
 * back, and determine how long the whole cycle took.
 *
 * This example requires a bit more complexity to determine which unit is which.
 * The pong receiver is identified by having its role_pin tied to ground.
 * The ping senders are further differentiated by a byte in eeprom.
 */

/**
 * @example pingpair_ack.ino
 * <b>Update: TMRh20</b><br>
 * This example continues to make use of all the normal functionality of the radios including
 * the auto-ack and auto-retry features, but allows ack-payloads to be written optionlly as well.<br>
 * This allows very fast call-response communication, with the responding radio never having to
 * switch out of Primary Receiver mode to send back a payload, but having the option to if wanting<br>
 * to initiate communication instead of respond to a commmunication.
 */

/**
 * @example pingpair_irq.ino
 * <b>Update: TMRh20</b><br>
 * This is an example of how to user interrupts to interact with the radio, and a demonstration
 * of how to use them to sleep when receiving, and not miss any payloads.<br>
 * The pingpair_sleepy example expands on sleep functionality with a timed sleep option for the transmitter.
 * Sleep functionality is built directly into my fork of the RF24Network library<br>
 */

/**
* @example pingpair_irq_simple.ino
* <b>Dec 2014 - TMRh20</b><br>
* This is an example of how to user interrupts to interact with the radio, with bidirectional communication.
*/

/**
 * @example pingpair_sleepy.ino
 * <b>Update: TMRh20</b><br>
 * This is an example of how to use the RF24 class to create a battery-
 * efficient system.  It is just like the GettingStarted_CallResponse example, but the<br>
 * ping node powers down the radio and sleeps the MCU after every
 * ping/pong cycle, and the receiver sleeps between payloads. <br>
 */

/**
* @example rf24ping85.ino
* <b>New: Contributed by https://github.com/tong67</b><br>
* This is an example of how to use the RF24 class to communicate with ATtiny85 and other node. <br>
*/

/**
* @example timingSearch3pin.ino
* <b>New: Contributed by https://github.com/tong67</b><br>
* This is an example of how to determine the correct timing for ATtiny when using only 3-pins
*/

/**
 * @example pingpair_dyn.ino
 *
 * This is an example of how to use payloads of a varying (dynamic) size on Arduino.
 */

/**
* @example pingpair_dyn.cpp
*
* This is an example of how to use payloads of a varying (dynamic) size on Linux.
*/

/**
 * @example pingpair_dyn.py
 *
 * This is a python example for RPi of how to use payloads of a varying (dynamic) size.
 */

/**
 * @example scanner.ino
 *
 * Example to detect interference on the various channels available.
 * This is a good diagnostic tool to check whether you're picking a
 * good channel for your application.
 *
 * Inspired by cpixip.
 * See http://arduino.cc/forum/index.php/topic,54795.0.html
 */

/**
 * @mainpage Optimized High Speed Driver for nRF24L01(+) 2.4GHz Wireless Transceiver
 *
 * @section Goals Design Goals
 *
 * This library fork is designed to be...
 * @li More compliant with the manufacturer specified operation of the chip, while allowing advanced users
 * to work outside the recommended operation.
 * @li Utilize the capabilities of the radio to their full potential via Arduino
 * @li More reliable, responsive, bug-free and feature rich
 * @li Easy for beginners to use, with well documented examples and features
 * @li Consumed with a public interface that's similar to other Arduino standard libraries
 *
 * @section News News
 *
 * **Dec 2015**<br>
 * - ESP8266 support via Arduino IDE
 * - <a href="https://github.com/stewarthou/Particle-RF24">Particle Photon/Core</a> fork available
 * - ATTiny2313/4313 support added
 * - Python 3 support added
 * - RF24 added to Arduino library manager
 * - RF24 added to PlatformIO library manager
 *
 * **March 2015**<br>
 * - Uses SPI transactions on Arduino
 * - New layout for <a href="Portability.html">easier portability:</a> Break out defines & includes for individual platforms to RF24/utility
 * - <a href="MRAA.html">MRAA</a> support added ( Galileo, Edison, etc)
 * - <a href="Linux.html">Generic Linux support (SPIDEV)</a> support
 * - Support for RPi 2 added
 * - Major Documentation cleanup & update (Move all docs to github.io)
 *
 *
 * If issues are discovered with the documentation, please report them <a href="https://github.com/TMRh20/tmrh20.github.io/issues"> here</a>
 *
 * <br>
 * @section Useful Useful References
 *
 *
 * @li <a href="http://tmrh20.github.io/RF24/classRF24.html"><b>RF24</b> Class Documentation</a>
 * @li <a href="https://github.com/TMRh20/RF24/archive/master.zip"><b>Download</b></a>
 * @li <a href="https://github.com/tmrh20/RF24/"><b>Source Code</b></a>
 * @li <a href="http://tmrh20.blogspot.com/2014/03/high-speed-data-transfers-and-wireless.html"><b>My Blog:</b> RF24 Optimization Overview</a> 
 * @li <a href="http://tmrh20.blogspot.com/2016/08/raspberry-pilinux-with-nrf24l01.html"><b>My Blog:</b> RPi/Linux w/RF24Gateway</a> 
 * @li <a href="http://www.nordicsemi.com/files/Product/data_sheet/nRF24L01_Product_Specification_v2_0.pdf">Chip Datasheet</a>
 *
 * **Additional Information and Add-ons**
 *
 * @li <a href="http://tmrh20.github.io/RF24Network"> <b>RF24Network:</b> OSI Network Layer for multi-device communication. Create a home sensor network.</a>
 * @li <a href="http://tmrh20.github.io/RF24Mesh"> <b>RF24Mesh:</b> Dynamic Mesh Layer for RF24Network</a>
 * @li <a href="http://tmrh20.github.io/RF24Ethernet"> <b>RF24Ethernet:</b> TCP/IP Radio Mesh Networking (shares Arduino Ethernet API)</a>
 * @li <a href="http://tmrh20.github.io/RF24Audio"> <b>RF24Audio:</b> Realtime Wireless Audio streaming</a>
 * @li <a href="http://tmrh20.github.io/">All TMRh20 Documentation Main Page</a>
 *
 * **More Information and RF24 Based Projects**
 *
 * @li <a href="http://TMRh20.blogspot.com"> Project Blog: TMRh20.blogspot.com </a>
 * @li <a href="http://maniacalbits.blogspot.ca/"> Maniacal Bits Blog</a>
 * @li <a href="http://www.mysensors.org/">MySensors.org (User friendly sensor networks/IoT)</a>
 * @li <a href="https://github.com/mannkind/RF24Node_MsgProto"> RF24Node_MsgProto (MQTT)</a>
 * @li <a href="https://bitbucket.org/pjhardy/rf24sensornet/"> RF24SensorNet </a>
 * @li <a href="http://www.homeautomationforgeeks.com/rf24software.shtml">Home Automation for Geeks</a>
 * @li <a href="https://maniacbug.wordpress.com/2012/03/30/rf24network/"> Original Maniacbug RF24Network Blog Post</a>
 * @li <a href="https://github.com/maniacbug/RF24"> ManiacBug on GitHub (Original Library Author)</a>
 * 
 *
 * <br>
 *
 * @section Platform_Support Platform Support Pages
 *
 * @li <a href="Arduino.html"><b>Arduino</b></a> (Uno, Nano, Mega, Due, Galileo, etc)
 * @li <a href="ATTiny.html"><b>ATTiny</b></a>
 * @li <a href="Linux.html"><b>Linux devices</b></a>( <a href="RPi.html"><b>RPi</b></a> , <a href="Linux.html"><b>Linux SPI userspace device</b></a>, <a href="MRAA.html"><b>MRAA</b></a> supported boards ( Galileo, Edison, etc), <a href="LittleWire.html"><b>LittleWire</b></a>)
 * @li <a href="CrossCompile.html"><b>Cross-compilation</b></a> for linux devices
 * @li <a href="Python.html"><b>Python</b></a> wrapper available for Linux devices
 *
 * <br>
 * **General µC Pin layout** (See the individual board support pages for more info)
 *
 * The table below shows how to connect the the pins of the NRF24L01(+) to different boards.
 * CE and CSN are configurable.
 *
 * | PIN | NRF24L01 | Arduino UNO | ATtiny25/45/85 [0] | ATtiny44/84 [1] | LittleWire [2]          |    RPI     | RPi -P1 Connector |
 * |-----|----------|-------------|--------------------|-----------------|-------------------------|------------|-------------------|
 * |  1  |   GND    |   GND       |     pin 4          |    pin 14       | GND                     | rpi-gnd    |     (25)          |
 * |  2  |   VCC    |   3.3V      |     pin 8          |    pin  1       | regulator 3.3V required | rpi-3v3    |     (17)          |
 * |  3  |   CE     |   digIO 7   |     pin 2          |    pin 12       | pin to 3.3V             | rpi-gpio22 |     (15)          |
 * |  4  |   CSN    |   digIO 8   |     pin 3          |    pin 11       | RESET                   | rpi-gpio8  |     (24)          |
 * |  5  |   SCK    |   digIO 13  |     pin 7          |    pin  9       | SCK                     | rpi-sckl   |     (23)          |
 * |  6  |   MOSI   |   digIO 11  |     pin 6          |    pin  7       | MOSI                    | rpi-mosi   |     (19)          |
 * |  7  |   MISO   |   digIO 12  |     pin 5          |    pin  8       | MISO                    | rpi-miso   |     (21)          |
 * |  8  |   IRQ    |      -      |        -           |         -       | -                       |    -       |       -           |
 *
 * @li [0] https://learn.sparkfun.com/tutorials/tiny-avr-programmer-hookup-guide/attiny85-use-hints
 * @li [1] http://highlowtech.org/?p=1695
 * @li [2] http://littlewire.cc/   
 * <br><br><br>
 *
 *
 *
 *
 * @page Arduino Arduino
 * 
 * RF24 is fully compatible with Arduino boards <br>
 * See <b> http://www.arduino.cc/en/Reference/Board </b> and <b> http://arduino.cc/en/Reference/SPI </b> for more information
 * 
 * RF24 makes use of the standard hardware SPI pins (MISO,MOSI,SCK) and requires two additional pins, to control
 * the chip-select and chip-enable functions.<br>
 * These pins must be chosen and designated by the user, in RF24 radio(ce_pin,cs_pin); and can use any 
 * available pins.
 * 
 * <br>
 * @section ARD_DUE Arduino Due
 * 
 * RF24 makes use of the extended SPI functionality available on the Arduino Due, and requires one of the
 * defined hardware SS/CS pins to be designated in RF24 radio(ce_pin,cs_pin);<br>
 * See http://arduino.cc/en/Reference/DueExtendedSPI for more information
 *
 * Initial Due support taken from https://github.com/mcrosson/RF24/tree/due
 *
 * <br>
 * @section Alternate_SPI Alternate SPI Support
 *
 * RF24 supports alternate SPI methods, in case the standard hardware SPI pins are otherwise unavailable.
 * 
 * <br>
 * **Software Driven SPI**
 *
 * Software driven SPI is provided by the <a href=https://github.com/greiman/DigitalIO>DigitalIO</a> library
 *
 * Setup:<br>
 * 1. Install the digitalIO library<br>
 * 2. Open RF24_config.h in a text editor. 
      Uncomment the line 
      @code
      #define SOFTSPI
      @endcode
      or add the build flag/option
      @code
      -DSOFTSPI
      @endcode
 * 3. In your sketch, add
 *     @code
 *     #include DigitalIO.h
 *     @endcode
 *
 * @note Note: Pins are listed as follows and can be modified by editing the RF24_config.h file<br>
 *
 *     #define SOFT_SPI_MISO_PIN 16
 *     #define SOFT_SPI_MOSI_PIN 15
 *     #define SOFT_SPI_SCK_PIN 14
 * Or add the build flag/option
 *
 *     -DSOFT_SPI_MISO_PIN=16 -DSOFT_SPI_MOSI_PIN=15 -DSOFT_SPI_SCK_PIN=14
 *
 * <br>
 * **Alternate Hardware (UART) Driven  SPI**
 *
 * The Serial Port (UART) on Arduino can also function in SPI mode, and can double-buffer data, while the 
 * default SPI hardware cannot.
 *
 * The SPI_UART library is available at https://github.com/TMRh20/Sketches/tree/master/SPI_UART
 * 
 * Enabling:
 * 1. Install the SPI_UART library
 * 2. Edit RF24_config.h and uncomment #define SPI_UART
 * 3. In your sketch, add @code #include <SPI_UART.h> @endcode
 *
 * SPI_UART SPI Pin Connections:
 * | NRF |Arduino Uno Pin|
 * |-----|---------------|
 * | MOSI| TX(0)         |
 * | MISO| RX(1)         |
 * | SCK | XCK(4)        |
 * | CE  | User Specified|
 * | CSN | User Specified|
 *
 *
 * @note SPI_UART on Mega boards requires soldering to an unused pin on the chip. <br>See
 * https://github.com/TMRh20/RF24/issues/24 for more information on SPI_UART.
 * 
 * @page ATTiny ATTiny
 *
 * ATTiny support is built into the library, so users are not required to include SPI.h in their sketches<br>
 * See the included rf24ping85 example for pin info and usage
 * 
 * Some versions of Arduino IDE may require a patch to allow use of the full program space on ATTiny<br>
 * See https://github.com/TCWORLD/ATTinyCore/tree/master/PCREL%20Patch%20for%20GCC for ATTiny patch
 *
 * ATTiny board support initially added from https://github.com/jscrane/RF24
 *
 * @section Hardware Hardware Configuration
 * By tong67 ( https://github.com/tong67 )
 * 
 *    **ATtiny25/45/85 Pin map with CE_PIN 3 and CSN_PIN 4**
 * @code
 *                                 +-\/-+
 *                   NC      PB5  1|o   |8  Vcc --- nRF24L01  VCC, pin2 --- LED --- 5V
 *    nRF24L01  CE, pin3 --- PB3  2|    |7  PB2 --- nRF24L01  SCK, pin5
 *    nRF24L01 CSN, pin4 --- PB4  3|    |6  PB1 --- nRF24L01 MOSI, pin6
 *    nRF24L01 GND, pin1 --- GND  4|    |5  PB0 --- nRF24L01 MISO, pin7
 *                                 +----+ 
 * @endcode
 *
 * <br>
 *    **ATtiny25/45/85 Pin map with CE_PIN 3 and CSN_PIN 3** => PB3 and PB4 are free to use for application <br>
 *    Circuit idea from http://nerdralph.blogspot.ca/2014/01/nrf24l01-control-with-3-attiny85-pins.html <br>
 *   Original RC combination was 1K/100nF. 22K/10nF combination worked better.                          <br>
 *	For best settletime delay value in RF24::csn() the timingSearch3pin.ino sketch can be used.         <br>
 *    This configuration is enabled when CE_PIN and CSN_PIN are equal, e.g. both 3                      <br>
 *    Because CE is always high the power consumption is higher than for 5 pins solution                <br>
 * @code
 *                                                                                           ^^         
 *                                 +-\/-+           nRF24L01   CE, pin3 ------|              //         
 *                           PB5  1|o   |8  Vcc --- nRF24L01  VCC, pin2 ------x----------x--|<|-- 5V    
 *                   NC      PB3  2|    |7  PB2 --- nRF24L01  SCK, pin5 --|<|---x-[22k]--|  LED         
 *                   NC      PB4  3|    |6  PB1 --- nRF24L01 MOSI, pin6  1n4148 |                       
 *    nRF24L01 GND, pin1 -x- GND  4|    |5  PB0 --- nRF24L01 MISO, pin7         |                       
 *                        |        +----+                                       |                       
 *                        |-----------------------------------------------||----x-- nRF24L01 CSN, pin4  
 *                                                                      10nF                            
 * @endcode
 *
 * <br>
 *    **ATtiny24/44/84 Pin map with CE_PIN 8 and CSN_PIN 7** <br>
 *	Schematic provided and successfully tested by Carmine Pastore (https://github.com/Carminepz) <br>
 * @code
 *                                  +-\/-+                                                              
 *    nRF24L01  VCC, pin2 --- VCC  1|o   |14 GND --- nRF24L01  GND, pin1
 *                            PB0  2|    |13 AREF
 *                            PB1  3|    |12 PA1
 *                            PB3  4|    |11 PA2 --- nRF24L01   CE, pin3
 *                            PB2  5|    |10 PA3 --- nRF24L01  CSN, pin4
 *                            PA7  6|    |9  PA4 --- nRF24L01  SCK, pin5
 *    nRF24L01 MISO, pin7 --- PA6  7|    |8  PA5 --- nRF24L01 MOSI, pin6
 *                                  +----+
 *	@endcode					 
 *	
 * <br>
 *    **ATtiny2313/4313 Pin map with CE_PIN 12 and CSN_PIN 13** <br>
 * @code
 *                                  +-\/-+                                                              
 *                            PA2  1|o   |20 VCC --- nRF24L01  VCC, pin2
 *                            PD0  2|    |19 PB7 --- nRF24L01  SCK, pin5
 *                            PD1  3|    |18 PB6 --- nRF24L01 MOSI, pin6
 *                            PA1  4|    |17 PB5 --- nRF24L01 MISO, pin7
 *                            PA0  5|    |16 PB4 --- nRF24L01  CSN, pin4
 *                            PD2  6|    |15 PB3 --- nRF24L01   CE, pin3
 *                            PD3  7|    |14 PB2
 *                            PD4  8|    |13 PB1
 *                            PD5  9|    |12 PB0
 *    nRF24L01  GND, pin1 --- GND 10|    |11 PD6
 *                                  +----+
 *	@endcode					 
 *
 * <br><br><br>
 *
 *
 * 
 * 
 *
 *
 * @page Linux Linux devices
 *
 * Generic Linux devices are supported via SPIDEV, MRAA, RPi native via BCM2835, or using LittleWire.
 *
 *  @note The SPIDEV option should work with most Linux systems supporting spi userspace device. <br>
 *
 * <br>
 * @section AutoInstall Automated Install 
 *(**Designed & Tested on RPi** - Defaults to SPIDEV on devices supporting it)
 *
 * 
 * 1. Install prerequisites if there are any (MRAA, LittleWire libraries, setup SPI device etc)
 * 2. Download the install.sh file from http://tmrh20.github.io/RF24Installer/RPi/install.sh
 * @code wget http://tmrh20.github.io/RF24Installer/RPi/install.sh @endcode
 * 3. Make it executable
 * @code chmod +x install.sh @endcode
 * 4. Run it and choose your options
 * @code ./install.sh @endcode
 * 5. Run an example from one of the libraries
 * @code 
 * cd rf24libs/RF24/examples_linux  
 * @endcode
 * Edit the gettingstarted example, to set your pin configuration
 * @code nano gettingstarted.cpp
 * make  
 * sudo ./gettingstarted  
 * @endcode
 *
 * <br>
 * @section ManInstall Manual Install
 * 1. Install prerequisites if there are any (MRAA, LittleWire libraries, setup SPI device etc)
 * @note See the <a href="http://iotdk.intel.com/docs/master/mraa/index.html">MRAA </a> documentation for more info on installing MRAA <br>
 * 2. Make a directory to contain the RF24 and possibly RF24Network lib and enter it
 * @code
 *  mkdir ~/rf24libs 
 *  cd ~/rf24libs
*  @endcode
 * 3. Clone the RF24 repo
 *    @code git clone https://github.com/tmrh20/RF24.git RF24 @endcode
 * 4. Change to the new RF24 directory
 *    @code cd RF24 @endcode
 * 5. Configure build environment using @code ./configure @endcode script. It auto detectes device and build environment. For overriding autodetections, use command-line switches, see @code ./configure --help @endcode for description.
 * 6. Build the library, and run an example file
 * @code sudo make install @endcode
 * @code
 * cd examples_linux  
 * @endcode
 * Edit the gettingstarted example, to set your pin configuration
 * @code nano gettingstarted.cpp 
 * make 
 * sudo ./gettingstarted
 * @endcode
 *
 * <br><br>
 *   
 * @page MRAA MRAA
 *  
 * MRAA is a Low Level Skeleton Library for Communication on GNU/Linux platforms <br>
 * See http://iotdk.intel.com/docs/master/mraa/index.html for more information
 *
 * RF24 supports all MRAA supported platforms, but might not be tested on each individual platform due to the wide range of hardware support:<br>
 * <a href="https://github.com/TMRh20/RF24/issues">Report an RF24 bug or issue </a>
 *
 * @section Setup Setup and installation
 * 1. Install the MRAA lib
 * 2. As per your device, SPI may need to be enabled
 * 3. Follow <a href="Linux.html">Linux installation steps</a> to install RF24 libraries
 * 
 *
 * <br><br><br>
 *
 * 
 *
 *
 * @page RPi Raspberry Pi
 *
 * RF24 supports a variety of Linux based devices via various drivers. Some boards like RPi can utilize multiple methods
 * to drive the GPIO and SPI functionality.
 *
 * <br>
 * @section PreConfig Potential PreConfiguration
 *
 * If SPI is not already enabled, load it on boot:
 * @code sudo raspi-config  @endcode
 * A. Update the tool via the menu as required<br>
 * B. Select **Advanced** and **enable the SPI kernel module** <br>
 * C. Update other software and libraries
 * @code sudo apt-get update @endcode
 * @code sudo apt-get upgrade @endcode 
 * <br><br>
 *
 * @section Build Build Options
 * The default build on Raspberry Pi utilizes the included **BCM2835** driver from http://www.airspayce.com/mikem/bcm2835
 * 1. @code sudo make install -B @endcode
 *
 * Build using the **MRAA** library from http://iotdk.intel.com/docs/master/mraa/index.html <br>
 * MRAA is not included. See the <a href="MRAA.html">MRAA</a> platform page for more information.
 *
 * 1. Install, and build MRAA
 * @code
 * git clone https://github.com/intel-iot-devkit/mraa.git
 * cd mraa
 * mkdir build
 * cd build
 * cmake .. -DBUILDSWIGNODE=OFF
 * sudo make install
 * @endcode
 *
 * 2. Complete the install <br>
 * @code nano /etc/ld.so.conf @endcode
 * Add the line @code /usr/local/lib/arm-linux-gnueabihf @endcode
 * Run @code sudo ldconfig @endcode
 *
 * 3. Install RF24, using MRAA
 * @code
 * ./configure --driver=MRAA
 * sudo make install -B
 * @endcode
 * See the gettingstarted example for an example of pin configuration
 *
 * Build using **SPIDEV**
 *
 * 1. Make sure that spi device support is enabled and /dev/spidev\<a\>.\<b\> is present
 * 2. Install RF24, using SPIDEV
 * @code
 * ./configure --driver=SPIDEV
 * sudo make install -B
 * @endcode
 * 3. See the gettingstarted example for an example of pin configuration
 *
 * <br>
 * @section Pins Connections and Pin Configuration
 *
 *
 * Using pin 15/GPIO 22 for CE, pin 24/GPIO8 (CE0) for CSN
 *
 * Can use either RPi CE0 or CE1 pins for radio CSN.<br>
 * Choose any RPi output pin for radio CE pin.
 *
 * **BCM2835 Constructor:**
 * @code
 *  RF24 radio(RPI_V2_GPIO_P1_15,BCM2835_SPI_CS0, BCM2835_SPI_SPEED_8MHZ);
 *   or
 *  RF24 radio(RPI_V2_GPIO_P1_15,BCM2835_SPI_CS1, BCM2835_SPI_SPEED_8MHZ);
 *	
 *  RPi B+:
 *  RF24 radio(RPI_BPLUS_GPIO_J8_15,RPI_BPLUS_GPIO_J8_24, BCM2835_SPI_SPEED_8MHZ);
 *  or
 *  RF24 radio(RPI_BPLUS_GPIO_J8_15,RPI_BPLUS_GPIO_J8_26, BCM2835_SPI_SPEED_8MHZ);
 *
 *  General:
 *  RF24 radio(22,0);
 *  or
 *  RF24 radio(22,1);
 *
 * @endcode
 * See the gettingstarted example for an example of pin configuration
 *
 * See http://www.airspayce.com/mikem/bcm2835/index.html for BCM2835 class documentation.
 * <br><br>
 * **MRAA Constructor:**
 *
 * @code RF24 radio(15,0); @endcode
 *
 * See http://iotdk.intel.com/docs/master/mraa/rasppi.html
 * <br><br>
 * **SPI_DEV Constructor**
 *
 * @code RF24 radio(22,0); @endcode
 * In general, use @code RF24 radio(<ce_pin>, <a>*10+<b>); @endcode for proper SPIDEV constructor to address correct spi device at /dev/spidev\<a\>.\<b\>
 *
 * See http://pi.gadgetoid.com/pinout
 *
 * **Pins:**  
 *
 * | PIN | NRF24L01 |    RPI     | RPi -P1 Connector |
 * |-----|----------|------------|-------------------|
 * |  1  |   GND    | rpi-gnd    |     (25)          |
 * |  2  |   VCC    | rpi-3v3    |     (17)          |
 * |  3  |   CE     | rpi-gpio22 |     (15)          |
 * |  4  |   CSN    | rpi-gpio8  |     (24)          |
 * |  5  |   SCK    | rpi-sckl   |     (23)          |
 * |  6  |   MOSI   | rpi-mosi   |     (19)          |
 * |  7  |   MISO   | rpi-miso   |     (21)          |
 * |  8  |   IRQ    |    -       |       -           |
 *   
 *   
 *  
 *  
 * <br><br>
 ****************
 *   
 * Based on the arduino lib from J. Coliz <maniacbug@ymail.com>  <br>
 * the library was berryfied by Purinda Gunasekara <purinda@gmail.com> <br>  
 * then forked from github stanleyseow/RF24 to https://github.com/jscrane/RF24-rpi  <br>
 * Network lib also based on https://github.com/farconada/RF24Network
 *
 * 
 *
 * 
 * <br><br><br>
 * 
 *
 *  
 * @page Python Python Wrapper (by https://github.com/mz-fuzzy)
 *
 * @note Both python2 and python3 are supported.
 *
 * @section Install Installation:  
 *
 * 1. Install the python-dev (or python3-dev) and boost libraries
 * @code sudo apt-get install python-dev libboost-python-dev @endcode
 * @note For python3 in Raspbian, it's needed to manually link python boost library, like this:
 * @code sudo ln -s /usr/lib/arm-linux-gnueabihf/libboost_python-py34.so /usr/lib/arm-linux-gnueabihf/libboost_python3.so @endcode
 *
 * 2. Install python-setuptools (or python3-setuptools)
 * @code sudo apt-get install python-setuptools @endcode
 *
 * 3. Build the library
 * @code ./setup.py build   @endcode
 * @note Build takes several minutes on arm-based machines. Machines with RAM <1GB may need to increase amount of swap for build.
 *
 * 4. Install the library
 * @code sudo ./setup.py install  @endcode
 * See the additional <a href="pages.html">Platform Support</a> pages for information on connecting your hardware  <br>
 * See the included <a href="pingpair_dyn_8py-example.html">example </a> for usage information.   
 * 
 * 5. Running the Example
 * Edit the pingpair_dyn.py example to configure the appropriate pins per the above documentation:  
 * @code nano pingpair_dyn.py   @endcode
 * Configure another device, Arduino or RPi with the <a href="pingpair_dyn_8py-example.html">pingpair_dyn</a> example <br>
 * Run the example  
 * @code sudo ./pingpair_dyn.py  @endcode
 *
 * <br><br><br>
 *
 * @page CrossCompile Linux cross-compilation
 * 
 * RF24 library supports cross-compilation. Advantages of cross-compilation:
 *  - development tools don't have to be installed on target machine
 *  - resources of target machine don't have to be sufficient for compilation
 *  - compilation time can be reduced for large projects
 *
 *  Following prerequisites need to be assured:
 *  - ssh passwordless access to target machine (https://linuxconfig.org/passwordless-ssh)
 *  - sudo of a remote user without password (http://askubuntu.com/questions/334318/sudoers-file-enable-nopasswd-for-user-all-commands)
 *  - cross-compilation toolchain for your target machine; for RPi
 *  @code git clone https://github.com/raspberrypi/tools rpi_tools @endcode
 *  and cross-compilation tools must be in PATH, for example
 *  @code export PATH=$PATH:/your/dir/rpi-tools/arm-bcm2708/gcc-linaro-arm-linux-gnueabihf-raspbian-x64/bin @endcode
 *
 * @section CxSteps Cross compilation steps:
 * 1. clone RF24 to a machine for cross-compilation
 * @code
 * git clone https://github.com/TMRh20/RF24
 * cd RF24
 * @endcode
 * 2. configure for cross compilation
 * @code ./configure --remote=pi@target_linux_host @endcode
 * eventually
 * @code ./configure --remote=pi@target_linux_host --driver=<driver> @endcode
 * 3. build
 * @code make @endcode
 * 4. (opt) install library to cross-compilation machine into cross-exvironment - important for compilation of examples
 * @code sudo make install @endcode
 * 5. upload library to target machine
 * @code make upload @endcode
 * 6. (opt) compile examples
 * @code
 * cd examples_linux
 * make
 * @endcode
 * 7. (opt) upload examples to target machine
 * @code make upload @endcode
 *
 * @section CxStepsPython Cross comilation steps for python wrapper
 *
 * Prerequisites:
 * - Python setuptools must be installed on both target and cross-compilation machines
 *   @code sudo pip install setuptools @endcode
 *   or
 *   @code sudo apt-get install python-setuptools @endcode
 *
 * Installation steps:
 * 1. Assure having libboost-python-dev library in your cross-compilation environment. Alternatively, you can install it into your target machine and copy /usr and /lib directories to the cross-compilation machine.
 * For example
 * @code
 * mkdir -p rpi_root && rsync -a pi@target_linux_host:/usr :/lib rpi_root
 * export CFLAGS="--sysroot=/your/dir/rpi_root -I/your/dir/rpi_root/usr/include/python2.7/"
 * @endcode
 *
 * 2. Build the python wrapper
 * @code
 * cd pyRF24
 * ./setup.py build --compiler=crossunix
 * @endcode
 *
 * 3. Make the egg package
 * @code ./setup.py bdist_egg --plat-name=cross @endcode
 * dist/RF24-<version>-cross.egg should be created.
 *
 * 4. Upload it to the target machine and install there:
 * @code
 * scp dist/RF24-*-cross.egg pi@target_linux_host:
 * ssh pi@target_linux_host 'sudo easy_install RF24-*-cross.egg'
 * @endcode
 *
 * <br><br><br>
 *
 * @page ATXMEGA ATXMEGA
 * 
 * The RF24 driver can be build as a static library with Atmel Studio 7 in order to be included as any other library in another program for the XMEGA family.
 *
 * Currently only the <b>ATXMEGA D3</b> family is implemented.
 * 
 * @section Preparation 
 * 
 * Create an empty GCC Static Library project in AS7.<br>
 * As not all files are required, copy the following directory structure in the project:
 * 
 * @code
 * utility\
 *   ATXMegaD3\
 *     compatibility.c
 *     compatibility.h
 *     gpio.cpp
 *     gpio.h
 *     gpio_helper.c
 *     gpio_helper.h
 *     includes.h
 *     RF24_arch_config.h
 *     spi.cpp
 *     spi.h
 * nRF24L01.h
 * printf.h
 * RF24.cpp
 * RF24.h
 * RF24_config.h
 * @endcode
 * 
 * @section Usage
 * 
 * Add the library to your project!<br>
 * In the file where the **main()** is put the following in order to update the millisecond functionality:
 * 
 * @code
 * ISR(TCE0_OVF_vect)
 * {
 * 	update_milisec();
 * }
 * @endcode
 * 
 * Declare the rf24 radio with **RF24 radio(XMEGA_PORTC_PIN3, XMEGA_SPI_PORT_C);**
 * 
 * First parameter is the CE pin which can be any available pin on the uC.
 * 
 * Second parameter is the CS which can be on port C (**XMEGA_SPI_PORT_C**) or on port D (**XMEGA_SPI_PORT_D**). 
 * 
 * Call the **__start_timer()** to start the millisecond timer.
 * 
 * @note Note about the millisecond functionality:<br>
 * 
 * 	The millisecond functionality is based on the TCE0 so don't use these pins as IO.<br>
 * 	The operating frequency of the uC is 32MHz. If you have other frequency change the TCE0 registers appropriatly in function **__start_timer()** in **compatibility.c** file for your frequency. 
 *
 * @page Portability RF24 Portability
 *
 * The RF24 radio driver mainly utilizes the <a href="http://arduino.cc/en/reference/homePage">Arduino API</a> for GPIO, SPI, and timing functions, which are easily replicated
 * on various platforms. <br>Support files for these platforms are stored under RF24/utility, and can be modified to provide 
 * the required functionality.
 * 
 * <br>
 * @section Hardware_Templates Basic Hardware Template
 *
 * **RF24/utility**
 *
 * The RF24 library now includes a basic hardware template to assist in porting to various platforms. <br> The following files can be included
 * to replicate standard Arduino functions as needed, allowing devices from ATTiny to Raspberry Pi to utilize the same core RF24 driver.
 *
 * | File               |                   Purpose                                                    | 
 * |--------------------|------------------------------------------------------------------------------| 
 * | RF24_arch_config.h | Basic Arduino/AVR compatibility, includes for remaining support files, etc   | 
 * | includes.h         | Linux only. Defines specific platform, include correct RF24_arch_config file | 
 * | spi.h              | Provides standardized SPI ( transfer() ) methods                         | 
 * | gpio.h             | Provides standardized GPIO ( digitalWrite() ) methods                        | 
 * | compatibility.h    | Provides standardized timing (millis(), delay()) methods                     | 
 * | your_custom_file.h | Provides access to custom drivers for spi,gpio, etc                          | 
 *
 * <br>
 * Examples are provided via the included hardware support templates in **RF24/utility** <br>
 * See the <a href="modules.html">modules</a> page for examples of class declarations 
 *
 *<br>
 * @section Device_Detection Device Detection
 *
 * 1. The main detection for Linux devices is done in the configure script, with the includes.h from the proper hardware directory copied to RF24/utility/includes.h <br>
 * 2. Secondary detection is completed in RF24_config.h, causing the include.h file to be included for all supported Linux devices <br>
 * 3. RF24.h contains the declaration for SPI and GPIO objects 'spi' and 'gpio' to be used for porting-in related functions.
 *
 * <br>
 * @section Ported_Code Code
 * To have your ported code included in this library, or for assistance in porting, create a pull request or open an issue at https://github.com/TMRh20/RF24
 * 
 *
 *<br><br><br>
 */

#endif // __RF24_H__
