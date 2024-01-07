# Add your special strings here
DEBUGDATA = [
    # "https://link1",
    # "...",
    # "https://link2",
    # "...",
    # "ABC123",
    # "...",
    # "four",
    # "...",
    # "http://five.com",
    # "...",
    # "https://SIXSIXSIXTHENUMBEROFTHEBEAST.com",
    # "...",
]

data = bytearray([255] * 4096)

# Need every 128th byte to be 0 to make strings "just work"
di = 0
for i in range(0, 4096, 128):
    if di < len(DEBUGDATA) and DEBUGDATA[di]:
        data[i:i+len(DEBUGDATA[di])] = DEBUGDATA[di].encode()
        data[i+len(DEBUGDATA[di])] = 0
    else:
        data[i] = 0
    di += 1

with open("save_init.bin", "wb") as f:
    f.write(data)

print("Done")