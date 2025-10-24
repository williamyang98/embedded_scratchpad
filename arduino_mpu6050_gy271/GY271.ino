// SOURCE: https://cdn-shop.adafruit.com/datasheets/HMC5883L_3-Axis_Digital_Compass_IC.pdf
#define HMC5883L_ADDR 0x1E

uint8_t hmc5883l_ping(void) {
  Wire.beginTransmission(HMC5883L_ADDR);
  return Wire.endTransmission();
}

uint8_t hmc5883l_get_readings(struct hmc5883l_data_t* m) {
  uint8_t buf[6];
  const uint8_t err = i2c_read_reg(HMC5883L_ADDR, 3, buf, 6);
  if (err != 0x00) return err;

  m->x = (int16_t(buf[0]) << 8) | buf[1];
  m->y = (int16_t(buf[2]) << 8) | buf[3];
  m->z = (int16_t(buf[4]) << 8) | buf[5];
  return 0x00;
}

uint8_t hmc5883l_get_chip_id(struct hmc5883l_id_t* id) {
  return i2c_read_reg(HMC5883L_ADDR, 10, id->id, 3);
}