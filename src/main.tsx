import React,{useState,useRef,useEffect,useMemo,lazy,Suspense} from 'react';
import {createRoot} from 'react-dom/client';
import {Button,IconButton as KitIconButton,Dialog} from '@stardrop/ui';
import {Layers,SlidersHorizontal,Orbit,ChartNoAxesCombined,ChevronRight,ChevronDown,Plus,Focus,Maximize,Minus,Trash2,ArrowUpRight,Rocket,Check,RefreshCw,Box,X} from 'lucide-react';
import {initialProject,validate,children,subtree,addPart,removePart,catalog,type Project,type Kind,type Part} from './domain/project';
import {createMockEngine} from './engine/mock';
import RocketView from './components/RocketView';
import HierarchyPanel from './components/HierarchyPanel';
import PartInspector from './components/PartInspector';
import CanvasEditActions from './components/CanvasEditActions';
import {editGeometry} from './domain/geometryEdit';
import {Magnet} from 'lucide-react';
import ResizableSplit from './components/ResizableSplit';
import Startup from './components/Startup';
import {initialFlight,cleanFlight,type FlightWorkspace} from './domain/flight';
import MotorWorkspace from './components/engineering/MotorWorkspace';
import SimulationWorkspace from './components/engineering/SimulationWorkspace';
import AnalysisWorkspace from './components/engineering/AnalysisWorkspace';
import './components/engineering/engineering.css';
import {movePart,type MoveTarget} from './domain/move';
import { focusPath, nextFocus } from './domain/navigation';
import {t} from './i18n';
import {parseStar,serializeStar,starFilename,STAR_MAX_BYTES} from './domain/starFile';
import {useProjectHistory} from './useProjectHistory';
import {createProjectInputs} from './engine/projectInputs';
import {download} from './components/engineering/shared';
import '@stardrop/ui/styles.css';
import './styles.css';
import './workspace.css';
const RocketView3D=lazy(()=>import('./components/RocketView3D'));
const KEY='yrt.design.project.v1';
function IconButton(props:React.ComponentProps<typeof Button> & {'aria-label':string}){return <KitIconButton {...props} label={props['aria-label']}/>}
function load(){try{const raw=localStorage.getItem(KEY);return {project:raw?validate(JSON.parse(raw)):initialProject(),error:''};}catch{return {project:initialProject(),error:'Yerel kayıt okunamadı. Başlangıç tasarımı açıldı; kayıt yapmak için bir değişiklik yapın.'};}}
function App(){
 const [page,setPage]=useState(0);const [visited,setVisited]=useState(new Set([0]));
 const [view,setView]=useState<'2d'|'3d'>('2d');
 const [canvasPreview,setCanvasPreview]=useState<Part|null>(null);const [viewRevision,setViewRevision]=useState(0);
 const [snapEnabled,setSnapEnabled]=useState(true);
 const [menu,setMenu]=useState<string|null>(null);const [showTree,setShowTree]=useState(true);const menuRoot=useRef<HTMLDivElement>(null);
 const [boot]=useState(load);const {project,setProject,undo,redo,reset,endGroup,canUndo,canRedo}=useProjectHistory(boot.project);const [selected,setSelected]=useState('sustainer');const [focus,setFocus]=useState('rocket');const [zoom,setZoom]=useState(1);const [notice,setNotice]=useState(boot.error);const [saved,setSaved]=useState(!boot.error);const [help,setHelp]=useState(false);const [adding,setAdding]=useState(false);const [deleting,setDeleting]=useState(false);const file=useRef<HTMLInputElement>(null);const engine=useMemo(createMockEngine,[]);const revision=useRef(0);
 const flight=useMemo(()=>project.flight??initialFlight(),[project.flight]);
 const changeFlight=(next:FlightWorkspace)=>setProject(current=>({...current,flight:cleanFlight(current,next)}));
 const openPage=(index:number)=>{setPage(index);setMenu(null);setVisited(old=>new Set([...old,index]))};
 const result=useMemo(()=>engine.execute({inputs:{structure:project},revision:++revision.current},['structure']),[project,engine]);
 useEffect(()=>{if(project===boot.project&&boot.error)return;setSaved(false);const timer=setTimeout(()=>{try{localStorage.setItem(KEY,JSON.stringify(project));setSaved(true)}catch{setNotice('Yerel kayıt yapılamadı. Projeni dışa aktararak saklayabilirsin.')}},250);return()=>clearTimeout(timer)},[project,boot]);
 const part=project.parts.find(n=>n.id===selected)||project.parts[0];const focusPart=project.parts.find(n=>n.id===focus);
 useEffect(()=>{const close=(e:PointerEvent)=>{if(!menuRoot.current?.contains(e.target as Node))setMenu(null)};const escape=(e:KeyboardEvent)=>{if(e.key==='Escape')setMenu(null)};document.addEventListener('pointerdown',close);document.addEventListener('keydown',escape);return()=>{document.removeEventListener('pointerdown',close);document.removeEventListener('keydown',escape)}},[]);
 function update(patch:Partial<Part>){setProject(p=>({...p,parts:p.parts.map(n=>n.id===part.id?{...n,...patch}:n)}));}
 async function importProject(f?:File){if(!f)return;try{if(f.size>STAR_MAX_BYTES)throw Error('Proje dosyası 50 MB sınırını aşıyor.');const next=parseStar(await f.text());reset(next);setSelected(next.parts.find(n=>n.kind==='rocket')!.id);setFocus(next.parts.find(n=>n.kind==='rocket')!.id);setZoom(1);setPage(0);setVisited(new Set([0]));}catch(e){setNotice(e instanceof Error?e.message:'Proje açılamadı.');}finally{if(file.current)file.current.value='';}}
 function exportProject(){try{const url=URL.createObjectURL(new Blob([serializeStar(project)],{type:'application/json'}));const a=document.createElement('a');a.href=url;a.download=starFilename(project.name);a.click();setTimeout(()=>URL.revokeObjectURL(url),1000);}catch(error){setNotice(error instanceof Error?error.message:'Proje kaydedilemedi.');}}
 const rootId=project.parts.find(n=>n.kind==='rocket')!.id;
 useEffect(()=>{if(!project.parts.some(p=>p.id===selected))setSelected('');if(!project.parts.some(p=>p.id===focus))setFocus(rootId)},[project,selected,focus,rootId]);
 const path=focusPath(project,focus);
 const focusSelected=()=>{setFocus(nextFocus(project,focus,selected));setZoom(1);setViewRevision(v=>v+1);setMenu(null)};
 const navigateTo=(id:string)=>{setFocus(id);setSelected('');setZoom(1);setViewRevision(v=>v+1)};
 useEffect(()=>{
   const onKey=(event:KeyboardEvent)=>{
     if(page!==0||event.key.toLowerCase()!=='f'||event.repeat||event.ctrlKey||event.metaKey||event.altKey||adding||deleting||help||menu)return;
     const target=event.target;
     if(target instanceof Element&&target.closest('input,textarea,select,[contenteditable]:not([contenteditable="false"]),[role="combobox"],dialog[open]'))return;
     event.preventDefault();
     setFocus(nextFocus(project,focus,selected));setZoom(1);setViewRevision(v=>v+1);
   };
   window.addEventListener('keydown',onKey);
   return()=>window.removeEventListener('keydown',onKey);
 },[project,focus,selected,adding,deleting,help,menu,page]);
 useEffect(()=>{
  const shortcut=(event:KeyboardEvent)=>{
   if(event.defaultPrevented||event.isComposing||event.altKey||document.querySelector('dialog[open]'))return;
   const key=event.key.toLowerCase(),command=event.ctrlKey||event.metaKey;
   if(command&&['z','y','s','o'].includes(key)){
    event.preventDefault();if(event.repeat)return;
    if(key==='z'){if(event.shiftKey)redo();else undo();setMenu(null)}
    else if(key==='y'){redo();setMenu(null)}else if(key==='s')exportProject();else file.current?.click();
   }else if(event.key==='Delete'&&!command&&!event.repeat&&page===0&&!adding&&!deleting&&!help&&selected&&part.kind!=='rocket'){
    if((event.target as Element)?.closest('input,textarea,[contenteditable=true],[role=combobox]'))return;
    event.preventDefault();setDeleting(true);
   }
  };
  window.addEventListener('keydown',shortcut);return()=>window.removeEventListener('keydown',shortcut);
 },[project,undo,redo,page,selected,part,adding,deleting,help]);
 const menuItems:Record<string,{label:string;action:()=>void;shortcut?:string;disabled?:boolean}[]>={
 'Dosya':[{label:'Proje aç…',shortcut:'Ctrl O',action:()=>file.current?.click()},{label:'.star olarak dışa aktar…',shortcut:'Ctrl S',action:exportProject}],
 'Düzenle':[{label:'Geri al',shortcut:'Ctrl Z',disabled:!canUndo,action:undo},{label:'İleri al',shortcut:'Ctrl Shift Z',disabled:!canRedo,action:redo},{label:'Bileşen ekle…',action:()=>{openPage(0);setAdding(true)}},{label:'Seçili bileşeni sil…',shortcut:'Del',disabled:part.kind==='rocket'||!selected,action:()=>{if(part.kind!=='rocket')setDeleting(true)}}],
 'Görünüm':[{label:showTree?'Bileşen ağacını gizle':'Bileşen ağacını göster',action:()=>setShowTree(v=>!v)},{label:'Seçilene odaklan',action:focusSelected},{label:'Tüm roketi göster',action:()=>navigateTo(rootId)}],
 'Araçlar':[{label:'Hesap girdilerini dışa aktar…',action:()=>{try{download(`${starFilename(project.name)}.inputs.json`,JSON.stringify(createProjectInputs(project),null,2),'application/json')}catch(error){setNotice(error instanceof Error?error.message:'Girdiler hazırlanamadı.')}}}],
 'Yardım':[{label:'Tasarım rehberi',action:()=>setHelp(true)}]};
 return <div className="app" onBlurCapture={endGroup}><main onPointerDownCapture={event=>{const target=event.target as Element;const canvas=target.closest<HTMLElement>('.drawing');if(canvas&&!target.closest('[data-canvas-controls]'))canvas.focus({preventScroll:true})}}>
 <div className="menu-bar" ref={menuRoot}><div className="menu-left"><div className="menus">{Object.entries(menuItems).map(([label,items])=><div className="menu-root" key={label} onPointerEnter={event=>{if(event.pointerType!=='touch')setMenu(label)}} onPointerLeave={event=>{if(event.pointerType!=='touch')setMenu(current=>current===label?null:current)}}><button aria-expanded={menu===label} aria-haspopup="true" onClick={()=>setMenu(label)}>{label}</button>{menu===label&&<div className="menu-popup">{items.map(item=><button key={item.label} disabled={item.disabled} onClick={()=>{item.action();setMenu(null)}}><span>{item.label}</span>{item.shortcut&&<kbd>{item.shortcut}</kbd>}</button>)}</div>}</div>)}</div></div><div className="menu-brand"><img className="brand-image" src="/brand/starlab.png" alt="STARLAB logosu"/> STARLAB</div><div className="menu-project" style={{'--title-width':Math.min(Math.max(project.name.length,8),26)+'ch'} as React.CSSProperties}><input aria-label="Proje adı" value={project.name} style={{width:'var(--title-width)'}} onChange={e=>setProject({...project,name:e.target.value})}/><span className={saved?'save-indicator saved':'save-indicator pending'} role="status" aria-label={saved?'Kaydedildi':'Kaydedilmedi'} title={saved?'Yerel kayıt güncel': 'Yerel kayıt bekliyor'}>{saved?<Check size={15}/>:<RefreshCw size={15}/>}</span></div></div>
 <input hidden ref={file} type="file" accept=".star,.json,application/json" onChange={e=>void importProject(e.target.files?.[0])}/>
 <nav className="task-tabs" aria-label="Çalışma alanları">{[[Layers,t('design')],[SlidersHorizontal,'Motorlar & konfigürasyon'],[Orbit,'Uçuş simülasyonları'],[ChartNoAxesCombined,t('analysis')]].map(([Icon,label],i)=>{const I=Icon as typeof Layers;return <button key={i} className={i===page?'active':''} aria-current={i===page?'page':undefined} onClick={()=>openPage(i)}><I size={15}/>{label as string}</button>})}</nav>
 <div className="page-slot" hidden={page!==0}>
 <ResizableSplit className="editor" axis="y" storageKey="workspace" label="Bileşen ve görünüm paneli boyutu" initialRatio={.45} minFirst={150} minSecond={220} collapsedFirst={!showTree}><HierarchyPanel inspector={<PartInspector part={canvasPreview?.id===selected?canvasPreview:project.parts.find(p=>p.id===selected)} onChange={update} onAdd={()=>setAdding(true)} onDelete={()=>setDeleting(true)} onFocus={focusSelected}/>} summary={`${project.parts.filter(p=>p.kind==='stage').length} kademe · ${result.results.inventory?.value??0} parça`} project={project} selected={selected} onSelect={setSelected} onEdit={id=>{setSelected(id);setShowTree(true)}} onAdd={()=>setAdding(true)} onFocus={focusSelected} onMove={(source:string,target:MoveTarget)=>{try{const next=movePart(project,source,target);setProject(next);setSelected(source);if(!subtree(next,focus).some(p=>p.id===source))setFocus(next.parts.find(p=>p.id===source)!.parentId||rootId);}catch(error){setNotice(error instanceof Error?error.message:'Taşıma başarısız.')}}}/>
 <section className="viewport"><nav className="breadcrumbs" aria-label="Tasarım yolu">{path.map((node,index)=><React.Fragment key={node.id}>{index>0&&<ChevronRight size={11} aria-hidden="true"/>}<button aria-current={index===path.length-1?'location':undefined} title={node.kind==='rocket'?project.name:node.name} onClick={()=>navigateTo(node.id)}>{node.kind==='rocket'?'Roket':node.name}</button></React.Fragment>)}</nav><div className="drawing" tabIndex={0} aria-label="Tasarım canvası; seçili elemana girmek veya seçim yokken üst seviyeye çıkmak için F" onPointerDown={event=>{if(view==='2d'&&!(event.target as Element).closest('.rocket-part')){setSelected('');event.currentTarget.focus()}}}><div className="canvas-controls" data-canvas-controls onPointerDown={event=>event.stopPropagation()}><div className="view-switch" role="group" aria-label="Görünüm türü">{(['2d','3d'] as const).map(mode=><button key={mode} aria-pressed={view===mode} onClick={()=>setView(mode)}>{mode.toUpperCase()}</button>)}</div><div className="canvas-control-actions">{view==='2d'&&<IconButton size="sm" variant="ghost" aria-label="Hizaya yapıştır" title="Hizaya yapıştır · Alt: geçici kapat" aria-pressed={snapEnabled} onClick={()=>setSnapEnabled(v=>!v)}><Magnet size={15}/></IconButton>}<IconButton size="sm" variant="ghost" aria-label={t('all')} onClick={()=>navigateTo(rootId)}><Maximize size={15}/></IconButton></div></div>{view==='2d'?<RocketView project={project} selected={selected} focus={focus} onSelect={setSelected} zoom={zoom} onZoom={setZoom} resetKey={viewRevision} snapEnabled={snapEnabled} onPreview={setCanvasPreview} onEdit={(id,patch)=>setProject(current=>editGeometry(current,id,patch))}/>:<Suspense fallback={<div className="view-fallback" role="status">3D görünüm yükleniyor…</div>}><RocketView3D project={project} selected={selected} focus={focus} onSelect={setSelected} zoom={zoom} onBack={()=>setView('2d')}/></Suspense>}<div className="axis">X <ArrowUpRight size={15}/></div></div><div className="viewport-tools"><Button size="sm" variant="ghost" onClick={focusSelected}><Focus size={14}/>{selected?t('focus'):'Üst seviyeye çık'} <kbd>F</kbd></Button><CanvasEditActions canUndo={canUndo} canRedo={canRedo} canDelete={!!selected&&part.kind!=='rocket'} onUndo={undo} onRedo={redo} onDelete={()=>setDeleting(true)}/><div className="zoom"><button aria-label="Uzaklaş" onClick={()=>setZoom(z=>Math.max(.25,z-.15))}><Minus size={13}/></button><span>{Math.round(zoom*100)}%</span><button aria-label="Yakınlaş" onClick={()=>setZoom(z=>Math.min(8,z+.15))}><Plus size={13}/></button></div></div></section>
 </ResizableSplit></div>
 <div className="page-slot" hidden={page!==1}>{visited.has(1)&&<MotorWorkspace project={project} flight={flight} onChange={changeFlight}/>}</div>
 <div className="page-slot" hidden={page!==2}>{visited.has(2)&&<SimulationWorkspace project={project} flight={flight} onChange={changeFlight}/>}</div>
 <div className="page-slot" hidden={page!==3}>{visited.has(3)&&<AnalysisWorkspace project={project} flight={flight}/>}</div>
 {notice&&<div className="toast" role="status">{notice}<button aria-label={t('close')} onClick={()=>setNotice('')}><X size={14}/></button></div>}
 </main><Dialog open={adding} onClose={()=>setAdding(false)} title={t('add')} description={`${part.name} içine eklenecek bileşeni seç.`}><div className="add-grid">{(Object.keys(catalog) as Kind[]).filter(k=>catalog[k].parents.includes(part.kind)).map(k=><button key={k} onClick={()=>{const next=addPart(project,part.id,k);setProject(next);setSelected(next.parts.at(-1)!.id);setShowTree(true);setAdding(false)}}><Box size={19}/><span>{catalog[k].label}</span><Plus size={14}/></button>)}</div>{!(Object.keys(catalog) as Kind[]).some(k=>catalog[k].parents.includes(part.kind))&&<p className="dialog-text">Bu parça alt bileşen kabul etmiyor. Ağaçtan bir kademe, gövde veya alt sistem seç.</p>}</Dialog>
 <Dialog open={deleting} onClose={()=>setDeleting(false)} title="Bileşen silinsin mi?"><p className="dialog-text">{part.name} ve ona bağlı {subtree(project,part.id).length-1} alt bileşen kaldırılacak.</p><Button variant="danger" onClick={()=>{const ids=new Set(subtree(project,part.id).map(n=>n.id));setProject(removePart(project,part.id));setSelected(part.parentId!);if(ids.has(focus))setFocus(part.parentId!);setDeleting(false)}}>Bileşeni ve alt bileşenleri sil</Button></Dialog>
 <Dialog open={help} onClose={()=>setHelp(false)} title="STARLAB"><div className="help-content"><p><strong>Tasarım</strong> Bileşeni seç; geometri, kütle ve aero girdilerini sağ panelde düzenle. Sürükleyerek sırala veya başka bir bileşenin içine taşı.</p><p><strong>Touchpad</strong> İki parmakla kaydır; pinch ile yakınlaştır. Parçayı taşı, sağ/üst tutamaçlardan boyutlandır. Mıknatıs hizaya yapıştırır; Alt geçici kapatır. Esc sürüklemeyi iptal eder.</p><p><strong>Odak</strong> Seçili bileşene F ile gir. Boşluğa tıklayıp F ile üst seviyeye çık. Yol çubuğundan da geri dönebilirsin.</p><p><strong>Proje</strong> Ctrl/Cmd O ile aç, Ctrl/Cmd S ile .star dosyasını dışa aktar. Ctrl/Cmd Z geri alır; Ctrl/Cmd Shift Z veya Y ileri alır.</p><p><strong>Veriler</strong> CG/CP ve diğer fizik alanları kullanıcı girdileridir. Boş alan bilinmiyor demektir. Araçlar menüsünden birimleri belirtilmiş hesap girdisi paketi çıkarabilirsin.</p></div></Dialog></div>
}
createRoot(document.getElementById('root')!).render(<React.StrictMode><Startup><App/></Startup></React.StrictMode>);
