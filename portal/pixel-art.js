const PixelArt=(()=>{
    const colors={off:'#000000',white:'#ffffff',dim:'#606060',green:'#00ff00',blue:'#00a0ff',yellow:'#ffbe00',red:'#ff3030'};
    const rainbow=['#00e6ff','#2064ff','#9c40ff','#ff30c0','#ff9000','#ffee00','#00ffa0'];
    const glyphs={
      '0':['111','101','101','101','111'],'1':['010','110','010','010','111'],'2':['111','001','111','100','111'],
      '3':['111','001','111','001','111'],'4':['101','101','111','001','001'],'5':['111','100','111','001','111'],
      '6':['111','100','111','101','111'],'7':['111','001','010','010','010'],'8':['111','101','111','101','111'],
      '9':['111','101','111','001','111'],'A':['010','101','111','101','101'],'B':['110','101','110','101','110'],
      'C':['111','100','100','100','111'],'D':['110','101','101','101','110'],'E':['111','100','110','100','111'],
      'F':['111','100','110','100','100'],'H':['101','101','111','101','101'],'I':['111','010','010','010','111'],
      'L':['100','100','100','100','111'],'M':['101','111','111','101','101'],'N':['101','111','111','111','101'],
      'O':['111','101','101','101','111'],'P':['110','101','110','100','100'],'R':['110','101','110','101','101'],
      'S':['111','100','111','001','111'],'T':['111','010','010','010','010'],'U':['101','101','101','101','111'],
      'V':['101','101','101','101','010'],'X':['101','101','010','101','101'],'Y':['101','101','010','010','010'],'Z':['111','001','010','100','111'],
      '%':['101','001','010','100','101'],':':['000','010','000','010','000'],'-':['000','000','111','000','000'],
      '.':['000','000','000','000','010'],'°':['110','110','000','000','000'],' ':['000','000','000','000','000']
    };

let state={};
// BEGIN GENERATED PRINT LAYOUT DATA
const layoutGlyphs={"0":["111","101","101","101","111"],"1":["010","110","010","010","111"],"2":["111","001","111","100","111"],"3":["111","001","111","001","111"],"4":["101","101","111","001","001"],"5":["111","100","111","001","111"],"6":["111","100","111","101","111"],"7":["111","001","010","010","010"],"8":["111","101","111","101","111"],"9":["111","101","111","001","111"],"A":["010","101","111","101","101"],"B":["110","101","110","101","110"],"C":["111","100","100","100","111"],"D":["110","101","101","101","110"],"E":["111","100","110","100","111"],"F":["111","100","110","100","100"],"H":["101","101","111","101","101"],"I":["111","010","010","010","111"],"K":["101","101","110","101","101"],"L":["100","100","100","100","111"],"M":["101","111","111","101","101"],"N":["101","111","111","111","101"],"O":["111","101","101","101","111"],"P":["110","101","110","100","100"],"R":["110","101","110","101","101"],"S":["111","100","111","001","111"],"T":["111","010","010","010","010"],"U":["101","101","101","101","111"],"W":["101","101","111","111","101"],"X":["101","101","010","101","101"],"%":["101","001","010","100","101"],"/":["001","001","010","100","100"],":":["0","1","0","1","0"],"-":["000","000","111","000","000"]," ":["000","000","000","000","000"],"G":["111","100","101","101","111"],"J":["001","001","001","101","111"],"Q":["111","101","101","111","001"],"V":["101","101","101","101","010"],"Y":["101","101","010","010","010"],"Z":["111","001","010","100","111"]};
const layoutColorIds={"LayerTop":32,"LayerBottom":33,"NozzleTop":34,"NozzleBottom":35,"BedTop":36,"BedBottom":37,"NumberBottom":38,"SeparatorTop":39,"SeparatorBottom":40,"PrepLabelTop":41,"PrepLabelBottom":42,"PrepNumberTop":43,"PrepNumberBottom":44,"PrepSeparatorTop":45,"PrepSeparatorBottom":46,"PrintGreen":47,"PrepareYellow":48};
const layoutPalette=["#000000","#000000","#ffffff","#606060","#00ff00","#00a0ff","#ffbe00","#ff3030","#00e6ff","#2064ff","#9c40ff","#ff30c0","#ff9000","#ffee00","#00ffa0","#102330","#00cdff","#205ce6","#1060b0","#000000","#2068a8","#00dcff","#143860","#000000","#00ff80","#1870ff","#60f5ff","#ff5818","#ffb020","#00f0c0","#a0ff00","#000000","#00e5c8","#158fff","#ffb344","#ff5b32","#b69bff","#617dff","#b8d8e8","#66899d","#47697d","#ffcf38","#ff861a","#fff3c9","#ffd694","#d5a955","#a77732","#00ed48","#ffc400","#000000","#000000","#000000","#000000","#000000","#000000","#000000","#000000","#000000","#000000","#000000","#000000","#000000","#000000","#000000"];
const layoutTiming={"frameMs":100,"headerHoldFrames":40,"scrollHoldFrames":8,"scrollPixelFrames":2,"minimumCycleFrames":120};
const layoutRainbow=[8, 14, 6, 12, 11, 10, 9];
const layoutRainbowBandPixels=2;
// END GENERATED PRINT LAYOUT DATA
    function pixel(a,x,y,c){if(Number.isInteger(x)&&Number.isInteger(y)&&x>=0&&x<32&&y>=0&&y<32)a[y*32+x]=c;}
    function rect(a,x,y,w,h,c){for(let j=0;j<h;j++)for(let i=0;i<w;i++)pixel(a,x+i,y+j,c);}
    function line(a,x0,y0,x1,y1,c){let dx=Math.abs(x1-x0),sx=x0<x1?1:-1,dy=-Math.abs(y1-y0),sy=y0<y1?1:-1,e=dx+dy;for(;;){pixel(a,x0,y0,c);if(x0===x1&&y0===y1)break;const k=2*e;if(k>=dy){e+=dy;x0+=sx;}if(k<=dx){e+=dx;y0+=sy;}}}
    function word(a,s,x,y,c,scale=1){[...s].forEach((ch,n)=>{const g=glyphs[ch]||glyphs[' '];g.forEach((row,j)=>[...row].forEach((v,i)=>{if(v==='1')rect(a,x+n*4*scale+i*scale,y+j*scale,scale,scale,c);}));});}
    function center(a,s,y,c,scale=1){word(a,s,Math.floor((32-((s.length*4-1)*scale))/2),y,c,scale);}
    function header(a,label,color){word(a,label,1,1,colors.white);rect(a,29,1,2,5,color);}
    function confetti(a){for(let i=0;i<14;i++){const x=(i*11+3)%30+1,y=7+((i*5+state.frame)%17);pixel(a,x,y,rainbow[i%rainbow.length]);if(i%3===0)pixel(a,x,y+1,rainbow[i%rainbow.length]);}}
    function check(a){line(a,6,16,13,23,colors.green);line(a,13,23,25,8,colors.green);line(a,6,15,13,22,colors.green);line(a,13,22,25,8,colors.green);}
    function warning(a){line(a,15,8,6,23,colors.red);line(a,16,8,25,23,colors.red);line(a,6,23,25,23,colors.red);rect(a,15,12,2,6,colors.red);rect(a,15,20,2,2,colors.red);}
    function paused(a){rect(a,6,8,5,16,colors.yellow);rect(a,21,8,5,16,colors.yellow);}
    function printer(a,drawHead=true){
      rect(a,1,7,30,2,'#00cdff');rect(a,1,9,2,15,'#2064ff');rect(a,29,9,2,15,'#205ce6');rect(a,1,24,30,2,'#1060b0');
      rect(a,3,9,26,15,'#000000');rect(a,4,10,24,1,'#2068a8');rect(a,5,23,22,1,'#00dcff');
      const h=state.currentLayer>0&&state.totalLayers>0?1+Math.floor((Math.min(state.currentLayer,state.totalLayers)*9+Math.floor(state.totalLayers/2))/state.totalLayers):0,top=23-h;
      for(let y=top;y<23;y++){rect(a,10,y,12,1,rainbow[(23-y)%rainbow.length]);pixel(a,21,y,'#143860');}
      if(drawHead){const step=state.frame%24,bounce=step<12?step:23-step;head(a,state.status==='prepare'?14:state.headParked?13:8+bounce,state.status==='prepare'?11:top-4);}
      for(let y=9;y<24;y++)pixel(a,2,y,y>=24-Math.round(state.progress/100*15)?(state.filamentKnown?state.filamentColor:colors.yellow):'#1870ff');
      pixel(a,27,8,state.frame%8<4?'#60f5ff':'#2064ff');
      if(state.chamberHeating&&!state.headParked)for(const side of [4,26])for(let k=0;k<8;k++){if((k+Math.floor(state.frame/2))%4===0)continue;const bend=(k+Math.floor(state.frame/3))%4>=2?1:0;pixel(a,side+bend,22-k,(k+state.frame)%3?'#ff9000':colors.yellow);}
    }
    function activity(stage){
      const base=stageActivity(stage);
      if(base&&base!=='heat')return base;
      return ({1:'feed',4:'calibrate',16:'check'})[state.amsMain]||base;
    }
    function stageActivity(stage){
      if([2,7,49,50,54,58,63].includes(stage))return 'heat';
      if([13,68,70].includes(stage))return 'home';
      if([1,9,40,47,48,57].includes(stage))return 'level';
      if(stage===14)return 'clean';
      if([4,22,24,77].includes(stage))return 'feed';
      if([3,8,12,18,19,25,36,37,38,39,43,45,46,51,53,56,60,61,62,65,67,71].includes(stage))return 'calibrate';
      if([10,11,15,41,42,44,52,55,64,72,73,74,75].includes(stage))return 'check';
      return '';
    }
    function visibleColor(color){
      color=Number.isInteger(color)?color&0xffffff:0;
      if(color===0xffffff)return '#d0d0d0';
      let r=color>>>16,g=color>>>8&255,b=color&255,peak=Math.max(r,g,b);
      if(!peak)return '#101010';
      if(peak<16){r=Math.floor(r*16/peak);g=Math.floor(g*16/peak);b=Math.floor(b*16/peak);}
      return '#'+((r<<16)|(g<<8)|b).toString(16).padStart(6,'0');
    }
    function head(a,x,y){const coloured=state.filamentKnown&&!(state.status==='prepare'&&!activity(state.stage));rect(a,x,y,5,2,colors.white);if(coloured&&!(state.headParked&&!activity(state.stage)))pixel(a,x+2,y+Math.floor(state.frame/2)%2,state.feedColor);rect(a,x+1,y+2,3,1,'#ff9000');pixel(a,x+2,y+3,coloured?state.filamentColor:colors.yellow);}
    function activityPicture(a,kind){
      if(kind==='heat'&&state.stage===49){printer(a);return;}
      printer(a,false);rect(a,6,11,20,12,colors.off);rect(a,5,23,22,1,'#00dcff');
      const t=state.frame%24,moving=8+Math.round((Math.sin(t/4)+1)*5);
      if(kind==='heat'){
        rect(a,8,22,16,1,'#ff9000');head(a,14,11);
        for(let k=0;k<3;k++)for(let y=17;y<21;y++)pixel(a,9+k*6+((y+t)%3===0?1:0),y,(y+t)%3===0?'#ff9000':colors.yellow);
      }else if(kind==='home'){
        head(a,6+(t<=12?t:24-t),11);line(a,8,21,14,21,'#00e6ff');line(a,8,21,8,15,'#00e6ff');pixel(a,7,16,'#00e6ff');pixel(a,9,16,'#00e6ff');
      }else if(kind==='level'){
        const points=[8,16,23],x=points[Math.floor(t/8)%3];head(a,x-2,18+(t%8>3?1:0));for(const point of points)pixel(a,point,23,point===x?colors.green:colors.blue);
      }else if(kind==='clean'){
        rect(a,9,22,15,1,colors.yellow);for(let x=10;x<24;x+=2)rect(a,x,20,1,2,'#ff9000');head(a,8+(t<=12?t:24-t),16);
      }else if(kind==='feed'){
        for(let i=0;i<20;i++){const angle=i*Math.PI/10;pixel(a,12+Math.round(Math.cos(angle)*4),16+Math.round(Math.sin(angle)*4),'#00e6ff');}
        pixel(a,12,16,state.filamentKnown?state.feedColor:colors.yellow);line(a,16,16,21,16,colors.yellow);line(a,21,16,21,19,colors.yellow);rect(a,20,17+t%3,3,1,'#ff9000');head(a,19,19);
      }else if(kind==='calibrate'){
        for(let x=9;x<24;x++)pixel(a,x,22,x%3===t%3?colors.yellow:'#00e6ff');head(a,moving,18);for(let k=0;k<4;k++)pixel(a,9+k*4,13+(k+t)%3,colors.yellow);
      }else if(kind==='check'){
        rect(a,9,13,12,1,'#00e6ff');rect(a,9,20,12,1,'#00e6ff');rect(a,8,14,1,6,'#00e6ff');rect(a,21,14,1,6,'#00e6ff');line(a,21,21,24,23,colors.white);rect(a,11,14+t%6,8,1,colors.yellow);
      }
    }
    function lGlyph(c){return layoutGlyphs[c]||layoutGlyphs[' '];}
    function lWidth(s,scale=1){let n=0;for(const c of s)n+=(lGlyph(c)[0].length+1)*scale;return n?n-scale:0;}
    function lWarm(){return state.status==='prepare';}
    function lActive(){return state.status==='prepare'||state.status==='print';}
    function lDashboard(layout){return state.status==='print'&&!activity(state.stage)&&layout==='progress';}
    function lValues(){const m=state.leftMinutes,h=m<0?0:Math.floor(m/60),minutes=m<0?0:m%60;return {layer:state.currentLayer<0||state.totalLayers<=0?'--/--':state.currentLayer+'/'+state.totalLayers,clock:m<0?'--:--':h+':'+String(minutes).padStart(2,'0'),duration:m<0?'--':h?h+'H'+String(minutes).padStart(2,'0')+'M':minutes+'M',nozzle:state.nozzle<0?'--':String(state.nozzle),bed:state.bed<0?'--':String(state.bed)};}
    function lCycle(size,available){return size<=available?0:layoutTiming.scrollHoldFrames*2+(size-available)*layoutTiming.scrollPixelFrames*2;}
    function lOffset(size,available,t,period){const d=Math.max(0,size-available),hold=layoutTiming.scrollHoldFrames,step=layoutTiming.scrollPixelFrames;if(!d)return 0;const cycle=hold*2+d*step*2;let p=t%Math.max(period,cycle);if(p>=cycle||p<hold)return 0;p-=hold;if(p<d*step)return Math.floor(p/step);p-=d*step;if(p<hold)return d;return d-Math.min(d,Math.floor((p-hold)/step));}
    function lInk(role,y){const top=y<3,warm=lWarm(),ids=layoutColorIds;let id;if(['L','N','B'].includes(role)){if(warm)id=top?ids.PrepLabelTop:ids.PrepLabelBottom;else{const k=role==='L'?'Layer':role==='N'?'Nozzle':'Bed';id=ids[k+(top?'Top':'Bottom')];}}else if(role==='number')id=warm?(top?ids.PrepNumberTop:ids.PrepNumberBottom):(top?2:ids.NumberBottom);else if(role==='separator')id=warm?(top?ids.PrepSeparatorTop:ids.PrepSeparatorBottom):(top?ids.SeparatorTop:ids.SeparatorBottom);else id=warm?ids.PrepareYellow:2;return layoutPalette[id];}
    function lText(a,s,x,y,role,left=0,right=32){for(const c of s){const g=lGlyph(c);for(let dy=0;dy<5;dy++)for(let dx=0;dx<g[0].length;dx++)if(g[dy][dx]==='1'&&x+dx>=left&&x+dx<right)pixel(a,x+dx,y+dy,lInk(c==='/'||c===':'?'separator':role,dy));x+=g[0].length+1;}return x;}
    function lPlain(a,s,x,y,color,scale=1){for(const c of s){const g=lGlyph(c);for(let dy=0;dy<5;dy++)for(let dx=0;dx<g[0].length;dx++)if(g[dy][dx]==='1')rect(a,x+dx*scale,y+dy*scale,scale,scale,color);x+=(g[0].length+1)*scale;}}
    function lField(a,label,value,role,x,y,available,t,period){lText(a,label,x,y,role,x,x+available);const begin=x+4;lText(a,value,begin-lOffset(lWidth(value),available-4,t,period),y,'number',begin,x+available);}
    function lTitle(){const kind=activity(state.stage);return kind==='heat'?(state.stage===49?'CH HEAT':'HEAT'):({home:'HOME',level:'LEVEL',clean:'CLEAN',feed:'FEED',calibrate:'CALIB',check:'CHECK'})[kind]||(lWarm()||state.headParked?'WAIT':state.model);}
    function lPlan(layout,v){const layer=Math.max(layoutTiming.headerHoldFrames,lCycle(lWidth(v.layer),23)),temps=Math.max(layoutTiming.headerHoldFrames,lCycle(9+lWidth(v.nozzle)+lWidth(v.bed),27));return lDashboard(layout)?[['title',layoutTiming.headerHoldFrames],['temps',temps]]:[['title',layoutTiming.headerHoldFrames],['layer',layer],['temps',temps]];}
    function lFrames(layout){const v=lValues(),plan=lPlan(layout,v),dash=lDashboard(layout);return Math.max(layoutTiming.minimumCycleFrames,plan.reduce((n,p)=>n+p[1],0),lCycle(lWidth(v.layer),26),lCycle(lWidth(dash?v.duration:v.clock),dash?30:15));}
    function lHeader(a,layout,v,t,period){rect(a,0,0,32,7,colors.off);const plan=lPlan(layout,v);let phase=t%plan.reduce((n,p)=>n+p[1],0),i=0;while(i+1<plan.length&&phase>=plan[i][1])phase-=plan[i++][1];if(plan[i][0]==='layer')lField(a,'L',v.layer,'L',1,1,27,phase,period);else if(plan[i][0]==='temps'){let x=1-lOffset(9+lWidth(v.nozzle)+lWidth(v.bed),27,phase,period);x=lText(a,'N',x,1,'N',1,28);x=lText(a,v.nozzle,x,1,'number',1,28);x=lText(a,'B',x,1,'B',1,28);lText(a,v.bed,x,1,'number',1,28);}else{const label=lTitle();lPlain(a,label,1,1,lWarm()?layoutPalette[layoutColorIds.PrepareYellow]:colors.white);if(label===state.model){const value=lDashboard(layout)?v.nozzle:state.currentLayer<0?'--':String(state.currentLayer),size=4+lWidth(value);if(lWidth(label)+1+size<=27)lField(a,lDashboard(layout)?'N':'L',value,lDashboard(layout)?'N':'L',28-size,1,size,phase,period);}}rect(a,29,1,2,5,layoutPalette[lWarm()?layoutColorIds.PrepareYellow:layoutColorIds.PrintGreen]);}
    function layoutDraw(a,layout){if(!lActive())return;const v=lValues(),dash=lDashboard(layout),period=lFrames(layout),t=state.frame,green=layoutPalette[layoutColorIds.PrintGreen],yellow=layoutPalette[layoutColorIds.PrepareYellow];if(dash)a.fill(colors.off);lHeader(a,layout,v,t,period);if(!dash){rect(a,0,25,32,7,colors.off);lPlain(a,state.progress+'%',1,26,lWarm()?yellow:green);lText(a,v.clock,Math.max(16,31-lWidth(v.clock))-lOffset(lWidth(v.clock),15,t,period),26,'number',16,31);return;}const percent=state.progress+'%';lPlain(a,percent,Math.trunc((32-lWidth(percent,2))/2),7,green,2);rect(a,1,18,30,1,state.filamentKnown?state.filamentColor:colors.yellow);const fill=Math.floor((state.progress*30+50)/100);const shift=Math.floor((t%period)*(layoutRainbow.length*layoutRainbowBandPixels)/period);for(let x=0;x<fill;x++)pixel(a,1+x,18,layoutPalette[layoutRainbow[Math.floor((x+layoutRainbow.length*layoutRainbowBandPixels-shift)%(layoutRainbow.length*layoutRainbowBandPixels)/layoutRainbowBandPixels)]]);if(state.chamberHeating&&!state.headParked)for(const x of [0,31])for(let y=8;y<16;y++)if((y+Math.floor(t/2))%4)pixel(a,x,y,(y+t)%3?'#ff9000':colors.yellow);const size=4+lWidth(v.layer),start=size<=30?Math.max(1,Math.trunc((32-size)/2)):1;lField(a,'L',v.layer,'L',start,20,size>30?30:size,t,period);lText(a,v.duration,Math.max(1,Math.trunc((32-lWidth(v.duration))/2))-lOffset(lWidth(v.duration),30,t,period),26,'number',1,31);}
    function frame(layout){
      const a=Array(1024).fill(colors.off),s=state.status;let ink=s==='error'?colors.red:s==='prepare'||s==='pause'?colors.yellow:s==='offline'||s==='cancel'?colors.dim:colors.green;
      header(a,state.model,ink);
      const kind=s==='prepare'||s==='print'?activity(state.stage):'';
      if(kind){activityPicture(a,kind);layoutDraw(a,layout);return a;}
      if(s==='idle'){
        const digits=[[14,17,19,21,25,17,14],[4,12,4,4,4,4,14],[14,17,1,2,4,8,31],[30,1,1,14,1,1,30],[2,6,10,18,31,2,2],[31,16,16,30,1,1,30],[6,8,16,30,17,17,14],[31,1,2,4,8,8,8],[14,17,17,14,17,17,14],[14,17,17,15,1,2,12]],positions=[3,9,15,17,23];
        [...state.clock].slice(0,5).forEach((ch,i)=>{if(ch===':'){if(state.frame%10<5){pixel(a,positions[i],14,rainbow[i]);pixel(a,positions[i],17,rainbow[i]);}}else if(ch==='-'){rect(a,positions[i]+1,15,3,1,rainbow[i]);}else if(ch>='0'&&ch<='9'){for(let y=0;y<7;y++)for(let x=0;x<5;x++)if(digits[Number(ch)][y]&(1<<(4-x)))pixel(a,positions[i]+x,12+y,rainbow[i]);}});
        center(a,'READY',26,colors.green);for(let i=0;i<5;i++)pixel(a,6+i*5,9,rainbow[(i+Math.floor(state.frame/3))%rainbow.length]);return a;
      }
      if(s==='prepare'){printer(a);layoutDraw(a,layout);return a;}
      if(s==='done'){confetti(a);check(a);center(a,'DONE',26,colors.green);return a;}
      if(s==='cancel'){for(let y=0;y<16;y++)for(let x=0;x<20;x++)if(Math.abs(x*15-y*19)<=19||Math.abs((19-x)*15-y*19)<=19)pixel(a,6+x,8+y,colors.dim);center(a,'STOP',26,colors.dim);return a;}
      if(s==='error'){warning(a);if(state.frame%10<5){rect(a,2,8,2,2,'#ff9000');rect(a,28,8,2,2,'#ff9000');}center(a,'ERROR',26,colors.red);return a;}
      if(s==='offline'){line(a,7,11,22,11,colors.dim);rect(a,6,12,2,5,colors.dim);rect(a,23,12,2,5,colors.dim);line(a,8,17,23,17,colors.dim);line(a,8,8,24,23,colors.red);center(a,'OFF',25,colors.dim);return a;}
      if(s==='environment'){center(a,'ENV',8,colors.blue);center(a,state.temperature==null?'--':state.temperature.toFixed(1)+'°',16,'#ffb020');center(a,state.humidity==null?'--':Math.round(state.humidity)+'%',24,'#00e6ff');rect(a,1,15,1,5,'#ff5818');rect(a,0,20,3,2,'#ff5818');pixel(a,28,23,'#00e6ff');rect(a,27,24,3,3,'#00e6ff');pixel(a,28,25,colors.white);return a;}
      if(s==='pause'){paused(a);center(a,String(state.progress)+'%',26,colors.yellow);return a;}
      if(layout!=='progress')printer(a);layoutDraw(a,layout);return a;
    }

return {
 frameMs:100,
 animationFrames(snapshot,layout){this.frame(snapshot,layout,0);return lActive()?lFrames(layout):['environment','cancel','pause','offline'].includes(state.status)?1:24;},
 paletteFor(snapshot){const p=this.palette.slice();p[23]=visibleColor(snapshot.feedColor);p[31]=visibleColor(snapshot.filamentColor);return p;},
 frame(snapshot,layout,tick){
  const names={offline:'offline',idle:'idle',preparing:'prepare',printing:'print',paused:'pause',complete:'done',cancelled:'cancel',error:'error',environment:'environment'};
  state={status:names[snapshot.state]||'offline',progress:Math.max(0,Math.min(100,snapshot.percent||0)),frame:tick,model:snapshot.model||'--',clock:snapshot.clock||'--:--',leftMinutes:snapshot.leftMinutes??-1,nozzle:snapshot.nozzle??-1,bed:snapshot.bed??-1,temperature:snapshot.temperature??null,humidity:snapshot.humidity??null,stage:snapshot.stage??-1,amsMain:snapshot.amsMain??-1,currentLayer:snapshot.currentLayer??-1,totalLayers:snapshot.totalLayers??0,filamentColor:visibleColor(snapshot.filamentColor),feedColor:visibleColor(snapshot.feedColor),filamentKnown:snapshot.filamentKnown===true,headParked:snapshot.headParked===true,chamberHeating:snapshot.chamberHeating===true};
  return frame(layout);
 },
 paint(canvas,snapshot,layout,tick){const pixels=this.frame(snapshot,layout,tick),ctx=canvas.getContext('2d');ctx.fillStyle=colors.off;ctx.fillRect(0,0,320,320);pixels.forEach((color,i)=>{ctx.fillStyle=color;ctx.fillRect(i%32*10+1,Math.floor(i/32)*10+1,8,8);});canvas.dataset.pixelFrame=String(tick);canvas.setAttribute('aria-label','32 × 32 LED');},
 palette:["#000000","#000000","#ffffff","#606060","#00ff00","#00a0ff","#ffbe00","#ff3030","#00e6ff","#2064ff","#9c40ff","#ff30c0","#ff9000","#ffee00","#00ffa0","#102330","#00cdff","#205ce6","#1060b0","#000000","#2068a8","#00dcff","#143860","#000000","#00ff80","#1870ff","#60f5ff","#ff5818","#ffb020","#00f0c0","#a0ff00","#000000","#00e5c8","#158fff","#ffb344","#ff5b32","#b69bff","#617dff","#b8d8e8","#66899d","#47697d","#ffcf38","#ff861a","#fff3c9","#ffd694","#d5a955","#a77732","#00ed48","#ffc400","#000000","#000000","#000000","#000000","#000000","#000000","#000000","#000000","#000000","#000000","#000000","#000000","#000000","#000000","#000000"]
};})();
