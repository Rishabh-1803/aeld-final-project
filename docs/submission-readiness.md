# Final submission readiness

## Ready before borrowing the Pi

- QEMU integration accepted with committed evidence.
- Owner laptop review passed; its skipped read-pointer case is explicitly recorded.
- Pi production/development images built and offline-verified.
- Local Wi-Fi/key-only SSH preparation exists; real connectivity untested.
- Public overview/schedule and sprint assignments prepared; saved view grouped by Sprint.
- Hardware procedure, results sheet and five-minute recording outline prepared.

## Completion gates

1. Complete [physical results sheet](final-session-results.md), resolve failures,
   commit sanitized evidence, and then close #2 and #11.
2. Record the actual demonstration using [the recording plan](final-demo-plan.md).
   Confirm audio, readable text and runtime; obtain the actual public video URL.
3. Update README/wiki/issue #12 with that URL and actual hardware conclusions.
4. Check the enrolled Coursera assignment page for its precise rubric, submission
   fields, permitted video hosting and deadline. The signed-in requirements and fields are now recorded in
   [Coursera requirements](coursera-submission-requirements.md); detailed scoring
   and peer-review count still need confirmation.
5. Verify final repository/wiki/video links without authentication; finish #12.
6. Create and push coursera-sprint-3 at the verified final commit, then publish a
   GitHub release with tested features, evidence links, limitations and artifact
   checksums. Preserve earlier submitted tags. Do not upload the locally provisioned
   SD image, network configuration or private keys; public images must be clean.
7. Enter the actual submission links in Coursera and retain the submission receipt.

## Recording talking points

Explain the goal, three-button grammar and explicit commit; show the actual
hardware, boot and diagnostics, four example characters, cancellation and
reconnection. Explain why bounded queues and event-loss recovery matter and
how QEMU helped isolate software from hardware. Describe only challenges and
lessons that match the evidence and personal experience. Finish with source
and evidence links and the deferred power feature. This is an outline, not a
claim that a physical demonstration has already been recorded.

The hardware session date, physical results, public video URL and submission
receipt remain pending. A final tag/release is deliberately not created yet.
