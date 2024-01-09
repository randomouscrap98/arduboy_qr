import os

data = bytearray([255] * 4096)

# Make it run in this directory
script_dir = os.path.dirname(os.path.abspath(__file__))
os.chdir(script_dir)

with open("save_init.bin", "wb") as f:
    f.write(data)

print("Done")