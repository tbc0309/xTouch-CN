"""Redraw original-style functional icons as clean geometric vectors.
The project-image logo is a separate retained master. Requires picosvg.
"""
from pathlib import Path
from picosvg.svg import SVG
from picosvg.svg_types import SVGPath
from picosvg import svg_pathops
ROOT=Path(__file__).resolve().parents[1];OUT=ROOT/'resources/ui-assets/original';DESIGN=ROOT/'resources/ui-assets/original-design';DESIGN.mkdir(exist_ok=True)
def path(d,fill='white',**kw):return '<path d="'+d+'" fill="'+fill+'" '+''.join(f'{k.replace("_","-")}="{v}" ' for k,v in kw.items())+'/>'
def line(d,w=2,color='white'):return path(d,'none',stroke=color,stroke_width=w,stroke_linecap='round',stroke_linejoin='round')
def rect(x,y,w,h,r=0,fill='white'):return f'<rect x="{x}" y="{y}" width="{w}" height="{h}" rx="{r}" fill="{fill}"/>'
def circle(x,y,r,fill='white'):return f'<circle cx="{x}" cy="{y}" r="{r}" fill="{fill}"/>'
def waves(xs,y):return ''.join(line(f'M{x},{y}c-3,3 3,3 0,6s3,3 0,6',1.8) for x in xs)
a={}
a['a']=path('M2 10 12 2 22 10V21H2Z')
a['b']=rect(2,2,20,20,4)+line('M14 6v8',4,'black')+circle(14,16,3.5,'black')+line('M14 6v10',1.5)+circle(14,16,1.5)+''.join(line(f'M5 {y}h3',1.4,'black') for y in [7,10,13])
a['c']=path('M12 1 16 5H13.5V10.5H19V8L23 12 19 16V13.5H13.5V19H16L12 23 8 19H10.5V13.5H5V16L1 12 5 8V10.5H10.5V5H8Z')+circle(12,12,1.5,'black')
a['d']=path('M6 2H18L24 12 18 22H6L0 12Z')+circle(12,12,4,'black')
a['e']=waves([5,12,19],1)+rect(2,16,20,7,1)+''.join(rect(x,18,2,5,0,'black') for x in [5,10,15])
a['f']=rect(3,2,13,7)+path('M4 9H15L11 15H8Z')+waves([14,20],10)
a['g']=rect(5,1,14,22,1)+rect(7,3,10,6,0,'black')+rect(7,11,10,10,0,'black')+rect(8,5,2,2)+rect(8,13,2,3)
a['h']=line('M3 20a11 11 0 1 1 18 0',2.4)+line('M12 12 18 6',2.5)+circle(12,12,2)+line('M5 11H3M7 6 6 4M17 6 18 4M19 11H21',1.5)
blade=path('M11 10C5 9 3 5 7 3 11 1 14 5 13 10Z');a['i']=''.join(f'<g transform="rotate({r} 12 12)">{blade}</g>' for r in [0,120,240])+circle(12,12,2)
a['j']=path('M8 3H23V21H8L0 12Z')+line('M11 8 19 16M19 8 11 16',2,'black')
a['k']=line('M3 2H10V22H3ZM3 6H7M3 10H6M3 14H7M3 18H6',1.8)+line('M18 2V22M15 5 18 2 21 5M15 19 18 22 21 19',1.8)
a['l']=line('M3 5H10L3 19H10M18 2V22M15 5 18 2 21 5M15 19 18 22 21 19',2)
a['m']=line('M12 2 22 7V18L12 23 2 18V7ZM2 7 12 12 22 7M12 12V23',1.8)
a['n']='<ellipse cx="17" cy="12" rx="5" ry="10" fill="white"/>'+''.join(line(f'M{x} 4c4 4 4 12 0 16',1,'black') for x in [15,18])+'<ellipse cx="6" cy="12" rx="5" ry="10" fill="white"/><ellipse cx="6" cy="12" rx="1.7" ry="4" fill="black"/>'
a['o']=path('M4 1H17L21 5V23H3V5Z')+''.join(rect(x,3,2,5,0,'black') for x in [6,10,14])+rect(6,13,12,7,0,'black')
a['p']=rect(4,2,16,8)+path('M5 10H19L14 18H10Z')+rect(10,18,4,4)
a['q']=circle(12,12,11)+line('M6 12 10 16 18 7',2.2,'black')
a['r']=circle(12,12,11)+line('M8 8 16 16M16 8 8 16',2.2,'black')
arrow=path('M12 1 23 12H17V23H7V12H1Z')
for ch,angle in [('s',0),('t',180),('u',270),('v',90)]:a[ch]=f'<g transform="rotate({angle} 12 12)">{arrow}</g>'
a['w']=circle(12,8.5,7)+rect(8.5,14,7,4,1)+rect(9,19.5,6,2,1)
a['x']=line('M2 7a16 16 0 0 1 20 0M5 11a11 11 0 0 1 14 0M8.5 15a5.5 5.5 0 0 1 7 0',2.6)+circle(12,20,1.7)
a['y']='<rect x="1.5" y="5" width="15" height="14" rx="3" fill="none" stroke="white" stroke-width="2.5"/>'+path('M17 9 23 5V19L17 15Z')
a['z']=path('M5 1 23 12 5 23Z');a['0']=rect(4,2,6,20,1)+rect(14,2,6,20,1);a['1']=rect(2,2,20,20,2)
a['2']=circle(12,12,11)+line('M12 5V12L17 15',2,'black');a['3']=path('M12 1 23 7 12 13 1 7Z')+line('M2 12 12 18 22 12M2 17 12 23 22 17',2)
for code,body in a.items():
 source='<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24">'+body+'</svg>';(DESIGN/(code+'.svg')).write_text(source,encoding='utf-8');doc=SVG.fromstring(source).topicosvg();shapes=list(doc.shapes());white=[s.as_cmd_seq() for s in shapes if s.fill not in ['black','#000','#000000']];black=[s.as_cmd_seq() for s in shapes if s.fill in ['black','#000','#000000']];united=list(svg_pathops.union(white,['nonzero']*len(white)))
 if black:united=list(svg_pathops.difference([united,list(svg_pathops.union(black,['nonzero']*len(black)))],['nonzero','nonzero']))
 d=SVGPath.from_commands(united).d;(OUT/(code+'.svg')).write_text('<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24"><path fill="white" d="'+d+'"/></svg>',encoding='utf-8')
print('Redrew',len(a),'clean geometric original-style masters')
