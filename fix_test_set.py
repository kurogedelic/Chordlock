#!/usr/bin/env python3

# Fix the music theory errors in test_set.txt

corrections = {
    # These are the confirmed music theory errors:
    "62,65,69 # D": "62,65,69 # Dm",        # D + F + A = D minor, not major
    "64,67,71 # E": "64,67,71 # Em",        # E + G + B = E minor, not major  
    "69,72,76 # A": "69,72,76 # Am",        # A + C + E = A minor, not major
}

# Read and fix test_set.txt
with open('test_set.txt', 'r') as f:
    lines = f.readlines()

fixed_lines = []
changes_made = 0

for line in lines:
    line_content = line.strip()
    if line_content in corrections:
        new_line = corrections[line_content] + "\n"
        fixed_lines.append(new_line)
        print(f"Fixed: {line_content} -> {corrections[line_content]}")
        changes_made += 1
    else:
        fixed_lines.append(line)

# Write corrected file
with open('test_set_fixed.txt', 'w') as f:
    f.writelines(fixed_lines)

print(f"\nFixed {changes_made} music theory errors")
print("Created test_set_fixed.txt with corrected chord names")