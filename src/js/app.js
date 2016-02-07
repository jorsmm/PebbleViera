function sendToViera(command, option) {
  console.log('####sendToViera'+command+','+option);

  var cuerpo='<?xml version="1.0" encoding="utf-8"?>'+
    '<s:Envelope xmlns:s="http://schemas.xmlsoap.org/soap/envelope/" s:encodingStyle="http://schemas.xmlsoap.org/soap/encoding/">'+
    '<s:Body><u:X_SendKey xmlns:u="urn:panasonic-com:service:p00NetworkControl:1">'+
    '<X_KeyEvent>'+command+'</X_KeyEvent>'+
    '</u:X_SendKey></s:Body></s:Envelope>';
  var req = new XMLHttpRequest();
  req.open('POST', 'http://192.168.1.8:55000/nrc/control_0');
//  req.open('POST', 'http://192.168.1.2:55000/nrc/control_0');
//    '?lat=' + latitude + '&lon=' + longitude + '&cnt=1&appid=' + myAPIKey, true);
  req.setRequestHeader("Content-Type", "text/xml; charset=utf-8");
  req.setRequestHeader("SOAPACTION", '"urn:panasonic-com:service:p00NetworkControl:1#X_SendKey"');         
  req.onload = function () {
    if (req.readyState === 4) {
      if (req.status === 200) {
        console.log("####"+req.responseText);
        sendMessage(0);
      } else {
        sendMessage(0);
        console.log('Error:'+req.status+","+req.statusText);
      }
    }
  };
  req.send(cuerpo);
}

// Function to send a message to the Pebble using AppMessage API
function sendMessage(status) {
	Pebble.sendAppMessage({"status": status});
	
	// PRO TIP: If you are sending more than one message, or a complex set of messages, 
	// it is important that you setup an ackHandler and a nackHandler and call 
	// Pebble.sendAppMessage({ /* Message here */ }, ackHandler, nackHandler), which 
	// will designate the ackHandler and nackHandler that will be called upon the Pebble 
	// ack-ing or nack-ing the message you just sent. The specified nackHandler will 
	// also be called if your message send attempt times out.
}


// Called when JS is ready
Pebble.addEventListener("ready",
							function(e) {
                console.log("####Event Ready");
//                sendToViera('NRC_VOLUP-ONOFF');
							});
												
// Called when incoming message from the Pebble is received
Pebble.addEventListener("appmessage",
							function(e) {
								console.log("####Received Status: " + e.payload.status);
                if (e.payload.status == 1) {
                sendToViera('NRC_VOLUP-ONOFF');
                }
                else if (e.payload.status == 2) {
                sendToViera('NRC_VOLDOWN-ONOFF');
                }
                else if (e.payload.status == 3) {
                sendToViera('NRC_POWER-ONOFF');
                }
                else {
                  console.log("####Received Status RARO: " + e.payload.status);
                }
                  //sendMessage();
							});