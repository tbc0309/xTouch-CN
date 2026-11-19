// Shared by the installer, error lookup and configuration generator.
export function renderFooter(language) {
  const footer=document.querySelector('footer');
  if(!footer||footer.dataset.language===language)return;
  footer.dataset.language=language;
  footer.querySelectorAll('[data-shared-footer]').forEach(el=>el.remove());
  const english=language==='en',links=document.createElement('div');links.dataset.sharedFooter='';
  for(const [id,href,label,external] of [
    ['guide','https://github.com/tbc0309/xTouch-CN/blob/main/README'+(english?'.en':'')+'.md',english?'Documentation ↗':'使用文档 ↗',true],
    ['blog','https://imnks.com/12504.html',english?'Project introduction ↗':'项目介绍 ↗',true],
    ['releases','https://github.com/tbc0309/xTouch-CN/releases','Releases ↗',true],
    ['config-link','config.html?lang='+language,english?'Configuration generator (alternative)':'配置文件生成器（备用）',false],
  ]) {
    const a=document.createElement('a');a.id=id;a.href=href;a.textContent=label;
    if(external){a.target='_blank';a.rel='noopener noreferrer';}
    links.append(a);
  }
  const note=document.createElement('p');note.dataset.sharedFooter='';note.textContent=english?
    'An independent open-source modification, not affiliated with Bambu Lab or hardware vendors. No technical support is provided; consult the documentation or AI.':
    '个人开源改版，与拓竹官方及硬件商家无关联。不提供技术支持，遇到问题请查阅文档或询问 AI。';
  footer.append(links,note);
}
