from pathlib import Path
from PIL import Image, ImageDraw
root=Path(__file__).resolve().parents[1]
files=[('pagina-0','PM2.5 / pantalla principal'),('pagina-1','Las tres concentraciones'),('pagina-2','Tendencia / 60 lecturas validas'),('pagina-3','Resumen reciente'),('pagina-4','Red y resultado de envios'),('enviando','Envio en segundo plano'),('desarrollador','Modo desarrollador'),('wifi','Seleccion WiFi')]
canvas=Image.new('RGB',(848,4*258+24),'#e8edf3');draw=ImageDraw.Draw(canvas)
for i,(filename,title) in enumerate(files):
    x,y=24+(i%2)*412,24+(i//2)*258
    draw.text((x,y),title,fill='#172033')
    tokens=(root/'.validation/screens'/f'{filename}.pbm').read_text().split()
    frame=Image.new('RGB',(128,64));frame.putdata([(0,0,0) if int(v) else (255,255,255) for v in tokens[3:]])
    frame=frame.resize((384,192),Image.Resampling.NEAREST)
    canvas.paste(frame,(x,y+26))
canvas.save(root/'Vista_previa_OLED.png')
print('Vista_previa_OLED.png')
