const DataStore = {
  ready: false, 
  data: {}
};

AppInit.register(async () => {
  if (window.location.pathname.length > 1 && window.location.pathname !== "/login" && window.pathname !== "register")
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
  let container = document.getElementById("restore_content");
  try {
    let response = await fetch("/" + game_id + "/restore", { method: "POST" });
    if (response.status == 400) {
      let text = await response.text();
      container.innerHTML = text;
    } else if (response.status == 200) {
      updateUrlWithMessage("Successfully restored game content.");
    } else {
      container.innerHTML = "Failed to restoring game: Unkown response: " + response.status;
    }
  } catch (error) {
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

async function RemoveElement(game_id, type, fullpath) {
  if (fullpath.startsWith("/")) {
    fullpath = fullpath.substring(1);
  }
  const modal = new bootstrap.Modal(document.getElementById('shareRemoveElemModal'));
  modal.show();
  
  // reset container and buttons
  let container = document.getElementById("references_content");
  container.innerHTML = "";
  document.getElementById("references-list").innerHTML = "";
  document.getElementById("remove_element_button").disabled = true;
  document.getElementById("remove_element_notice").style.display = "block";

  try {
    let response = await fetch("/api/" + type + "/references/" + game_id + "?path=" + fullpath);
    if (response.status == 400) {
      let text = await response.text();
      container.innerHTML = text;
    } else if (response.status == 200) {
      let json = await response.json();
      // If no references, allow deleting
      if (json === undefined || json.length === 0) {
        document.getElementById("remove_element_form").action += type + "?path=" + fullpath; 
        document.getElementById("remove_element_button").disabled = false;
        document.getElementById("remove_element_notice").style.display = "None";
        container.innerHTML = "No dangling references, free to remove <i>" + fullpath + "</i>.";
      } 
      // otherwise, render references
      else {
        container.innerHTML = "List of places where <i>" + fullpath + "</i> is referenced:";
        renderReferences(json, "references-list", game_id);
      }
    } else {
      container.innerHTML = `Failed getting references for ${fullpath} (${type}): Unkown response: ${response.status}`;
    }
  } catch (error) {
    console.log("FAIL: " + error.message);
    container.innerHTML = error.message;
    return false;
  }
}


function renderReferences(list, containerId, gameId) {
  const container = document.getElementById(containerId);
  if (!container) throw new Error(`Container not found: #${containerId}`);

  // Ensure list-group semantics even if container is a <div>
  container.classList.add("list-group");
  container.innerHTML = "";

  const buildBaseUrl = (type, id) => {
    // <game-id>?path=<ctx-id>&type=CTX|TXT
    const base = `${gameId}`;
    const qp = `?path=${encodeURIComponent(id)}&type=${encodeURIComponent(type)}`;
    return base + qp;
  };

  const buildOpenUrl = (baseUrl, openVal) => {
    const sep = baseUrl.includes("?") ? "&" : "?";
    return `${baseUrl}${sep}open=${encodeURIComponent(openVal)}`;
  };

  // Helpers for badges / styles using your Bootstrap theme colors
  const kindBadge = (kind) => {
    // TXT: uses your custom theme color "plum" if available, otherwise fall back to "secondary"
    if (kind === "TXT") return `<span class="badge bg-plum me-2">TXT</span>`;
    if (kind === "CTX") return `<span class="badge bg-info me-2">CTX</span>`;
    return `<span class="badge bg-warning text-dark me-2">SETTINGS</span>`;
  };

  // Parse functions
  const parseSettings = (s) => {
    // Examples:
    // "SETTINGS -> initial_events: <event>"
    // "SETTINGS -> initial_contexts"
    const m = s.match(/^SETTINGS\s*->\s*([a-zA-Z0-9_]+)(?::\s*(.*))?$/);
    if (!m) return null;
    return { kind: "SETTINGS", key: m[1], value: m[2] ?? "" };
  };

  const parseCtx = (s) => {
    // Examples:
    // "CTX: catch -> Listener Q1 (arguments: ...)"
    // "CTX: something -> Listener Inp2 (linked-ctx)"
    const m = s.match(/^CTX:\s*(.*?)\s*->\s*Listener\s*([^\s]+)\s*\((.*)\)\s*$/);
    if (!m) return null;
    return { kind: "CTX", ctxId: m[1], listenerId: m[2], details: m[3] };
  };

  const parseTxt = (s) => {
    // Examples:
    // "TXT myTextId[3] -> logic: ..."
    // "TXT myTextId[0] -> permanent_events: ..."
    const m = s.match(/^TXT\s+(.+?)\[(\d+)\]\s*->\s*([^:]+):\s*(.*)\s*$/);
    if (!m) return null;
    return { kind: "TXT", txtId: m[1], index: Number(m[2]), field: m[3], payload: m[4] };
  };

  // Render each row
  list.forEach((raw, idx) => {
    const item = document.createElement("div");
    item.className = "list-group-item";

    const settings = parseSettings(raw);
    const ctx = settings ? null : parseCtx(raw);
    const txt = settings || ctx ? null : parseTxt(raw);

    // Common layout pieces (Bootstrap-ish)
    const header = document.createElement("div");
    header.className = "d-flex justify-content-between align-items-start gap-3";

    const left = document.createElement("div");
    left.className = "flex-grow-1";

    const right = document.createElement("div");
    right.className = "text-end";

    const meta = document.createElement("div");
    meta.className = "mt-1 small text-muted";

    // ---- SETTINGS ----
    if (settings) {
      // Badge + title
      left.innerHTML = `
        <div class="fw-semibold">
          ${kindBadge("SETTINGS")}
          <span>${settings.key}</span>
        </div>
        ${settings.value ? `<div class="small text-muted">${escapeHtml(settings.value)}</div>` : ""}
      `;
    }

    // ---- CTX ----
    else if (ctx) {
      const baseUrl = buildBaseUrl("CTX", ctx.ctxId);
      const openUrl = buildOpenUrl(baseUrl, ctx.listenerId);

      left.innerHTML = `
        <div class="fw-semibold">
          ${kindBadge("CTX")}
          <a class="link-info text-decoration-none" href="${baseUrl}">
            ${escapeHtml(ctx.ctxId)}
          </a>
        </div>
        <div class="small text-muted">
          Listener <span class="fw-semibold">${escapeHtml(ctx.listenerId)}</span>
          <span class="ms-1">(${escapeHtml(ctx.details)})</span>
        </div>
      `;

      meta.innerHTML = `
        <a class="btn btn-sm btn-outline-info" href="${openUrl}">
          🔗 Link
        </a>
      `;
    }

    // ---- TXT ----
    else if (txt) {
      // In your generator, TXT refs do not have a special "(linked-ctx)" direct marker.
      // So: treat TXT as NOT direct match (auto-remove), and still provide a Link? You asked:
      // “implement this for TXT as well (link format given above).”
      // Interpreting that as: provide base link AND the "Link" link with open=index, but keep auto-remove note.
      const baseUrl = buildBaseUrl("TXT", txt.txtId);
      const openUrl = buildOpenUrl(baseUrl, String(txt.index));

      left.innerHTML = `
        <div class="fw-semibold">
          ${kindBadge("TXT")}
          <a class="link-plum text-decoration-none" href="${baseUrl}">
            ${escapeHtml(txt.txtId)}[${txt.index}]
          </a>
        </div>
        <div class="small text-muted">
          <span class="fw-semibold">${escapeHtml(txt.field)}</span>:
          <span>${escapeHtml(txt.payload)}</span>
        </div>
      `;

      meta.innerHTML = `
        <a class="btn btn-sm btn-outline-plum" href="${openUrl}">
          🔗 Link
        </a>
      `;
    }

    // ---- Fallback ----
    else {
      left.innerHTML = `
        <div class="fw-semibold">
          <span class="badge bg-secondary me-2">REF</span>
          <span>Unknown format</span>
        </div>
        <div class="small text-muted">${escapeHtml(raw)}</div>
      `;
      meta.innerHTML = `<span class="fst-italic">will be removed automatically</span>`;
    }

    header.appendChild(left);
    header.appendChild(right);
    item.appendChild(header);
    if (meta.textContent.trim() || meta.innerHTML.trim()) item.appendChild(meta);

    container.appendChild(item);
  });
}

/** Simple HTML escaper for user-controlled text */
function escapeHtml(str) {
  return String(str)
    .replaceAll("&", "&amp;")
    .replaceAll("<", "&lt;")
    .replaceAll(">", "&gt;")
    .replaceAll('"', "&quot;")
    .replaceAll("'", "&#039;");
}
