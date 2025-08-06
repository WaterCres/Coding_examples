# dm510 linux module

This was the second assignment in the dm510 operating systems course

The module is a driver which will let a user communicate with two character devices, each with their own separate buffer for reading and writing, the read buffer of one device maps to the write buffer of the other
as illustrated.
![Bounded buffers](/media/driver.png)

In the media folder there is a video demonstrating the use of this module