### Sharing USB COM ports with WSL2
1. Install usbipd and restart computer (powershell+admin): ```winget install usbipd```
2. List devices: ```usbipd list```
    - For the common ```CH340``` USB to UART bridge IC it shares the same hardware id ```<VID:PID>```
    - The only way to disambiguate this is with the bus id
3. Bind device (admin): ```usbipd bind --busid <BUS_ID>```
4. Start up WSL2 instance
5. Attach device: ```usbipd attach --wsl --auto-attach --busid <BUS_ID>```
6. List USB devices within WSL2 shell: ```lsusb```
7. List TTY USB devices within WSL2 shell: ```ls /dev/ttyUSB*```
7. Detach device: ```usbipd detach --busid <BUS_ID>```

**NOTE**: On the next boot simply start up a WSL2 instance, and then in Windows run: ```usbipd attach --wsl --auto-attach --busid <BUS_ID>```

### Fixing VBoxUSBMon
- You may get a warning or error about ```VBoxUSBMon``` not being installed or running
1. Check the status of VBoxUSBMon (admin): ```sc query VBoxUsbMon```
2. Try reinstalling usbipd and restarting computer (powershell+admin): ```winget uninstall usbipd``` then ```winget install usbipd```

