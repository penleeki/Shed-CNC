# Shed-CNC
This is all of the source code for my home made CNC machine:
https://hackaday.io/project/7603-garden-shed-cnc

It includes the Arduino sketch, and the Arduino Library I wrote to take some of the heavy lifting out of the 
Arduino sketch. It also includes some scripts for use inside the Unity editor (unity3d.com) that allow you to
generate and transmit GCode over the serial port.

I would probably advise against trying to use the code as is, as I wrote it for my specific machine and don't
really want to develop it further than that. It is also still a work in progress. It may come in handy to pick 
through for anyone else building something similar though, as I couldn't find many resources when I was 
writing it.

Everything is internally integer based to the scale of the smallest unit of movement my machine can detect (0.1mm).

The curve generating code is based on the principles of the midpoint circle algorithm:
(https://en.wikipedia.org/wiki/Midpoint_circle_algorithm)
But because I only need to know the next pixel to move to instead of the whole circle it goes about it slightly
differently.



