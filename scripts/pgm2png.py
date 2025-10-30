from PIL import Image

# Open the PGM file
img = Image.open("test.pgm")

# Save as PNG
img.save("output.png")
