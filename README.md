# WT32-SC01_TripMaster
Rally tripmaster based on the WT32-SC01 development board and the Beitian BS-280 GPS module

![WT32-SC01_TripMaster](https://i.imgur.com/ztwyEtg.jpg)

# Setup
You will need to dowload the TFT_eSPI library, add the WT32-SC01-User_Setup.h to the TFT_eSPI\User_Setups folder and modify the TFT_eSPI\User_Setup_Select.h file to include the user setup in order to make the screen in the WT32-SC01 development board work.
#include <User_Setups/WT32-SC01-User_Setup.h>
