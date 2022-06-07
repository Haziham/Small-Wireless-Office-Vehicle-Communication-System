# Small-Wireless-Office-Vehicle-Communication-System
This project contains the software file for the wireless communication between a small office vehicle and a computer.  The user can send waypoint commands to the vehicle and receive key value pairs back. 

Component Manual:

This communication system allows the user to input high level waypoint commands which can be transmitted to a small office vehicle. It also allows the user to view vehicle status updates. 
Setup:
1.	If required, flash the AVD_Remote_Side_Communication_Program.hex file [1] to the ATmega328PB.
2.	Download and install Node.js v16.14.2.
3.	Using a laptop with a type A port, download the Office_Vehicle_Server folder from GigHub [1]. Open command prompt with admin privileges and navigate into the Office_Vehicle_Server folder.
4.	Install the required libraries specified in package.json. If you are using npm package manager run the command “npm install i”, then wait for the libraries to install.
5.	Download and install the FTDI drivers.
6.	Enter the command “node server.js” to start the server (Crtl + c to exit).
7.	Find the local ip address of the device running the server. This can be done by opening a new command prompt and entering “ipconfig” then search for “IPv4 Address”.
8.	Open a web browser that can run JavaScript (Google Chrome is suggested), on the same device running the server and enter the following URL: http://<ip address from step 6>:5000. The GUI (3.4) should be displayed on screen. If the device running the server is connected to the local network, this step will also work for any device connected to the same local network.
9.	Connect the wireless user transceiver module (3.2) to the computer, under 13 seconds the user interface will display “Connection status: OK”

Usage:

From the GUI (3.4), waypoints can be queued up using the confirm waypoint button. The start button can be used to start the vehicle moving through the waypoints. The pause button will pause the vehicle before the next waypoint. Emergency stop will reset the vehicle, stopping it. Any telemetry sent from the vehicle will be logged to the telemetry display.

Subcomponent usage:

The GUI (3.4) is a HTML file that can be used to send requests, based on user input, to any server. A list of user inputs and corresponding requests made can be seen in Table 2.
The server (3.7) is a JavaScript file that is run in the node.js runtime environment, the sever was designed and tested using a windows machine but has the potential to run on other operating systems. Any webpage can make request to the server, a list of requests and action taken by the server can be found in Table 3. The server can be set up to auto connect to any device. This, along with other serial port parameters, can be altered by changing the variables at the top of the server.js file. The server is capable of reading in KVP packets (3.6) and storing them in a list. It can also encode and send waypoint packets (3.5).
The user transmitter module can be easily used as a general-purpose USB to wireless UART radio signal device. While a USB A to USB micro-B cable is included, any USB cable with a male USB micro-B attachment will work. If the UART settings are changed, make sure to adjust the GT-38 setting to match the changes (See the GT-38 datasheet [2] for instructions).
The UART library (3.8) used for programming the vehicle can decode waypoint packets (3.5) and place the waypoint data in a queue. It also provides KVP packet encryption (3.6). The UART library does need access to a queue data structure to work. The UART library may enable the watch dog timer which will need to be turned off elsewhere when the MCU is starting up.

Limitations:

The GT-38 modules are only capable of half-duplex communication. It also prevents transmission of more than 256 at one time (222 was the highest achieved in testing).
Multiple users can access the server at one time and simultaneously send commands to the office vehicle. However, the server will only be able to display received KVP’s to one user.
The communication system has error checking in place to prevent communication between devices being lost. However, in the unlikely case that the error occurs in the string length byte of the KVP packet (3.6), it could take up to 35 seconds (worst case) to reconnect with the office vehicle. Parity checking also could not be enabled on the user side of the system which may result in the occasional unwanted symbol being displayed on the user interface. This is very unlikely to occur if vehicle stays within the recommended 40m distance.
