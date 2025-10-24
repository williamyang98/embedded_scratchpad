#include <Wire.h>

uint8_t check_i2c_error(const uint8_t err, const int line_number, const char* code) {
  if (err != 0x00) {
    Serial.print("i2c error (");
    Serial.print(err);
    Serial.print(") [line ");
    Serial.print(line_number);
    Serial.print("]: ");
    Serial.println(code);
  }
  return err;
}

#define CHECK_I2C_ERROR(x) check_i2c_error(x, __LINE__, #x)

#define MPU6050_ACCEL_MODE 3
#define MPU6050_GYRO_MODE 0

void setup(void) {
  // put your setup code here, to run once:
  Wire.begin();
  Serial.begin(9600);
  delay(100);

  CHECK_I2C_ERROR(mpu6050_ping());
  CHECK_I2C_ERROR(hmc5883l_ping());

  CHECK_I2C_ERROR(mpu6050_set_clock_source(0));
  CHECK_I2C_ERROR(mpu6050_set_gyro_fullscale_range(MPU6050_GYRO_MODE));
  CHECK_I2C_ERROR(mpu6050_set_accel_fullscale_range(MPU6050_ACCEL_MODE));
  CHECK_I2C_ERROR(mpu6050_set_sleep(0));
}

struct mpu6050_data_t {
  struct {
    int16_t x;
    int16_t y;
    int16_t z;
  } acceleration;
  int16_t temperature;
  struct {
    int16_t x;
    int16_t y;
    int16_t z;
  } gyroscope;
};

struct hmc5883l_data_t {
  int16_t x;
  int16_t y;
  int16_t z;
};

struct hmc5883l_id_t {
  uint8_t id[3];
};

struct mpu6050_data_t mpu6050_data;
struct hmc5883l_data_t hmc5883l_data;

void loop(void) {
  if (CHECK_I2C_ERROR(mpu6050_get_readings(&mpu6050_data)) == 0x00) {
    Serial.print("mpu6050: accel=[");
    Serial.print(mpu6050_data.acceleration.x);
    Serial.print(',');
    Serial.print(mpu6050_data.acceleration.y);
    Serial.print(',');
    Serial.print(mpu6050_data.acceleration.z);
    Serial.print("], temp=");
    Serial.print(mpu6050_data.temperature);
    Serial.print(", gyro=[");
    Serial.print(mpu6050_data.gyroscope.x);
    Serial.print(',');
    Serial.print(mpu6050_data.gyroscope.y);
    Serial.print(',');
    Serial.print(mpu6050_data.gyroscope.z);
    Serial.println("]");
  }
  
  if (CHECK_I2C_ERROR(hmc5883l_get_readings(&hmc5883l_data)) == 0x00) {
    Serial.print("hmc5883l: magnetic=[");
    Serial.print(hmc5883l_data.x);
    Serial.print(',');
    Serial.print(hmc5883l_data.y);
    Serial.print(',');
    Serial.print(hmc5883l_data.z);
    Serial.println("]");
  }
  delay(100);
}
