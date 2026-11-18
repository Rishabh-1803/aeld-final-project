# Windows mouse observations — 18 November 2026

Receiver detected after insertion: VID 3151 / PID 1020, HID mouse interface present, Windows status OK. No serial numbers retained in this report.

User performed the physical input checks using the supplied browser page; the page cannot independently identify the source device.

Observed before correcting the overlap test:

- Left: 3 presses, 3 releases.
- Middle: 3 presses, 3 releases.
- Right: 3 presses, 3 releases.
- Wheel: 72 up events, 46 down events (browser events, not physical notch counts).
- Held buttons at inspection: none.
- Overlap: not observed; tester needed a correction to handle pointermove button-mask transitions. Corrected retest completed below.

These are Windows/browser observations. They do not establish Linux descriptors, Pi compatibility, kernel-driver matching/grab, Morse decoding or RF reliability.

## Corrected overlap retest

Observed left+right overlap: yes. Held buttons at inspection: none.
Retest counters: left 3/3, middle 1/1, right 2/2 (presses/releases).
Wheel event counts: up 1, down 116. The retest was requested for overlap only;
it is not a second three-cycle acceptance run for every individual button.

The tester now handles changed button masks on pointerdown, pointermove and
pointerup, because chord transitions can be delivered as pointermove events.
The original observation remains historical rather than being overwritten.

Conclusion: basic Windows button, wheel and simultaneous-left/right operation
observed successfully during this user-operated session. Browser counts do not
prove switch debouncing or RF reliability. The receiver was separately detected
as a healthy Windows HID mouse; browser events cannot independently identify it.
