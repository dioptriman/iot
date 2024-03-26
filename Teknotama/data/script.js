var gateway = `ws://${window.location.hostname}/ws`;
var websocket;
// Init web socket when the page loads
window.addEventListener("load", onload);

function onload(event) {
  initWebSocket();
  initButton();
}

function getReadings() {
  websocket.send("getReadings");
}

function initButton() {
  document.getElementById("button").addEventListener("click", toggle);
}
function toggle() {
  websocket.send("toggle");
}

function initWebSocket() {
  console.log("Trying to open a WebSocket connection…");
  websocket = new WebSocket(gateway);
  websocket.onopen = onOpen;
  websocket.onclose = onClose;
  websocket.onmessage = onMessage;
}

// When websocket is established, call the getReadings() function
function onOpen(event) {
  console.log("Connection opened");
  getReadings();
}

function onClose(event) {
  console.log("Connection closed");
  setTimeout(initWebSocket, 2000);
}

// Function that receives the message from the ESP32 with the readings
function onMessage(event) {
  // Try parsing the data as JSON
  try {
    var myObj = JSON.parse(event.data);
    var keys = Object.keys(myObj);

    for (var i = 0; i < keys.length; i++) {
      var key = keys[i];
      document.getElementById(key).innerHTML = myObj[key];
    }
  } catch (error) {
    // If parsing fails, assume it's a simple string message ("1" or "0")
    console.log(event.data);
    var state;
    if (event.data == "1") {
      state = "ON";
    } else {
      state = "OFF";
    }
    document.getElementById("state").innerHTML = state;
  }
}
