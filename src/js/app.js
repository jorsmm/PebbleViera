var commands=['','VOLUP','MUTE','VOLDOWN',
                 'CH_UP','TV','CH_DOWN',
                 'CHG_INPUT','POWER','INFO'];                            
var ipAddress="192.168.1.8";
var port="55000";

// urls
var URL_NETWORK			= '/nrc/control_0';
var URL_RENDERING		= '/dmr/control_0';
// urns
var URN_NETWORK			= 'panasonic-com:service:p00NetworkControl:1';
var URN_RENDERING		= 'schemas-upnp-org:service:RenderingControl:1';
// actions
var ACTION_SENDKEY		= 'X_SendKey';
var ACTION_GETVOLUME	= 'GetVolume';
var ACTION_GETMUTE		= 'GetMute';
// args
var ARG_SENDKEY			= 'X_KeyEvent';

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
function submitRequest (url, urn, action, options) {
	var command_str = "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"+
"<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">\n"+
" <s:Body>\n"+
"  <u:"+action+" xmlns:u=\"urn:"+urn+"\">\n"+
"   "+options.args+"\n"+
"  </u:"+action+">\n"+
" </s:Body>\n"+
"</s:Envelope>\n";
  console.log('###submitRequest ['+command_str+']');

  var req = new XMLHttpRequest();
  req.open('POST', 'http://'+ipAddress+':'+port+url);
  req.setRequestHeader("Content-Type", "text/xml; charset=utf-8");
  req.setRequestHeader("SOAPACTION", '"urn:'+urn+'#'+action+'"');
  req.onload = function () {
    console.log(">>>>>>>>>>##readyState##"+req.readyState);
    if (req.readyState === 4) {
      if (req.status === 200) {
        console.log("##respuesta##"+req.responseText);
        sendStatus(0);
        console.log("##options##"+options+","+options.hasOwnProperty('callback'));
        if(options.hasOwnProperty('callback')){
          console.log("##llamar a callback##");
          options.callback(req.responseText);
        }
      } else {
        sendStatus(-1);
        console.log('Error:'+req.status+","+req.statusText);
      }
    }
  };
  req.timeout = 2000;
  req.ontimeout = function () { 
    sendStatus(-2);
    console.log('Error TIMEOUT'); 
  };
  req.send(command_str);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * Send a key state event to the TV
 * 
 * @param  {String} key   The key value to send
 * @param  {String} state <ON|OFF|ONOFF>
 */
var sendKey = function(key, state){
	submitRequest(
		URL_NETWORK, 
		URN_NETWORK, 
		ACTION_SENDKEY, 
		{
			args: "<"+ARG_SENDKEY+">NRC_"+key+"-"+state+"</"+ARG_SENDKEY+">"
		}
	);
};
/**
 * Send a key push/release event to the TV
 * 
 * @param  {String} key   The key that has been released
 */
var send = function(key){
	sendKey(key,'ONOFF');
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * Get the current volume value
 * 
 * @param  {Function} callback A function of the form function(volume) to return the volume value to
 */
var getVolume = function(callback){
	submitRequest(
		URL_RENDERING,
		URN_RENDERING,
		ACTION_GETVOLUME,
		{
			args: "<InstanceID>0</InstanceID><Channel>Master</Channel>",
			callback: function(data){
				var regex = /<CurrentVolume>(\d*)<\/CurrentVolume>/gm;
				var match = regex.exec(data);
				if(match !== null){
					var volume = match[1];
					callback(volume);
				}
        else {
          console.log(">>getVolume no encontrada respuesta esperada");
        }
			}
		}
	);
};

/**
 * Get the current mute setting
 * 
 * @param  {Function} callback A function of the form function(mute) to return the volume value to
 */
var getMute = function(callback){
	submitRequest(
		URL_RENDERING,
		URN_RENDERING,
		ACTION_GETMUTE,
		{
			args: "<InstanceID>0</InstanceID><Channel>Master</Channel>",
			callback: function(data){
				var regex = /<CurrentMute>([0-1])<\/CurrentMute>/gm;
				var match = regex.exec(data);
				if(match !== null){
					//var mute = (match[1] == '1');
          var mute = match[1];
					callback(mute);
				}
        else {
          console.log(">>getMute no encontrada respuesta esperada");
        }
			}
		}
	);
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
function sendMyMessage(mymessage) {
	Pebble.sendAppMessage({"message": mymessage});
}

// Function to send a message to the Pebble using AppMessage API
function sendStatus(status) {
	Pebble.sendAppMessage({"status": status});
	
	// PRO TIP: If you are sending more than one message, or a complex set of messages, 
	// it is important that you setup an ackHandler and a nackHandler and call 
	// Pebble.sendAppMessage({ /* Message here */ }, ackHandler, nackHandler), which 
	// will designate the ackHandler and nackHandler that will be called upon the Pebble 
	// ack-ing or nack-ing the message you just sent. The specified nackHandler will 
	// also be called if your message send attempt times out.
}

var pintaRespuestaVolumen = function(respuesta){
  console.log(">>>>>pintaRespuestaVolumen ["+respuesta+"]");
  sendMyMessage("v="+respuesta);
};
var pintaRespuestaMute = function(respuesta){
  console.log(">>>>>pintaRespuestaMute ["+respuesta+"]");
  sendMyMessage("m="+respuesta);
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Called when JS is ready
Pebble.addEventListener("ready",
   function(e) {
      console.log("##2##Event Ready");
      getVolume(pintaRespuestaVolumen);
      getMute(pintaRespuestaMute);
   });

// Called when incoming message from the Pebble is received
Pebble.addEventListener("appmessage",
   function(e) {
      console.log("####Received Status: " + e.payload.status);
      send(commands[e.payload.status]);
   }
);