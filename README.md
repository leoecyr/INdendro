INdendro dendrometer
This is an Arduino compatible firmware targeted to the 8MHz, 3.3v pro-mini for its small size and low power capability which is essential for the INdendro dendrometer nodes.

enabling debug in the dendrometer node will increase the amount of RAM required to the point where it will not behave reliably.  At the moment, the limit of what can easily run safely in 2k of RAM is being pushed.  Careful optimization and correctness must follow to allow stable operation when debugging is enabled.

## Troubleshooting
If you believe you have correctly constructed and flashed nodes (dendrometer and hub), the first thing to verify are the serial settings for debugging the dendrometer node and the -hub side to ensure that you are using the correct serial port parameters.  Double check the BAUD define in the code, in case your version is different.  The default is 9600 bps, 8 data bits, parity None, and 1 stop bit or: 9600 8N1 as it is often written.  An incorrectly configured serial connection to the hub will hide all the the dendrometry data!

