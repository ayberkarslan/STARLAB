import {it,expect} from 'vitest';
import {renderToStaticMarkup} from 'react-dom/server';
import {calculationExample} from '../domain/calculationExample';
import RocketView from './RocketView';

it('renders calculated cg and cp markers in design coordinates',()=>{
 const html=renderToStaticMarkup(<RocketView project={calculationExample()} selected="" focus="rocket" onSelect={()=>{}} zoom={1} stability={{mass:3,cg:.4,cp:.8,cna:5,margin:2,mach:.3,angleOfAttack:0}}/>);
 expect(html).toContain('translate(400 0)');expect(html).toContain('translate(800 0)');expect(html).toContain('CG 400.0 mm, CP 800.0 mm');
});
