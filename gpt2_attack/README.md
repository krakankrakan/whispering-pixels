# LLM (GPT-2) Input/Output Leakage
We conducted this experiment on the NVIDIA RTX 2060, where we are able to leak  inputs/outputs of a LLM which is executed on a shared GPU. Experiment setup:
```
- NVIDIA RTX 2060
- Driver Version: 535.54.03
- CUDA Version: 12.2
- Ubuntu 23.04
- Kernel: Linux 6.2.0-25-generic #25-Ubuntu SMP PREEMPT_DYNAMIC Fri Jun 16 17:05:07 UTC 2023 x86_64 x86_64 x86_64 GNU/Linux
```

## Building Attacker Code
Build the attacker code via:
```
build.sh
```

## GPT-2 Victim
In our experiments, we attacked GPT-2 based on the tinygrad framework. For this, clone the repo and apply the git diff:
```
pip3 install tinygrad==0.7.0
pip3 install tiktoken
https://github.com/tinygrad/tinygrad.git
git checkout c18a497dde73c3787bfbb5c26351399132d3313b
cd tinygrad
git apply ../tinygrad.diff
```

## Getting the Embeddings
Before the attacker can determine which embeddings have been leaked, we first have to export the embeddings from tinygrad. This is done everytime GPT-2 is executed, sou you need to execute it once. Also, the weights are downloaded the first time you execute GPT-2.
```
cd tinygrad/examples
python3 gpt2.py --model_size gpt2 
```
Afterwards, copy the produced `wte.npy` and `wpe.npy` in this folder. Then, execute:
```
python3 convert_np_to_bin.py
```
This generates `wte.bin` and `wpe.bin`, which can be read by the embedding reconstruction code.

## Executing the Attack

### Victim
Let GPT-2 execute with the prompt of your choice:
```
cd tinygrad/examples
python3 gpt2.py --model_size gpt2 --prompt="What is the answer to life, the universe, and everything?"
```

### Attacker
To leak and reconstruct the LLM data, execute:
```
# Leak data from GPU
./attacker.sh

# After some time, Ctrl-C to stop the leakage

# Analyze the leaked data
./find_embedding leaked_gpt2_output.bin
python3 leaked_tokens_to_text.py
```
Reconstruction for 32 tokens takes ~20min on an AMD Ryzen 7 3700X 8-Core Processor with 32 GB RAM. Be aware that the generated look up tables in the reconstruction step use a lot of memory.