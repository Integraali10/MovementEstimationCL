from PIL import Image
import glob, os



for infile in glob.glob("exp/0021*.bmp"):
    file, ext = os.path.splitext(infile)
    im = Image.open(infile)
    #im = im.convert('L')
    im.save(file + ".png")