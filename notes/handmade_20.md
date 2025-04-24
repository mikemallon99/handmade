audio card problems:
- gap between play cursor and write cursor (where its safe to start writing)
- play/write cursor positions update infrequently (480 samples / 10ms)
- we can ask for the positions & write earlier to lower audio latency after the frame
- how much audio should we write to the buffer
- sometimes the p/w cursor can be close to ask time, sometimes its far away. depends on card
