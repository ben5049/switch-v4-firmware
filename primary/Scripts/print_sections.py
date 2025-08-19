#!/usr/bin/env python3
from elftools.elf.elffile import ELFFile

# Configuration
elf_file = "primary_Secure_tmp.elf"

# Print and run the command
if __name__ == "__main__":
    print("\nCreating duplicate of secure region (for backup and bank swap)")

    # Read file
    elf = None
    with open(elf_file, "rb") as f:
        elf = ELFFile(f)

        # Get and print the section info
        print("\n", "Section Name".ljust(20), " | ", "Offset (hex)".ljust(12), " | ", "Size (B)".ljust(10), sep="", end="")
        print("\n", "-" * 20, " | ", "-" * 12, " | ", "-" * 10, sep="")
        for section in elf.iter_sections():

            if not section.name:
                continue

            # Get section info
            section_addr = section['sh_addr']
            section_offset = section['sh_offset']
            section_size = section['sh_size']
            section_end = section_offset + section_size

            # Print info
            print(str(section.name).ljust(20), " | ", hex(section_addr + section_offset)[2:].zfill(8).ljust(12), " | ", f"{section_size:,}".ljust(10), sep="")
