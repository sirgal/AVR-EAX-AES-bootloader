AVR-EAX-AES-bootloader
======================

Bootloader for AVR microcontrollers featuring EAX mode encryption (AES underlying) in 1024 bytes

Usage:
- Create unique key file using key_creator
- Attach bootloader project to your own
- Change all the settings as desired
- Replace key_const.asm with the newly generated one
- Upload bootloader to your device
- Encrypt new firmware (with no bootloader) with key_creator using the same key file
- Write your own uploader or use the provided one to upload new firmware to the bootloader, the protocol is simple

Remember: if you ever lose key_const for the device, you will be unable to upload patches to that device! There is no way to do it! 
Keep your key files in a safe place.
