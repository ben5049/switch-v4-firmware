LINKER_FILE = "../STM32H573IIKXQ_FLASH_MMT_TEMPLATE.ld"

contents = None
with open(LINKER_FILE, "r") as file:
    print(f"Reading '{LINKER_FILE}'")
    contents = file.read()

if contents is None:
    exit(1)

lines = contents.split("\n")

found = False
for i, line in enumerate(lines):
    if ".BACKUP_Section :" in line:
        print("Adding 'NOLOAD' to backup section")
        found = True
        print(f"\tOriginal line = '{line}'")
        lines[i] = line.split(":", 1)[0] + "(NOLOAD):"
        print(f"\tNew line = '{lines[i]}'")

if found:
    new_contents = "\n".join(lines)
    with open(LINKER_FILE, "w") as file:
        print(f"Writing '{LINKER_FILE}'")
        file.write(new_contents)
