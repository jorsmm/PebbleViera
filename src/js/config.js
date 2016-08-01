module.exports = [
  { 
    "type": "heading", 
    "defaultValue": "Viera Remote Configuration ⚙"
  }, 
  { 
    "type": "text", 
    "defaultValue": "IP addres of your Viera TV must be stablished.<p/>It's not possible to autodetect 😞. I recommend configuring static IP on your TV" 
  },
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "TV Configuration 📺"
      },
      {
        "type": "input",
        "messageKey": "ipaddr",
        "label": "IP Address",
        "defaultValue": "192.168.1.8",
        "attributes": {
          "placeholder": "eg: 192.168.1.8",
          "type": "text"
        }
      },
      {
        "type": "input",
        "messageKey": "port",
        "label": "Port",
        "defaultValue": "55000",
        "attributes": {
          "placeholder": "eg: 55000",
          "limit": 6,
          "type": "number"
        }
      },
    ]
  },
  {
    "type": "submit",
    "defaultValue": "Save"
  }
];
console.log("####Cargando config.js");