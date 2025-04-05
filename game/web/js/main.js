window.onload = function() {
  document.getElementById("cmd").focus();
};

function AddInput(event) {
  if (event.keyCode == 13) {
    var p = document.createElement("p");
    p.classList.add("line");
    p.innerHTML = document.getElementById("cmd").value;
    document.getElementById("content").appendChild(p);
    document.getElementById("cmd").value = "";
  }
}
