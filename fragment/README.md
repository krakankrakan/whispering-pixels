# Fragment Shader Leakage Experiment

Victim continuously renders a 32x32 texture:
```
./sdl_test
```

Attacker:
```
./attacker
```

You can see how many fragments of the original image have been leaked via:
```
python3 show_seen_fragments.py parrot32.png leak_file
```
Show all fragments:
```
python3 show_all_fragments.py leak_file
```

Conduct the leakage at least 2 times and merge the leaked fragments (`merge.py`).

In the merged output, you should see the original input image after applying the puzzle solving algorithm.

We conducted this experiment on a mac Mini M1 2020 in its 8GB configuration running Arch Asahi Linux. Leakage seems to be heavily dependent on work scheduling on the GPU, which is also influenced by the workgroup size.