# Haku

A 5.8 GHz video scanner receiver using FatShark race band receiver module with the following features:

 * 32 channel scanning in frequency order
 * channel match optimizer by comparing surrounding channels
 * automatic rssi noise floor calibration
 * continuous scan mode, next channel is search if currently locked channel stops broadcasting for predefined time
 * band, channel number, channel frequency and relative rssi display
 * channel hold mode regardless of rssi
 * automatic display brightness selection
 * menu for active band selection

## Why

Having used two commercial 5.8 GHz video scanner receivers left me wanting for something better. One had a habit of locking to whatever noisy signal it was first able to find, usually finding the wrong channel. The other wasn't able to get a lock unless the transmitter was within few meters of the receiver. The scanning logic in both was to lock on the first channel that peaks enough from the noise floor. If that didn't result in the correct channel then fine tuning wasn't straightforward as channels weren't in frequency order.

A spectator display with such receiver connected wasn't an optimal solution since someone needed to manually restart the channel scan every time transmission on the found channel ended. Haku ended up being born to solve these issues and give me time to fly instead of having to constantly fiddle with the spectator display receiver.

## Usage

Once powered, Haku will display its version string will performing an initial rssi range scan. This step takes few seconds. The number of active channels and list of active bands will then be shown for few seconds. When all bands are active, it's normal for the channel count to show 31 instead of 32 as one duplicate/unofficial channel (FatShark/ImmersionRC channel 8) is ignored.

While scanning, a spinner animation is shown as the first character of the primary display while the secondary display shows the current frequency. The primary display will also show the raw rssi value (usually values over 100 due to background noise) as long as no distinctive signal has been found. Once a signal has been seen, the rssi value will switch to display a relative rssi value (0-99%).

When a strong enough signal is found, Haku will check surrounding channels to locate the rssi peak and then lock to the channel with the highest rssi value. At that point, the spinner animation gets replaces with the band id (D = FatShark/ImmersionRC, E = Boscam E, A = Boscam A, R = Race Band) followed by channel number and live relative rssi value. The secondary display will show the actual frequency. Note that depending on antenna tuning, the found channel may not be exactly the same as what has been selected on the transmitter.

If the relative rssi value stays below 50% for over 5 seconds, Haku will restart scanning for a new channel.

At any point after the initial calibrations have been completed, pressing the HOLD button will force the current frequency to be kept. Pressing the NEXT button will select the next channel making possible using the scanner manually if hold mode is also active or skipping to the next transmitting channel if the current channel doesn't show desired content.

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
 * control for external recorder

