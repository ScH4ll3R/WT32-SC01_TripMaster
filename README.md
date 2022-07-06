# WT32-SC01_TripMaster
Rally tripmaster based on the WT32-SC01 development board and the Beitian BS-280 GPS module

![WT32-SC01_TripMaster](https://i.imgur.com/ztwyEtg.jpg)

# Setup
You will need to dowload the TFT_eSPI library, add the WT32-SC01-User_Setup.h to the TFT_eSPI\User_Setups folder  
Then modify the TFT_eSPI\User_Setup_Select.h file to include the user setup in order to make the screen in the WT32-SC01 development board work:  
#include <User_Setups/WT32-SC01-User_Setup.h>  
  
You will need TinyGPS++ library, no modifications required.
  
To flash the module you'll need to add ESP32 support to the Arduino IDE, and select "ESP32 Wrover Kit (all versions)"  
  
You might also need the ![CP210x Universal Windows Driver](https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers) if you can't reach the COM port  
  
# Documentation
![Here's](https://datasheet.lcsc.com/lcsc/2005181307_Wireless-tag-WT32-SC01_C555472.pdf) the datasheet I've been using to check the pins on the board and other information  
  
# Hardware
Here's the hardware I've used:  
- WT32-SC01 development board  
- Beitian BS-280 GPS module  
- Any kind of buttons x2 (I've used a motorcycle handlebar switch module)  
- 115 x 85 x 35 mm Transparent case (There's some space left)  
- 2 GX16-5pin connectors for the buttons and the USB connection  
