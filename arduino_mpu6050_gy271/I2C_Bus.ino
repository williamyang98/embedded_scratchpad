/* @returns Error code for i2c write
 *  0: success.
 *  1: data too long to fit in transmit buffer.
 *  2: received NACK on transmit of address.
 *  3: received NACK on transmit of data.
 *  4: other error.
 *  5: timeout
 */
uint8_t i2c_write_reg(const uint8_t addr, const uint8_t reg, const uint8_t* buf, const uint8_t length) {
  Wire.beginTransmission(addr);
  Wire.write(reg);
  Wire.write(buf, length);
  return Wire.endTransmission();
}

/* @returns Error code for i2c read
 *  0: success.
 *  1: data too long to fit in transmit buffer.
 *  2: received NACK on transmit of address.
 *  3: received NACK on transmit of data.
 *  4: other error.
 *  5: timeout
 *  6: incorrect length on data request
 */
uint8_t i2c_read_reg(const uint8_t addr, const uint8_t reg, uint8_t* buf, const uint8_t length) {
  Wire.beginTransmission(addr);
  Wire.write(reg);
  const uint8_t err = Wire.endTransmission();
  if (err != 0x00) {
    return err;
  }
  
  const uint8_t n_actual = Wire.requestFrom(addr, length);
  if (n_actual != length) {
    return 0x06;
  }
    
  for (int i = 0; i < length; i++) {
    buf[i] = Wire.read();  
  }
  return 0x00;
}