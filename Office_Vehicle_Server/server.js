/*
    Office vehicle server

        This JavaScript file is used to communicate waypoint instructions from a GUI running in a web browser to a small office vehicle.
        It receives high level waypoint commands from the GUI via TCP/IP protocol. Converts it into a custom UART protocol and sends it to a 
        FTDI USB-UART module. The server will automatically detect and attach itself to FTDI USB-UART modules.

        This JavaScript file sets up a local server on port 5000. It serves up a html file when the port is accessed. 
        This file is designed to work with GUI.html.

    Last update: 28/5/2022
    Authors: UTAS ENG232 2022 Group 2 
    Version: 0
*/


let outgoingWaypoints = Buffer.alloc(0); //A buffer to hold all waypoint messages to be sent to the transmitter module

let incomingBytes = Buffer.alloc(0); //Stores all bytes from transmitter module until processed
let incomingKVPs = []; //Queues all incoming KVPs as a list of strings to be sent to GUI when requested
waitingForTelemetry = true; //Specifies if start byte is found
messageLengthKnown = false; //Specifies if current KVP length has been read
let currentMessageLength = 0; //Length of the current KVP

let connection = false; //Specifies if the server can communicate with the office vehicle
let testConnection = false; //Specifies if a response was received from the office vehicle

let USBtoUART = 'FTDI';//Manufacture name of USB-UART module used
let baud = 9600;//UART connection baud rate
let parity = 'even';//Parity setting for UART connection
let maxByteSendLimit = 207 //max byte send limit per 2 seconds

//instructionByteLUT holds the possible instruction byte values that are sent at the
//beginning of a waypoint message.
let instructionByteLUT = {
    "waypoint" : 10,
    "start" : 11,
    "pause" : 12,
    "stop" : 13,
    "sendKVP": 14
}

//Initialisation of server using express package
let express = require('express');
const { response } = require('express');
const { json, send } = require('express/lib/response');
let server = express();
server.listen(5000); //Open local server of port 5000
server.use(express.json());


//Initialisation of server using serialport package
const { SerialPort } = require('serialport');
var port = new SerialPort({ path: 'COM3', baudRate: baud, parity: parity, autoOpen: false }); //Initialise with arbitrary parameters
connectUartModule();
initReadTelemetry();

//Initialisation of FTDI auto detection using usb-detection package
var usbDetect = require('usb-detection');
usbDetect.startMonitoring();

/*
    This function is called when a new USB device is connected to the computer.
    If it is the FTDI module it will start connecting to it
*/
usbDetect.on('add', function (device) {
    if (!device.manufacturer.localeCompare(USBtoUART)) {
        connectUartModule();
    }
});


/*
    This function will search connected serial devices for FTDI module,
    if it is found it will find its communication path and connect to the device.
*/
function connectUartModule() {
    SerialPort.list().then(function (devices) {
        devices.forEach(function (device) {
            if (!device.manufacturer.localeCompare(USBtoUART)) {
                port = new SerialPort({ path: device.path, baudRate: baud, parity: parity, autoOpen: true }, function (error) {console.log(error)})
                initReadTelemetry();
            }
        })
    })
}

/*
    This function does three things:
        1: Send queued waypoints to office vehicle
        1: Requests KVPs from the office vehicle
        2: Updates the current connection status if it received KVP's from the vehicle
    It ensures that all waypoints have been sent before calling itself again in 2 seconds.
    This ensures the vehicle has at lease 2 seconds to respond.
*/
function sendOutgoingWaypoints () {
    connection = testConnection ? true : false; //Was a KVP received
    if (port.isOpen) { //Don't try and send bytes if transmitter device is not connected
        sendWaypoint(instructionByteLUT.sendKVP, 0, 0, 0); //Requests KVPs
        port.write(outgoingWaypoints.subarray(0, maxByteSendLimit)); //Send waypoints (maximum of 207 bytes at a time)
        port.drain(function (error) { //When finished sending bytes, send next set of bytes in 2 seconds
            setTimeout(sendOutgoingWaypoints, 2000);
        })
        outgoingWaypoints = outgoingWaypoints.subarray(207, outgoingWaypoints.length); //Keep unsent waypoints
    }
    else
    {
        setTimeout(sendOutgoingWaypoints, 2000); //send next set of bytes in 2 seconds
    }
    testConnection = false;
    
}

sendOutgoingWaypoints()

/*
    This function can be called to ensure incoming bytes are read correctly if 
    the parameters of the SerialPort "port" has been recently updated.
*/
function initReadTelemetry() {
    
    /*
        When data is received on the connected serial port this function will store
        the bytes an process them. When precessing the bytes it will search for an encoded message (KVP)
        as specified below:

        Encoded message:    <2><Length of label><ASCII encoded label><Float><3>
        Number of Bytes:    (1)      (1)                (<200)        (4)   (1)
    */
    port.on('data', function (data) {
        //Store all incoming bytes in one buffer
        incomingBytes = Buffer.concat([incomingBytes, data], incomingBytes.length + data.length);
        
        //If the start of the message has not been found, search for it
        if (waitingForTelemetry) 
        {
            for (let i = 0; i < incomingBytes.length; i++)
            {
                if (incomingBytes[i]==2) //Start of message
                {
                    incomingBytes = incomingBytes.subarray(i+1); //Delete all bytes before start of message
                    waitingForTelemetry = false;
                    i = incomingBytes.length;
                }
            }
            if (waitingForTelemetry){ //If start byte not found
                incomingBytes = Buffer.alloc(0); //Delete all current incoming bytes
            }
        }

        //If the start of the message has been found
        else
        {
            //Search and decode the length of the message
            if (!messageLengthKnown && incomingBytes.length > 0)
            {
                currentMessageLength = incomingBytes.readUInt8()+5;
                incomingBytes = incomingBytes.subarray(1);
                messageLengthKnown = true;
            }
            //If the incoming bytes buffer is the expected length
            if(messageLengthKnown && incomingBytes.length >= currentMessageLength)
            {  
                if(incomingBytes[currentMessageLength-1] == 3) //Check if it is a valid message
                {   
                    //Decode KVP
                    let label = incomingBytes.toString('utf8', 0, currentMessageLength-5);
                    let number = incomingBytes.readFloatLE(currentMessageLength-5);
                    
                    if (label.localeCompare("Status OK")) { //If the label is not "Status OK"
                        if (label.localeCompare("Waypoint reached")) //And the label is not "Waypoint Reached"
                        {
                            incomingKVPs.push(label + ": " + number.toFixed(3)); //Queue KVP to be send to GUI
                        }
                        else incomingKVPs.push(label); //Queue label without value
                    }
                    testConnection = true; 
                }
                else //If the message was an invalid length
                {
                    console.log('Message length was incorrect')
                    incomingKVPs.push('Error in receiving telemetry')
                }
                //Reset values to start searching for next KVP
                incomingBytes = incomingBytes.subarray(currentMessageLength); //Keep bytes part of the next message
                waitingForTelemetry = true;
                messageLengthKnown = false;
            }
        }
    }); 

    /*
        Catches any uncaught errors when using serialport library. These often occur when disconnecting 
        USB device while trying to transmit bytes. If these errors are not caught the server will shut down. 
    */
    port.on('error', function (error) {
        console.log(error);
    })
}

/*
    This function will serve up GUI.html when
    home get request is made.
*/
server.get('/', function (request, response) {
    response.sendFile(__dirname + '/GUI.html');
})

/*
    This function will encode the waypoint message and queue it up to be sent to the 
    office vehicle.

    Encoded message: <2><instruction - float><xDistance - float><yDistance - float><speed - float><3>
    Byte size:       (1)        (1)                    (4)              (4)            (4)        (1)
*/
function sendWaypoint(instruction, xDistance, yDistance, speed){
    let newWaypointBuffer = Buffer.alloc(15);   //Initialise buffer to be sent

    newWaypointBuffer.writeUInt8(2, 0); //Start of message
    newWaypointBuffer.writeUInt8(instruction, 1) 
    newWaypointBuffer.writeFloatLE(xDistance, 2);
    newWaypointBuffer.writeFloatLE(yDistance, 6);
    newWaypointBuffer.writeFloatLE(speed, 10);
    newWaypointBuffer.writeUInt8(3, 14); //End of message

    //Add message to outgoing buffer
    outgoingWaypoints = Buffer.concat([outgoingWaypoints, newWaypointBuffer], outgoingWaypoints.length + newWaypointBuffer.length);
}

/*
    On receiving a post request on /tele this function will
    decode waypoint information xDistance, yDistance and speed 
    from the body of the request and send waypoint message with waypoint instruction
    if the serial port is open. Otherwise it will respond with a transmitter not 
    connected error.
*/
server.post('/tele', function (request, response) {

    if (!port.isOpen) { //Don't try and send bytes if transmitter device is not connected
        response.json("Transmitter device is not connected")
    }
    else {
        response.end();
        sendWaypoint(instructionByteLUT.waypoint, 
                request.body.xDistance,
                request.body.yDistance,
                request.body.speed);
    }
})


/*
    On receiving a post request on /start this function will
    send waypoint message with start instruction if the serial 
    port is open. Otherwise it will respond with a transmitter not 
    connected error.
*/
server.post('/start', function (request, response) {

    if (!port.isOpen) { //Don't try and send bytes if transmitter device is not connected
        response.json("Transmitter device is not connected")
    }
    else {
        response.end();
        sendWaypoint(instructionByteLUT.start, 0, 0, 0);
    }
})

/*
    On receiving a post request on /pause this function will
    send waypoint message with pause instruction if the serial 
    port is open. Otherwise it will respond with a transmitter not 
    connected error.
*/
server.post('/pause', function (request, response) {
    if (!port.isOpen) { //Don't try and send bytes if transmitter device is not connected
        response.json("Transmitter device is not connected")
    }
    else {
        response.end();
        sendWaypoint(instructionByteLUT.pause, 0, 0, 0);
    }
})

/*
    On receiving a post request on /stop this function will
    send waypoint message with stop instruction if the serial 
    port is open. Otherwise it will respond with a transmitter not 
    connected error.
*/
server.post('/stop', function (request, response) {
    if (!port.isOpen) { //Don't try and send bytes if transmitter device is not connected
        response.json("Transmitter device is not connected")
    }
    else {
        response.end();
        sendWaypoint(instructionByteLUT.stop, 0, 0, 0);
    }
})

/*
    On receiving a get request on /tele this function will
    send back a list of KVPs to the GUI if the list is not 
    empty.
*/
server.get('/tele', function (request, response) {
    if (incomingKVPs.length != 0) { //If there are KVPs to send back
        response.send(JSON.stringify(incomingKVPs))
        incomingKVPs.length = 0; //Clear KVP buffer
    }
    else response.end();
});


/*
    On receiving a get request on /status this function will
    send back the current connection status with the vehicle.
    1 representing connected, and 0 representing disconnected.
*/
server.get('/status', function (request, response) {
    response.send(JSON.stringify(connection ? 1 : 0));
});




