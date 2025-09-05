
def ethernet_crc32(hex_list):
    """
    Calculate Ethernet CRC32 for a list of 32-bit hex numbers.

    Args:
        hex_list (list of int): List of 32-bit hex integers, e.g. [0x06000000, 0x0000005a]

    Returns:
        int: CRC32 result as integer
        str: CRC32 result as hex string
    """
    data = b""
    for h in hex_list:
        # Convert each 32-bit int to 4 bytes (big-endian, network order)
        data += h.to_bytes(4, byteorder="little")

    # Ethernet CRC polynomial (reflected)
    crc = 0xFFFFFFFF
    for byte in data:
        crc ^= byte
        for _ in range(8):
            if crc & 1:
                crc = (crc >> 1) ^ 0xEDB88320
            else:
                crc >>= 1
    crc ^= 0xFFFFFFFF

    return f"0x{crc:08X}".lower()


loader = [
    0x00000009, 0x28000000, 0xC93AB40D,
    0x00000000, 0x000000FE, 0x01000000, 0x0201FC08,
    0xE44F40BF, 0x02FB17E0, 0xCF40FF06, 0xFC3BF0FF,
    0x00000000, 0x000000FE, 0x01000000, 0x0001FC08,
    0xE44F40BF, 0x02FB17E0, 0xCF40FF06, 0xFC3BF0FF,
    0x00000000, 0x000000FE, 0x01000000, 0x0001FC08,
    0xE44F40BF, 0x02FB17E0, 0xCF40FF06, 0xFC3BF0FF,
    0x00000000, 0x000000FE, 0x01000000, 0x0601FC08,
    0xE44F40BF, 0x02FB17E0, 0xCF40FF06, 0xFC3BF0FF,
    0x00000000, 0x000000FE, 0x01000000, 0x0401FC08,
    0xE44F40BF, 0x02FB17E0, 0xCF40FF06, 0xFC3BF0FF,
    0xCDE13738
]


loader_unswizzled = [((word & 0xff000000) >> 24) | ((word & 0x00ff0000) >> 8) | ((word & 0x0000ff00) << 8) | ((word & 0x000000ff) << 24) for word in loader]

print(f"\nLength is {loader_unswizzled[1]} words\n")
crc = int(ethernet_crc32(loader_unswizzled[0:2]), 16)
if not (crc == loader_unswizzled[2]):
    print(f"Incorrect header CRC: {hex(crc)}\n")
crc = int(ethernet_crc32(loader_unswizzled[3:-1]), 16)
if not (crc == loader_unswizzled[-1]):
    print(f"Incorrect data CRC: {hex(crc)}\n")

print("Table data:\n")

for n, _ in enumerate(loader_unswizzled[3:-1]):
    i = len(loader_unswizzled[3:-1]) - n - 1
    word = loader_unswizzled[3 + i]
    print(f"    [{str(32 * i + 31).ljust(4)}:{str(32 * i).ljust(4)}] =", f"0x{hex(word)[2:].zfill(8)} =", " ".join([f"{bin(word)[2:].zfill(32)[(j * 8):((j + 1) * 8)]}" for j in range(4)]))
