window.onload = function() {
  const params = new URLSearchParams(window.location.search)
  if (params.has("msg")) {
    document.getElementById("snackbar").classList.add("show");
    document.getElementById("msg").innerHTML = params.get("msg");
  }
}

function updateUrlWithMessage(message) {
  // Get current URL and search params
  const url = new URL(window.location.href);
  const params = new URLSearchParams(url.search);
  
  // Set or replace the 'msg' parameter
  params.set('msg', message);
  
  // Update the URL search string
  url.search = params.toString();
  
  // Replace the current URL and reload
  window.location.href = url.toString();
}

function RemoveListElement(base_id, index) {
  const element = document.getElementById("parent_" + base_id + "_" + index);
  console.log(element);
  let elem = element;
  while (true) {
    elem = elem.nextElementSibling;
    console.log(elem);
    // Check that still div of same kind
    if (elem && elem.id.indexOf("parent_" + base_id + "_") != -1) {
      const label_id = elem.id.replace("parent_", "label_");
      const label = document.getElementById(label_id);
      const i = parseInt(label.innerHTML);
      label.innerHTML = (i-1).toString();
    } else{
      break;
    }
  }
  element.remove();
}

function AddListElement(base_id) {
  const list = document.getElementById(base_id);
  const new_elem = list.children[0].cloneNode(true);
  console.log(new_elem);
  const last_elem = list.children[list.children.length-1];
  const index = (parseInt(last_elem.id.substring(last_elem.id.lastIndexOf("_")+1))+1).toString();
  new_elem.id = "parent_" + base_id + "_" + index;
  new_elem.children[0].innerHTML = index;
  new_elem.children[0].id = "label_" + base_id + "_" + index;
  new_elem.children[1].id = base_id + "_" + index;
  new_elem.children[1].value = "";
  new_elem.children[2].onclick = function () { RemoveListElement(base_id, index); }
  list.appendChild(new_elem);
}

function clearInputs(root) {
  root.querySelectorAll("input, textarea").forEach(el => {
    if (el.type === "checkbox" || el.type === "radio") el.checked = false;
    else el.value = "";
  });
}

function addAccordionItem({
  accordionId,
  itemId,
  title,
  bodyHtml,
  error,
  open = false
}) {
  const accordion = document.getElementById(accordionId);
  if (!accordion) {
    throw new Error(`Accordion not found: #${accordionId}`);
  }

  const headerId = `${itemId}-header`;
  const collapseId = `${itemId}-collapse`;

  const item = document.createElement("div");
  item.className = "accordion-item";
  if (error) {
    item.classList.add("border");
    item.classList.add("border-danger");
  }

  item.innerHTML = `
    <h2 class="accordion-header" id="${headerId}">
      <button class="accordion-button ${open ? "" : "collapsed"}"
              type="button"
              data-bs-toggle="collapse"
              data-bs-target="#${collapseId}"
              aria-expanded="${open}"
              aria-controls="${collapseId}">
        ${title}
      </button>
    </h2>

    <div id="${collapseId}"
         class="accordion-collapse collapse ${open ? "show" : ""}"
         aria-labelledby="${headerId}"
         data-bs-parent="#${accordionId}">
      <div class="accordion-body">
        ${bodyHtml}
      </div>
    </div>
  `;

  accordion.appendChild(item);
}
