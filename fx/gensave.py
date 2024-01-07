
with open("save_init.bin", "wb") as f:
    f.write(bytes([255] * 4096))

print("Done")