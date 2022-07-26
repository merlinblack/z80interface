#!/usr/bin/env python

def generate_rom_include_file():
  try:
    with open('mybios.lst', 'r') as listing:
      globalSection = False
      globalsDict = {}
      for line in listing:
        if line == '; +++ global symbols +++\n':
          globalSection = True

        if line == '; +++ local symbols +++\n':
          break

        if globalSection:
          process_line(globalsDict, line)

      if len(globalsDict) == 0:
        print('Did not find the globals section in mybios.lst')
        return

      print(globalsDict)
      print();

      with open('mybios.inc', 'w') as includeFile:
        includeFile.write("; -- mybios.inc\n")
        includeFile.write("; -- Automatically generated. Do not edit\n")
        includeFile.write("\n")

        for symbol, address in globalsDict.items():
          includeFile.write(f"{symbol}\t\tequ\t{address}\n")

        includeFile.write("\n")

  except FileNotFoundError:
    print("Could not open mybios.lst for processing.")

  return


def process_line(globals_dict, line):
  if line[0] == ';':
    return
  if line[0] == '\n':
    return
  if line[0] == '_':
    return

  parts = line.split()
  globals_dict[parts[0]] = parts[2]


if __name__ == '__main__':
  generate_rom_include_file()
