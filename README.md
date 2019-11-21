# 2560pro-smt172-needlevalve
Controls a needle valve mechanism using a drv8825 steppermotor driver, reports temperature from 2 smt172 sensors and measures differential pressure using a mpx7002dp sensor.
It is meant to be used on a Arduino Mega 2560 or Mega 2560 Pro in combination with a raspberry pi.

The commands that can be sent are the following:

`cal:x:y:z`

This calibrates the needle valve mechanism. It seeks the home-switch and settles there, then it moves with speed z(steps/s) and acceleration y(steps/s/s), x steps CW or CCW (-x = CCW, x = CW) and defines that position to be 0. This makes manual fine-tuning possible to make sure the needle valve can be opened with the motor without loosing steps.

`spd:x`

This defines the speed in steps/s that is to be used when moving or positioning the motor of the needle valve mechanism

`acc:x`

This sets the acceleration that will be used when moving or positioning the motor of the needle valve mechanism

`move:x`

This opens (-x) or closes (x) the needle valve. It does not respond until calibrated.

`pos:x`

This takes care of the absolute positioning. -x is a position that can be reached going CCW and x a position that can be reached in the CW direction and has the calibrated 0 as the starting point. It does not respond until calibrated.

`es`

This returns the state of the home-switch (NO): 0(closed) or 1(open)

`mode:x`

This selects the microstepping mode on the drv8825 steppermotor driver. `1` is full step, `2` is 1/2 step, `4` is 1/4, `8` is 1/8 step, `16` is 1/16 step and `32` is 1/32 step. The default is full step mode.

`motor:x`

This enables or disables the motor, so it is not held in position actively. x can be `disable` or `enable`, so `motor:disable` will free the motor. The default is enabled.  

The Mega will report every second the temperatures in Celsius and the differential pressure in kPa using the format `t1:xx,t2:yy,p:zz`. If the temperature sensors are not present,
it will report -1 as the value for that sensor. Absence of the pressure sensor will result in a negative value caused by the floating A0 pin.

The install script is meant to be run on a raspberry pi like this:
`wget -O - https://raw.githubusercontent.com/nrbrt/2560pro-smt172-needlevalve/master/install.sh | sh`

This will program the Mega, that needs to be connected at that moment, without any user interaction and is meant for novice users
and easy installation.

The smt172 sensors need to be connected to pin 48 and pin 49 and need some electronics to connect to the Arduino as shown in the connection diagram, the drv8825 connects to pin 12(dir) and pin 11(step), the home-switch to pin 10 and the mpx7002dp connects to A0. For microstepping that can be set by software, D29 on the mega connects to m0 on the drv8825, D27 to m1 and D25 to m2. If you want to be able to enable/disable the motor, D31 on the mega connects to "enable" on the drv8825.

This sketch uses the great smt172 library and smt172 connection diagram picture by Edwin Croissant.
