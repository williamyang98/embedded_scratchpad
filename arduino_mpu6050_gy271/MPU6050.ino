// SOURCE: https://invensense.tdk.com/wp-content/uploads/2015/02/MPU-6000-Datasheet1.pdf
#define MPU6050_ADDR 0x68

uint8_t mpu6050_ping(void) {
  Wire.beginTransmission(MPU6050_ADDR);
  return Wire.endTransmission();
}

/* Select clock source
 * 0: Internal 8MHz oscillator
 * 1: PLL with X axis gyroscope reference
 * 2: PLL with Y axis gyroscope reference
 * 3: PLL with Z axis gyroscope reference
 * 4: PLL with external 32.768kHz reference
 * 5: PLL with external 19.2MHz reference
 * 6: Reserved
 * 7: Stops the clock and keeps the timing generator in reset
 */
uint8_t mpu6050_set_clock_source(uint8_t v) {
  uint8_t d;
  const uint8_t read_err = i2c_read_reg(MPU6050_ADDR, 0x6B, &d, 1);
  if (read_err != 0) return read_err;

  d = (d & 0b11110000) | (v & 0b111);
  return i2c_write_reg(MPU6050_ADDR, 0x6B, &d, 1);
}

/* Set sensivity of gyroscope
 * 0: 250'/s
 * 1: 500'/s
 * 2: 1000'/s
 * 3: 2000'/s
 */
uint8_t mpu6050_set_gyro_fullscale_range(const uint8_t v) {
  uint8_t d;
  uint8_t rv = i2c_read_reg(MPU6050_ADDR, 0x1B, &d, 1);
  if (rv != 0) return rv;

  d = (d & 0b11100111) | ((v & 0b11) << 3);
  return i2c_write_reg(MPU6050_ADDR, 0x1B, &d, 1);
}

/* Set sensitivity of accelerometer
 * 0: 2g
 * 1: 4g
 * 2: 8g
 * 3: 16g
 */
uint8_t mpu6050_set_accel_fullscale_range(const uint8_t v) {
  uint8_t d;
  const uint8_t read_err = i2c_read_reg(MPU6050_ADDR, 0x1C, &d, 1);
  if (read_err != 0) return read_err;

  d = (d & 0b11100111) | ((v & 0b11) << 3);
  return i2c_write_reg(MPU6050_ADDR, 0x1C, &d, 1);
}

// make the sensor chip idle or active
uint8_t mpu6050_set_sleep(bool is_sleep) {
  uint8_t d;
  const uint8_t read_err = i2c_read_reg(MPU6050_ADDR, 0x6B, &d, 1);
  if (read_err != 0x00) return read_err;

  if (!is_sleep) {
    d = d & ~(1u << 6);    
  } else {
    d = d | (1u << 6);
  }
  return i2c_write_reg(MPU6050_ADDR, 0x6B, &d, 1);
}

uint8_t mpu6050_is_ready(bool* is_ready) {
  uint8_t d;
  const uint8_t err = i2c_read_reg(MPU6050_ADDR, 0x3A, &d, 1);
  if (err != 0x00) return err;
  
  *is_ready = ((d & 0b1) != 0);
  return 0x00;
}

/* Readings are scaled based on mpu6050_set_accel_fullscale_range and mpu6050_set_gyro_fullscale_range
 * Accelerometer
 *    0:  2g, 16384=1g
 *    1:  4g,  8192=1g
 *    2:  8g,  4096=1g
 *    3: 16g,  2048=1g
 * Gyroscope
 *    0:  250'/s, 131=1'/s
 *    1:  500'/s, 65.5=1'/s
 *    2: 1000'/s, 32.8=1'/s
 *    3: 2000'/s, 16.4=1'/s
 * Temperature
 *    temp ('C) = value/340 + 36.53
 */
uint8_t mpu6050_get_readings(struct mpu6050_data_t* data) {
  uint8_t buf[14];
  const uint8_t err = i2c_read_reg(MPU6050_ADDR, 0x3B, buf, 14);
  if (err != 0x00) return err;

  data->acceleration.x = (int16_t(buf[0]) << 8) | buf[1];
  data->acceleration.y = (int16_t(buf[2]) << 8) | buf[3];
  data->acceleration.z = (int16_t(buf[4]) << 8) | buf[5];
  data->temperature = (int16_t(buf[6]) << 8) | buf[7];
  data->gyroscope.x = (int16_t(buf[8]) << 8) | buf[9];
  data->gyroscope.y = (int16_t(buf[10]) << 8) | buf[11];
  data->gyroscope.z = (int16_t(buf[12]) << 8) | buf[13];
  return 0x00;
}