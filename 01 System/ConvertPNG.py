from PIL import Image

def rgb888_to_rgb565(r, g, b):
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)

def png_to_rgb565_array(image_path, array_name):
    # Open the image file
    img = Image.open(image_path)
    img = img.convert("RGB")  # Ensure image is in RGB format
    width, height = img.size
    img_data = img.load()

    # Generate C array string
    c_array  = f"const uint16_t {array_name}_WIDTH = {width};\n"
    c_array += f"const uint16_t {array_name}_HEIGHT = {height};\n"
    c_array += "\n"
    c_array += f"static const uint16_t {array_name}_PIXELS[] PROGMEM = {{\n    "
    line_length = 0

    for y in range(height):
        for x in range(width):
            r, g, b = img_data[x, y]
            rgb565 = rgb888_to_rgb565(r, g, b)
            #high_byte = (rgb565 >> 8) & 0xFF
            #low_byte = rgb565 & 0xFF
            #c_array += f"0x{high_byte:02X}, 0x{low_byte:02X}, "
            #c_array += f"0x{low_byte:02X}, 0x{high_byte:02X}, "
            c_array += f"0x{rgb565:04X}, "
            line_length += 6
            if line_length >= 70:  # Wrap lines for readability
                c_array += "\n    "
                line_length = 0

    c_array = c_array.rstrip(", \n    ") + "\n};"

    return c_array

DIR = "C:\\Users\\henkj\\OneDrive\\03 HenkJan\\014 InternetOfThings\\03 ESP\\19 Nachtklok Amir\\01 System\\"

for image in ['icon_moon', 'icon_sun', 'sync_error']:
    image_file = DIR+image+'.png'  # Path to your PNG image
    c_file = DIR+image+'.h'
    array_name = image.upper()   # Desired name of the C array
    c_byte_array = png_to_rgb565_array(image_file, array_name)
    with open(c_file, "w") as file:
        file.write(c_byte_array)
    print(f"{image} written to c_file")
