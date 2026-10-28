import {IconButton} from '@stardrop/ui';
import {Undo2,Redo2,Trash2} from 'lucide-react';

interface Props {canUndo:boolean;canRedo:boolean;canDelete:boolean;onUndo:()=>void;onRedo:()=>void;onDelete:()=>void}
export default function CanvasEditActions({canUndo,canRedo,canDelete,onUndo,onRedo,onDelete}:Props){
 return <div className="canvas-edit-actions" role="group" aria-label="Görünüm düzenleme araçları">
 <IconButton size="sm" variant="ghost" label="Geri al" title="Geri al · Ctrl/Cmd Z" disabled={!canUndo} onClick={onUndo}><Undo2 size={15}/></IconButton>
 <IconButton size="sm" variant="ghost" label="İleri al" title="İleri al · Ctrl/Cmd Shift Z" disabled={!canRedo} onClick={onRedo}><Redo2 size={15}/></IconButton>
 <IconButton size="sm" variant="ghost" label="Seçili bileşeni sil" title="Seçili bileşeni sil · Del" disabled={!canDelete} onClick={onDelete}><Trash2 size={15}/></IconButton>
 </div>;
}
