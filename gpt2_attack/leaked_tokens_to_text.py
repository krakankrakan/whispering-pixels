import tiktoken
import struct

def read_binary_int_file(file_path):
    int_array = []
    with open(file_path, 'rb') as file:
        # Read 4 bytes at a time (assuming each integer is 4 bytes)
        int_bytes = file.read(4)
        while int_bytes:
            # Unpack bytes into a single integer value
            int_value = struct.unpack('i', int_bytes)[0]
            int_array.append(int_value)
            # Read next 4 bytes
            int_bytes = file.read(4)
    return int_array

tokens = read_binary_int_file("reconstructed_leaked.bin")

tokenizer = tiktoken.get_encoding("gpt2")

print(tokenizer.decode(tokens))