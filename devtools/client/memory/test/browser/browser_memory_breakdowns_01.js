/* Any copyright is dedicated to the Public Domain.
   http://creativecommons.org/publicdomain/zero/1.0/ */

/**
 * Tests that the heap tree renders rows based on the breakdown
 */

const TEST_URL = "http://example.com/browser/devtools/client/memory/test/browser/doc_steady_allocation.html";

this.test = makeMemoryTest(TEST_URL, function* ({ tab, panel }) {
  const { gStore, document } = panel.panelWin;
  const $$ = document.querySelectorAll.bind(document);

  yield takeSnapshot(panel.panelWin);

  yield waitUntilState(gStore, state =>
    state.snapshots[0].state === states.SAVED_CENSUS);

  info("Check coarse type heap view");
  ["objects", "other", "scripts", "strings"].forEach(findNameCell);

  yield setBreakdown(panel.panelWin, "allocationStack");
  info("Check allocation stack heap view");
  [L10N.getStr("tree-item.nostack")].forEach(findNameCell);

  function findNameCell (name) {
    let el = Array.prototype.find.call($$(".tree .heap-tree-item-name span"), el => el.textContent === name);
    ok(el, `Found heap tree item cell for ${name}.`);
  }
});
