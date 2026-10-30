import {calculationExample} from './domain/calculationExample';
import IdleShowroom from './components/IdleShowroom';
import {isStructural} from './domain/componentCatalog';
import React,{useState,useRef,useEffect,useMemo,lazy,Suspense} from 'react';
import {createRoot} from 'react-dom/client';
import {Button,IconButton as KitIconButton,Dialog,TextField,MenuBar,TabBar,Breadcrumbs,SaveIndicator,Toast,ZoomControls} from '@stardrop/ui';
import {Layers,SlidersHorizontal,Orbit,ChartNoAxesCombined,ChevronRight,ChevronDown,Plus,Focus,Maximize,Minus,Trash2,ArrowUpRight,Rocket,Check,RefreshCw,Box,X} from '@stardrop/ui/icons';
import {initialProject,emptyProject,validate,children,subtree,addPart,removePart,catalog,type Project,type Kind,type Part} from './domain/project';
import ComponentLibrary from './components/ComponentLibrary';
import CanvasSummary from './components/CanvasSummary';
import {createComponent} from './domain/createComponent';
import RocketView from './components/RocketView';
import HierarchyPanel from './components/HierarchyPanel';
import PartInspector from './components/PartInspector';
import SelectionInspector from './components/SelectionInspector';
import {selectIds,patchSelection,removeSelection,selectionRoots,commonFocus} from './domain/selection';
import {transformSelection} from './domain/selectionGeometry';
import {canGroup,groupSelection} from './domain/groupSelection';
import CanvasEditActions from './components/CanvasEditActions';
import {editGeometry} from './domain/geometryEdit';
import {Magnet} from '@stardrop/ui/icons';
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
function load(){try{const raw=localStorage.getItem(KEY);return {project:raw?validate(JSON.parse(raw)):emptyProject(),error:''};}catch{return {project:emptyProject(),error:'Yerel kayıt okunamadı. Boş proje açıldı; kayıt yapmak için bir değişiklik yapın.'};}}
function App(){
 const [page,setPage]=useState(0);const [visited,setVisited]=useState(new Set([0]));
 const [view,setView]=useState<'2d'|'3d'>('2d');
 const [canvasPreview,setCanvasPreview]=useState<Part|null>(null);const [viewRevision,setViewRevision]=useState(0);
 const [batchPreview,setBatchPreview]=useState<Part[]|null>(null);
 const [snapEnabled,setSnapEnabled]=useState(true);
 const [creating,setCreating]=useState(false),[newName,setNewName]=useState('Yeni proje');
 const [documentRevision,setDocumentRevision]=useState(0);
 const [previousProject,setPreviousProject]=useState<Project|null>(()=>{try{const raw=localStorage.getItem(KEY+'.previous');return raw?validate(JSON.parse(raw)):null}catch{return null}});
 const [menu,setMenu]=useState<string|null>(null);const [showTree,setShowTree]=useState(true);const menuRoot=useRef<HTMLDivElement>(null);
 const [boot]=useState(load);const {project,setProject,undo,redo,reset,endGroup,canUndo,canRedo}=useProjectHistory(boot.project);const [selection,setSelection]=useState<string[]>([]);const selected=selection.at(-1)??'';const setSelected=(id:string)=>setSelection(id?[id]:[]);const choose=(id:string,additive=false)=>setSelection(old=>selectIds(old,id,additive));const [focus,setFocus]=useState('rocket');const [zoom,setZoom]=useState(1);const [notice,setNotice]=useState(boot.error);const [saved,setSaved]=useState(!boot.error);const [help,setHelp]=useState(false);const [adding,setAdding]=useState(false);const [deleting,setDeleting]=useState(false);const file=useRef<HTMLInputElement>(null);
 const flight=useMemo(()=>project.flight??initialFlight(),[project.flight]);
 const emptyCanvas=!subtree(project,focus).some(p=>!isStructural(p.kind));
 function replaceDocument(next:Project){
  setPreviousProject(project);let backupError=false;try{localStorage.setItem(KEY+'.previous',JSON.stringify(project))}catch{backupError=true}
  reset(next);setDocumentRevision(v=>v+1);setSelected('');setFocus(next.parts.find(p=>p.kind==='rocket')!.id);setZoom(1);setViewRevision(v=>v+1);setView('2d');setPage(0);setVisited(new Set([0]));setAdding(false);setCreating(false);setCanvasPreview(null);setBatchPreview(null);setNotice(backupError?'Önceki proje bu oturumda tutuluyor. Dosya olarak dışa aktarabilirsin.':'');
 }
 function beginNewProject(){setNewName('Yeni proje');setCreating(true);setMenu(null)}
 useEffect(()=>{if(!creating)return;const frame=requestAnimationFrame(()=>{const input=document.querySelector<HTMLInputElement>('input[aria-label="Yeni proje adı"]');input?.focus();input?.select()});return()=>cancelAnimationFrame(frame)},[creating]);
 function addComponent(target:string,preset:string){try{const next=createComponent(project,target,preset);setProject(next.project);setSelected(next.id);if(!subtree(next.project,focus).some(p=>p.id===next.id))setFocus(next.project.parts.find(p=>p.id===next.id)!.parentId??rootId);setShowTree(true);setAdding(false)}catch(error){setNotice(error instanceof Error?error.message:'Bileşen eklenemedi.')}}
 const changeFlight=(next:FlightWorkspace)=>setProject(current=>({...current,flight:cleanFlight(current,next)}));
 const openPage=(index:number)=>{setPage(index);setMenu(null);setVisited(old=>new Set([...old,index]))};
 useEffect(()=>{if(project===boot.project&&boot.error)return;setSaved(false);const timer=setTimeout(()=>{try{localStorage.setItem(KEY,JSON.stringify(project));setSaved(true)}catch{setNotice('Yerel kayıt yapılamadı. Projeni dışa aktararak saklayabilirsin.')}},250);return()=>clearTimeout(timer)},[project,boot]);
 const selectedParts=selection.map(id=>project.parts.find(p=>p.id===id)).filter((p):p is Part=>!!p);
 const part=project.parts.find(n=>n.id===(selection.length>1?commonFocus(project,selection):selected))||project.parts.find(n=>n.id===focus)||project.parts[0];const focusPart=project.parts.find(n=>n.id===focus);
 const canDelete=selectedParts.length>0&&selectedParts.every(p=>p.kind!=='rocket');
 const deleteIds=new Set(selectionRoots(project,selection).flatMap(p=>subtree(project,p.id).map(p=>p.id)));
 function deleteSelected(){if(!canDelete)return;const next=removeSelection(project,selection);setProject(next);setSelected(part.parentId&&next.parts.some(p=>p.id===part.parentId)?part.parentId:'');if(deleteIds.has(focus))setFocus(next.parts.find(p=>p.id===focusPart?.parentId)?.id??next.parts.find(p=>p.kind==='rocket')!.id);setDeleting(false)}
 useEffect(()=>{const close=(e:PointerEvent)=>{if(!menuRoot.current?.contains(e.target as Node))setMenu(null)};const escape=(e:KeyboardEvent)=>{if(e.key==='Escape')setMenu(null)};document.addEventListener('pointerdown',close);document.addEventListener('keydown',escape);return()=>{document.removeEventListener('pointerdown',close);document.removeEventListener('keydown',escape)}},[]);
 function update(patch:Partial<Part>){try{setProject(patchSelection(project,selection,patch))}catch(error){setNotice(error instanceof Error?error.message:'Değer uygulanamadı.')}}
 function groupSelected(){try{const next=groupSelection(project,selection);setProject(next.project);setSelected(next.id);setFocus(next.id);setZoom(1);setViewRevision(v=>v+1)}catch(error){setNotice(error instanceof Error?error.message:'Alt sistem oluşturulamadı.')}}
 async function importProject(f?:File){if(!f)return;try{if(f.size>STAR_MAX_BYTES)throw Error('Proje dosyası 50 MB sınırını aşıyor.');const next=parseStar(await f.text());replaceDocument(next);}catch(e){setNotice(e instanceof Error?e.message:'Proje açılamadı.');}finally{if(file.current)file.current.value='';}}
 function exportProject(){try{const url=URL.createObjectURL(new Blob([serializeStar(project)],{type:'application/json'}));const a=document.createElement('a');a.href=url;a.download=starFilename(project.name);a.click();setTimeout(()=>URL.revokeObjectURL(url),1000);}catch(error){setNotice(error instanceof Error?error.message:'Proje kaydedilemedi.');}}
 const rootId=project.parts.find(n=>n.kind==='rocket')!.id;
 useEffect(()=>{const validFocus=project.parts.some(p=>p.id===focus)?focus:rootId;const visible=new Set(subtree(project,validFocus).map(p=>p.id));setSelection(old=>{const next=old.filter(id=>visible.has(id));return next.length===old.length?old:next});if(validFocus!==focus)setFocus(validFocus)},[project,focus,rootId]);
 const path=focusPath(project,focus);
 const focusSelected=()=>{setFocus(selection.length>1?commonFocus(project,selection):nextFocus(project,focus,selected));setZoom(1);setViewRevision(v=>v+1);setMenu(null)};
 const navigateTo=(id:string)=>{setFocus(id);setSelected('');setZoom(1);setViewRevision(v=>v+1)};
 useEffect(()=>{
   const onKey=(event:KeyboardEvent)=>{
     if(page!==0||event.key.toLowerCase()!=='f'||event.repeat||event.ctrlKey||event.metaKey||event.altKey||adding||deleting||help||menu)return;
     const target=event.target;
     if(target instanceof Element&&target.closest('input,textarea,select,[contenteditable]:not([contenteditable="false"]),[role="combobox"],dialog[open]'))return;
     event.preventDefault();
     setFocus(selection.length>1?commonFocus(project,selection):nextFocus(project,focus,selected));setZoom(1);setViewRevision(v=>v+1);
   };
   window.addEventListener('keydown',onKey);
   return()=>window.removeEventListener('keydown',onKey);
 },[project,focus,selected,selection,adding,deleting,help,menu,page]);
 useEffect(()=>{
  const shortcut=(event:KeyboardEvent)=>{
   if(event.defaultPrevented||event.isComposing||event.altKey||document.querySelector('dialog[open]'))return;
   const key=event.key.toLowerCase(),command=event.ctrlKey||event.metaKey;
   if(command&&['z','y'].includes(key)&&(event.target as Element)?.closest('.series-editor input'))return;
   if(command&&['z','y','s','o','n'].includes(key)){
    event.preventDefault();if(event.repeat)return;
    if(key==='z'){if(event.shiftKey)redo();else undo();setMenu(null)}
    else if(key==='y'){redo();setMenu(null)}else if(key==='s')exportProject();else if(key==='n')beginNewProject();else file.current?.click();
   }else if(command&&key==='a'&&page===0&&(event.target as Element)?.closest('.drawing,.hierarchy-browser')&&!(event.target as Element).closest('input,textarea')){
    event.preventDefault();setSelection(subtree(project,focus).filter(p=>!isStructural(p.kind)).map(p=>p.id));
   }else if(event.key==='Delete'&&!command&&!event.repeat&&page===0&&!adding&&!deleting&&!help&&canDelete){
    if((event.target as Element)?.closest('input,textarea,[contenteditable=true],[role=combobox]'))return;
    event.preventDefault();setDeleting(true);
   }
  };
  window.addEventListener('keydown',shortcut);return()=>window.removeEventListener('keydown',shortcut);
 },[project,undo,redo,page,selected,part,selection,canDelete,focus,adding,deleting,help]);
 const menuItems:Record<string,{label:string;action:()=>void;shortcut?:string;disabled?:boolean}[]>={
 'Dosya':[{label:'Yeni proje…',shortcut:'Ctrl N',action:beginNewProject},{label:'Proje aç…',shortcut:'Ctrl O',action:()=>file.current?.click()},{label:'.star olarak dışa aktar…',shortcut:'Ctrl S',action:exportProject},{label:'Örnek roket aç',action:()=>replaceDocument(initialProject())},{label:'Yapı hesabı örneği aç',action:()=>replaceDocument(calculationExample())},{label:'Önceki projeyi aç',disabled:!previousProject,action:()=>{if(previousProject)replaceDocument(previousProject)}}],
 'Düzenle':[{label:'Geri al',shortcut:'Ctrl Z',disabled:!canUndo,action:undo},{label:'İleri al',shortcut:'Ctrl Shift Z',disabled:!canRedo,action:redo},{label:'Bileşen ekle…',action:()=>{openPage(0);setAdding(true)}},{label:'Seçili bileşeni sil…',shortcut:'Del',disabled:!canDelete,action:()=>{if(canDelete)setDeleting(true)}}],
 'Görünüm':[{label:'Showroom',disabled:!project.parts.some(p=>!isStructural(p.kind)),action:()=>window.dispatchEvent(new Event('starlab:showroom'))},{label:showTree?'Bileşen ağacını gizle':'Bileşen ağacını göster',action:()=>setShowTree(v=>!v)},{label:'Seçilene odaklan',action:focusSelected},{label:'Tüm roketi göster',action:()=>navigateTo(rootId)}],
 'Araçlar':[{label:'Hesap girdilerini dışa aktar…',action:()=>{try{download(`${starFilename(project.name)}.inputs.json`,JSON.stringify(createProjectInputs(project),null,2),'application/json')}catch(error){setNotice(error instanceof Error?error.message:'Girdiler hazırlanamadı.')}}}],
 'Yardım':[{label:'Tasarım rehberi',action:()=>setHelp(true)}]};
 return <div className="app sd-theme" onBlurCapture={endGroup}><main onPointerDownCapture={event=>{const target=event.target as Element;const canvas=target.closest<HTMLElement>('.drawing');if(canvas&&!target.closest('[data-canvas-controls]'))canvas.focus({preventScroll:true})}}>
 <div className="menu-bar" ref={menuRoot}><div className="menu-left"><MenuBar menus={Object.entries(menuItems).map(([label,items])=>({label,items}))} openMenu={menu} onOpenChange={setMenu}/></div><div className="menu-brand"><img className="brand-image" src="/brand/starlab.png" alt="STARLAB logosu"/> STARLAB</div><div className="menu-project" style={{'--title-width':Math.min(Math.max(project.name.length,8),26)+'ch'} as React.CSSProperties}><input aria-label="Proje adı" value={project.name} style={{width:'var(--title-width)'}} onChange={e=>setProject({...project,name:e.target.value})}/><SaveIndicator saved={saved}/></div></div>
 <input hidden ref={file} type="file" accept=".star,.json,application/json" onChange={e=>void importProject(e.target.files?.[0])}/>
 <TabBar className="task-tabs" label="Çalışma alanları" value={String(page)} onChange={value=>openPage(Number(value))} items={[[Layers,t('design')],[SlidersHorizontal,'Motorlar & konfigürasyon'],[Orbit,'Uçuş simülasyonları'],[ChartNoAxesCombined,t('analysis')]].map(([Glyph,label],i)=>{const I=Glyph as typeof Layers;return {value:String(i),label:label as string,icon:<I size={15}/>}})}/>
 <div className="page-slot" hidden={page!==0} key={documentRevision}>
 <ResizableSplit className="editor" axis="y" storageKey="workspace" label="Bileşen ve görünüm paneli boyutu" initialRatio={.45} minFirst={150} minSecond={220} collapsedFirst={!showTree}><HierarchyPanel inspector={selection.length>1?<SelectionInspector parts={selectedParts.map(p=>batchPreview?.find(n=>n.id===p.id)??p)} onChange={update} onSelect={setSelected} onRemove={id=>choose(id,true)} onGroup={canGroup(project,selection)?groupSelected:undefined} onDelete={()=>setDeleting(true)} onFocus={focusSelected}/>:<PartInspector part={canvasPreview?.id===selected?canvasPreview:batchPreview?.find(p=>p.id===selected)??project.parts.find(p=>p.id===selected)} onChange={update} onAdd={()=>setAdding(true)} onDelete={()=>setDeleting(true)} onFocus={focusSelected}/>} summary={`${project.parts.filter(p=>p.kind==='stage').length} kademe · ${project.parts.filter(p=>!isStructural(p.kind)).length} parça`} project={project} selected={selected} selectedIds={selection} focus={focus} onNavigate={navigateTo} onSelect={choose} onEdit={id=>{setSelected(id);setShowTree(true)}} onAdd={()=>setAdding(true)} onFocus={focusSelected} onMove={(source:string,target:MoveTarget)=>{try{const next=movePart(project,source,target);setProject(next);setSelected(source);if(!subtree(next,focus).some(p=>p.id===source))setFocus(next.parts.find(p=>p.id===source)!.parentId||rootId);}catch(error){setNotice(error instanceof Error?error.message:'Taşıma başarısız.')}}}/>
 <section className="viewport"><Breadcrumbs label="Tasarım yolu" items={path.map(node=>({id:node.id,label:node.kind==='rocket'?'Roket':node.name}))} onNavigate={navigateTo}/><div className={`drawing ${emptyCanvas?'empty':''}`} tabIndex={0} aria-label="Tasarım canvası; seçili elemana girmek veya seçim yokken üst seviyeye çıkmak için F" onPointerDown={event=>{if(view==='2d'&&!(event.target as Element).closest('.rocket-part')){setSelected('');event.currentTarget.focus()}}}>{emptyCanvas?<button className="empty-canvas-add" aria-label="Bileşen ekle" onPointerDown={event=>event.stopPropagation()} onClick={()=>setAdding(true)}><Plus size={36} strokeWidth={1.2}/></button>:<><div className="canvas-controls" data-canvas-controls onPointerDown={event=>event.stopPropagation()}><div className="view-switch" role="group" aria-label="Görünüm türü">{(['2d','3d'] as const).map(mode=><button key={mode} aria-pressed={view===mode} onClick={()=>setView(mode)}>{mode.toUpperCase()}</button>)}</div><div className="canvas-control-actions">{view==='2d'&&<IconButton size="sm" variant="ghost" aria-label="Hizaya yapıştır" title="Hizaya yapıştır · Alt: geçici kapat" aria-pressed={snapEnabled} onClick={()=>setSnapEnabled(v=>!v)}><Magnet size={15}/></IconButton>}<IconButton size="sm" variant="ghost" aria-label={t('all')} onClick={()=>navigateTo(rootId)}><Maximize size={15}/></IconButton></div></div>{view==='2d'?<RocketView project={project} selected={selected} selectedIds={selection} focus={focus} onSelect={choose} zoom={zoom} onZoom={setZoom} resetKey={viewRevision} snapEnabled={snapEnabled} onPreview={setCanvasPreview} onBatchPreview={setBatchPreview} onBatchEdit={setProject} onEdit={(id,patch)=>setProject(current=>editGeometry(current,id,patch))}/>:<Suspense fallback={<div className="view-fallback" role="status">3D görünüm yükleniyor…</div>}><RocketView3D project={project} selected={selected} selectedIds={selection} focus={focus} onSelect={choose} zoom={zoom} onBack={()=>setView('2d')}/></Suspense>}<div className="axis">X <ArrowUpRight size={15}/></div><CanvasSummary project={project} focus={focus} selection={selection} onSelect={setSelected} onFocus={focusSelected}/></>}</div>{!emptyCanvas&&<div className="viewport-tools"><Button size="sm" variant="ghost" onClick={focusSelected}><Focus size={14}/>{selected?t('focus'):'Üst seviyeye çık'} <kbd>F</kbd></Button><CanvasEditActions canUndo={canUndo} canRedo={canRedo} canDelete={canDelete} onUndo={undo} onRedo={redo} onDelete={()=>setDeleting(true)}/><ZoomControls value={zoom} onChange={setZoom}/></div>}</section>
 </ResizableSplit></div>
 <div className="page-slot" hidden={page!==1}>{visited.has(1)&&<MotorWorkspace project={project} flight={flight} onChange={changeFlight}/>}</div>
 <div className="page-slot" hidden={page!==2}>{visited.has(2)&&<SimulationWorkspace project={project} flight={flight} onChange={changeFlight}/>}</div>
 <div className="page-slot" hidden={page!==3}>{visited.has(3)&&<AnalysisWorkspace project={project} flight={flight} onAerodynamicsChange={aerodynamics=>setProject(current=>({...current,aerodynamics}))} onInspect={id=>{navigateTo(rootId);setSelected(id);setShowTree(true);setPage(0)}} onWorkspace={openPage} onPartChange={(id,patch)=>{try{setProject(patchSelection(project,[id],patch))}catch(e){setNotice(e instanceof Error?e.message:'Girdi uygulanamadı.')}}}/>}</div>
 {notice&&<Toast onClose={()=>setNotice('')}>{notice}</Toast>}
 </main><IdleShowroom project={project} enabled={!creating&&!adding&&!deleting&&!help}/><ComponentLibrary open={adding} onClose={()=>setAdding(false)} project={project} parentId={part.id} onAdd={addComponent}/>
 <Dialog open={creating} onClose={()=>setCreating(false)} title="Yeni proje"><TextField aria-label="Yeni proje adı" value={newName} onChange={event=>setNewName(event.target.value)} onKeyDown={event=>{if(event.key==='Enter'&&!event.nativeEvent.isComposing){event.preventDefault();replaceDocument(emptyProject(newName.trim()||'Yeni proje'))}}}/><div className="new-project-actions"><Button variant="ghost" onClick={exportProject}>Mevcut projeyi dışa aktar</Button><Button onClick={()=>replaceDocument(emptyProject(newName.trim()||"Yeni proje"))}>Oluştur</Button></div></Dialog>
 <Dialog open={deleting} onClose={()=>setDeleting(false)} title={selection.length>1?"Seçili bileşenler silinsin mi?":"Bileşen silinsin mi?"}><p className="dialog-text">{selectedParts.map(p=>p.name).join(", ")} · {deleteIds.size} bileşen kaldırılacak.</p><Button variant="danger" onClick={deleteSelected}>{selection.length>1?"Seçili bileşenleri sil":"Bileşeni ve alt bileşenleri sil"}</Button></Dialog>
 <Dialog open={help} onClose={()=>setHelp(false)} title="STARLAB"><div className="help-content"><p><strong>Tasarım</strong> Bileşeni seç; geometri, kütle ve aero girdilerini sağ panelde düzenle. Sürükleyerek sırala veya başka bir bileşenin içine taşı.</p><p><strong>Touchpad</strong> İki parmakla kaydır; pinch ile yakınlaştır. Parçayı taşı, sağ/üst tutamaçlardan boyutlandır. Mıknatıs hizaya yapıştırır; Alt geçici kapatır. Esc sürüklemeyi iptal eder.</p><p><strong>Seçim</strong> Shift ile parçaları seçime ekle/çıkar. Ortak alanları sağ panelden birlikte düzenle; tutamaçlardan birlikte boyutlandır. Alt sistem oluştur düğmesi uyumlu seçimleri gruplar.</p><p><strong>Odak</strong> Seçili bileşene F ile gir. Boşluğa tıklayıp F ile üst seviyeye çık. Ağaç ve araçlar da bu sisteme odaklanır. Yol çubuğundan geri dönebilirsin.</p><p><strong>Proje</strong> Ctrl/Cmd O ile aç, Ctrl/Cmd S ile .star dosyasını dışa aktar. Ctrl/Cmd Z geri alır; Ctrl/Cmd Shift Z veya Y ileri alır.</p><p><strong>Veriler</strong> CG/CP ve diğer fizik alanları kullanıcı girdileridir. Boş alan bilinmiyor demektir. Araçlar menüsünden birimleri belirtilmiş hesap girdisi paketi çıkarabilirsin.</p></div></Dialog></div>
}
createRoot(document.getElementById('root')!).render(<React.StrictMode><Startup><App/></Startup></React.StrictMode>);
