from pathlib import Path
from PIL import Image,ImageDraw
import sys
p=Path(__file__).resolve().parent
for f in p.glob('*.rgb565'):
 a=memoryview(f.read_bytes()).cast('H');i=Image.new('RGB',(240,240));i.putdata([(((x>>11)&31)*255//31,((x>>5)&63)*255//63,(x&31)*255//31) for x in a]);i.save(f.with_suffix('.png'))
files=sorted(p.glob(sys.argv[1] if len(sys.argv)>1 else 'elite-navigation-*.png'))
cols=5;rows=(len(files)+cols-1)//cols;o=Image.new('RGB',(cols*260,rows*270),'#344048');d=ImageDraw.Draw(o)
for j,f in enumerate(files):
 x=(j%cols)*260;y=(j//cols)*270;o.paste(Image.open(f),(x,y+24));d.text((x+3,y+5),f.stem.replace('elite-navigation-','').replace('pro-navigation-',''),fill='white')
o.save(p/'contact.png')
print('Rendered',len(files),'frames')
