let socket = undefined;

window.onload = function() {
  document.getElementById("cmd").focus();
  
  socket = new WebSocket('ws://localhost:4181');

  socket.onopen = function(event) { 
    console.log("WebSocket connection open!")
    socket.send(CreateEvent("#new_connection"));
  };

  socket.onmessage = function(event) {
    console.log("Received payload: ", event.data)
    if (event.data == "#clear")
      ClearContent()
    else 
      AddInput(event.data);
  };
};

window.onbeforeunload = function() {
  websocket.onclose = function () {}; // disable onclose handler first
  websocket.close();
};

function SendInput(event) {
  if (event.keyCode == 13) {
    socket.send(CreateEvent(document.getElementById("cmd").value));
    document.getElementById("cmd").value = "";
  }
}

function AddInput(payload) {
  var p = document.createElement("p");
  p.classList.add("line");
  p.innerHTML = payload;
  document.getElementById("content").appendChild(p);
  document.getElementById("cmd").value = "";
}

function ClearContent() {
  document.getElementById("content").innerHTML = "";
}

function CreateEvent(event) {
  const payload = {
    game: location.pathname.substring(1, location.pathname.length-1),
    event: event
  }
  return JSON.stringify(payload);
}
