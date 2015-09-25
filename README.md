# Haku
A 5.8 GHz video scanner receiver using FatShark race band receiver module with the following features:

 * 32 channel scanning in frequency order
 * channel match optimizer by comparing surrounding channels
 * automatic noise floor calibration
 * continuous scan mode, next channel is search if currently locked channel stops broadcasting for predefined time
 * band, channel number, channel frequency and relative rssi display
 * channel hold mode regardless of rssi

## Why
Having used two commercial 5.8 GHz video scanner receivers left me wanting for something better. One had a habit of locking to whatever noisy signal it was first able to find, usually finding the wrong channel. The other wasn't able to get a lock unless the transmitter was within few meters of the receiver. The scanning logic in both was to lock on the first channel that peaks enough from the noise floor. If that didn't result in the correct channel then fine tuning wasn't straightforward as channels weren't in frequency order.

Having such receiver connected to a spectator display wasn't an optimal solution since someone needed to manually restart the channel scan every time transmission on the found channel ended. Haku ended up being born to solve these issues and give me time to fly instead of having to constantly fiddle with the spectator display receiver.

## Hardware

 * Arduino Nano V3
 * FatShark Race Band receiver module
 * 2 x Adafruit Quad Alphanumeric Display (second display is optional)
 * 2 x buttons
 * photo resistor (optional)
 * resistors for voltage divider
 * antenna for receiver module

## TODO

 * build diagram
 * more documentation
 * field test with >3 active transmitters
 * control for external recorder

