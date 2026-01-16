async function SaveContent(game_id) {
  console.log("SaveContent: ", game_id);
  let container = document.getElementById("save_results");
  let tests_successfull = await RunTests(game_id, "save_test_results");
  if (tests_successfull) {
    console.log("SaveContent. All Tests successfull");
    try {
      let response = await fetch("/" + game_id + "/save", { method: "POST" });
      if (response.status == 400) {
        console.log("SaveContent: Saving failed.");
        let text = await response.text();
        container.innerHTML = text;
      } else if (response.status == 200) {
        console.log("SaveContent. Saving successfull");
        updateUrlWithMessage("Successfully saved game content.");
      } else {
        container.innerHTML = "Failed to save: Unkown response: " + response.status;
      }
    } catch (error) {
      console.log("SaveContent. Saving crashed");
      container.innerHTML = error.message;
      return false;
    }
  } else {
    console.log("SaveContent. Some Tests failed");
  }
}

function SetTextIndex(index) {
  let form = document.getElementById('textAddModal').getElementsByTagName("FORM")[0];
  form.action = form.action.replace("new_index", index.toString());
}
