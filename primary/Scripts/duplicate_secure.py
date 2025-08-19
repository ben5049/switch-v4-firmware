#!/usr/bin/env python3

import subprocess
from os import remove
import lief
from elftools.elf.elffile import ELFFile

elf_input = "primary_Secure.elf"
elf_output = "primary_Secure_dup.elf"

# Sections to copy
sections_to_copy = [
    ".isr_vector",
    ".text",
    ".rodata",
    ".ARM.extab",
    ".ARM",
    ".ARM.exidx",
    ".preinit_array",
    ".init_array",
    ".fini_array",
    ".data",
    ".gnu.sgstubs",
]

bank1_base = 0x0c000000
bank2_base = 0x0c100000

if __name__ == "__main__":

    new_sections = []

    # Read file
    elf = None
    with open(elf_input, "rb") as f:
        elf = ELFFile(f)

        # Get and print the section info
        print("\n", "Section Name".ljust(20), " | ", "Addr (hex)".ljust(12), " | ", "Size (B)".ljust(10), sep="", end="")
        print("\n", "-" * 20, " | ", "-" * 12, " | ", "-" * 10, sep="")
        for section_name in sections_to_copy:
            section = elf.get_section_by_name(section_name)

            if section is None:
                print(f"\nERROR: Couldn't find section '{section_name}'\n")
                continue

            # Get section info
            section_vma = section['sh_addr']
            section_size = section['sh_size']
            section_lma = section['sh_link']
            section_type = section['sh_type']
            section_flags = section['sh_flags']
            section_file = f"section{section.name.replace('.', '_')}.bin"
            new_sections.append([
                f"{section_name}_copy",
                section_file,
                section_vma + (bank2_base - bank1_base),
                section_lma + (bank2_base - bank1_base),
                section_size,
                section_flags,
                section_type])

            # Print info
            print(str(section_name).ljust(20), " | ", hex(section_vma)[2:].zfill(8).ljust(12), " | ", f"{section_size:,}".ljust(10), sep="")

            # Dump secton
            objdump_cmd = ["arm-none-eabi-objcopy", "--dump-section", f"{section_name}={section_file}", elf_input]
            ret = subprocess.run(objdump_cmd)
            if (ret.returncode != 0):
                print(f"\tCommand returned code '{ret.returncode}':\n{ret.stderr}")

    # Parse the elf using a different parser
    elf: lief.ELF.Binary = lief.parse(elf_input)

    # Go through each dumped section and add it to the output elf
    sections_added = []
    for section_info in new_sections:
        section_name = section_info[0]
        section_file = section_info[1]
        section_vma = section_info[2]
        section_lma = section_info[3]
        section_size = section_info[4]
        section_flags = section_info[5]
        section_type = section_info[6]

        if section_name in sections_added:
            continue

        # Read raw binary data
        bin_data = None
        with open(section_file, "rb") as f:
            bin_data = f.read()

        assert section_size == len(list(bin_data)), f"Incorrect sizes for section '{section_name}', {section_size} != {len(list(bin_data))}"

        if section_type == "SHT_PROGBITS":
            section_type = 1
        elif section_type == "SHT_INIT_ARRAY":
            section_type = 14
        elif section_type == "SHT_FINI_ARRAY":
            section_type = 15
        elif section_type == "SHT_PREINIT_ARRAY":
            section_type = 16
        else:
            print("type: ", section_type)

        # Create the new section
        new_section = lief.ELF.Section(section_name, type=section_type)
        # new_section = lief.ELF.Segment()
        # new_section.type = section_type
        new_section.content = list(bin_data)
        new_section.entry_size = section_size
        new_section.virtual_address = section_vma
        # new_section.physical_address = section_lma
        new_section.flags = section_flags

        # Add section to ELF
        # new_segment: lief.ELF.Segment = elf.add(new_section)
        elf.add(new_section, loaded=False)

        # Cleanup
        remove(section_file)

    # Write new ELF
    elf.write(elf_output)
