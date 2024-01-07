
data = bytearray([255] * 4096)

# Need every 128th byte to be 0 to make strings "just work"
for i in range(0, 4096, 128):
    data[i] = 0

with open("save_init.bin", "wb") as f:
    f.write(data)

print("Done")