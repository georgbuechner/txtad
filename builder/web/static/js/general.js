window.onload = function() {
  const params = new URLSearchParams(window.location.search)
  if (params.has("msg")) {
    document.getElementById("snackbar").classList.add("show");
    document.getElementById("msg").innerHTML = params.get("msg");
  }
}
