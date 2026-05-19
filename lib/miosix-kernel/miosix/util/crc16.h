
#ifndef CRC16_H
#define CRC16_H

namespace miosix {

/**
 * Calculate the ccitt crc16 on a string of bytes
 * \param message string of bytes
 * \param length message length
 * \return the crc16
 */
unsigned short crc16(const void *message, unsigned int length);

} //namespace miosix

#endif //CRC16_H
