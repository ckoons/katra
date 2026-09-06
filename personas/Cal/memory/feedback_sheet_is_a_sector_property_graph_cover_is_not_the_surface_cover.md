---
name: feedback_sheet_is_a_sector_property_graph_cover_is_not_the_surface_cover
description: Instrument seam (2026-09-02) — building a branched surface cover, the sheet flips on CROSSING a cut edge face-to-face, not on WALKING ALONG it; flipping per traversal builds a graph double cover, a different object; the k=2 control (nonzero class on genus 0) is what caught it
metadata:
  type: feedback
---

When instrumenting a branched double cover of a triangulated surface from a cut set F of edges: a sheet is a property of a SECTOR at a vertex (parity of F-edges between a reference face and the current face around that vertex), never of the vertex or of an edge traversal. My first E1 instrument flipped sheets per traversal of an F-edge; that is the ℤ₂-graph-cover of the 1-skeleton and it reported a nonzero cohomology class on a genus-0 cover.

**Why:** the two constructions agree on closed faces and on Euler characteristic, so nothing looks wrong until a topological impossibility (H¹(S²) ≠ 0) fires. The cheap control that catches it is a case where the formula forces zero (k = 2).

**How to apply:** for any cover instrument, run a control whose answer is forced to be ZERO by topology before trusting a nonzero reading; and build vertex-lifts from face-sector parity, not from edge crossings. Same family as [[feedback_validate_the_instrument_before_reporting_a_negative]] and [[feedback_a_search_that_cannot_succeed_proves_nothing_empty]].
