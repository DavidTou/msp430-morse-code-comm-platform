# msp430-morse-code-comm-platform
This project consists of a Morse Code Communication Platform using MSP430FG4618/F2013 Experimenter Board and Serial Communication with a Workstation.  The big square pad at the center of the Capacitive Touch Pad is used as a button to create the Morse code and the F2013 sends the state of the capacitive pad to the FG4618 through I2C.  In the FG4618, the buzzer will play the corresponding dots/lines and decode the Morse code and send it to the workstation using UART.  The workstation will receive the resulting characters of the Morse code on a Serial terminal. The Workstation will also be able to send a message that will then be played by the buzzer using the equivalent Morse code. 

[IMAGE]

## Set Up

MSP430FG4618 (4618_code.c)
- Open, compile, and load it into the MSP430FG4619 on the board
- Launch HyperTerminal
- Configure the COM port and set the configuration to 115200 bps with no Parity
MSP4302013 (2013_code.c)
- Open, compile, and load it into the MSP4302013
- Header H1 must have only jumpers 1-2 and 3-4 fitted
- Jumper JP2 must be removed, so LED3 does not affect touch pad sensing

## User Instruction

After the set up you will see the following messages on the terminal:

[IMAGE]

### HyperTerminal To Board

“Insert Text To Send:” informs the user that the microcontroller is ready to receive a message.
After the insertion of a message (max length of 25 characters) and the ENTER key to end it, the message is processed by the board and outputted through the LED/Buzzer.

[IMAGE]

The progress of the Message-To-Morse can be seen on the terminal. “#” are printed when the letter has been outputted.
In the case a user inserts an unknown Morse code character in the terminal, this following message is outputted:

[IMAGE]

### Board To HyperTerminal

When a message is being sent from the Board to the HyperTerminal, the corresponding Morse-To-Letter character is outputted.

[IMAGE]

A user can send Morse code messages using the capacitive button on the board. When the user is done with the tapping of the corresponding Morse character, SW1 can be pressed to send the letter.  SW2 is pressed when the user is done with the whole message to give the prompt back to the terminal.
