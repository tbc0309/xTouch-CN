"""Export test board identities for the web installer without copying board drivers."""
import json
from pathlib import Path
import sys
root=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(root/'boards'))
from board_catalog import BOARDS
targets={key:{field:board[field] for field in ['directory','stem','chip','chip_id','family','project','name','flash','boot_offset','app_offset','screen','hint']} for key,board in BOARDS.items() if key not in ['cyd','s3','p4','es3c35p']}
for key in targets:
    targets[key]['manifest']='test/manifest-'+key+'.json'
    targets[key]['siliconRevision']=BOARDS[key].get('siliconRevision')
path=root/'web/test-board-targets.json'
path.write_text(json.dumps(targets,ensure_ascii=False,indent=2)+'\n','utf8')
(root/'web/test-board-targets.js').write_text('// Generated test board identities. Availability is validated from the release catalog.\nexport default '+json.dumps(targets,ensure_ascii=False,separators=(',',':'))+';\n','utf8')
print('Exported test web identities:',len(targets))
