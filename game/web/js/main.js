let _socket = undefined;
let _wait = false;
let _audio_block = false;
let _receiving = false;
let _content = null;
let _cmd = null;

let _bg_sound_file = ''
let _bg_sound = ''
let _bg_sound_play = null;
const delay = ms => new Promise(res => setTimeout(res, ms));

const PROMPT = '$prompt';
const BGSOUND_SET = '$bgsset_';
const BGSOUND_PLAY = '$bgsplay';
const BGSOUND_PAUSE = '$bgspause';
const FGSOUND_PLAY = '$fgsplay_';

const STD_PROMPT = "Press enter to continue...";
const FG_PROMPT = "Waiting...";

window.onload = function() {
  document.getElementById("cmd").focus();
  _content = document.getElementById("content");
  _cmd = document.getElementById("cmd");
  
  const wsUrl =
  location.protocol === 'https:'
    ? `wss://${location.host}/ws`
    : `ws://${location.hostname}:4181`;

  _socket = new WebSocket(wsUrl);

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
      const payload = event.data; 
      if (payload.indexOf(PROMPT) !== -1) {
        for (let part of payload.split(PROMPT)) {
          AddInput(part);
          AddInput((_audio_block) ? FG_PROMPT : STD_PROMPT);
          _wait = true;
          while (_wait) { await delay(100); }
        }
      } else {
        AddInput(event.data)
      }
    }
    _receiving = false;
  };

  // Audio handling: 
  const unlockAndPlay = () => {
    // If WebAudio is suspended (common on first page load), resume it.
    if (Howler.ctx && Howler.ctx.state === 'suspended') {
      Howler.ctx.resume().catch(e => console.error('resume failed', e));
    }
    if (_bg_sound_file !== "")
      _bg_sound_play = _bg_sound.play();
    console.log('play() called, id=', _bg_sound_play, 'ctx=', Howler.ctx?.state);
  };
  // Optional: auto-unlock on first pointer/keydown anywhere on the page
  const autoUnlock = () => { unlockAndPlay(); window.removeEventListener('pointerdown', autoUnlock); window.removeEventListener('keydown', autoUnlock); };
  window.addEventListener('pointerdown', autoUnlock, { once: true });
  window.addEventListener('keydown', autoUnlock, { once: true });
};

window.onbeforeunload = function() {
  websocket.onclose = function () {}; // disable onclose handler first
  websocket.close();
};

function SendInput(event) {
  if (event.keyCode == 13 && !_audio_block) {
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

function FgBlock() {
  _audio_block = true;
}
function RemoveFgBlock() {
  _audio_block = false;
  _content.lastChild.innerHTML = STD_PROMPT;
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

function ParseParam(in_cmd, payload) {
  let pos = payload.indexOf(in_cmd);
  let audio_cmd_end_pos = (payload.indexOf(' ', pos) !== -1) 
    ? payload.indexOf(' ', pos)
    : (payload.indexOf('$', pos+1) !== -1) 
        ? payload.indexOf('$', pos+1)
        : payload.length;
  const cmd = payload.substring(pos, audio_cmd_end_pos);
  return [cmd.substring(cmd.indexOf('_')+1), payload.replace(cmd, '')];
}

function SetBackgroundSound(audio_file) {
  console.log("[bg-audio] setting: ", audio_file);
  PauseBackgroundAudio();
  _bg_sound_file = audio_file;
  _bg_sound = new Howl({ src: ['game_files/' + _bg_sound_file ] });
  PlayBackgroundAudio();
}

function PlayBackgroundAudio() {
  console.log("[bg-audio] play");
  if (_bg_sound) {
    _bg_sound_play = _bg_sound.play();
  }
}

function PauseBackgroundAudio() {
  console.log("[bg-audio] pause");
  if (_bg_sound) {
    _bg_sound_play = _bg_sound.pause();
  }
}

function PlayForegroundMusik(audio_file, {duckTo=0.5, fadeMS = 200 } = {}) {
  return new Promise((resolve, reject) => {
    // Duck background (if present) 
    const preVol = (_bg_sound) ? _bg_sound.volume() : null; 
    if (_bg_sound && preVol !== null) {
      _bg_sound.fade(_bg_sound.volume(), duckTo, fadeMS); 
    }
    FgBlock();

    const duckBack = () => {
      if (_bg_sound&& preVol !== null) {
        _bg_sound.fade(_bg_sound.volume(), preVol, fadeMS);
      }
    };

    const audio = new Howl({
      src: ['media/' + audio_file],
      volume: 1.0,
      onloadererror: (_, err) => {
        duckBack();
        reject(err);
      },
      onplayerror: (_, err) => {
        duckBack();
        reject(err);
      }
    }); 

    audio.once('end', () => {
      RemoveFgBlock();
      duckBack(); 
      resolve();
    });

    audio.once('stop', () => {
      RemoveFgBlock();
      duckBack(); 
      resolve();
    });

    audio.play();
  });
}

function AddInput(payload) {
  if (payload.indexOf(BGSOUND_SET) !== -1) {
    const [audio_file, modified_payload] = ParseParam(BGSOUND_SET, payload);
    SetBackgroundSound(audio_file);
    payload = modified_payload;
  }
  if (payload.indexOf(BGSOUND_PLAY) !== -1) {
    PlayBackgroundAudio();
    payload = payload.replace(BGSOUND_PLAY, '');
  }
  if (payload.indexOf(BGSOUND_PAUSE) !== -1) {
    PauseBackgroundAudio();
    payload = payload.replace(BGSOUND_PAUSE, '');
  }
  if (payload.indexOf(FGSOUND_PLAY) !== -1) {
    const [audio_file, modified_payload] = ParseParam(FGSOUND_PLAY, payload);
    console.log("FG: ", audio_file);
    PlayForegroundMusik(audio_file);
    payload = modified_payload;
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
