const DataStore = {
  ready: false, 
  data: {}
};

AppInit.register(async () => {
  await LoadAllData(); 
});

async function LoadAllData() {
  const types = ["ctx-ids", "text-ids", "types", "ctx-attributes", "type-attributes"];

  // Request data from server
  const results = await Promise.all(
    types.map(t => fetch(`/api/data/${t}/${game_id()}`) 
      .then(r => r.ok ? r.json() : []))
  );

  // Add to DataStore
  types.forEach((t, i) => {
    DataStore.data[t] = results[i];
  });
  // Construct all-ids and ctx-types from existing data
  DataStore.data["ids"] = DataStore.data["ctx-ids"].concat(DataStore.data["text-ids"]);
  DataStore.data["ctx-types"] = DataStore.data["ctx-ids"].concat(DataStore.data["types"]);

  DataStore.ready = true;
}

function game_id() {
  console.log("game_id: ", window.location.pathname.substr(1));
  return window.location.pathname.substr(1);
}
function context_ids() {
  return DataStore.data["ctx-ids"] ?? [];
}
function text_ids() {
  return DataStore.data["text-ids"] ?? [];
}
function all_ids() {
  return DataStore.data["ids"] ?? [];
}
function types() {
  return DataStore.data["types"] ?? [];
}
function context_types() {
  return DataStore.data["ctx-types"] ?? [];
}
function context_attributes(ctx_id) {
  if (ctx_id === undefined)
    return DataStore.data["ctx-attributes"] ?? {};
  return DataStore.data["ctx-attributes"][ctx_id] ?? [];
}
function type_attributes(type) {
  if (type === undefined)
    return DataStore.data["type-attributes"] ?? [];
  return DataStore.data["type-attributes"][type] ?? [];
}
async function SaveContent(game_id) {
  let container = document.getElementById("save_results");
  let tests_successfull = await RunTests(game_id, "save_test_results");
  if (tests_successfull) {
    try {
      let response = await fetch("/" + game_id + "/save", { method: "POST" });
      if (response.status == 400) {
        let text = await response.text();
        container.innerHTML = text;
      } else if (response.status == 200) {
        updateUrlWithMessage("Successfully saved game content.");
      } else {
        container.innerHTML = "Failed to save: Unkown response: " + response.status;
      }
    } catch (error) {
      container.innerHTML = error.message;
      return false;
    }
  } 
}

async function RestoreContent(game_id) {
  console.log("RestoreContent: ", game_id);
  let container = document.getElementById("restore_content");
    console.log("RestoreContent. All Tests successfull");
  try {
    let response = await fetch("/" + game_id + "/restore", { method: "POST" });
    if (response.status == 400) {
      console.log("RestoreContent: Saving failed.");
      let text = await response.text();
      container.innerHTML = text;
    } else if (response.status == 200) {
      console.log("RestoreContent. Saving successfull");
      updateUrlWithMessage("Successfully restored game content.");
    } else {
      container.innerHTML = "Failed to restoring game: Unkown response: " + response.status;
    }
  } catch (error) {
    console.log("RestoreContent. Saving crashed");
    container.innerHTML = error.message;
    return false;
  }
}

function SetTextIndex(index) {
  let form = document.getElementById('textAddModal').getElementsByTagName("FORM")[0];
  form.action = form.action.replace("new_index", index.toString());
}


function isValidIdStr(str) {
  var code, i, len;

  for (i = 0, len = str.length; i < len; i++) {
    code = str.charCodeAt(i);
    console.log(str[i], code);
    if (!(code > 47 && code < 58) && // numeric (0-9)
        !(code > 64 && code < 91) && // upper alpha (A-Z)
        !(code > 96 && code < 123) && // lower alpha (a-z)
        !(code == 47 || code == 42)) { // slash ('/')
      return false;
    }
  }
  return true;
};
