# Counts the lit pixels (any channel over 24) of a loading frame (a P6 PPM, rows from the top) in three places:
# the lower left 640x480 corner, the upper right (the part of the window the corner bug left black), and the
# middle (where mac.loading_indicator draws its label). Prints "corner N", "upper N", "middle N". Python 2.6 or 3.
import sys
data = open(sys.argv[1], 'rb').read()
parts = data.split(b'\n', 3)
width, height = [int(v) for v in parts[1].split()]
pixels = bytearray(parts[3])
corner = upper = middle = 0
for y in range(0, height, 2):
    row = pixels[y * width * 3:(y + 1) * width * 3]
    for x in range(0, width, 2):
        if max(row[x * 3], row[x * 3 + 1], row[x * 3 + 2]) > 24:
            if x < 640 and y >= height - 480:
                corner += 1
            if x >= width * 3 // 4 and y < height // 4:
                upper += 1
        if min(row[x * 3], row[x * 3 + 1], row[x * 3 + 2]) > 190 and abs(x - width // 2) < width // 8 and abs(y - height // 2) < height // 10:
            middle += 1
print("corner %d" % corner)
print("upper %d" % upper)
print("middle %d" % middle)
