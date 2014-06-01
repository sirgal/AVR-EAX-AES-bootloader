AVR-EAX-AES-bootloader
======================

Bootloader for AVR microcontrollers featuring EAX mode encryption (AES underlying) in 1024 bytes

Usage:
1. Create unique key file using key_creator
2. Attach bootloader project to your own
3. Change all the settings as desired
4. Replace key_const.asm with the newly generated one
5. Upload bootloader to your device
5. Encrypt new firmware (with no bootloader) with key_creator using the same key file
6. Write your own uploader or use the provided one to upload new firmware to the bootloader, the protocol is simple

Remember: if you ever lose key_const for the device, you will be unable to upload patches to that device! There is no way to do it! 
Keep your key files in a safe place.
