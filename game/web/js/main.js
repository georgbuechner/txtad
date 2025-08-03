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
    if (event.data == "$done") {
      RemovePromptIfExists();
      _receiving = false;
      return;
    }
    while (_wait) { await delay(100); }

    // Print
    if (event.data == "$clear")
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

function GetCmd(payload_part) {
  let pos = -1;
  for (let i=1; i<payload_part.length; i++) {
    if (payload_part[i] === ' ' || payload_part[i] === '$') {
      pos = i;
      break;
    }
  }
  if (pos == -1)
    return ["", "", 1];
  let full_cmd = payload_part.substring(1, pos); 
  if (full_cmd.indexOf("_") == -1)
    return [full_cmd, "", pos];
  const [cmd, param] = full_cmd.split("_");
  return [cmd, param, pos];
}

function InsertAtCursor(cursor, text, cur) {
  if (cursor == cur.length) {
    return cur + text; 
  } else {
    return cur.substring(0, cursor) + text + cur.substring(cursor+1);
  }
}

function AddInput(payload) {
  var add_prompt = false;
  if (payload.indexOf('$prompt') !== -1) {
    add_prompt = true;
    payload = payload.replace('$prompt', '');
  }

  let cursor = 0;
  let closing = []
  let text = "";
  console.log("Parsing ", payload, payload.length);
  for (let i=0; i<payload.length; i++) {
    if (payload[i] == '$') {
      const [cmd, param, pos] = GetCmd(payload.substring(i));
      if (cmd === "color") {
        const str = '<span style="color: ' + param + '";>';
        text = InsertAtCursor(cursor, str, text);
        closing.push("</span>");
        cursor += str.length;
      } else if (cmd === "italic") {
        const str = '<i>';
        text = InsertAtCursor(cursor, str, text);
        closing.push("</i>");
        cursor += str.length;
      } else if (cmd === "mr") {
        const str = '<span style="margin-right: 0.5em;">';
        text = InsertAtCursor(cursor, str, text);
        closing.push("</span>");
        cursor += str.length;
      } else if (cmd === "center") {
        const str = '<div style="text-align: center;">';
        text = InsertAtCursor(cursor, str, text);
        closing.push("</div>");
        cursor += str.length;
      } else if (cmd === "" && closing.length > 0) {
        const elem = closing.pop();
        text = InsertAtCursor(cursor, elem, text);
        cursor+=elem.length;
      } 
      i += pos-1;
    } else {
      text = InsertAtCursor(cursor, payload[i], text);
      cursor++;
    }
  }
  for (let i=0; i<closing.length; i++) {
    const elem = closing.pop();
    text = InsertAtCursor(cursor, elem, text);
    cursor+=elem.length;
  }

  console.log("-> ", text);

  var p = document.createElement("p");
  p.classList.add("line");
  p.innerHTML = text;

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
