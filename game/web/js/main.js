let _socket = undefined;
let _wait = false;
let _receiving = false;
let _content = null;
let _cmd = null;

const delay = ms => new Promise(res => setTimeout(res, ms));

window.onload = function() {
  document.getElementById("cmd").focus();
  _content = document.getElementById("content");
  _cmd = document.getElementById("cmd");
  
  _socket = new WebSocket('ws://localhost:4181');

  _socket.onopen = function(event) { 
    console.log("WebSocket connection open!")
    _socket.send(CreateEvent("#new_connection"));
  };

  _socket.onmessage = async function(event) {
    while (_receiving) { 
      await delay(100); 
    }
    _receiving = true;
    
    console.log("Received payload: ", event.data)
    // Wait till last promt was accepted
    if (event.data == "#remove_prompt") {
      RemovePromptIfExists();
      _receiving = false;
      return;
    }
    while (_wait) { await delay(100); }

    // Print
    if (event.data == "#clear")
      ClearContent()
    else {
      if (AddInput(event.data)) {
        AddInput("Press enter to continue...");
        _wait = true
      }
    }
    _receiving = false;
  };
};

window.onbeforeunload = function() {
  websocket.onclose = function () {}; // disable onclose handler first
  websocket.close();
};

function SendInput(event) {
  if (event.keyCode == 13) {
    RemovePromptIfExists();
    if (_cmd.value != "")
      _socket.send(CreateEvent(_cmd.value));
    _cmd.value = "";
  }
}

function RemovePromptIfExists() {
  if (_wait) {
    _content.removeChild(_content.lastChild);
    _wait = false;
  }
}

function AddInput(payload) {
  var add_prompt = true;
  if (payload[0] === '$') {
    payload = payload.substring(1);
    add_prompt = false;
  }
  var p = document.createElement("p");
  p.classList.add("line");
  p.innerHTML = payload;
  _content.appendChild(p);
  _cmd.value = "";
  return add_prompt;
}

function ClearContent() {
  _content.innerHTML = "";
}

function CreateEvent(event) {
  const payload = {
    game: location.pathname.substring(1, location.pathname.length-1),
    event: event
  }
  return JSON.stringify(payload);
}
