import {websiteLanguage,saveWebsiteLanguage} from './language.js';
import {search} from './error-search.js';
import {renderFooter} from './page-footer.js';
const $ = id => document.getElementById(id);
let lang = websiteLanguage();
let data, timer, failed=false;
const words={title:['故障码查询','Error code lookup'],intro:['输入完整代码或前几位，自动显示匹配结果。','Type a full code or a prefix to see matching codes.'],queryLabel:['错误码','Error code'],hint:['支持 HMS、十六进制打印错误码和十进制命令错误码。','HMS, hexadecimal print errors and decimal command errors are supported.'],back:['在线刷写','Online installer'],projectLink:['项目地址','Project']};
function tr(zh,en){return lang==='zh'?zh:en;}
function render(){
  renderFooter(lang);
  document.querySelector('.brand').href='index.html?lang='+lang;
  $('back').href='index.html?lang='+lang;
  document.documentElement.lang=lang==='zh'?'zh-CN':'en';
  $('language').value=lang;
  for(const [k,v] of Object.entries(words))$(k).textContent=v[lang==='zh'?0:1];
  $('results').replaceChildren();
  document.title='xTouch-CN · '+$('title').textContent;
  if(!data){$('status').textContent=failed?tr('码表加载失败，请刷新重试。','Could not load codes. Please reload.'):tr('正在载入码表…','Loading code tables…');return;}
  const q=$('query').value;
  const result=search(data.rows,q);
  $('status').textContent=!q.trim()?tr('输入代码开始查询；结果随输入自动补全。','Start typing a code; suggestions update automatically.'):result.total?tr(`找到 ${result.total} 项${result.total>30?'，显示前 30 项，请继续输入缩小范围':''}`,`${result.total} ${result.total===1?'match':'matches'}${result.total>30?'; showing 30. Keep typing to narrow results.':''}`):tr('没有匹配代码；请核对代码类型和格式。','No matching code. Check the code and format.');
  for(const r of result.rows){
    const card=document.createElement('article');card.className='error-result';
    const button=document.createElement('button');button.type='button';button.textContent=r.kind==='hms'?'HMS '+r.code.match(/.{4}/g).join('-'):'0x'+r.code+' · '+parseInt(r.code,16);button.addEventListener('click',()=>{$('query').value=r.code;render();$('query').focus();});card.append(button);
    const p=document.createElement('p');p.textContent=(lang==='zh'?r.zh:r.en)||r.en;card.append(p);
    if(r.hint){const h=document.createElement('p');h.textContent=r.hint[lang==='zh'?0:1];card.append(h);}
    const models=document.createElement('p');models.className='error-models';models.textContent=r.models.length?tr('机型代码：','Model codes: ')+r.models.join(', '):tr('命令授权错误','Command authorization error');card.append(models);
    if(r.variants&&Object.keys(r.variants).length){
      const details=document.createElement('details'),summary=document.createElement('summary');
      summary.textContent=tr('按机型查看原始文案（英文）','Model-specific messages (English)');details.append(summary);
      for(const [model,text] of Object.entries(r.variants)){const variant=document.createElement('p');variant.textContent='['+model+'] '+text;details.append(variant);}
      card.append(details);
    }
    $('results').append(card);
  }
  $('source').textContent=tr('文案来源：BambuStudio 码表。设备、固件版本不同可能有差异；未收录不代表没有故障。','Descriptions: BambuStudio code tables. Messages vary by device/firmware; unknown codes can still indicate faults.');
}
$('query').addEventListener('input',()=>{clearTimeout(timer);timer=setTimeout(render,160);});
$('language').addEventListener('change',()=>{lang=$('language').value;saveWebsiteLanguage(lang);render();});
render();
fetch('error-codes.json').then(r=>{if(!r.ok)throw Error();return r.json();}).then(v=>{data=v;render();}).catch(()=>{failed=true;render();});
