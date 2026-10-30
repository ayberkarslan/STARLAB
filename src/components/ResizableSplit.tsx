import {ResizableSplit as Split,type ResizableSplitProps} from '@stardrop/ui';
import './resizable-split.css';
export default function ResizableSplit(props:ResizableSplitProps){return <Split {...props} storagePrefix="starlab.layout."/>}
