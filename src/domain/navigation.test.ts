import { describe, expect, it } from 'vitest';
import { initialProject, type Part } from './project';
import { focusPath, nextFocus } from './navigation';

describe('canvas focus navigation', () => {
  it('enters successive selections and walks back with an empty selection', () => {
    const project = initialProject();
    let scope = nextFocus(project, 'rocket', 'sustainer');
    expect(scope).toBe('sustainer');
    scope = nextFocus(project, scope, 'avionics');
    expect(scope).toBe('avionics');
    expect(focusPath(project, scope).map(part => part.id)).toEqual(['rocket', 'sustainer', 'avionics']);
    scope = nextFocus(project, scope, '');
    expect(scope).toBe('sustainer');
    scope = nextFocus(project, scope, '');
    expect(scope).toBe('rocket');
    expect(nextFocus(project, scope, '')).toBe('rocket');
  });
  it('supports nested subsystems and focusing a leaf directly', () => {
    const project = initialProject();
    project.parts.push({ ...project.parts.find(part => part.id === 'avionics')!, id: 'nested', parentId: 'avionics' } as Part);
    expect(focusPath(project, 'nested').map(part => part.id)).toEqual(['rocket', 'sustainer', 'avionics', 'nested']);
    expect(nextFocus(project, 'avionics', 'chute')).toBe('chute');
    expect(nextFocus(project, 'chute', '')).toBe('avionics');
    expect(nextFocus(project, 'chute', 'chute')).toBe('chute');
  });
  it('handles a removed selection by navigating to the parent', () => {
    expect(nextFocus(initialProject(), 'avionics', 'missing')).toBe('sustainer');
  });
});
