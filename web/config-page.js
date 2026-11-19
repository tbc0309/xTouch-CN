import {websiteLanguage,saveWebsiteLanguage} from './language.js';
import {initConfigGenerator} from './config-generator.js';
import {renderFooter} from './page-footer.js';

const language=document.getElementById('language');
language.value=websiteLanguage();
const generator=initConfigGenerator(document.getElementById('configuration'),language.value,'h1',new URLSearchParams(location.search).get('board')||'cyd');
function translate() {
  renderFooter(language.value);
  const english=language.value==='en';
  document.documentElement.lang=english?'en':'zh-CN';
  document.title='xTouch-CN · '+(english?'Configuration generator (alternative)':'配置文件生成器（备用）');
  const back=document.getElementById('back-installer');
  back.textContent=english?'Online installer':'在线刷写';
  back.href='index.html?lang='+language.value;
  document.querySelector('.brand').href=back.href;
  document.getElementById('project-link').textContent=english?'Project':'项目地址';
  generator.setLanguage(language.value);
  const url=new URL(location.href);url.searchParams.set('lang',language.value);history.replaceState(null,'',url);
}
language.addEventListener('change',()=>{saveWebsiteLanguage(language.value);translate();});
translate();
