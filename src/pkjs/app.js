var Clay = require('pebble-clay');
var clayConfig = require('./config');
var clay = new Clay(clayConfig);
var messageKeys = require('message_keys');

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
var commands=['','VOLUP','MUTE','VOLDOWN',
                 'CH_UP','TV','CH_DOWN',
                 'CHG_INPUT','POWER','INFO'];
var ipAddress="192.168.0.8";
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

var STATUS_CONSULTAR_ESTADO_MUTE_Y_VOLUMEN = 100;
var STATUS_CONSULTAR_ESTADO_MUTE_Y_VOLUMEN_CON_DELAY = 101;
var STATUS_ENVIAR_TV_APAGADA = 204;
var STATUS_ENVIAR_ERROR = 201;
var STATUS_ENVIAR_ERROR_TIMEOUT = 202;

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
//  console.log('###submitRequest ['+command_str+']');

  var req = new XMLHttpRequest();
  req.open('POST', 'http://'+ipAddress+':'+port+url);
  console.log('###submitRequest POST ['+'http://'+ipAddress+':'+port+url+']');
  req.setRequestHeader("Content-Type", "text/xml; charset=utf-8");
  req.setRequestHeader("SOAPACTION", '"urn:'+urn+'#'+action+'"');
  req.onload = function () {
    console.info("["+action+"]>>>>>>>>>>##readyState##"+req.readyState);
    if (req.readyState === 4) {
      if (req.status === 200) {
        //console.log("##respuesta##"+req.responseText);
        console.log("["+action+"]##options##"+options+","+options.hasOwnProperty('callback'));
        if(options.hasOwnProperty('callback')){
          console.log("["+action+"]##llamar a callback##");
          options.callback(req.responseText);
        }
        else {
          console.log("["+action+"]##No hay Callback: Enviar Status OK (0)##");
          sendStatus(0);
        }
      }
      else if (req.status === 400) {
        console.log("["+action+"]Error 400, Enviar que La TV está apagada ("+STATUS_ENVIAR_TV_APAGADA+"):"+req.status+","+req.statusText);
        sendStatus(STATUS_ENVIAR_TV_APAGADA);
      }
      else {
        console.log("["+action+"]Error:"+req.status+","+req.statusText);
        sendStatus(STATUS_ENVIAR_ERROR);
      }
    }
  };
  req.timeout = 2000;
  req.ontimeout = function () {
    //jsmm 02/06/2016 para probar falsear como que me ha llegado volumen, descomentar 2 lineas siguientes y comentar la última de STATUS_ENVIAR_ERROR_TIMEOUT
//    console.log('Error de TIMEOUT. falsear enviando volumen 8');
//    pintaRespuestaVolumen(100);
    console.log("["+action+"]Error de TIMEOUT. Enviar status "+STATUS_ENVIAR_ERROR_TIMEOUT);
    sendStatus(STATUS_ENVIAR_ERROR_TIMEOUT);
  };
  req.onerror = function () {
    console.log("["+action+"]Error. Enviar status "+STATUS_ENVIAR_ERROR);
    sendStatus(STATUS_ENVIAR_ERROR);
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
  var command=">NRC_"+key+"-"+state;
  console.log("sendkey ["+command+"]");
	submitRequest(
		URL_NETWORK,
		URN_NETWORK,
		ACTION_SENDKEY,
		{
			args: "<"+ARG_SENDKEY+command+"</"+ARG_SENDKEY+">"
		}
	);
};
/**
 * Send a key push/release event to the TV
 *
 * @param  {String} key   The key that has been released
 */
var send = function(key){
  console.log("send ["+key+"]");
	sendKey(key,'ONOFF');
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
var pintaRespuestaVolumen = function(respuesta){
  console.log(">>>>>pintaRespuestaVolumen ["+respuesta+"]");
  sendMyMessage("v="+respuesta);
};

/**
 * Get the current volume value
 *
 * @param  {Function} callback A function of the form function(volume) to return the volume value to
 */
var getVolume = function(){
  console.log("getVolume");
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
					pintaRespuestaVolumen(volume);
				}
        else {
          console.log(">>getVolume no encontrada respuesta esperada");
        }
			}
		}
	);
};
var pintaRespuestaMute = function(respuesta){
  console.log(">>>>>pintaRespuestaMute ["+respuesta+"]");
  sendMyMessage("m="+respuesta);
  getVolume();
};

/**
 * Get the current mute setting
 *
 * @param  {Function} callback A function of the form function(mute) to return the volume value to
 */
var getMute = function(){
  console.log("getMute");
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
					pintaRespuestaMute(mute);
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

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Called when JS is ready
Pebble.addEventListener("ready",
   function(e) {
      console.log("####JavaScript Ready");
      sendMyMessage("init=true");
   }
);

// Called when incoming message from the Pebble is received
Pebble.addEventListener("appmessage",
   function(dict) {
     console.log("####Received from Watch: " + JSON.stringify(dict.payload));

     if(dict.payload.status) {
       console.log("####Received Status from Watch: " + dict.payload.status);
       if (dict.payload.status == STATUS_CONSULTAR_ESTADO_MUTE_Y_VOLUMEN) {
         // se pide el mute, y en la respuesta de este se pide el volumen secuencialmente
         getMute();
       }
       else if (dict.payload.status == STATUS_CONSULTAR_ESTADO_MUTE_Y_VOLUMEN_CON_DELAY) {
         // se pide con delay porque se acaba de pedir encender: el mute, y en la respuesta de este se pide el volumen secuencialmente
         setTimeout(getMute,500);
       }
       else {
         send(commands[dict.payload.status]);
       }
     }
     if(dict.payload.configipaddr) {
       console.log("####Received configipaddr message from Watch: " + dict.payload.configipaddr);
       ipAddress=dict.payload.configipaddr;
     }
     if(dict.payload.configport) {
       console.log("####Received configport message from Watch: " + dict.payload.configport);
       port=dict.payload.configport;
     }
     if(dict.payload.configure) {
       console.log("####Received configure message from Watch: " + dict.payload.configure);
       sendMyMessage("configured=true");
     }
   }
);

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// called when config page is closed
Pebble.addEventListener('webviewclosed', function(e) {
  var claySettings = clay.getSettings(e.response);
  ipAddress=claySettings[messageKeys.ipaddr];
  console.log('IP Addr is ' + ipAddress);
  port=claySettings[messageKeys.port];
  console.log('Port is ' + port);
});
