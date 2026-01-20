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
