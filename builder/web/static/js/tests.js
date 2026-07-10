function displayTestResults(data, results_container) {
  if (results_container === undefined) 
    results_container = "test_result";
  const container = document.getElementById(results_container);
  
  // Clear any existing content
  container.innerHTML = '';
  
  // Create a container for all results
  const resultsContainer = document.createElement('div');
  resultsContainer.style.fontFamily = 'monospace';
  resultsContainer.style.padding = '10px';
  
  // Process each item in the JSON data
  data.forEach(item => {
    const resultElement = document.createElement('div');
    resultElement.style.margin = '5px 0';
    resultElement.style.padding = '8px';
    resultElement.style.borderRadius = '4px';
    
    // Set background color based on success status
    if (item.success) {
      resultElement.style.backgroundColor = '#d4edda';
      resultElement.style.color = '#155724';
      resultElement.style.border = '1px solid #c3e6cb';
    } else {
      resultElement.style.backgroundColor = '#f8d7da';
      resultElement.style.color = '#721c24';
      resultElement.style.border = '1px solid #f5c6cb';
    }
    
    // Create the text content
    const statusText = item.success ? '✓ SUCCESS' : '✗ FAILURE';
    const messageText = ` ${item.desc}`;
    
    // Add error message if present and success is false
    let errorText = '';
    if (!item.success && item.error) {
      errorText = ` - Error: ${item.error}`;
    }
    
    resultElement.textContent = statusText + messageText + errorText;
    
    resultsContainer.appendChild(resultElement);
  });
  
  container.appendChild(resultsContainer);
}

async function RunTests(game_id, results_container) {
  let container = document.getElementById(results_container);
  try {
    const response = await fetch("/api/tests/run/" + game_id);
    if (!response.ok) {
      container.innerHTML = "Request to /api/tests/run failed: " + response.status;
      return false;
    }
    const json = await response.json();
    displayTestResults(json, results_container);
    return json.every(test => test.success === true);
  } catch (error) {
    container.innerHTML = error.message;
    return false;
  }
}

function buildTestsJson() {
  const data = { test_cases: [] };

  // Find all test case containers
  document.querySelectorAll("[data-tc]").forEach(tcEl => {
    const tcIndex = parseInt(tcEl.dataset.tc, 10);

    const desc = tcEl.querySelector(`[name="tc[${tcIndex}][desc]"]`)?.value ?? "";
    if (desc === "")
      return;
    console.log("desc: ", desc);

    const tc = { desc, tests: [] };

    tcEl.querySelectorAll("[data-test]").forEach(tEl => {
      const ti = parseInt(tEl.dataset.test, 10);
      console.log(tcIndex, ti);
      const cmd = tEl.querySelector(`[name="tc[${tcIndex}][tests][${ti}][cmd]"]`)?.value ?? "";
      const result = tEl.querySelector(`[name="tc[${tcIndex}][tests][${ti}][result]"]`)?.value ?? "";
      let checks = tEl.querySelector(`[name="tc[${tcIndex}][tests][${ti}][checks]"]`)?.value ?? "";
      checks = checks.split(";").map((item) => item.trim());

      // Keep empty tests if you want, or skip if all empty:
      if (cmd || result || checks) {
        tc.tests.push({ cmd, result, checks });
      } 
    });

    // Handle empty test_cases: keep them or skip
    // If you want to keep empty ones, push regardless:
    data.test_cases.push(tc);
  });

  return data;
}

function SaveTests(game_id) {
  const payload = buildTestsJson();
  console.log(payload);
  fetch("/" + game_id + "/save/tests", {
    method: "POST", 
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify(payload)
  })
    .then(response => response.text()) 
    .then(text => alert(text));
}

function DelTest(btn) {
  const tc = btn.closest(".test-case");
  const tests = tc.querySelectorAll(".test");
  const test = btn.closest(".test");

  if (tests.length <= 1) {
    clearInputs(test);
  } else {
    test.remove();
  }
  reindexAll();
}

function DelTestCase(btn) {
  const container = document.getElementById("testCasesContainer");
  const tcs = container.querySelectorAll(".test-case");
  const tc = btn.closest(".test-case");

  if (tcs.length <= 1) {
    // keep one testcase; clear desc and clear all tests except first, then clear that test
    const desc = tc.querySelector('input[name*="[desc]"]');
    if (desc) desc.value = "";

    const tests = tc.querySelectorAll(".test");
    tests.forEach((t, i) => { if (i > 0) t.remove(); });
    clearInputs(tc.querySelector(".test"));
  } else {
    tc.remove();
  }
  reindexAll();
}

// Rule: add new test by cloning previous, clear fields, append, reindex
function AddTest(btn) {
  const tc = btn.closest(".test-case");
  const testsContainer = tc.querySelector("[data-tests]");
  const tests = testsContainer.querySelectorAll(".test");
  const last = tests[tests.length - 1];

  const clone = last.cloneNode(true);
  clearInputs(clone);

  // Make Bootstrap collapse safe (avoid duplicate ids/targets)
  resetAccordionIds(clone);

  testsContainer.appendChild(clone);
  reindexAll();
}

// Rule: add new testcase by cloning previous, keep only first test, clear fields
function AddTestCase() {
  const container = document.getElementById("testCasesContainer");
  const tcs = container.querySelectorAll(".test-case");
  const last = tcs[tcs.length - 1];

  const clone = last.cloneNode(true);

  // keep only the first test in that testcase
  const tests = clone.querySelectorAll(".test");
  tests.forEach((t, i) => { if (i > 0) t.remove(); });

  // clear all inputs (desc + first test)
  clearInputs(clone);

  // Fix accordion ids/targets inside the cloned testcase
  resetAccordionIds(clone);

  container.appendChild(clone);
  reindexAll();
}

// IMPORTANT: if you have id=collapseTest.. / headingTest.. / data-bs-target,
// cloning duplicates IDs and Bootstrap gets confused.
// This function removes IDs/targets and lets reindexAll() recreate them.
function resetAccordionIds(root) {
  root.querySelectorAll("[id]").forEach(el => el.removeAttribute("id"));
  root.querySelectorAll("[data-bs-target]").forEach(el => el.removeAttribute("data-bs-target"));
  root.querySelectorAll("[aria-controls]").forEach(el => el.removeAttribute("aria-controls"));
  root.querySelectorAll("[aria-labelledby]").forEach(el => el.removeAttribute("aria-labelledby"));
}

// Rebuild names + accordion ids based on current ordering
function reindexAll() {
  const container = document.getElementById("testCasesContainer");

  container.querySelectorAll(".test-case").forEach((tcEl, tcIndex) => {
    tcEl.dataset.tc = tcIndex;

    // desc input
    const desc = tcEl.querySelector('input[name*="[desc]"]') || tcEl.querySelector("input");
    if (desc) desc.name = `tc[${tcIndex}][desc]`;

    const accordion = tcEl.querySelector("[data-tests]");
    if (accordion) {
      // give each testcase its own accordion parent id
      accordion.id = `accordionTestCase${tcIndex}`;
    }

    tcEl.querySelectorAll(".test").forEach((testEl, testIndex) => {
      testEl.dataset.test = testIndex;

      // update field names
      const fields = ["cmd", "result", "checks"];
      fields.forEach(field => {
        const input = testEl.querySelector(`input[name*="[${field}]"]`);
        if (input) input.name = `tc[${tcIndex}][tests][${testIndex}][${field}]`;
      });

      // rebuild bootstrap ids/targets (make sure your markup has these elements)
      const header = testEl.querySelector(".accordion-header");
      const button = testEl.querySelector(".accordion-button");
      const collapse = testEl.querySelector(".accordion-collapse");

      if (header) header.id = `headingTest${tcIndex}_${testIndex}`;
      if (collapse) {
        collapse.id = `collapseTest${tcIndex}_${testIndex}`;
        collapse.setAttribute("aria-labelledby", `headingTest${tcIndex}_${testIndex}`);
        collapse.setAttribute("data-bs-parent", `#accordionTestCase${tcIndex}`);
      }
      if (button && collapse) {
        button.setAttribute("data-bs-target", `#${collapse.id}`);
        button.setAttribute("aria-controls", collapse.id);
      }
    });
  });
}
