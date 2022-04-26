/********************************************************************
 * WeatherFlow Tempest Gauges 
 * 
 * Provide interactive calls and handling for the web GUI of
 * the gauges controller.
 * 
 * Copyright: Lincoln Lavoie, 2021
 ********************************************************************/

var wxGaugesWsGateway = `ws://${window.location.hostname}/ws`;
var wxGaugesWS;

const fetchValue = id => document.getElementById(id).value;

// Websocket event handler to update the pages
function onWsMessage(event){
    var jsonMsg = JSON.parse(event.data);
    switch(jsonMsg.type){

        // Update the current weather conditions
        case "status_weather":
            for( var key in jsonMsg.status){
                if( jsonMsg.status.hasOwnProperty(key) ){
                    document.getElementById(key).innerHTML = jsonMsg.status[key];
                }
            }
            break;

        // Update the current system status
        case "status_system":
            for( var key in jsonMsg.status){
                if( jsonMsg.status.hasOwnProperty(key) ){
                    document.getElementById(key).innerHTML = jsonMsg.status[key];
                }
            }
            break;
    }
}

// Update the System Settings
function submitSystemSettings(){
    var jsonSettings = {
        wind: {
            min: Number(fetchValue("min_wind")),
            max: Number(fetchValue("max_wind")),
            step: Number(fetchValue("step_wind")),
            gain: Number(fetchValue("gain_wind")),
            threshold: Number(fetchValue("threshold_wind"))
        },
        temp: {
            min: Number(fetchValue("min_temp")),
            max: Number(fetchValue("max_temp")),
            step: Number(fetchValue("step_temp")),
            gain: Number(fetchValue("gain_temp"))
        },
        cal: {
            mode: fetchValue("cal_mode"),
        }
        
    };
    var jsonMsg = {
        type: "updateSettings",
        payload: jsonSettings
    };
    wxGaugesWS.send(JSON.stringify(jsonMsg));
    console.log(JSON.stringify(jsonMsg));
    return false;
}

// Update the WiFi Settings
function submitWiFi(){
    // FIXME: Prevent submmitting empty ssid and password
    var jsonSettings = {
        wifi:{
            ssid: fetchValue("ssid"),
            pw: fetchValue("wifipass")
        },
    };
    var jsonMsg = {
        type: "updateWiFi",
        payload: jsonSettings
    };
    wxGaugesWS.send(JSON.stringify(jsonMsg));
    console.log(JSON.stringify(jsonMsg));
    return false;
}

// Update the Login Settings
function submitLogin(){
    // FIXME: Prevent submmitting empty username and password
    var jsonSettings = {
        auth:{
            user: fetchValue("username"),
            pass: fetchValue("userpass")
        },
    };
    var jsonMsg = {
        type: "updateUser",
        payload: jsonSettings
    };
    wxGaugesWS.send(JSON.stringify(jsonMsg));
    console.log(JSON.stringify(jsonMsg));
    return false;
}

// Setup websocket connection on page load
window.addEventListener('load', function() {
    console.log('Trying to open a WebSocket connection...');
    wxGaugesWS = new WebSocket(wxGaugesWsGateway);
    //wxGaugesWS.onopen    = onOpen;
    //wxGaugesWS.onclose   = onClose;
    wxGaugesWS.onmessage = onWsMessage; 
    }
);

// Deal with logging out
function logout(){
    var xhr = new XMLHttpRequest();
    xhr.open("GET", "/logout", true);
    //xhr.setRequestHeader("Authorization", "Basic NULLandVOID");
    wxGaugesWS.close(1000, "logging out");
    xhr.send();
    setTimeout(function(){ window.open("/logged-out.html","_self"); }, 500);
}