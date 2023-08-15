#include "lm75bd.h"
#include "i2c_io.h"
#include "errors.h"

#include <stdint.h>
#include <string.h>
#include <math.h>

/* LM75BD Registers (p.8) */
#define LM75BD_REG_CONF 0x01U  /* Configuration Register (R/W) */

error_code_t lm75bdInit(lm75bd_config_t *config) {
  error_code_t errCode;

  if (config == NULL) return ERR_CODE_INVALID_ARG;

  errCode = writeConfigLM75BD(config->devAddr, config->osFaultQueueSize, config->osPolarity,
                                         config->osOperationMode, config->devOperationMode);
  
  if (errCode != ERR_CODE_SUCCESS) return errCode;

  // Assume that the overtemperature and hysteresis thresholds are already set
  // Hysteresis: 75 degrees Celsius
  // Overtemperature: 80 degrees Celsius

  return ERR_CODE_SUCCESS;
}

error_code_t readTempLM75BD(uint8_t devAddr, float *temp) {
  /* Implement this driver function */
  // Initialize local variables
  uint8_t buffer[READ_BYTES];
  uint8_t tempReg = TEMPERATURE_ADDR;

  // Set temperature registry
  if (i2cSendTo(devAddr, &tempReg, WRITE_BYTES) != ERR_CODE_SUCCESS) return ERR_CODE_INVALID_ARG;

  // read from temperature registry
  if (i2cReceiveFrom(devAddr, buffer, READ_BYTES) != ERR_CODE_SUCCESS) return ERR_CODE_INVALID_ARG;

  // Bit shift by 5 to get rid of the last 5 bits
  buffer[1] >>= 5;

  // Get most significant bit
  uint8_t msb = buffer[0] & 0b10000000;

  // Remove most significant bit
  // buffer[0] &= 0b01111111;

  // Calculate temperature
  if (msb == 0) {
    *temp = buffer[0] + (buffer[1] * TEMPERATURE_CONVERSION_FACTOR);
  } else {
    buffer[1] = (~buffer[1]) + 1;
    buffer[1] = (buffer[1] == 8) ? 0 : buffer[1];
    buffer[0] = (~buffer[0]) + (buffer[1] == 0 ? 1 : 0);
    *temp = -buffer[0] - buffer[1] * TEMPERATURE_CONVERSION_FACTOR;
  }
  
  return ERR_CODE_SUCCESS;
}

#define CONF_WRITE_BUFF_SIZE 2U
error_code_t writeConfigLM75BD(uint8_t devAddr, uint8_t osFaultQueueSize, uint8_t osPolarity,
                                   uint8_t osOperationMode, uint8_t devOperationMode) {
  error_code_t errCode;

  // Stores the register address and data to be written
  // 0: Register address
  // 1: Data
  uint8_t buff[CONF_WRITE_BUFF_SIZE] = {0};

  buff[0] = LM75BD_REG_CONF;

  uint8_t osFaltQueueRegData = 0;
  switch (osFaultQueueSize) {
    case 1:
      osFaltQueueRegData = 0;
      break;
    case 2:
      osFaltQueueRegData = 1;
      break;
    case 4:
      osFaltQueueRegData = 2;
      break;
    case 6:
      osFaltQueueRegData = 3;
      break;
    default:
      return ERR_CODE_INVALID_ARG;
  }

  buff[1] |= (osFaltQueueRegData << 3);
  buff[1] |= (osPolarity << 2);
  buff[1] |= (osOperationMode << 1);
  buff[1] |= devOperationMode;

  errCode = i2cSendTo(LM75BD_OBC_I2C_ADDR, buff, CONF_WRITE_BUFF_SIZE);
  if (errCode != ERR_CODE_SUCCESS) return errCode;

  return ERR_CODE_SUCCESS;
}