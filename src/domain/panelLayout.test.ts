import {it,expect} from 'vitest';
import {panelSizes} from './panelLayout';
it('clamps both panel sizes while reserving the handle',()=>{const low=panelSizes(1000,0,200,300);expect(low.first).toBe(200);const high=panelSizes(1000,1,200,300);expect(high.second).toBe(300);expect(high.first+high.second).toBe(994)});
it('fits small windows without overflow or negative sizes',()=>{for(const total of [0,100,320,800]){const p=panelSizes(total,.8,200,300);expect(p.first).toBeGreaterThanOrEqual(0);expect(p.second).toBeGreaterThanOrEqual(0);expect(p.first+p.second).toBeCloseTo(Math.max(0,total-6))}});
