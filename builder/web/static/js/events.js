const vars = ["id", "name", "desc"];
const ptrs = [".", "->"];
const opts = ["+=", "-=", "++", "--", "*=", "/=", "="];

let using_events = "";
let using_event_id = "";
let using_cur_ctx = "";

function OpenEventEditor(event_id, cur_ctx) {
  const modal = new bootstrap.Modal(document.getElementById('shareEventModal'));
  modal.show();
  const events = document.getElementById(event_id).value;
  console.log(document.getElementById(event_id), events);
  document.getElementById("events_preview").innerHTML = events;
  using_events = events;
  using_event_id = event_id;
  using_cur_ctx = cur_ctx;
  UpdateEventList(events);
}

function UpdateEventList(events, i_open = -1) {
  using_events = events;
  document.getElementById("events_list").innerHTML = "";

  const events_list = events.split(";");
  for (let i=0; i<events_list.length; i++) {
    const trimmed_evnt = events_list[i].trimStart();
    const cmd = Object.keys(events_config).find(cmd => trimmed_evnt.startsWith(cmd + " "));
    const error = (cmd === undefined);
    console.log(trimmed_evnt, i, events_list.length, cmd, error);

    const body_html = `
      <form id="event-form_${i}" class="m-3"> 
        <div class="input-group mb-3">
          <span class="input-group-text">Command:</span>
          <select class="form-select" id="event_cmd_${i}" onchange="UpdateEventCmd(this, ${i});"> 
            <option selected >---</option>
            ${Object.keys(events_config).map(cmd =>
              '<option' + ((trimmed_evnt.startsWith(cmd)) ? ' selected >' : '>') + cmd + '</option>'
            )}
          </select>
        </div>
        ${CreateEventFields(cmd, (cmd) 
          ? trimmed_evnt.substr(cmd.length) 
          : trimmed_evnt.substr(trimmed_evnt.indexOf(" ")))}
      </form>
      <div class="align-bottom text-center mt-2">
        <button type="button" class="btn btn-secondary fs-6" onclick="AddEvent(${i})" title="Add Before" >
          <i class="bi bi-arrow-left"></i>
          <i class="bi bi-plus-lg"></i>
        </button>
        <button type="button" class="btn btn-secondary fs-6" onclick="AddEvent(${i+1})" title="Add After" >
          <i class="bi bi-plus-lg"></i>
          <i class="bi bi-arrow-right"></i>
        </button>
        <button type="button" class="btn btn-success fs-6" onclick="ApplyEvent(${i})">
          <i class="bi bi-floppy"></i>
        </button>
        <button type="button" class="btn btn-danger fs-6" onclick="RemoveEvent(${i})">
          <i class="bi bi-trash3"></i>
        </button>
      </div>
    `;
    
    addAccordionItem({
      accordionId: "events_list",
      itemId: "events_list_" + i,
      title: trimmed_evnt,
      bodyHtml: body_html,
      error: error,
      open: (i_open === i)
    });
  }
}

function UpdateEventCmd(elem, index) {
  const events = using_events.split(";");
  events[index] = elem.value + " "; 
  UpdateEventList(events.join(';'), index); 
}

function AddEvent(index) {
  const events = using_events.split(";");
  events.splice(index, 0, " ");
  UpdateEventList(events.join(';'), index); 
}

function RemoveEvent(index) {
  const events = using_events.split(";");
  events.splice(index, 1);
  UpdateEventList(events.join(';'), index); 
}

function ApplyEvent(index) {
  const events = using_events.split(";");
  const form = document.getElementById(`event-form_${index}`);
  let cmd = document.getElementById(`event_cmd_${index}`).value;
  let event_str = cmd + " ";
  let event_parts = events_config[cmd];
  let p_i = 1;
  
  console.log("building...");
  Array.from(form.elements).forEach((input, i) => {
    // args
    if (i > 0) {
      console.log(i, input.value, event_str);
      event_str += input.value; 
      console.log("->", event_str);
      if (event_parts.length > p_i) {
        console.log(" [apply joiner? " + event_parts[p_i] + "]");
        if (event_parts[p_i].indexOf("[") == -1 && event_parts[p_i].indexOf("<") == -1) {
          event_str += event_parts[p_i];
          p_i++;
        } 
      }
      p_i++;
    }
  });
  console.log("=>", event_str);
  events[index] = event_str;
  UpdateEventList(events.join(';'), index); 
}

function ApplyEvents() {
  document.getElementById(using_event_id).value = using_events;
  const el = document.getElementById('shareEventModal');
  const modal = bootstrap.Modal.getInstance(el);
  modal.hide();
}

function CreateEventFields(cmd, args) {
  console.log("CreateEventFields: ", cmd, args);
  if (cmd === undefined) {
    console.log(cmd, "CreateEventFields: returns nothing");
    return "";
  }
  let res = "";
  const elems = SplitArgs(cmd, args);
  let i=0;
  for (const arg of events_config[cmd]) {
    const value = elems[i] ?? "";
    const tvalue = value.trim();
    console.log("value: ", value, "arg: ", arg, "i-1: ", elems[i-1], "i-2: ", elems[i-2]);
    if (arg === "<free>") {
      const inp_id = "evt_free_inp";
      res += `
        <div class="input-group mb-3">
          <span class="input-group-text">Free Input:</span>
          <div class="flex-grow-1">
            <input 
              type="text" 
              class="form-control" 
              placeholder=" " 
              id="${inp_id}" 
              value="${tvalue}" 
              onfocus="SetUpTypeahead('${inp_id}');"
              onkeydown="Typeahead(event, '${inp_id}');"
              autocomplete="off"
            >
            <div class="form-text" id="hints_${inp_id}" class="overflow" for="${inp_id}"></div>
          </div>
        </div>
      `
      i++;
    } else if (arg === "<ctx>") {
      res += `
        <div class="input-group mb-3">
          <span class="input-group-text">Context (match):</span>
          <select class="form-select"> 
            '<option selected>---</option>'
            '<option selected>#event</option>'
            ${context_ids().map(ctx_id =>
              '<option' + ((tvalue.startsWith(ctx_id)) ? ' selected >' : '>') + ctx_id + '</option>'
            )}
          </select>
        </div>
      `
      i++;
    } else if (arg === "[ctx]") {
      res += `
        <div class="input-group mb-3">
          <span class="input-group-text">Context (type):</span>
          <select class="form-select"> 
            <option selected>---</option>
            <option ${(tvalue.startsWith('_')) ? ' selected >' : '>'}_</option>
            ${context_types().map(ctx_id =>
              '<option' + ((tvalue.startsWith(ctx_id)) ? ' selected >' : '>') + ctx_id + '</option>'
            )}
          </select>
        </div>
      `
      i++;
    } else if (arg === "[attr]") {
      console.log(arg, elems[i-1], context_attributes(elems[i-1].trim()));
      let ctx_id = elems[i-1].trim();
      if (ctx_id === "_") {
        ctx_id = using_cur_ctx;
      }
      res += `
        <div class="input-group mb-3">
          <span class="input-group-text">Attribute:</span>
          <select class="form-select"> 
            '<option selected>---</option>'
            ${context_attributes(ctx_id).map(attr_key =>
              '<option' + ((tvalue.startsWith(attr_key)) ? ' selected >' : '>') + attr_key + '</option>'
            )}
          </select>
        </div>
      `
      i++;
    } else if (arg === "[tattr]" || (arg === "[tattr|var]" && elems[i-1] == ".")) {
      let ctx_id_hint = (arg === "[tattr]") ? elems[i-1] : elems[i-2];
      res += `
        <div class="input-group mb-3">
          <span class="input-group-text">Attribute:</span>
          <select class="form-select"> 
            '<option selected>---</option>'
            ${type_attributes(ctx_id_hint.trim()).map(attr_key =>
              '<option' + ((tvalue.startsWith(attr_key)) ? ' selected >' : '>') + attr_key + '</option>'
            )}
          </select>
        </div>
      `
      i++;
    } else if (arg === "[type]") {
      res += `
        <div class="input-group mb-3">
          <span class="input-group-text">Type:</span>
          <select class="form-select"> 
            <option selected>---</option>
            ${types().map(type =>
              '<option' + ((tvalue.startsWith(type)) ? ' selected >' : '>') + type + '</option>'
            )}
          </select>
        </div>
      `
      i++;
    } else if (arg === "<var>" || (arg === "[tattr|var]" && elems[i-1] == "->")) {
      res += `
        <div class="input-group mb-3">
          <span class="input-group-text">Operand:</span>
          <select class="form-select"> 
            <option selected>---</option>
            ${vars.map(v => '<option' + ((tvalue.startsWith(v)) ? ' selected >' : '>') + v + '</option>')}
          </select>
        </div>
      `
      i++;
    } else if (arg === "<opt>") {
      res += `
        <div class="input-group mb-3">
          <span class="input-group-text">Operand:</span>
          <select class="form-select"> 
            '<option selected>---</option>'
            ${opts.map(opt => 
              '<option' + ((tvalue.startsWith(opt)) ? ' selected >' : '>') + opt + '</option>'
            )}
          </select>
        </div>
      `
      i++;
    } else if (arg === "<ptr>") {
      res += `
        <div class="input-group mb-3">
          <span class="input-group-text">Ptr:</span>
          <select class="form-select"> 
            '<option selected>---</option>'
            ${ptrs.map(ptr => 
              '<option' + ((tvalue.startsWith(ptr)) ? ' selected >' : '>') + ptr + '</option>'
            )}
          </select>
        </div>
      `
      i++;
    }
  }
  return res;
}

function SplitArgs(cmd, args) {
  if (cmd === "#ctx replace") {
    return args.split(" -> ");
  } else if (cmd === "#ctx name") {
    return args.split(" = ");
  } else if (cmd === "#remove_user") {
    return args.split(".");
  } else if (cmd === "#sa") {
    return setAttribute(args);
  } else if (cmd === "#lst linked_ctxs") {
    return listLinkedCtxs(args);
  } else if (cmd === "#lst* ctxs") {
    return listCtxs(args);
  } else {
    return [args];
  }
}

function setAttribute(inp) {
  // Find operator (first match in opts order, like your C++)
  let opt = "";
  let pos = -1;

  for (const it of opts) {
    const p = inp.indexOf(it);
    if (p !== -1) {
      opt = it;
      pos = p;
      break;
    }
  }

  if (pos === -1) {
    console.warn(`setAttribute: no operator found: ${inp}`);
    return ["", "", "", ""];
  }

  // Split input into attribute and expression
  const tmp_attribute = inp.slice(0, pos);
  const expression = inp.slice(pos + opt.length);

  console.debug(
    `setAttribute: attribute: ${tmp_attribute}, opt: ${opt}, expression: ${expression}`
  );

  // Split attribute into ctx-id and attribute-id
  const dot = tmp_attribute.lastIndexOf(".");
  if (dot === -1) {
    console.warn(`setAttribute: invalid attribute id: ${tmp_attribute}`);
    return ["", "", opt, expression];
  }

  const ctx_id = tmp_attribute.slice(0, dot);
  const attribute_id = tmp_attribute.slice(dot + 1);

  return [ctx_id, attribute_id, opt, expression];
}

function listLinkedCtxs(inp) {
  let pos = inp.indexOf("->");
  if (pos === -1) {
    return ["", "", "", ""];
  }
  const ctx_type = inp.substr(0, pos);
  inp = inp.substr(pos+2);
  let ptr = ".";
  pos = inp.indexOf(ptr);
  if (pos === -1) {
    ptr = "->";
    pos = inp.indexOf(ptr);
    if (pos === -1) {
      return [ctx_type, "", "", ""];
    }
  }
  const type = inp.substr(0, pos);
  const tattr = inp.substr(pos+ptr.length); 
  return [ctx_type, type, ptr, tattr];
}
function listCtxs(inp) {
  let ptr = ".";
  pos = inp.indexOf(ptr);
  if (pos === -1) {
    ptr = "->";
    pos = inp.indexOf(ptr);
    if (pos === -1) {
      return ["", "", ""];
    }
  }
  const ctx_type = inp.substr(0, pos);
  const tattr = inp.substr(pos+ptr.length); 
  return [ctx_type, ptr, tattr];
}
