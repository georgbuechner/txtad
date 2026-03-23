const OPERATORS_HINT = 'Use "."/"->" to access element attributes/variables';

class State {
  constructor(name) {
    this.name = name;
    this.inp = "";
    this.suggestions = [];
    this.index = 0;
    this.prev = null;
  }

  handler(e) {}
  render() {}
};

class STD extends State {
  constructor() {
    super("STD");
  }
  handler(e) { 
    if (e.key == "{") {
      StateMaschine.transition(new REP_All(this));
    }    
    else if (e.key == "$") {
      StateMaschine.transition(new GOPT(this));
    }
  }
};

class REP extends State {
  constructor(name, initial_suggestions, prev) {
    super(`REP_${name}`);
    this.base_suggestions = initial_suggestions;
    this.suggestions = initial_suggestions;
    this.prev = prev;
  }
  handler(e) { 
    if (e.key.length == 1 && isValidIdStr(e.key)) {
      this.inp += e.key;
      this.suggestions = this.base_suggestions.filter((id) => id.indexOf(this.inp) == 0);
    } else if (e.key === "Backspace") {
      if (this.inp.length > 0) {
        this.inp = this.inp.substr(0, this.inp.length-1);
      } else {
        StateMaschine.transition(this.prev);
      }
    } else if (e.key == "ArrowDown") {
      e.preventDefault();
      if (this.index < this.suggestions.length-1) {
        this.index++;
      }
    } else if (e.key == "ArrowUp") {
      e.preventDefault();
      if (this.index > 0) {
        this.index--;
      }
    }
  }

  static ApplySuggestion(state, id) {
    if (state.index < state.suggestions.length) {
      let sug = state.suggestions[state.index];  // get typeahead suggestions
      let cur_str = StateMaschine.event_container.value; // get current input
      cur_str = cur_str.substr(0, cur_str.length - state.inp.length)
      cur_str += sug;
      StateMaschine.event_container.value = cur_str;
      return true;
    } else {
      console.log(state.index, state.suggestions.length);
      return false;
    }
  }

  render() { 
    // Render suggestions
    let suggestions_str = "";
    for (let i=0; i<this.suggestions.length; i++) {
      if (i>0) suggestions_str += ", ";

      if (i === this.index) {
        suggestions_str += "<span class='th_sel'>" + this.suggestions[i] + "</span>";
      } else {
        suggestions_str += this.suggestions[i];
      }
    }
    PrintHints(suggestions_str, this.index);
  }
};

class REP_All extends REP {
  constructor(prev) {
    super("ALL", all_ids().concat(types()), prev);
  }
  handler(e) {
    if (e.key == "Enter") {
      e.preventDefault();
      if (REP.ApplySuggestion(this)) {
        let sug = this.suggestions[this.index];
        if (text_ids().includes(sug)) {
          StateMaschine.event_container.value += "} ";
          StateMaschine.transition(new STD());
        } else {
          StateMaschine.transition(new OPT(this));
        }
      }
    } else {
      super.handler(e);
    }
  }
};

class REP_ATTS extends REP {
  constructor(initial_suggestions, prev) {
    super("ATTS", initial_suggestions, prev);
  }
  handler(e) {
    if (e.key == "Enter") {
      e.preventDefault();
      if (REP.ApplySuggestion(this)) {
        StateMaschine.event_container.value += "} ";
        StateMaschine.transition(new STD());
      }
    } else {
      super.handler(e);
    }
  }
}

class REP_VAR extends REP {
  constructor(prev, full) {
    super("VAR", (full) ? ["name", "desc", "*"] : ["name", "desc"], prev);
    this.full = full;
  }
  handler(e) {
    if (this.full && e.key == "*") {
      this.transition_type()
    } else if (e.key == "Enter") {
      e.preventDefault();
      if (REP.ApplySuggestion(this)) {
        if (this.index < this.suggestions.length && this.suggestions[this.index] == "*") {
          this.transition_type();
        } else {
          this.transition_std();
        }
      }
    } else {
      super.handler(e);
    }
  }

  transition_type() {
    const new_state = new REP_TYPE();
    new_state.inp = "*";
    StateMaschine.transition(new_state);
  }

  transition_std() {
    StateMaschine.event_container.value += "} ";
    StateMaschine.transition(new STD());
  }
};

class REP_TYPE extends REP {
  constructor() {
    super("TYPE", types());
  }
  handler(e) {
    if (e.key == "Enter") {
      e.preventDefault();
      if (REP.ApplySuggestion(this)) {
        StateMaschine.transition(new OPT(this));
      }
    } else {
      super.handler(e);
    }
  }
};

class OPT extends State {
  constructor(prev) {
    super("OPT");
    this.prev = prev;
  }
  handler(e) {
    if (e.key == "}") {
      StateMaschine.transition(new STD());
      return;
    } else if (e.key.length == 1 && e.key == ".") {
      const id = this.prev.suggestions[this.prev.index];
      const options = (this.prev.name == "REP_ALL") ? context_attributes(id) : type_attributes(id);
      StateMaschine.transition(new REP_ATTS(options, this)); 
    } else if (e.key.length == 1 && e.key == "-") {
      this.inp = "-";
    } else if (e.key.length == 1 && e.key == ">" && this.inp == "-") {
      StateMaschine.transition(new REP_VAR(this, this.prev.name == "REP_ALL")); 
    }
  }
  render() { 
    PrintHints(OPERATORS_HINT); 
  }
}

class GOPT extends REP {
  constructor(prev, name, suggestions) {
    if (name === undefined) {
      name = "GOPT";
    }
    if (suggestions === undefined) {
      suggestions = ["prompt", "color_", "bgsplay", "bgspause", "bgsset_", "fgsound_"];
    }
    super(name, suggestions, prev);
    this.prev = prev;
  }
  handler(e) {
    if (e.key == "Enter") {
      e.preventDefault();
      if (REP.ApplySuggestion(this)) {
        let sug = this.suggestions[this.index];
        if (["bgsset_", "fgsound_"].includes(sug)) {
          StateMaschine.transition(new GOPT_ARGS(this, media_audios()));
        } else {
          StateMaschine.transition(new STD());
        }
      }
    } else if (e.key == " ") {
      StateMaschine.transition(new STD());
      return;
    } else {
      super.handler(e);
    }
  }
}

class GOPT_ARGS extends GOPT {
  constructor(prev, suggestions) {
    super(prev, "GOPT_ARGS", suggestions);
  }
  handler(e) {
    if (e.key == "Enter") {
      e.preventDefault();
      if (REP.ApplySuggestion(this)) {
        StateMaschine.transition(new STD());
        StateMaschine.event_container.value += " ";
      }
    } else {
      super.handler(e);
    }
  }

}

const StateMaschine = {
  current_state: null,
  hints_container: null,
  event_container: null,
  init: (id) => {
    StateMaschine.hints_container = document.getElementById("hints_" + id);
    StateMaschine.event_container = document.getElementById(id);
    StateMaschine.hints_container.innerHTML = "";
    // StateMaschine.event_container.innerHTML = "";

    StateMaschine.current_state = new STD();
  },

  transition: (next) => {
    StateMaschine.hints_container.innerHTML = "";
    if (next)
      StateMaschine.current_state = next;
    else 
      StateMaschine.current_state = new STD();
  },

  run: (e) => {
    console.log("Handling", e.key, "with", StateMaschine.current_state.name, 
      "[current: ", StateMaschine.current_state.inp + "] (" + StateMaschine.current_state.suggestions + ")");
    // On closing, reset to STD State and clear states
    if (e.key == "}") {
      StateMaschine.current_state = new STD();
    } else {
      StateMaschine.current_state.handler(e);
      console.log("rendering from", StateMaschine.current_state.name);
      StateMaschine.current_state.render();
    }
  },
};

function PrintHints(hinst_str, index=-1) {
  const view = parseInt(StateMaschine.event_container.offsetWidth/8) + 28; // 28 = html-code for highlightling
  console.log("[print hints]", hinst_str, view, index);
  let print = hinst_str;
  if (view < hinst_str.length) {
    const hints = hinst_str.split(", ");
    const left_elems = hints.slice(0, index);
    const cur_str_pos = left_elems.join().length + left_elems.length*2 + hints[index].length/2;

    let start = Math.max(0, cur_str_pos-parseInt(view/2));
    if (start + view > hinst_str.length) {
      rest = hinst_str.length-1;
      start -= (start+view)-hinst_str.length;
    }
    print = (start > 0) ? "..." : "";
    print += hinst_str.substr(start, view);
    if (start + view < hinst_str.length) {
      print += "...";
    }
  }
  StateMaschine.hints_container.innerHTML = print;
}

function SetUpTypeahead(id) {
  StateMaschine.init(id);
}

function Typeahead(e, _) {
  StateMaschine.run(e);
}
