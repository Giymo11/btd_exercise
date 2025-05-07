
How many bytes of data do you send/receive?

We send (additionally to the TCP overhead) exactly the length of the message. This means, for e.g. '+X' we send two chars (=bytes), for 'undefined' we send nine for each orientation change.


Are you converting to String and back? Is this efficient?

No, as the server simply prints the message as received, there is no converting back.


How could you make the method more efficient?

More efficient would be to transmit not a string representation, but a simple number indicating the current orientation (from enum device_orientation_t). This would mean we only send a single byte payload each time. 
Another option would be to use UDP to transmit the data, to reduce the protocol overhead.

