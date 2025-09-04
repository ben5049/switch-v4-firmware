
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
    0x00000082, 0x15000000, 0x369BC3C9, 0x00000000,
    0x00000000, 0x65000000, 0x00000002, 0x07040412,
    0x12061200, 0x23230000, 0x23230000, 0x23230000,
    0x23230000, 0x23230000, 0x00000000, 0x06060606,
    0x00000000, 0x06060606, 0x00000000, 0x1E1E1E1E,
    0x00000000, 0x1E1E1E1E, 0x00000000, 0x1E1E1E1E,
    0x997D0651
]

loader_unswizzled = [((word & 0xff000000) >> 24) | ((word & 0x00ff0000) >> 8) | ((word & 0x0000ff00) << 8) | ((word & 0x000000ff) << 24) for word in loader]

assert int(ethernet_crc32(loader_unswizzled[0:2]), 16) == loader_unswizzled[2], "Incorrect header CRC"
assert int(ethernet_crc32(loader_unswizzled[3:-1]), 16) == loader_unswizzled[-1], "Incorrect data CRC"

print("\nACU data:\n")
for i, word in enumerate(loader_unswizzled[3:-1]):
    print(f"    [{str(32 * i + 31).ljust(3)}:{str(32 * i).ljust(3)}] =", f"0x{hex(word)[2:].zfill(8)} =", " ".join([f"{bin(word)[2:].zfill(32)[(j * 8):((j + 1) * 8)]}" for j in range(4)]))
