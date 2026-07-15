1. Replace 16MHz crystal on Arduino UNO R3 with a 20MHz crystal that you can by, usually a through hole
2. Adjust ```F_CPU``` or board configuration files for the higher clock speed
    - For Atmel studio ```#define F_CPU 20000000UL``` in global compile definitions
    - For Arduino IDE install minicore: ```https://github.com/MCUdude/MiniCore#boards-manager-installation```