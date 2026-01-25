window.AppInit = (function () {
  const tasks = []; 

  function register(fn) {
    tasks.push(fn);
  }

  window.addEventListener("load", async () => {
    for (const task of tasks) {
      await task();
    }
  });
  return { register };
})();
